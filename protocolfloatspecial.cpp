#include "protocolfloatspecial.h"

ProtocolFloatSpecial::ProtocolFloatSpecial(ProtocolSupport protocolsupport) :
    header(protocolsupport),
    source(protocolsupport),
    support(protocolsupport)
{}

//! Perform the generation, writing out the files
bool ProtocolFloatSpecial::generate(std::vector<std::string>& fileNameList, std::vector<std::string>& filePathList)
{
    if(support.specialFloat && generateHeader())
    {
        fileNameList.push_back(header.fileName());
        filePathList.push_back(header.filePath());

        if(generateSource())
        {
            fileNameList.push_back(source.fileName());
            filePathList.push_back(source.filePath());

            return true;
        }
    }

    return false;
}


//! Generate the header file
bool ProtocolFloatSpecial::generateHeader(void)
{
    header.setModuleNameAndPath("floatspecial", support.outputpath);

// Raw string magic here
header.setFileComment(R"(\brief Special routines for floating point manipulation

These routines allow floating point values to be compressed to smaller
formats by discarding resolution and dynamic range. This is useful for
saving space in data messages for fields that have a lot of dynamic range,
but not a lot of required resolution.

float16 and float24 are not defined by IEEE-754, but use the same rules.
The most significant bit is a sign bit, the next bits are biased exponent
bits, and the remaining bits are used for the significand. float16 and
float24 have a variable number of signifcand bits, which can be adjusted as
needed to best fit the application.

Note that IEEE-754 defines a binary16 format (also called half-precision),
which uses a 10-bit significand (therefore 5 bits of exponent). float16 with
10 bits significand is the same as IEEE-754 half precision.

float16 and float24 cannot be used for arithmetic. Accordingly this module
only provides routines to convert between these and binary32 (float). In
memory floating point numbers are always IEEE-754 binary32 or IEEE-754
binary64. The in-memory representation of a float16 or float24 is actually
an integer which can be encoded into a data message like any integer.)");

header.makeLineSeparator();
header.writeIncludeDirective("stdint.h", std::string(), true);
header.makeLineSeparator();

// Raw string magic here
header.write(R"(//! Determine if a 32-bit field represents a valid 32-bit IEEE-754 floating point number.
int isFloat32Valid(uint32_t value);

//! Determine if a 64-bit field represents a valid 64-bit IEEE-754 floating point number.
int isFloat64Valid(uint64_t value);

//! Convert a 32-bit floating point value to 24-bit floating point
uint32_t float32ToFloat24(const float value, const uint8_t sigbits);

//! Convert a IEEE-754 binary24 floating point representation to binary32
float float24ToFloat32(const uint32_t value, const uint8_t sigbits);

//! Convert a 32-bit floating point value to 16-bit floating point representation
uint16_t float32ToFloat16(const float value, const uint8_t sigbits);

//! Convert a 16 bit floating point representation to binary32
float float16ToFloat32(const uint16_t value, const uint8_t sigbits);

//! test the special float functionality
int testSpecialFloat(void);)");

header.makeLineSeparator();

return header.flush();

}// ProtocolFloatSpecial::generateHeader

