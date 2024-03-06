/*
 * uRISCV - A general purpose computer system simulator
 *
 * Copyright (C) 2004 Mauro Morsiani
 * Copyright (C) 2011 Tomislav Jonjic
 * Copyright (C) 2023 Gianmaria Rovelli
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

/*
 * This is a stand-alone program which converts an ELF object or executable
 * file into one of MPS project file formats: .core, .rom or .aout.
 * See external documentation for format details.
 */

// #include <config.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <iterator>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#include <libelf.h>

#include "base/bit_tricks.h"
#include "uriscv/aout.h"
#include "uriscv/blockdev_params.h"

/*
 * Functions throughtout this module close over this global!
 */
static Elf *elf;
static Elf32_Ehdr *elfHeader;

static const size_t kBlockSize = 4096;

#define forEachSection(sd)                                                     \
  for (sd = elf_nextscn(elf, NULL); sd != NULL; sd = elf_nextscn(elf, sd))

#define forEachSectionData(sd, data)                                           \
  for (data = elf_getdata(sd, NULL); data != NULL; data = elf_getdata(sd, data))

struct Symbol {
  Symbol(const std::string &name, Elf32_Sym *details)
      : name(name), details(details) {}

  std::string name;
  Elf32_Sym *details;
};

/*
 * Iterator class that factors out the logic behind the iteration
 * through an ELF symbol table.
 */
class SymbolTableIterator
    : public std::iterator<std::input_iterator_tag, Symbol> {
public:
  explicit SymbolTableIterator(Elf *elf);
  SymbolTableIterator();

  SymbolTableIterator &operator++();
  SymbolTableIterator operator++(int);

  Symbol operator*() const;

  bool operator==(const SymbolTableIterator &other) {
    return current == other.current;
  }
  bool operator!=(const SymbolTableIterator &other) {
    return current != other.current;
  }

private:
  Elf_Scn *const stSec;
  Elf32_Shdr *const stSecHeader;
  Elf_Data *data;
  Elf32_Sym *current;
};

static void fatalError(const char *format, ...);
static void elfError();
static void printHelp();

uint32_t toTargetEndian(uint32_t x);

static Elf_Scn *getSectionByType(Elf32_Word type);
static Elf32_Addr getGPValue();

static void elf2aout(bool isCore);
static void createSymbolTable();
static void elf2bios();

const char *programName;

// Input file name
const char *fileName = NULL;

// Be verbose
static bool verbose = false;

int main(int argc, char **argv) {
  programName = argv[0];

  if (elf_version(EV_CURRENT) == EV_NONE)
    fatalError("ELF library out of date");

  uint32_t fileId = 0;
  bool createMap = false;

  // Parse args, lamely
  int i = 1;
  while (i < argc && fileName == NULL) {
    if (!strcmp("-v", argv[i])) {
      verbose = true;
    } else if (!strcmp("-m", argv[i])) {
      createMap = true;
    } else if (!strcmp("-k", argv[i])) {
      fileId = COREFILEID;
      if (i + 1 < argc)
        fileName = argv[i + 1];
    } else if (!strcmp("-b", argv[i])) {
      fileId = BIOSFILEID;
      if (i + 1 < argc)
        fileName = argv[i + 1];
    } else if (!strcmp("-a", argv[i])) {
      fileId = AOUTFILEID;
      if (i + 1 < argc)
        fileName = argv[i + 1];
    }
    i++;
  }

  if (fileName == NULL) {
    fprintf(stderr, "%s : Wrong/unknown argument(s)\n", programName);
    printHelp();
    exit(1);
  }

  int fd = open(fileName, O_RDONLY);
  if (fd == -1)
    fatalError("Cannot access %s: %s", fileName, strerror(errno));

  elf = elf_begin(fd, ELF_C_READ, NULL);
  if (elf == NULL)
    elfError();

  if (elf_kind(elf) != ELF_K_ELF)
    fatalError("`%s' is not an ELF file", fileName);

  // Check ELF file version and arch
  elfHeader = elf32_getehdr(elf);
  if (elfHeader == NULL)
    elfError();
  if (elfHeader->e_version != EV_CURRENT)
    fatalError("%s: unsupported ELF file version", fileName);
  if (elfHeader->e_machine != EM_RISCV)
    fatalError("%s: invalid machine type (RISCV expected)", fileName);

  switch (fileId) {
  case BIOSFILEID:
    elf2bios();
    break;
  case AOUTFILEID:
    elf2aout(false);
    break;
  case COREFILEID:
    elf2aout(true);
    break;
  default:
    // Assert not reached
    assert(0);
  }

  if (fileId == COREFILEID || createMap)
    createSymbolTable();

  return 0;
}

