#include "floatspecial.h"
#include <math.h>



/*!
 * Determine if a 32-bit field represents a valid 32-bit IEEE-754 floating
 * point number. If the field is infinity, NaN, or de-normalized then it is
 * not valid. This determination is made without using any floating point
 * instructions.
 * \param value is the field to evaluate
 * \return 0 if field is infinity, NaN, or de-normalized, else 1
 */
int isFloat32Valid(uint32_t value)
{
    // Five cases for floating point numbers:
    // 0) The eponent is greater than zero and less than the maximum. This is a normal non-zero number.
    // 1) The exponent and the significand are zero. This is zero.
    // 2) The exponent is zero, and the significant is non-zero. This is denormalized.
    // 3) The exponent is the maximum value, and the significand is zero. This is infinity.
    // 4) The exponent is the maximum value, and the significand is non-zero. This is NaN.
    // We check for cases 2, 3, 4 and return 0 if it happens

    if((value & 0x7F800000) == (0x7F800000))
    {
        // inifinity or NaN.
        return 0;

    }// if the exponent is the maximum value
    else if((value & 0x7F800000) == 0)
    {
        // Check for denormalized number
        if(value & 0x007FFFFF)
            return 0;

    }// else if the exponent is zero

    // If we get here then its a valid float
    return 1;

}// isFloat32Valid


/*!
 * Determine if a 64-bit field represents a valid 64-bit IEEE-754 floating
 * point number. If the field is infinity, NaN, or de-normalized then it is
 * not valid. This determination is made without using any floating point
 * instructions.
 * \param value is the field to evaluate
 * \return 0 if field is infinity, NaN, or de-normalized, else 1
 */
int isFloat64Valid(uint64_t value)
{
    // Five cases for floating point numbers:
    // 0) The eponent is greater than zero and less than the maximum. This is a normal non-zero number.
    // 1) The exponent and the significand are zero. This is zero.
    // 2) The exponent is zero, and the significant is non-zero. This is denormalized.
    // 3) The exponent is the maximum value, and the significand is zero. This is infinity.
    // 4) The exponent is the maximum value, and the significand is non-zero. This is NaN.
    // We check for cases 2, 3, 4 and return 0 if it happens

    if((value & 0x7FF0000000000000ULL) == (0x7FF0000000000000ULL))
    {
        // inifinity or NaN.
        return 0;

    }// if the exponent is the maximum value
    else if((value & 0x7FF0000000000000ULL) == 0)
    {
        // Check for denormalized number
        if(value & 0x000FFFFFFFFFFFFFULL)
            return 0;

    }// else if the exponent is zero

    // If we get here then its a valid float
    return 1;

}// isFloat64Valid


/*!
 * Convert a 32-bit floating point value (IEEE-754 binary32) to 24-bit floating
 * point value. This is done by limiting the signficand to 16 bits. Underflow
 * will be returned as zero and overflow as the maximum possible value
 * \param value is the 32-bit floating point data to convert.
 * \return The 24-bit floating point as a simple 32-bit integer with the most
 *         significant byte clear.
 */
uint32_t float32ToFloat24(float value)
{
    union
    {
        float Float;
        uint32_t Integer;
    }field;

    uint32_t significand;
    uint32_t unsignedExponent;
    uint32_t output;

    // Write the floating point value to our union so we can access its bits.
    // Note that C99 and C++2011 have built in goodness for this sort of
    // thing, but not all compilers support that (sigh...)
    field.Float = value;

    // The significand is the least significant 23 bits (IEEE754)
    significand = field.Integer & 0x007FFFFF;

    // Exponent occupies the next 8 bits (IEEE754)
    unsignedExponent = (field.Integer & 0x7F800000) >> 23;

    // Get rid of some bits, here is where we sacrifice resolution
    output = (uint32_t)(significand >> 8);

    // If significand and exponent are zero means a number of zero
    if((output == 0) && (unsignedExponent == 0))
    {
        // return correctly signed result
        if(field.Integer & 0x80000000)
            return 0x00800000;
        else
            return 0;
    }

    // Put the exponent in the output
    output |= unsignedExponent << 15;

    // Account for the sign
    if(field.Integer & 0x80000000)
        output |= 0x00800000;

    // return the 24-bit representation
    return output;

}// float32ToFloat24


/*!
 * Convert a 24-bit floating point representation to binary32 (IEEE-754)
 * \param value is the 24-bit representation to convert.
 * \return the binary32 version as a float.
 */
float float24ToFloat32(uint32_t value)
{
    union
    {
        float Float;
        uint32_t Integer;
    }field;

    // Zero is a special case
    if((value & 0x007FFFFF) == 0)
    {
        field.Integer = 0;
    }
    else
    {
        // 8 bits of exponent, biased with 127
        uint32_t unsignedExponent = (value >> 15) & 0xFF;

        // 15 bits of signficand, shift it up to 23 bits
        field.Integer = (value & 0x00007FFF) << 8;

        // Put the exponent in
        field.Integer |= (unsignedExponent << 23);
    }

    // And the sign bit
    if(value & 0x00800000)
        field.Integer |= 0x80000000;

    return field.Float;

}// float24ToFloat32