//! Generate the encode source file
bool ProtocolFloatSpecial::generateSource(void)
{
    source.setModuleNameAndPath("floatspecial", support.outputpath);
    source.writeIncludeDirective("math.h", "", true);
    source.makeLineSeparator();

    // Raw string magic here
source.write(R"===(// Exponent bias is 2 raised to the number of exponent bits divided
// by two, minus 1. This can be simplified: 2^(exponent bits -1) - 1
// The number of exponent bits is 16 - 1 - sigbits
#define COMPUTE_BIAS(width, sigbits) ((1 << (((width) - 2) - (sigbits))) - 1)

// Maximum value that can be stored in an unsigned significand
#define COMPUTE_MAX_SIG(sigbits)     ((1 << (sigbits)) - 1)

// Number of signficand bits (not counting leading 1)
#define BINARY32_SIG_BITS 23            // IEEE-754 Standard

// Number of exponent bits
#define BINARY32_EXP_BITS 8            // IEEE-754 Standard

// Mask for the exponent bits
#define BINARY32_EXP_MASK 0x7F800000ul  // COMPUTE_MAX_SIG(BINARY32_EXP_BITS) << BINARY32_SIG_BITS

// Mask for the significand bits (not counting leading 1)
#define BINARY32_SIG_MASK 0x007FFFFFul  // COMPUTE_MAX_SIG(BINARY32_SIG_BITS)

// Mask for the location of the implied leading 1
#define BINARY32_LEADING1 0x00800000ul  // BINARY32_SIG_MASK + 1

// Exponent bias
#define BINARY32_BIAS     127           // COMPUTE_BIAS(32, BINARY32_SIG_BITS)

// Number of signficand bits (not counting leading 1)
#define BINARY64_SIG_BITS 52                    // IEEE-754 Standard

// Number of exponent bits
#define BINARY64_EXP_BITS 11                    // IEEE-754 Standard

// Mask for the exponent bits
#define BINARY64_EXP_MASK 0x7FF0000000000000ULL  // COMPUTE_MAX_SIG(BINARY64_EXP_BITS) << BINARY64_SIG_BITS

// Mask for the significand bits (not counting leading 1)
#define BINARY64_SIG_MASK 0x000FFFFFFFFFFFFFULL  // COMPUTE_MAX_SIG(BINARY64_SIG_BITS)

// Mask for the location of the implied leading 1
#define BINARY64_LEADING1 0x0010000000000000ULL  // BINARY64_SIG_MASK + 1

// Exponent bias
#define BINARY64_BIAS     1023                  // COMPUTE_BIAS(64, BINARY64_SIG_BITS)

//! Right shift a 32-bit number with rounding and overflow protection
uint32_t rightShift32To32(uint32_t input, uint8_t shift, uint32_t max);

//! Right shift a 32-bit number to 16 bits with rounding and overflow protection
uint16_t rightShift32To16(uint32_t input, uint8_t shift, uint16_t max);

//! Right shift a 16-bit number with rounding and overflow protection
uint16_t rightShift16To16(uint16_t input, uint8_t shift, uint16_t max);

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
    // 0) The exponent is greater than zero and less than the maximum. This is a normal non-zero number.
    // 1) The exponent and the significand are zero. This is zero.
    // 2) The exponent is zero, and the significant is non-zero. This is denormalized.
    // 3) The exponent is the maximum value, and the significand is zero. This is infinity.
    // 4) The exponent is the maximum value, and the significand is non-zero. This is NaN.
    // We check for cases 2, 3, 4 and return 0 if it happens

    if((value & BINARY32_EXP_MASK) == (BINARY32_EXP_MASK))
    {
        // inifinity or NaN.
        return 0;

    }// if the exponent is the maximum value
    else if((value & BINARY32_EXP_MASK) == 0)
    {
        // Check for denormalized number
        if(value & BINARY32_SIG_MASK)
            return 0;

    }// else if the exponent is zero

    // If we get here its a valid float
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
    // 0) The exponent is greater than zero and less than the maximum. This is a normal non-zero number.
    // 1) The exponent and the significand are zero. This is zero.
    // 2) The exponent is zero, and the significant is non-zero. This is denormalized.
    // 3) The exponent is the maximum value, and the significand is zero. This is infinity.
    // 4) The exponent is the maximum value, and the significand is non-zero. This is NaN.
    // We check for cases 2, 3, 4 and return 0 if it happens

    if((value & BINARY64_EXP_MASK) == (BINARY64_EXP_MASK))
    {
        // inifinity or NaN.
        return 0;

    }// if the exponent is the maximum value
    else if((value & BINARY64_EXP_MASK) == 0)
    {
        // Check for denormalized number
        if(value & BINARY64_SIG_MASK)
            return 0;

    }// else if the exponent is zero

    // If we get here its a valid float
    return 1;

}// isFloat64Valid


/*!
 * Convert a 32-bit floating point value (IEEE-754 binary32) to 24-bit floating
 * point representation with a variable number of bits for the significand.
 * Underflow will be returned as denormalized or zero and overflow as the
 * maximum possible value.
 * \param value is the 32-bit floating point data to convert.
 * \param sigbits is the number of bits to use for the significand, and must be
 *        between 4 and 20 bits inclusive.
 * \return The float24 as a simple 24-bit integer (most significant byte clear).
 */
uint32_t float32ToFloat24(const float value, const uint8_t sigbits)
{
    union FP32
    {
        float Float;
        uint32_t Integer;
    };

    // Write the floating point value to our union so we can access its bits.
    // Note that C99 and C++2011 have built in goodness for this sort of
    // thing, but not all compilers support that (sigh...)
    union FP32 field = {.Float = value};

    // Exponent occupies 8 bits after the sign (IEEE754 Binary32)
    uint32_t unsignedExponent = (field.Integer & BINARY32_EXP_MASK) >> BINARY32_SIG_BITS;

    // Get the un-biased exponent. Binary32 is biased by 127
    int32_t signedExponent = unsignedExponent - BINARY32_BIAS;

    // Bias based on width and significant bits
    int32_t bias = COMPUTE_BIAS(24, sigbits);

    // The significand is the least significant 23 bits (IEEE754 Binary32)
    uint32_t significand = (field.Integer & BINARY32_SIG_MASK);

    // Maximum value of the significand
    uint32_t maxsignificand = (uint32_t)COMPUTE_MAX_SIG(sigbits);

    // Throw away resolution. Notice that this may round up
    uint32_t output = rightShift32To32(significand, BINARY32_SIG_BITS - sigbits, maxsignificand);

    // With a 6-bit exponent we can support exponents of
    // exponent : biased value
    // -31      : 0 (value is zero or denormalized)
    // -30      : 1
    //  -1      : 30
    //   0      : 31
    //   1      : 32
    //  31      : 62
    //  32      : NaN (all exponent bits are 1)

    // With a 5-bit exponent we get
    // exponent : biased value
    // -15      : 0 (value is zero or denormalized)
    // -14      : 1
    //  -1      : 14
    //   0      : 15
    //   1      : 16
    //  15      : 32
    //  16      : NaN (all exponent bits are 1)

    if(signedExponent <= -bias)
    {
        // Not enough exponent bits; the result needs to be de-normalized.
        // This means we have to shift right 1 for each missing exponent,
        // plus 1 more for the implied leading 1. And we need to include
        // the implied leading 1, since it is no longer implied.
        uint8_t shiftneeded = 1 + (-bias -signedExponent);

        // Include the implied leading 1
        output |= (1 << sigbits);

        if(shiftneeded < 16)
            output = rightShift32To32(output, shiftneeded, maxsignificand);
        else
            output = 0;   // underflow to zero
    }
    else
    {
        if(signedExponent > bias)
        {
            // Largest possible exponent and significand without making a NaN or Inf
            signedExponent = bias;
            output = maxsignificand;
        }

        // re-bias with the new bias
        unsignedExponent = (uint32_t)(signedExponent + bias);

        // Put the exponent in the output
        output |= (uint32_t)(unsignedExponent << sigbits);
    }

    // Account for the sign
    if(field.Integer & 0x80000000ul)
        output |= 0x800000;

    // return the binary24 representation
    return output;

}// float32ToFloat24


/*!
 * Convert a 24-bit floating point representation with variable number of
 * significand bits to binary32
 * \param value is the float16 representation to convert.
 * \param sigbits is the number of bits to use for the significand of the
 *        24-bit float, and must be between 4 and 20 bits inclusive.
 * \return the binary32 version as a float.
 */
float float24ToFloat32(const uint32_t value, const uint8_t sigbits)
{
    union
    {
        float Float;
        uint32_t Integer;
    }field;

    // Zero is a special case
    if((value & 0x7FFFFF) == 0)
    {
        field.Integer = 0;
    }
    else
    {
        // Maximum value of the significand
        uint32_t sigmask = COMPUTE_MAX_SIG(sigbits);

        // Bias based on width and significant bits
        int32_t bias = COMPUTE_BIAS(24, sigbits);

        // The unsigned exponent, mask off the leading sign bit
        uint32_t unsignedExponent = ((value & 0x7FFFFF) >> sigbits);

        // If unsignedExponent is zero this number is denormalized, but this
        // will change when it gets rebiased. The leading 1 needs to be handled
        // specially in this case.
        if(unsignedExponent == 0)
        {
            // Mask off all but the significand bits and shift into place,
            // shifting one extra bit because we are denormalized.
            field.Integer = (value & sigmask) << ((BINARY32_SIG_BITS+1) - sigbits);

            // Rebias to 127
            unsignedExponent += (BINARY32_BIAS - bias);

            // This is like scientific notation: keep shifting left until the
            // leading bit is a 1 then mask if off (because it is implied now
            // that we are not de-normalzed). Note that if we are here
            // field.Integer is guaranteed to be nonzero.
            while((field.Integer & 0x00800000ul) == 0)
            {
                field.Integer <<= 1;
                unsignedExponent--;
            }

            // Mask off the bit we just shifted out of significand
            field.Integer &= BINARY32_SIG_MASK;
        }
        else
        {
            // Mask off all but the significand bits and shift into place
            field.Integer = (value & sigmask) << (BINARY32_SIG_BITS - sigbits);

            // Rebias to 127
            unsignedExponent += (BINARY32_BIAS - bias);
        }

        // Put the exponent in
        field.Integer |= (unsignedExponent << BINARY32_SIG_BITS);
    }

    // And the sign bit
    if(value & 0x800000)
        field.Integer |= 0x80000000ul;

    return field.Float;

}// float24ToFloat32


/*!
 * Convert a 32-bit floating point value (IEEE-754 binary32) to 16-bit floating
 * point representation with a variable number of bits for the significand.
 * Underflow will be returned as denormalized or zero and overflow as the
 * maximum possible value.
 * \param value is the 32-bit floating point data to convert.
 * \param sigbits is the number of bits to use for the significand, and must be
 *        between 4 and 12 bits inclusive.
 * \return The float16 as a simple 16-bit integer.
 */
uint16_t float32ToFloat16(const float value, const uint8_t sigbits)
{
    union FP32
    {
        float Float;
        uint32_t Integer;
    };

    // Write the floating point value to our union so we can access its bits.
    // Note that C99 and C++2011 have built in goodness for this sort of
    // thing, but not all compilers support that (sigh...)
    union FP32 field = {.Float = value};

    // Exponent occupies 8 bits after the sign (IEEE754 Binary32)
    uint32_t unsignedExponent = (field.Integer & BINARY32_EXP_MASK) >> BINARY32_SIG_BITS;

    // Get the un-biased exponent. Binary32 is biased by 127
    int32_t signedExponent = unsignedExponent - BINARY32_BIAS;

    // Bias based on width and significant bits
    int32_t bias = COMPUTE_BIAS(16, sigbits);

    // The significand is the least significant 23 bits (IEEE754 Binary32)
    uint32_t significand = (field.Integer & BINARY32_SIG_MASK);

    // Maximum value of the significand
    uint16_t maxsignificand = (uint16_t)COMPUTE_MAX_SIG(sigbits);

    // Throw away resolution. Notice that this may round up
    uint16_t output = rightShift32To16(significand, BINARY32_SIG_BITS - sigbits, maxsignificand);

    // With a 6-bit exponent we can support exponents of
    // exponent : biased value
    // -31      : 0 (value is zero or denormalized)
    // -30      : 1
    //  -1      : 30
    //   0      : 31
    //   1      : 32
    //  31      : 62
    //  32      : NaN (all exponent bits are 1)

    // With a 5-bit exponent we get
    // exponent : biased value
    // -15      : 0 (value is zero or denormalized)
    // -14      : 1
    //  -1      : 14
    //   0      : 15
    //   1      : 16
    //  15      : 32
    //  16      : NaN (all exponent bits are 1)

    if(signedExponent <= -bias)
    {
        // Not enough exponent bits; the result needs to be de-normalized.
        // This means we have to shift right 1 for each missing exponent,
        // plus 1 more for the implied leading 1. And we need to include
        // the implied leading 1, since it is no longer implied.
        uint8_t shiftneeded = 1 + (-bias -signedExponent);

        // Include the implied leading 1
        output |= (1 << sigbits);

        if(shiftneeded < 16)
            output = rightShift16To16(output, shiftneeded, maxsignificand);
        else
            output = 0;   // underflow to zero
    }
    else
    {
        if(signedExponent > bias)
        {
            // Largest possible exponent and significand without making a NaN or Inf
            signedExponent = bias;
            output = maxsignificand;
        }

        // re-bias with the new bias
        unsignedExponent = (uint32_t)(signedExponent + bias);

        // Put the exponent in the output
        output |= (uint16_t)(unsignedExponent << sigbits);
    }

    // Account for the sign
    if(field.Integer & 0x80000000ul)
        output |= 0x8000;

    // return the binary16 representation
    return output;

}// float32ToFloat16


/*!
 * Convert a 16-bit floating point representation with variable number of
 * significand bits to binary32
 * \param value is the float16 representation to convert.
 * \param sigbits is the number of bits to use for the significand of the
 *        16-bit float, and must be between 4 and 12 bits inclusive.
 * \return the binary32 version as a float.
 */
float float16ToFloat32(const uint16_t value, const uint8_t sigbits)
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
        // Maximum value of the significand
        uint16_t sigmask = COMPUTE_MAX_SIG(sigbits);

        // Bias based on width and significant bits
        int32_t bias = COMPUTE_BIAS(16, sigbits);

        // The unsigned exponent, mask off the leading sign bit
        uint32_t unsignedExponent = ((value & 0x7FFF) >> sigbits);

        // If unsignedExponent is zero this number is denormalized, but this
        // will change when it gets rebiased. The leading 1 needs to be handled
        // specially in this case.
        if(unsignedExponent == 0)
        {
            // Mask off all but the significand bits and shift into place,
            // shifting one extra bit because we are denormalized.
            field.Integer = (value & sigmask) << ((BINARY32_SIG_BITS+1) - sigbits);

            // Rebias to 127
            unsignedExponent += (BINARY32_BIAS - bias);

            // This is like scientific notation: keep shifting left until the
            // leading bit is a 1 then mask if off (because it is implied now
            // that we are not de-normalzed). Note that if we are here
            // field.Integer is guaranteed to be nonzero.
            while((field.Integer & 0x00800000ul) == 0)
            {
                field.Integer <<= 1;
                unsignedExponent--;
            }

            // Mask off the bit we just shifted out of significand
            field.Integer &= BINARY32_SIG_MASK;
        }
        else
        {
            // Mask off all but the significand bits and shift into place
            field.Integer = (value & sigmask) << (BINARY32_SIG_BITS - sigbits);

            // Rebias to 127
            unsignedExponent += (BINARY32_BIAS - bias);
        }

        // Put the exponent in
        field.Integer |= (unsignedExponent << BINARY32_SIG_BITS);
    }

    // And the sign bit
    if(value & 0x8000)
        field.Integer |= 0x80000000ul;

    return field.Float;

}// float16ToFloat32


