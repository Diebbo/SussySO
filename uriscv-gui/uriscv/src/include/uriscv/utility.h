#ifndef URISCV_UTILITY_H
#define URISCV_UTILITY_H

#include "uriscv/const.h"
#include "uriscv/types.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

extern bool DEBUG;
extern bool DISASS;

#define ERROR(msg)                                                             \
  printf("\n[x] %s\n", msg);                                                   \
  exit(-1);

#define DEBUGMSG(...)                                                          \
  if (DEBUG)                                                                   \
    printf(__VA_ARGS__);

#define DISASSMSG(...)                                                         \
  if (DISASS)                                                                  \
    printf(__VA_ARGS__);

#define ERRORMSG(...) printf(__VA_ARGS__);

namespace Utility {
void readFile(std::string filename, char *&dst, Word *size);
// Assumes little endian
void printb(size_t const size, void const *const ptr);
}; // namespace Utility

// This function sets to 1 the (bitPos % 32) bit of the word w
Word SetBit(Word w, unsigned int bitPos);

// This function resets to 0 the (bitPos % 32) bit of the word w
Word ResetBit(Word w, unsigned int bitPos);

// This function returns the bitPos bit value in w
bool BitVal(Word w, unsigned int bitPos);

// This function adds the _unsigned_ quantities a1 and a2,
// puts result into dest, and returns TRUE if a overflow occurred,
// FALSE otherwise
bool UnsAdd(Word *dest, Word a1, Word a2);

// This function subtacts the _unsigned_ quantity s2 from s1,
// puts result into dest, and returns TRUE if a underflow occurred,
// FALSE otherwise
bool UnsSub(Word *dest, Word s1, Word s2);

// This function adds the _signed_ quantities a1 and a2, puts result into
// dest (casting it to unsigned), and returns TRUE if a overflow occurred,
// FALSE otherwise
bool SignAdd(Word *dest, SWord a1, SWord a2);

// This function subtracts the _signed_ quantity s2 from s1, puts result
// into dest (casting it to unsigned), and returns TRUE if a underflow
// occurred, FALSE otherwise
bool SignSub(Word *dest, SWord s1, SWord s2);

// This function multiplies the _unsigned_ quantities m1 and m2,
// returning back the high and low part of the unsigned 64 bit result via
// hip and lop pointers
void SignMult(SWord m1, SWord m2, SWord *hip, SWord *lop);

// This function multiplies the _signed_ quantities m1 and m2,
// returning back the high and low part of the signed 64 bit result
// via hip and lop pointers
void UnsMult(Word m1, Word m2, Word *hip, Word *lop);

// This function multiplies the _unsigned_ quantity m1 and the
// __signed__quantity m2, returning back the high and low part of the unsigned
// 64 bit result via hip and lop pointers
void UnsSignMult(Word m1, SWord m2, SWord *hip, SWord *lop);

// This function prints a variable list of arguments to the standard
// error, and waits for an input to continue. Used for debugging
void trace(char *format, ...);

// This function converts a string to a Word (typically, an address) value.
// Returns TRUE if conversion was successful, FALSE otherwise
bool StrToWord(const char *str, Word *value);

uint8_t *ParseMACId(const std::string &input, uint8_t *id);

std::string MACIdToString(const uint8_t *id);

std::string IntToHexString(Word addr);

#endif
