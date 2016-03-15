#ifndef BITFIELDSPECIAL_H
#define BITFIELDSPECIAL_H

// C++ compilers: don't mangle us
#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \file
 * Routines for encoding and decoding bitfields into and out of byte arrays
 */

#include <stdint.h>

//! Add a bit field to a byte stream.
void encodeBitfield(unsigned int value, uint8_t* bytes, int* index, int* bitcount, int numbits);

//! Decode a bit field from a byte stream.
unsigned int decodeBitfield(const uint8_t* bytes, int* index, int* bitcount, int numbits);

//! Test the bit fields
int testBitfield(void);

#ifdef __cplusplus
}
#endif
#endif // BITFIELDSPECIAL_H
