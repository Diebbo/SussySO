#include "uriscv/utility.h"
#include "uriscv/const.h"
#include "uriscv/types.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#include <algorithm>
#include <ctype.h>
#include <sstream>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

bool DEBUG = false;
bool DISASS = false;

void Utility::readFile(std::string filename, char *&dst, Word *size) {
  std::ifstream file(filename,
                     std::ios::in | std::ifstream::ate | std::ios::binary);
  std::string buffer, line;
  if (file.is_open()) {
    file.seekg(0, file.end);
    *size = file.tellg();
    file.seekg(0, file.beg);
    dst = new char[*size];
    file.read(dst, *size);
    file.close();
    printf("File size %d\n", *size);
  } else {
    std::cout << "File not found!\n";
  }
}
void Utility::printb(size_t const size, void const *const ptr) {
  unsigned char *b = (unsigned char *)ptr;
  unsigned char byte;
  int i, j;

  for (i = size - 1; i >= 0; i--) {
    for (j = 7; j >= 0; j--) {
      byte = (b[i] >> j) & 1;
      printf("%u", byte);
    }
  }
  puts("");
}

// This function sets to 1 the (bitPos % 32) bit of the word w
Word SetBit(Word w, unsigned bitPos) { return (w | (1 << bitPos)); }

// This function resets to 0 the (bitPos % 32) bit of the word w
Word ResetBit(Word w, unsigned bitPos) {
  return (w & ((~0) ^ SetBit(0, bitPos)));
}

// This function returns the bitPos bit value in w
bool BitVal(Word w, unsigned int bitPos) { return (w >> bitPos) & 1UL; }

// This function adds the _unsigned_ quantities a1 and a2,
// puts result into dest, and returns TRUE if a overflow occurred,
// FALSE otherwise
bool UnsAdd(Word *dest, Word a1, Word a2) {
  *dest = a1 + a2;
  return ~a1 < a2;
}

// This function subtacts the _unsigned_ quantity s2 from s1,
// puts result into dest, and returns TRUE if a underflow occurred,
// FALSE otherwise
bool UnsSub(Word *dest, Word s1, Word s2) {
  *dest = s1 - s2;
  return s1 < s2;
}

// This function adds the _signed_ quantities a1 and a2, puts result into
// dest (casting it to unsigned), and returns TRUE if a overflow occurred,
// FALSE otherwise
bool SignAdd(Word *dest, SWord a1, SWord a2) {
  *dest = (Word)(a1 + a2);
  return (SIGNBIT(a1) == SIGNBIT(a2) && SIGNBIT(*dest) != SIGNBIT(a1));
}

// This function subtracts the _signed_ quantity s2 from s1, puts result
// into dest (casting it to unsigned), and returns TRUE if a underflow
// occurred, FALSE otherwise
bool SignSub(Word *dest, SWord s1, SWord s2) {
  *dest = (Word)(s1 - s2);
  return (SIGNBIT(s1) != SIGNBIT(s2) && SIGNBIT(*dest) != SIGNBIT(s1));
}

// This function multiplies the _unsigned_ quantities m1 and m2,
// returning back the high and low part of the unsigned 64 bit result via
// hip and lop pointers
// Algorithm used is "classical":
// given the 32 bit quantities AB and CD, divided into high and low 16 bit
// parts A, B, C and D
//
//              AB x
//              CD =
//             ------
//             AD.BD +
//          AC.BC.0
//         ----------
//
// where AD, BD etc. are the 32 bit results of the multiplication of A by D,
// etc., and X.Y means (X << 16) + Y to allow the addition of the intermediate
// results.
// This chunk of code (C) J. Larus (SPIM, 1990) (with minor modifications)
void UnsMult(Word m1, Word m2, Word *hip, Word *lop) {
  Word a, b, c, d, x, y;

  a = (m1 & ~(IMMMASK)) >> HWORDLEN;
  b = (m1 & IMMMASK);
  c = (m2 & ~(IMMMASK)) >> HWORDLEN;
  d = (m2 & IMMMASK);

  *lop = b * d;
  x = (a * d) + (b * c);
  y = (((*lop) >> HWORDLEN) & IMMMASK) + x;

  *lop = ((*lop) & IMMMASK) | ((y & IMMMASK) << HWORDLEN);
  *hip = ((y >> HWORDLEN) & IMMMASK) + (a * c);
}

