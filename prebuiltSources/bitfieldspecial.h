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

//! Scale a float64 to the base integer type used for bitfield
unsigned int float64ScaledToBitfield(double value, double min, double scaler);

//! Scale a float32 to the base integer type used for bitfield
unsigned int float32ScaledToBitfield(float value, float min, float scaler);

//! Scale the bitfield base integer type to a float64
double float64ScaledFromBitfield(unsigned int value, double min, double invscaler);

//! Scale the bitfield base integer type to a float32
float float32ScaledFromBitfield(unsigned int value, float min, float invscaler);

//! Test the bit fields
int testBitfield(void);

#ifdef __cplusplus
}
#endif
#endif // BITFIELDSPECIAL_H