static void fatalError(const char *format, ...) {
  fprintf(stderr, "%s: ", programName);

  va_list ap;
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
  fputc('\n', stderr);

  exit(1);
}

static void elfError() { fatalError("ELF error: %s", elf_errmsg(-1)); }

static void printHelp() {
  fprintf(stderr,
          "%s syntax : %s [-v] [-m] {-k | -b | -a} <file>\n"
          "\n"
          "where:\n\t-v\tverbose operation\n\t-m\tmake map file "
          "<file>.stab.uriscv\n"
          "\n"
          "\t-k\tmake kernel core file <file>.core.uriscv + map file\n"
          "\t-b\tmake BIOS file <file>.rom.uriscv\n\t-a\tmake a.out file "
          "<file>.aout.uriscv\n\n",
          programName, programName);
}

inline uint32_t toTargetEndian(uint32_t x) {
#ifdef WORDS_BIGENDIAN
  return (elfHeader->e_ident[EI_DATA] == ELFDATA2LSB) ? SwapEndian32(x) : x;
#else
  return (elfHeader->e_ident[EI_DATA] == ELFDATA2LSB) ? x : SwapEndian32(x);
#endif
}

SymbolTableIterator::SymbolTableIterator(Elf *elf)
    : stSec(getSectionByType(SHT_SYMTAB)), stSecHeader(elf32_getshdr(stSec)) {
  current = NULL;

  data = NULL;
  while ((data = elf_getdata(stSec, data)) != NULL)
    if (data->d_type == ELF_T_SYM && data->d_size > 0)
      break;
  if (data != NULL)
    current = (Elf32_Sym *)data->d_buf;
}

SymbolTableIterator::SymbolTableIterator()
    : stSec(NULL), stSecHeader(NULL), current(NULL) {}

Symbol SymbolTableIterator::operator*() const {
  assert(current != NULL);

  const char *name = elf_strptr(elf, stSecHeader->sh_link, current->st_name);
  assert(name != NULL);

  return Symbol(name, current);
}

SymbolTableIterator &SymbolTableIterator::operator++() {
  if (current == NULL)
    return *this;

  ++current;
  if (current >= (Elf32_Sym *)data->d_buf + data->d_size / sizeof(Elf32_Sym)) {
    while ((data = elf_getdata(stSec, data)) != NULL)
      if (data->d_type == ELF_T_SYM && data->d_size > 0)
        break;
    if (data == NULL)
      current = NULL;
    else
      current = (Elf32_Sym *)data->d_buf;
  }

  return *this;
}

SymbolTableIterator SymbolTableIterator::operator++(int) {
  SymbolTableIterator result = *this;
  ++(*this);
  return result;
}

static Elf_Scn *getSectionByType(Elf32_Word type) {
  Elf_Scn *sd;
  forEachSection(sd) {
    Elf32_Shdr *sh = elf32_getshdr(sd);
    if (sh == NULL)
      elfError();
    if (sh->sh_type == type)
      return sd;
  }
  return NULL;
}

/*
 * Retreive the starting $gp value
 */