/*!
 * Given a 32-bit input number right shift it into a 32-bit value, rounding
 * based on the discarded bits.
 * \param input is the input number to right shift.
 * \param shift is the number of bits to shift from 1 to 31.
 * \param max is the maximum allowed value following shift and round.
 * \return the shifted and rounded number.
 */
uint32_t rightShift32To32(uint32_t input, const uint8_t shift, const uint32_t max)
{
    // This is a `1` bit in the most significant position that will be discarded
    uint32_t roundmask = 1 << (shift-1);

    // Add this to the input, if the msb_discarded is 0 this will not cause an
    // increment to the next bit, otherwise it will.
    input += roundmask;

    // Do the shift
    input >>= shift;
    if(input >= max)
        return max;
    else
        return input;

}// rightShift32To32


/*!
 * Given a 32-bit input number right shift it into a 16-bit value, rounding
 * based on the discarded bits.
 * \param input is the input number to right shift.
 * \param shift is the number of bits to shift from 1 to 31.
 * \param max is the maximum allowed value following shift and round.
 * \return the shifted and rounded number.
 */
uint16_t rightShift32To16(uint32_t input, const uint8_t shift, const uint16_t max)
{
    // This is a `1` bit in the most significant position that will be discarded
    uint32_t roundmask = 1 << (shift-1);

    // Add this to the input, if the msb_discarded is 0 this will not cause an
    // increment to the next bit, otherwise it will.
    input += roundmask;

    // Do the shift
    input >>= shift;
    if(input >= max)
        return max;
    else
        return (uint16_t)input;

}// rightShift32To16