/*!
 * Convert a 32-bit floating point value (IEEE-754 binary32) to 16-bit floating
 * point value (IEEE-754 binary16). This is done by limiting the exponent to 6
 * bits and the signficand to 9 bits. Underflow will be returned as zero and
 * overflow as the maximum possible value.
 * \param value is the 32-bit floating point data to convert.
 * \return The binary16 as a simple 16-bit integer.
 */
uint16_t float32ToFloat16(float value)
{
    union
    {
        float Float;
        uint32_t Integer;
    }field;

    uint32_t significand;
    uint32_t unsignedExponent;
    int32_t  signedExponent;
    uint16_t output;

    // Write the floating point value to our union so we can access its bits.
    // Note that C99 and C++2011 have built in goodness for this sort of
    // thing, but not all compilers support that (sigh...)
    field.Float = value;

    // The significand is the least significant 23 bits (IEEE754)
    significand = field.Integer & 0x007FFFFF;

    // Exponent occupies the next 8 bits (IEEE754)
    unsignedExponent = (field.Integer & 0x7F800000) >> 23;

    // Get rid of some bits, here is where we sacrifice resolution
    output = (uint16_t)(significand >> 14);

    // If significand and exponent are zero means a number of zero
    if((output == 0) && (unsignedExponent == 0))
    {
        // return correctly signed result
        if(field.Integer & 0x80000000)
            return 0x8000;
        else
            return 0;
    }

    // Get the un-biased exponent. Binary32 is biased by 127
    signedExponent = unsignedExponent - 127;

    // With a 6-bit exponent we can support exponents of
    // exponent : biased value
    // -31      : 0 (value is zero or denormalized)
    // -30      : 1
    //  -1      : 30
    //   0      : 31
    //   1      : 32
    //  31      : 62
    //  32      : NaN (all exponent bits are 1)

    // We can support (un-biased) exponents of -62 through 63 inclusive
    if(signedExponent < -31)
        output = 0;   // underflow to zero
    else if(signedExponent > 31)
    {
        // Largest possible exponent and significand without making a NaN or Inf
        output = (uint16_t)(0x7DFF);
    }
    else
    {
        // re-bias with 31
        unsignedExponent = (uint32_t)(signedExponent + 31);

        // Put the exponent in the output
        output |= (uint16_t)(unsignedExponent << 9);
    }

    // Account for the sign
    if(field.Integer & 0x80000000)
        output |= 0x8000;

    // return the binary16 representation
    return output;

}// float32ToFloat16


/*!
 * Convert a IEEE-754 binary16 floating point representation to binary32
 * \param value is the binary16 representation to convert.
 * \return the binary32 version as a float.
 */
float float16ToFloat32(uint16_t value)
{
    union
    {
        float Float;
        uint32_t Integer;
    }field;

    // Zero is a special case
    if((value & 0x7FFF) == 0)
    {
        field.Integer = 0;
    }
    else
    {
        // 6 bits of exponent, biased with 31
        uint32_t unsignedExponent = (value >> 9) & 0x3F;

        // We want to subtract 31 to get un-biased, and then add 127 for the new bias
        unsignedExponent += (127 - 31);

        // 9 bits of signficand, shift it up to 23 bits
        field.Integer = (value & 0x01FF) << 14;

        // Put the exponent in
        field.Integer |= (unsignedExponent << 23);
    }

    // And the sign bit
    if(value & 0x8000)
        field.Integer |= 0x80000000;

    return field.Float;

}// float16ToFloat32


/*!
 * Use this routine (and a debugger) to verify the special float functionality
 * return 1 if test passed
 */
int testSpecialFloat(void)
{
    int i;
    float dataIn[6], dataOut16[6], dataOut24[6];
    float test;
    float error = 0;

    test = -.123456789f;

    for(i = 0; i < 3; i++)
    {
        test *= 10.0f;
        dataIn[i] = test;
        dataOut16[i] = float16ToFloat32(float32ToFloat16(dataIn[i]));
        dataOut24[i] = float24ToFloat32(float32ToFloat24(dataIn[i]));
        error += (float)fabs((dataIn[i] - dataOut16[i])/dataIn[i]);
        error += (float)fabs((dataIn[i] - dataOut24[i])/dataIn[i]);
    }

    test = 12.3456789f;
    for(;i < 6; i++)
    {
        test /= 10.0f;
        dataIn[i] = test;
        dataOut16[i] = float16ToFloat32(float32ToFloat16(dataIn[i]));
        dataOut24[i] = float24ToFloat32(float32ToFloat24(dataIn[i]));
        error += (float)fabs((dataIn[i] - dataOut16[i])/dataIn[i]);
        error += (float)fabs((dataIn[i] - dataOut24[i])/dataIn[i]);
    }

    if(error < 0.01f)
        return 1;
    else
        return 0;

}// testSpecialFloat