static Elf32_Addr getGPValue() {
  SymbolTableIterator it(elf), end;
  for (; it != end; ++it)
    if ((*it).name == "_gp")
      return (*it).details->st_value;
  return (Elf32_Addr)-1;
}

/*
 * Convert an ELF executable into a uRISCV a.out or core executable.
 */
static void elf2aout(bool isCore) {
  // Check ELF object type
  if (elfHeader->e_type != ET_EXEC)
    fatalError("ELF object file is not executable");

  uint32_t header[N_AOUT_HDR_ENT];
  std::fill_n(header, N_AOUT_HDR_ENT, 0);
  header[AOUT_HE_TAG] = AOUTFILEID;

  // Set program entry
  header[AOUT_HE_ENTRY] = elfHeader->e_entry;

  // Set initial $gp entry
  header[AOUT_HE_GP_VALUE] = getGPValue();
  if (header[AOUT_HE_GP_VALUE] == (Elf32_Addr)-1)
    fatalError("Cannot obtain initial $gp value");

  // Obtain the program header table
  Elf32_Phdr *pht = elf32_getphdr(elf);
  if (pht == NULL)
    elfError();
  size_t phtSize;
  if (elf_getphdrnum(elf, &phtSize))
    elfError();

  // Obtain segment info
  bool foundDataSeg = false;
  uint8_t *dataBuf = NULL;
  bool foundTextSeg = false;
  uint8_t *textBuf = NULL;

  for (size_t i = 0; i < phtSize; i++) {
    if (pht[i].p_type != PT_LOAD)
      continue;
    if (pht[i].p_flags == (PF_R | PF_W)) {
      if (foundDataSeg)
        fatalError("Redundant .data program header table entry %u",
                   (unsigned int)i);
      foundDataSeg = true;
      header[AOUT_HE_DATA_MEMSZ] = pht[i].p_memsz;
      header[AOUT_HE_DATA_VADDR] = pht[i].p_vaddr;
      uint32_t size = isCore ? pht[i].p_memsz : pht[i].p_filesz;
      header[AOUT_HE_DATA_FILESZ] = (size / kBlockSize) * kBlockSize;
      if (header[AOUT_HE_DATA_FILESZ] < size)
        header[AOUT_HE_DATA_FILESZ] += kBlockSize;
      if (header[AOUT_HE_DATA_FILESZ] > 0) {
        dataBuf = new uint8_t[header[AOUT_HE_DATA_FILESZ]];
        std::fill_n(dataBuf, header[AOUT_HE_DATA_FILESZ], 0);
      } else {
        dataBuf = NULL;
      }
    } else if (pht[i].p_flags == (PF_R | PF_X)) {
      if (foundTextSeg)
        fatalError("Redundant .text program header table entry %u",
                   (unsigned int)i);
      if (pht[i].p_memsz == 0)
        fatalError("Empty .text segment");
      foundTextSeg = true;
      header[AOUT_HE_TEXT_MEMSZ] = pht[i].p_memsz;
      header[AOUT_HE_TEXT_VADDR] = pht[i].p_vaddr;
      header[AOUT_HE_TEXT_FILESZ] = (pht[i].p_memsz / kBlockSize) * kBlockSize;
      if (header[AOUT_HE_TEXT_FILESZ] < pht[i].p_memsz)
        header[AOUT_HE_TEXT_FILESZ] += kBlockSize;
      textBuf = new uint8_t[header[AOUT_HE_TEXT_FILESZ]];
      std::fill_n(textBuf, header[AOUT_HE_TEXT_FILESZ], 0);
    } else {
      fprintf(stderr, "Warning: unknown program header table entry %u\n",
              (unsigned int)i);
    }
  }

  if (!foundTextSeg)
    fatalError("Missing .text program header");

  if (!foundDataSeg) {
    header[AOUT_HE_DATA_MEMSZ] = header[AOUT_HE_DATA_FILESZ] = 0;
    header[AOUT_HE_DATA_VADDR] =
        header[AOUT_HE_TEXT_VADDR] + header[AOUT_HE_TEXT_FILESZ];
    dataBuf = NULL;
  }

  header[AOUT_HE_TEXT_OFFSET] = 0;
  header[AOUT_HE_DATA_OFFSET] = header[AOUT_HE_TEXT_FILESZ];

  // Scan sections and copy data to a.out segments
  Elf_Scn *sd;
  forEachSection(sd) {
    Elf32_Shdr *sh = elf32_getshdr(sd);
    if (sh == NULL)
      elfError();
    if (sh->sh_type == SHT_PROGBITS && (sh->sh_flags & SHF_ALLOC)) {
      uint8_t *buf;
      if (sh->sh_addr >= header[AOUT_HE_DATA_VADDR])
        buf = dataBuf + (sh->sh_addr - header[AOUT_HE_DATA_VADDR]);
      else
        buf = textBuf + (sh->sh_addr - header[AOUT_HE_TEXT_VADDR]);

      Elf_Data *data;
      forEachSectionData(sd, data) {
        std::memcpy(buf, data->d_buf, data->d_size);
        buf += data->d_size;
      }
    }
  }

  // Write out the .aout header
  uint32_t *headerBuf = (uint32_t *)textBuf;
  for (size_t i = 0; i < N_AOUT_HDR_ENT; i++) {
    if (headerBuf[i])
      fatalError("No space for a.out header");
    headerBuf[i] = toTargetEndian(header[i]);
  }

  std::string outName = fileName;
  if (isCore)
    outName += ".core.uriscv";
  else
    outName += ".aout.uriscv";
  FILE *file = fopen(outName.c_str(), "w");
  if (file == NULL)
    fatalError("Cannot create a.out file `%s'", outName.c_str());

  // If it's a core file, write the RRF padding first
  if (isCore) {
    uint32_t tag = toTargetEndian(COREFILEID);
    if (fwrite(&tag, sizeof(tag), 1, file) != 1)
      fatalError("Error writing a.out file `%s'", outName.c_str());
    uint32_t pad = 0;
    for (size_t i = 0; i < 1024; i++)
      if (fwrite(&pad, sizeof(pad), 1, file) != 1)
        fatalError("Error writing a.out file `%s'", outName.c_str());
  }

  // Write the segments, finally.
  if (fwrite(textBuf, 1, header[AOUT_HE_TEXT_FILESZ], file) !=
      header[AOUT_HE_TEXT_FILESZ])
    fatalError("Error writing a.out file `%s'", outName.c_str());

  if (dataBuf != NULL) {
    if (fwrite(dataBuf, 1, header[AOUT_HE_DATA_FILESZ], file) !=
        header[AOUT_HE_DATA_FILESZ])
      fatalError("Error writing a.out file `%s'", outName.c_str());
  }

  fclose(file);
  delete dataBuf;
  delete textBuf;
}

