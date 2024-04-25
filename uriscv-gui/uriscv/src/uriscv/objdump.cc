/*
 * uRISCV - A general purpose computer system simulator
 *
 * Copyright (C) 2004 Mauro Morsiani
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

/****************************************************************************
 *
 * This s a stand-alone program which recognizes .core, .aout and .rom file
 * formats and prints format and contents information on the standard output.
 * The following information may be retrieved:
 *
 * - header contents for .core/.aout files;
 * - machine code listing for all file types;
 * - hexadecimal word dump of file contents;
 * - byte dump of file contents.
 *
 * Both byte and word dumps are provided since, while word dump is almost
 * the same for code compiled on big/little endian machines, byte dump
 * differs.
 *
 * All zero-filled spaces are skipped, code lines are numbered with proper
 * (virtual) address, and byte/word dumps are tagged with file offsets.
 *
 ****************************************************************************/

#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "uriscv/aout.h"
#include "uriscv/blockdev_params.h"
#include "uriscv/disassemble.h"
#include "uriscv/processor_defs.h"
#include "uriscv/symbol_table.h"
#include "uriscv/types.h"
#include <uriscv/const.h>

static const size_t AOUTENTNUM = 10;

// number of NOPS, empty words and characters before skip in displaying
#define NOPSMIN 2
#define EMPTYWMIN 3
#define EMPTYCMIN 3

// character and word buffer sizes for file read
#define WBUFSIZE 4
#define CBUFSIZE 16

// .aout header field names (copied from elf2umps.cc)
HIDDEN const char *const aoutName[] = {"",
                                       "Program (virtual) starting address",
                                       ".text (virtual) start address",
                                       ".text memory size",
                                       ".text file start offset",
                                       ".text file size",
                                       ".data (virtual) start address",
                                       ".data memory size",
                                       ".data file start offset",
                                       ".data file size"};

HIDDEN bool forceTextOverflow = false;

//
// Program functions
//

HIDDEN void showHelp(const char *prgName);
HIDDEN int hdrDump(const char *prgName, const char *fileName);
HIDDEN int disAsm(const char *prgName, const char *fileName);
HIDDEN int asmPrint(const char *prg, const char *fname, FILE *inF, int asmStart,
                    int asmSize, bool coreAoutFile);
HIDDEN int xDump(const char *prgName, const char *fileName);
HIDDEN int bDump(const char *prgName, const char *fileName);

/****************************************************************************/
/* Definitions to be exported.                                              */
/****************************************************************************/

// This function scans the line arguments; if no error is found, the
// required analysis is performed, or a warning/help message is printed.
// Returns an EXIT_SUCCESS/FAILURE code
int main(int argc, char *argv[]) {
  // required analysis possible
  bool hdr = false;
  bool disasm = false;
  bool xdump = false;
  bool bdump = false;
  int ret = EXIT_SUCCESS;
  int i;

  if (argc == 1)
    showHelp(argv[0]);
  else {
    // scan line arguments
    for (i = 1; i < argc - 1 && ret != EXIT_FAILURE; i++) {
      if (SAMESTRING("-h", argv[i]))
        hdr = true;
      else if (SAMESTRING("-d", argv[i]))
        disasm = true;
      else if (SAMESTRING("-x", argv[i]))
        xdump = true;
      else if (SAMESTRING("-b", argv[i]))
        bdump = true;
      else if (SAMESTRING("-f", argv[i]))
        forceTextOverflow = true;
      else if (SAMESTRING("-a", argv[i])) {
        hdr = true;
        disasm = true;
        xdump = true;
        bdump = true;
      } else
        // unrecognized option
        ret = EXIT_FAILURE;
    }
    if (ret != EXIT_FAILURE && strstr(argv[argc - 1], MPSFILETYPE) != NULL) {
      if (hdr == true)
        ret = hdrDump(argv[0], argv[argc - 1]);

      if (disasm == true && ret != EXIT_FAILURE)
        ret = disAsm(argv[0], argv[argc - 1]);

      if (xdump == true && ret != EXIT_FAILURE)
        ret = xDump(argv[0], argv[argc - 1]);

      if (bdump == true && ret != EXIT_FAILURE)
        ret = bDump(argv[0], argv[argc - 1]);
    } else {
      fprintf(stderr, "%s: Wrong/unknown argument(s)\n", argv[0]);
      showHelp(argv[0]);
      ret = EXIT_FAILURE;
    }
  }
  return (ret);
}

/****************************************************************************/
/* Definitions strictly local to the module.                                */
/****************************************************************************/

