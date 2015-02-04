#ifndef FLOATSPECIAL_H
#define FLOATSPECIAL_H

#ifdef __cplusplus
extern "C"{
#endif

/*!
 * \file
 * \brief Special routines for floating point manipulation
 *
 * These routines allow floating point values to be compressed to
 * smaller formats by discarding resolution and dynamic range. This is
 * useful for saving space in data messages for fields that have a lot of
 * dynamic range, but not a lot of required resolution.
 *
 * float24 is not defined by IEEE-754, but uses the same rules. float24 uses
 * 8-bits for the exponent (bias of 127) and 15 bits for the significand. This
 * is the same exponent range as float32, which gives a similar
 * range as the float32, but with less resolution.
 *
 * float16 does *not* uses the IEEE-754 binary16 format (also called
 * half-precision), because it does not have enough range in the exponent.
 * Instead float16 is defined using 6-bits for the exponent (bias of 31)
 * and 9 bits for the significant. The range of a float16 is therefore a
 * quarter of a float32 and float24, and the resolution is much less.
 *
 * float16 and float24 cannot be used for arithmetic. Accordingly this module
 * only provides routine to convert between these and binary32 (float). In
 * memory floating point numbers are always IEEE-754 binary32 or IEEE-754
 * binary64. The in-memory representation of a float16 or float24 is actually
 * an integer which can be encoded into a data message like any integer.
 */


#include <stdint.h>


//! Determine if a 32-bit field represents a valid 32-bit IEEE-754 floating point number.
int isFloat32Valid(uint32_t value);

//! Determine if a 64-bit field represents a valid 64-bit IEEE-754 floating point number.
int isFloat64Valid(uint64_t value);

//! Convert a 32-bit floating point value to 24-bit floating point
uint32_t float32ToFloat24(float value);

//! Convert a 24-bit floating point representation to binary32
float float24ToFloat32(uint32_t value);

//! Convert a 32-bit floating point value to 16-bit floating point
uint16_t float32ToFloat16(float value);

//! Convert a IEEE-754 binary16 floating point representation to binary32
float float16ToFloat32(uint16_t value);

//! test the special float functionality
int testSpecialFloat(void);

#ifdef __cplusplus
}
#endif
#endif // FLOATSPECIAL_H