/*
 * Extract the symbol table from an ELF object file (ET_EXEC or
 * ET_REL) and create a symbol table in uRISCV .stab format.
 */
static void createSymbolTable() {
  static const char *const typeName[] = {"", "OBJ", "FUN"};
  static const char *const bindingName[] = {"LOC", "GLB", "WK "};

  std::string outName = fileName;
  outName += ".stab.uriscv";
  FILE *file = fopen(outName.c_str(), "w");
  if (file == NULL)
    fatalError("Cannot create symbol table file `%s': %s", outName.c_str(),
               strerror(errno));

  uint32_t tag = toTargetEndian(STABFILEID);
  if (fwrite(&tag, sizeof(tag), 1, file) != 1)
    fatalError("Error writing symbol table file `%s': %s", outName.c_str(),
               strerror(errno));

  // Write a header template, to be patched later.
  if (fprintf(file, "%.8X %.8X\n", 0, 0) < 0)
    fatalError("Error writing symbol table file `%s': %s", outName.c_str(),
               strerror(errno));

  // Write symbol records
  uint32_t funCount = 0, objCount = 0;
  SymbolTableIterator it(elf), end;
  int rc;
  for (; it != end; ++it) {
    Symbol s = *it;
    unsigned char type = ELF32_ST_TYPE(s.details->st_info);
    unsigned char binding = ELF32_ST_BIND(s.details->st_info);
    if (!s.name.empty() && (type == STT_FUNC || type == STT_OBJECT)) {
      rc = fprintf(file, "%-32.32s :%s:0x%.8lX:0x%.8lX:%s\n", s.name.c_str(),
                   typeName[type], (unsigned long)s.details->st_value,
                   (unsigned long)s.details->st_size, bindingName[binding]);
      if (rc < 0)
        fatalError("Error writing symbol table file `%s': %s", outName.c_str(),
                   strerror(errno));

      if (type == STT_FUNC)
        funCount++;
      else
        objCount++;
    }
  }

  // Write symbol counts
  if (fseek(file, sizeof(tag), SEEK_SET) ||
      fprintf(file, "%.8X %.8X", (unsigned int)funCount,
              (unsigned int)objCount) < 0) {
    fatalError("Error writing symbol table file `%s': %s", outName.c_str(),
               strerror(errno));
  }

  fclose(file);
}