// This function prints a warning/help message on standard error
HIDDEN void showHelp(const char *prgName) {
  fprintf(stderr, "%s syntax: %s [-h] [-d] [-x] [-b] [-a] [-f] <file>%s\n\n",
          prgName, prgName, MPSFILETYPE);
  fprintf(stderr,
          "where:\n\t-h\tshow file header\n\t-d\tdisassemble .text area\n");
  fprintf(stderr, "\t-b\tfull byte dump\n\t-x\tfull word dump\n\t-a\tall of "
                  "the above\n\t-f\tforce proper .text area overflow\n\t\tuse "
                  "if the stab file does not contain all function symbols\n");
}

// This function locates and prints the header contents of a .aout/.core file;
// Returns an EXIT_SUCCESS/FAILURE code
HIDDEN int hdrDump(const char *prgName, const char *fileName) {
  int ret = EXIT_SUCCESS;
  FILE *inFile = NULL;
  Word tag;
  Word aoutHdr[AOUTENTNUM];
  unsigned int i;
  SWord offs;

  if ((inFile = fopen(fileName, "r")) == NULL) {
    fprintf(stderr, "%s: Error opening file %s: %s\n", prgName, fileName,
            strerror(errno));
    ret = EXIT_FAILURE;
  } else if (fread((void *)&tag, WORDLEN, 1, inFile) != 1 ||
             !(tag == AOUTFILEID || tag == COREFILEID || tag == BIOSFILEID)) {
    fprintf(stderr, "%s: Error opening file %s: invalid/corrupted file\n",
            prgName, fileName);
    ret = EXIT_FAILURE;
  } else {
    // file is of correct type
    if (tag == BIOSFILEID)
      printf("%s: ROM file type: it has no a.out header\n\n", fileName);
    else {
      if (tag == AOUTFILEID) {
        offs = 0L;
        printf("%s: a.out file type\n\n", fileName);
      } else {
        offs = CORE_HDR_SIZE * WORDLEN;
        printf("%s: core file type\n\n", fileName);
      }
      // load header

      if (fseek(inFile, offs, SEEK_SET) == EOF ||
          fread((void *)aoutHdr, WORDLEN, AOUTENTNUM, inFile) != AOUTENTNUM ||
          fclose(inFile) == EOF) {
        fprintf(stderr, "%s: Error opening file %s: invalid/corrupted file\n",
                prgName, fileName);
        ret = EXIT_FAILURE;
      } else {
        // print header
        for (i = 1; i < AOUTENTNUM; i++)
          printf("%-35.35s %.8x\n", aoutName[i], aoutHdr[i]);
        printf("\n");
      }
    }
  }
  return (ret);
}

// This function locates and prints a machine code listing for executable
// instructions contained in a .rom or .aout/.core file.
// Returns an EXIT_SUCCESS/FAILURE code
HIDDEN int disAsm(const char *prgName, const char *fileName) {
  setDisassembleSep("");

  int ret = EXIT_SUCCESS;
  FILE *inFile = NULL;
  Word tag, size;
  Word aoutHdr[AOUTENTNUM];
  Word phdrOffs;
  Word textOffs;

  if ((inFile = fopen(fileName, "r")) == NULL) {
    fprintf(stderr, "%s: Error opening file %s: %s\n", prgName, fileName,
            strerror(errno));
    ret = EXIT_FAILURE;
  } else {
    // file exists
    if (fread((void *)&tag, WORDLEN, 1, inFile) != 1 ||
        !(tag == AOUTFILEID || tag == COREFILEID || tag == BIOSFILEID)) {
      fprintf(stderr, "%s: Error opening file %s: invalid/corrupted file\n",
              prgName, fileName);
      ret = EXIT_FAILURE;
    } else {
      // file is of correct type
      if (tag == BIOSFILEID) {
        if (fread((void *)&size, WORDLEN, 1, inFile) == 1)
          // size read correctly
          ret = asmPrint(prgName, fileName, inFile, 0, size * WORDLEN, false);
        else
          ret = EXIT_FAILURE;
      } else {
        if (tag == AOUTFILEID)
          phdrOffs = 0;
        else
          phdrOffs = CORE_HDR_SIZE * WORDLEN;

        textOffs = phdrOffs + AOUT_PHDR_SIZE;

        int offsetSeek = fseek(inFile, phdrOffs, SEEK_SET) == EOF;
        int headerRead = fread((void *)aoutHdr, WORDLEN, AOUTENTNUM, inFile) !=
                         AOUTENTNUM; // load header
        int textSeek = fseek(inFile, (SWord)textOffs, SEEK_SET) == EOF;

        if (offsetSeek || headerRead || textSeek) {
          fprintf(stderr, "%s: Error reading file %s: invalid/corrupted file\n",
                  prgName, fileName);
          ret = EXIT_FAILURE;
        } else {
          ret = asmPrint(prgName, fileName, inFile,
                         aoutHdr[AOUT_HE_TEXT_VADDR] + AOUT_PHDR_SIZE,
                         aoutHdr[AOUT_HE_TEXT_MEMSZ], true);
        }
      }
    }
    fclose(inFile);
  }
  return (ret);
}

