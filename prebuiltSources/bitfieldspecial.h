// bitfieldspecial.h was hand-coded for protogen, which copied it here

#ifndef _BITFIELDSPECIAL_H
#define _BITFIELDSPECIAL_H

// C++ compilers: don't mangle us
#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \file
 * Routines for encoding and decoding bitfields into and out of byte arrays
 */

#define __STDC_CONSTANT_MACROS
#include <stdint.h>

//! Add a bit field to a byte stream, making sure value bits in numbits
void encodeBitfield(unsigned int value, uint8_t* bytes, int* index, int* bitcount, int numbits);

//! Add a bit field to a byte stream, value must fit in numbits
void encodeBitfieldUnchecked(unsigned int value, uint8_t* bytes, int* index, int* bitcount, int numbits);

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

#ifdef UINT64_MAX

//! Add a bit field to a byte stream, making sure value fits in numbits
void encodeLongBitfield(uint64_t value, uint8_t* bytes, int* index, int* bitcount, int numbits);

//! Add a bit field to a byte stream, value must fit in numbits
void encodeLongBitfieldUnchecked(uint64_t value, uint8_t* bytes, int* index, int* bitcount, int numbits);

//! Decode a bit field from a byte stream.
uint64_t decodeLongBitfield(const uint8_t* bytes, int* index, int* bitcount, int numbits);

//! Scale a float64 to a 64 bit integer type used for bitfield
uint64_t float64ScaledToLongBitfield(double value, double min, double scaler);

//! Scale a float32 to a 64 bit integer type used for bitfield
uint64_t float32ScaledToLongBitfield(float value, float min, float scaler);

//! Scale the 64 bit integer type to a float64
double float64ScaledFromLongBitfield(uint64_t value, double min, double invscaler);

//! Scale the 64 bit integer type to a float32
float float32ScaledFromLongBitfield(uint64_t value, float min, float invscaler);

#endif // UINT64_MAX

//! Test the bit fields
int testBitfield(void);

#ifdef __cplusplus
}
#endif
#endif