/*
 * Convert an ELF relocatable object file into an uRISCV ROM (BIOS) file.
 */
static void elf2bios() {
  // Check ELF object type
  if (elfHeader->e_type != ET_REL)
    fatalError("`%s' is not a relocatable ELF object file", fileName);

  // Find .text section
  Elf_Scn *sd;
  forEachSection(sd) {
    Elf32_Shdr *sh = elf32_getshdr(sd);
    if (sh == NULL)
      elfError();
    if ((sh->sh_type == SHT_PROGBITS) && (sh->sh_flags & SHF_ALLOC) &&
        (sh->sh_flags & SHF_EXECINSTR)) {
      break;
    }
  }
  if (sd == NULL)
    fatalError("Could not find .text section");

  std::string outName = fileName;
  outName += ".rom.uriscv";
  FILE *file = fopen(outName.c_str(), "w");
  if (file == NULL)
    fatalError("Cannot create BIOS file `%s'", outName.c_str());

  Elf_Data *data;

  // Write header
  uint32_t tag = toTargetEndian(BIOSFILEID);
  uint32_t size = 0;
  forEachSectionData(sd, data) if (data->d_type == ELF_T_BYTE) size +=
      data->d_size / 4;
  size = toTargetEndian(size);
  if (fwrite(&tag, sizeof(tag), 1, file) != 1 ||
      fwrite(&size, sizeof(size), 1, file) != 1) {
    fatalError("Error writing BIOS file `%s': %s", outName.c_str(),
               strerror(errno));
  }

  // Copy .text into file
  forEachSectionData(sd, data) {
    if (data->d_type == ELF_T_BYTE) {
      if (fwrite(data->d_buf, 1, data->d_size, file) != data->d_size)
        fatalError("Error writing BIOS file `%s': %s", outName.c_str(),
                   strerror(errno));
    }
  }

  fclose(file);

  // Gratuitous sanity check
  size_t relocs = 0;
  forEachSection(sd) {
    Elf32_Shdr *sh = elf32_getshdr(sd);
    if (sh == NULL)
      elfError();
    if (sh->sh_type == SHT_REL || sh->sh_type == SHT_RELA) {
      forEachSectionData(sd, data) {
        switch (data->d_type) {
        case ELF_T_REL:
          relocs += data->d_size / sizeof(Elf32_Rel);
          break;
        case ELF_T_RELA:
          relocs += data->d_size / sizeof(Elf32_Rela);
          break;
        default:
          break;
        }
      }
    }
  }
  if (relocs) {
    fprintf(stderr,
            "%s: Warning: BIOS code may contain %u unresolved relocations\n",
            programName, (unsigned int)relocs);
  }
}