// This function loads the executable code contained in the file inF
// from current position, and prints a readable machine listing for
// instructions contained there (up to asmSize) numbering them by beginning
// with asmStart.
HIDDEN int asmPrint(const char *prg, const char *fname, FILE *inF, int asmStart,
                    int asmSize, bool coreAoutFile) {
  SymbolTable *symt = 0;

  if (coreAoutFile) {
    char localFname[4096];
    strcpy(localFname, fname);

    // Look for .stab file in the fname file dir
    char *inputFileBaseName = basename((char *)localFname);
    char *inputFilePath = dirname((char *)localFname);

    char prefix[256];
    char *suffix = strstr(inputFileBaseName, ".uriscv");

    if (suffix == 0) {
      printf("No .stab file found, function names will not be printed\n\n");
    } else {
      char stabFilePath[4096];

      // Terminate basename before .aout/core.uriscv
      // .core = 5 chars = .aout
      *(suffix - 5) = '\0';
      strcpy(prefix, inputFileBaseName);

      sprintf(stabFilePath, "%s/%s.stab.uriscv", inputFilePath, prefix);
      if (!access(stabFilePath, F_OK)) {
        symt = new SymbolTable(0, stabFilePath);
      } else {
        printf("No .stab file found, function names will not be printed\n\n");
      }
    }
  }
  printf("Disassembly of section .text:\n%s", symt == 0 ? "\n" : "");

  Word instr;
  bool lastInstrRet;

  const char *currentFunction = 0;
  uint32_t currentFunctionSize = 1;
  for (; asmSize > 0 && !feof(inF) && !ferror(inF);
       asmStart += WORDLEN, asmSize -= WORDLEN) {
    if (instr == INSTR_RET)
      lastInstrRet = true;
    else
      lastInstrRet = false;

    if (symt != 0) {
      const Symbol *newSym = symt->Probe(0, asmStart, true);
      if (newSym != 0) {
        if (currentFunction == 0 ||
            strcmp(currentFunction, newSym->getName())) {
          currentFunction = newSym->getName();
          currentFunctionSize = newSym->getEnd() - newSym->getStart();
          printf("\n%.8x <%s>:\n", asmStart, currentFunction);
        }
      }
      // If for some reason the .stab file does not contain all symbols, the
      // forceTextOverflow option allows for printing even after
      // finding memory sections not associated with any known symbol,
      // albeit at the cost of not stopping at the end of the instructions,
      // thereby skeweing the last part of the output
      //
      // If the option is not set, the invariant that needs to be upheld
      // to ensure correct execution is that all of the function symbols
      // are mapped into the .stab file
      else if (forceTextOverflow) {
        if (currentFunction != 0) {
          printf("\n%.8x <\?\?\?>:\n", asmStart);
        }
        currentFunction = 0;
      } else {
        // If the current function has a proper size in the .stab file,
        // the symbol table correctly identified an unknown section and
        // there is no risk of skipping real instructions
        if (currentFunctionSize != 0)
          break;

        // Last instruction was ret, therefore we can assume
        // we have reached the end of the proper .text section
        if (lastInstrRet)
          break;
        // Last instruction wasn't ret and we're in a function without
        // a size definition in the .stab file, disassemble until
        // ret is found
        else {
          for (; asmSize > 0 && !feof(inF) && !ferror(inF);
               asmStart += WORDLEN, asmSize -= WORDLEN) {
            if (fread((void *)&instr, WORDLEN, 1, inF) == 1)
              printf("%.8x:\t%.8x\t\t%s\n", asmStart, instr, StrInstr(instr));
            if (instr == INSTR_RET)
              break;
          }
          break;
        }
      }
    }
    // read one instruction
    if (fread((void *)&instr, WORDLEN, 1, inF) == 1) {
      printf("%.8x:\t%.8x\t\t%s\n", asmStart, instr, StrInstr(instr));
    }
  }
  if (ferror(inF)) {
    fprintf(stderr, "%s: Error	disassembling file %s: %s\n", prg, fname,
            strerror(errno));
    return (EXIT_FAILURE);
  } else {
    printf("\n");
    return (EXIT_SUCCESS);
  }
}