// This function multiplies the _signed_ quantities m1 and m2,
// returning back the high and low part of the signed 64 bit result
// via hip and lop pointers
// This too (C) J. Larus (SPIM, 1990)
void SignMult(SWord m1, SWord m2, SWord *hip, SWord *lop) {
  bool negResult = false;

  // convert negative numbers to positive for unsigned multipl.
  // and keep track of result sign
  if (m1 < 0) {
    negResult = !negResult;
    m1 = -m1;
  }
  if (m2 < 0) {
    negResult = !negResult;
    m2 = -m2;
  }

  UnsMult((Word)m1, (Word)m2, (Word *)hip, (Word *)lop);

  if (negResult) {
    // must 2-complement result (and keep count of *lop -> *hip carry)

    // 1-complement
    *hip = ~(*hip);
    *lop = ~(*lop);

    // add 1 to lower word to get 2-complement and check for carry
    if (UnsAdd((Word *)lop, (Word)(*lop), 1UL))
      // overflow occurred: carry out to hip
      (*hip)++;
  }
}

// This function multiplies the _signed_ quantities m1 and m2,
// returning back the high and low part of the signed 64 bit result
// via hip and lop pointers
// This too (C) J. Larus (SPIM, 1990)
void UnsSignMult(Word m1, SWord m2, SWord *hip, SWord *lop) {
  bool negResult = false;

  // convert negative numbers to positive for unsigned multipl.
  // and keep track of result sign
  if (m1 < 0) {
    negResult = !negResult;
    m1 = -m1;
  }

  UnsMult((Word)m1, (Word)m2, (Word *)hip, (Word *)lop);

  if (negResult) {
    // must 2-complement result (and keep count of *lop -> *hip carry)

    // 1-complement
    *hip = ~(*hip);
    *lop = ~(*lop);

    // add 1 to lower word to get 2-complement and check for carry
    if (UnsAdd((Word *)lop, (Word)(*lop), 1UL))
      // overflow occurred: carry out to hip
      (*hip)++;
  }
}

// This function prints a variable list of arguments to the standard
// error, and waits for an input to continue. Used for debugging
void trace(char *format, ...) {
  va_list args;

  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);

  getchar();
}

// This function converts a string to a Word (typically, an address) value.
// Returns TRUE if conversion was successful, FALSE otherwise
bool StrToWord(const char *str, Word *value) {
  char *endp;
  bool valid = true;

  // tries to convert the string into a unsigned long
  *value = strtoul(str, &endp, 0);

  if (endp != NULL) {
    // there may be some garbage
    while (*endp != EOS && valid) {
      if (!isspace(*endp))
        valid = false;
      endp++;
    }
  }
  return (valid);
}

uint8_t *ParseMACId(const std::string &input, uint8_t *id) {
  unsigned int groups[6];
  if (sscanf(input.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x", &groups[0],
             &groups[1], &groups[2], &groups[3], &groups[4], &groups[5]) != 6)
    return NULL;

  if (groups[0] % 2)
    return NULL;

  std::copy(groups, groups + 6, id);
  return id;
}

std::string MACIdToString(const uint8_t *id) {
  char buf[6 * 3 + 1];
  sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", (unsigned int)id[0],
          (unsigned int)id[1], (unsigned int)id[2], (unsigned int)id[3],
          (unsigned int)id[4], (unsigned int)id[5]);
  return std::string(buf);
}

std::string IntToHexString(Word addr) {
  std::stringstream stream;
  stream << "0x" << std::setfill('0') << std::setw(sizeof(Word) * 2) << std::hex
         << addr;
  return stream.str();
}