/*!
 * Given a 16-bit input number right shift it into a 16-bit value, rounding
 * based on the discarded bits.
 * \param input is the input number to right shift.
 * \param shift is the number of bits to shift from 1 to 15.
 * \param max is the maximum allowed value following shift and round.
 * \return the shifted and rounded number.
 */
uint16_t rightShift16To16(uint16_t input, const uint8_t shift, const uint16_t max)
{
    // This is a `1` bit in the most significant position that will be discarded
    uint16_t roundmask = 1 << (shift-1);

    // Add this to the input, if the msb_discarded is 0 this will not cause an
    // increment to the next bit, otherwise it will.
    input += roundmask;

    // Do the shift
    input >>= shift;
    if(input >= max)
        return max;
    else
        return input;

}// rightShift16To16


/*!
 * Use this routine (and a debugger) to verify the special float functionality
 * \return 1 if test passed
 */
int testSpecialFloat(void)
{
    int i;
    float dataIn[6], dataOut16[6], dataOut24[6];

    union
    {
        float Float;
        uint32_t Integer;
    }test;

    float error = 0;

    test.Float = -.123456789f;

    for(i = 0; i < 3; i++)
    {
        test.Float *= 10.0f;
        dataIn[i] = test.Float;
        dataOut16[i] = float16ToFloat32(float32ToFloat16(dataIn[i], 9), 9);
        dataOut24[i] = float24ToFloat32(float32ToFloat24(dataIn[i], 15), 15);
        error += (float)fabs((dataIn[i] - dataOut16[i])/dataIn[i]);
        error += (float)fabs((dataIn[i] - dataOut24[i])/dataIn[i]);
    }

    test.Float = 12.3456789f;
    for(;i < 6; i++)
    {
        test.Float /= 10.0f;
        dataIn[i] = test.Float;
        dataOut16[i] = float16ToFloat32(float32ToFloat16(dataIn[i], 9), 9);
        dataOut24[i] = float24ToFloat32(float32ToFloat24(dataIn[i], 15), 15);
        error += (float)fabs((dataIn[i] - dataOut16[i])/dataIn[i]);
        error += (float)fabs((dataIn[i] - dataOut24[i])/dataIn[i]);
    }

    if(error > 0.01f)
        return 0;

    // Test rounding
    test.Float = float16ToFloat32(float32ToFloat16(33.34f, 10), 10);
    if(test.Float < 33.34f)
        return 0;

    test.Float = float16ToFloat32(float32ToFloat16(33.32f, 10), 10);
    if(test.Float > 33.32f)
        return 0;

    // Maximum possible float without Inf or Nan
    test.Integer = 0x7F7FFFFF;

    // This loop exercises the overflow and underflow, use the debugger to verify functionality
    for(i = 0; i < 6; i++)
    {
        dataIn[i] = test.Float;
        dataOut16[i] = float16ToFloat32(float32ToFloat16(dataIn[i], 9), 9);
        dataOut24[i] = float24ToFloat32(float32ToFloat24(dataIn[i], 15), 15);
        test.Float /= 1000000000000.0f;
    }

    return 1;

}// testSpecialFloat)===");

    source.makeLineSeparator();

    return source.flush();

}// ProtocolFloatSpecial::generateSource