// This function loads the file contents of fileName, and prints a
// hexadecimal word dump of them. It skips magic file number for .core
// and .rom file types, since it is not "seen" inside simulation.
// It skips blocks of zero-filled words after finding EMPTYWMIN ones.
// Returns an EXIT_SUCCESS/FAILURE code
HIDDEN int xDump(const char *prgName, const char *fileName) {
  int ret = EXIT_SUCCESS;
  FILE *inFile = NULL;
  Word tag, size;
  Word buf[WBUFSIZE];
  unsigned int i, words;
  bool empty;
  unsigned int emptyl = 0;
  unsigned int idx = 0;

  if ((inFile = fopen(fileName, "r")) == NULL) {
    fprintf(stderr, "%s: Error opening file %s: %s\n", prgName, fileName,
            strerror(errno));
    ret = EXIT_FAILURE;
  } else {
    // identifies file type
    if (fread((void *)&tag, WORDLEN, 1, inFile) != 1 ||
        !(tag == AOUTFILEID || tag == COREFILEID || tag == BIOSFILEID)) {
      fprintf(stderr, "%s: Error opening file %s: invalid/corrupted file\n",
              prgName, fileName);
      ret = EXIT_FAILURE;
    } else {
      // file is of correct type
      if (tag == AOUTFILEID)
        rewind(inFile);
      // core and bios file tag should be skipped, since it is not "seen"
      // inside mps simulations
      else if (tag == BIOSFILEID)
        // skip size header too
        if (fread((void *)&size, WORDLEN, 1, inFile) != 1)
          if (ferror(inFile)) {
            fprintf(stderr, "%s: Error	reading file %s: %s\n", prgName,
                    fileName, strerror(errno));
            ret = EXIT_FAILURE;
          }
      // else all is ok

      while (!ferror(inFile) && !feof(inFile))
        // scans file
        if ((words = fread((void *)buf, WORDLEN, WBUFSIZE, inFile)) > 0) {
          for (i = 0, empty = true; i < words && empty; i++)
            if (buf[i] != 0UL)
              empty = false;

          // count empty lines
          if (empty)
            emptyl++;
          else
            emptyl = 0;

          if (emptyl < EMPTYWMIN) {
            printf("%.8x: ", idx);
            for (i = 0; i < words; i++)
              printf("%.8x  ", buf[i]);
            printf("\n");
          } else if (emptyl == EMPTYWMIN)
            // skips empty lines
            printf("*\n");
          idx += (words * WORDLEN);
        }
      printf("\n");
    }
    fclose(inFile);
  }
  return (ret);
}

// This function loads the file contents of fileName, and prints a
// byte dump of them. It skips magic file number for .core
// and .rom file types, since it is not "seen" inside simulation.
// It skips blocks of zero-filled bytes after finding EMPTYBMIN ones.
// Returns an EXIT_SUCCESS/FAILURE code
HIDDEN int bDump(const char *prgName, const char *fileName) {
  int ret = EXIT_SUCCESS;
  FILE *inFile = NULL;
  Word tag, size;
  unsigned char buf[CBUFSIZE];
  unsigned int i, chars;
  bool empty;
  unsigned int emptyl = 0;
  unsigned int idx = 0;

  if ((inFile = fopen(fileName, "r")) == NULL) {
    fprintf(stderr, "%s: Error opening file %s: %s\n", prgName, fileName,
            strerror(errno));
    ret = EXIT_FAILURE;
  } else {
    // tries file recognition
    if (fread((void *)&tag, WORDLEN, 1, inFile) != 1 ||
        !(tag == AOUTFILEID || tag == COREFILEID || tag == BIOSFILEID)) {
      fprintf(stderr, "%s: Error opening file %s: invalid/corrupted file\n",
              prgName, fileName);
      ret = EXIT_FAILURE;
    } else {
      // file is of correct type
      if (tag == AOUTFILEID)
        rewind(inFile);
      // core and bios file tag should be skipped, since it is not
      // "seen" inside mps simulations
      else if (tag == BIOSFILEID)
        // skip size header too
        if (fread((void *)&size, WORDLEN, 1, inFile) != 1)
          if (ferror(inFile)) {
            fprintf(stderr, "%s: Error	reading file %s: %s\n", prgName,
                    fileName, strerror(errno));
            ret = EXIT_FAILURE;
          }
      // else all is ok

      while (!ferror(inFile) && !feof(inFile))
        if ((chars = fread((void *)buf, sizeof(unsigned char), CBUFSIZE,
                           inFile)) > 0) {
          for (i = 0, empty = true; i < chars && empty; i++)
            if (buf[i] != 0)
              empty = false;

          if (empty)
            // counts empty lines
            emptyl++;
          else
            emptyl = 0;

          if (emptyl < EMPTYCMIN) {
            printf("%.8x: ", idx);
            for (i = 0; i < chars; i++)
              printf("%.2x  ", buf[i]);
            printf("\n");
          } else if (emptyl == EMPTYCMIN)
            // skips empty lines
            printf("*\n");
          idx += chars;
        }
      printf("\n");
    }
    fclose(inFile);
  }
  return (ret);
}
