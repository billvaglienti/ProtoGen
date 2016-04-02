#include "protocolbitfield.h"

ProtocolBitfield::ProtocolBitfield(ProtocolSupport sup) :
    support(sup)
{
}


/*!
 * Generate the source and header files for bitfield support
 */
void ProtocolBitfield::generate(void)
{
    generateHeader();
    generateSource();
}


/*!
 * Generate the header file for bitfield support
 */
void ProtocolBitfield::generateHeader(void)
{
    header.setModuleName("bitfieldspecial");
    header.setVersionOnly(true);

    // Make sure empty
    header.clear();

    // Top level comment
    header.write(
"/*!\n\
 * \\file\n\
 * Routines for encoding and decoding bitfields into and out of byte arrays\n\
 */\n");

    header.write("\n");
    header.write("#include <stdint.h>\n");
    header.write("\n");
    header.write("//! Add a bit field to a byte stream.\n");
    header.write("void encodeBitfield(unsigned int value, uint8_t* bytes, int* index, int* bitcount, int numbits);\n");
    header.write("\n");
    header.write("//! Decode a bit field from a byte stream.\n");
    header.write("unsigned int decodeBitfield(const uint8_t* bytes, int* index, int* bitcount, int numbits);\n");

    if(support.float64)
    {
        header.write("\n");
        header.write("//! Scale a float64 to the base integer type used for bitfield\n");
        header.write("unsigned int float64ScaledToBitfield(double value, double min, double scaler);\n");
    }

    header.write("\n");
    header.write("//! Scale a float32 to the base integer type used for bitfield\n");
    header.write("unsigned int float32ScaledToBitfield(float value, float min, float scaler);\n");

    if(support.float64)
    {
        header.write("\n");
        header.write("//! Scale the bitfield base integer type to a float64\n");
        header.write("double float64ScaledFromBitfield(unsigned int value, double min, double invscaler);\n");
    }

    header.write("\n");
    header.write("//! Scale the bitfield base integer type to a float32\n");
    header.write("float float32ScaledFromBitfield(unsigned int value, float min, float invscaler);\n");

    if(support.longbitfield)
    {
        header.write("\n");
        header.write("//! Add a bit field to a byte stream.\n");
        header.write("void encodeLongBitfield(uint64_t value, uint8_t* bytes, int* index, int* bitcount, int numbits);\n");
        header.write("\n");
        header.write("//! Decode a bit field from a byte stream.\n");
        header.write("uint64_t decodeLongBitfield(const uint8_t* bytes, int* index, int* bitcount, int numbits);\n");

        if(support.float64)
        {
            header.write("\n");
            header.write("//! Scale a float64 to a 64 bit integer type used for bitfield\n");
            header.write("uint64_t float64ScaledToLongBitfield(double value, double min, double scaler);\n");
        }

        header.write("\n");
        header.write("//! Scale a float32 to a 64 bit integer type used for bitfield\n");
        header.write("uint64_t float32ScaledToLongBitfield(float value, float min, float scaler);\n");

        if(support.float64)
        {
            header.write("\n");
            header.write("//! Scale the 64 bit integer type to a float64\n");
            header.write("double float64ScaledFromLongBitfield(uint64_t value, double min, double invscaler);\n");
        }

        header.write("\n");
        header.write("//! Scale the 64 bit integer type to a float32\n");
        header.write("float float32ScaledFromLongBitfield(uint64_t value, float min, float invscaler);\n");
    }

    if(support.bitfieldtest)
    {
        header.write("\n");
        header.write("//! Test the bit fields\n");
        header.write("int testBitfield(void);\n");
    }

    header.write("\n");
    header.flush();
}


/*!
 * Generate the source file for bitfield support
 */
void ProtocolBitfield::generateSource(void)
{
    source.setModuleName("bitfieldspecial");
    source.setVersionOnly(true);

    // Make sure empty
    source.clear();

    if(support.bitfieldtest)
        source.write("#include <string.h>\n\n");

    source.write("\
/*!\n\
 * Add a bit field to a byte stream.\n\
 * \\param value is the unsigned integer to encode, which can vary from 1 to 32\n\
 *        bits in length. The bits encoded are the least significant (right\n\
 *        most) bits of value\n\
 * \\param bytes is the byte stream to receive the bits\n\
 * \\param index is the current byte stream index, which will be incremented as\n\
 *        needed.\n\
 * \\param bitcount is the current bit counter index in the current byte, which\n\
 *        will be incremented as needed.\n\
 * \\param numbits is the number of bits in value to encode\n\
 */\n\
void encodeBitfield(unsigned int value, uint8_t* bytes, int* index, int* bitcount, int numbits)\n\
{\n\
    int bitstomove;\n\
    while(numbits > 0)\n\
    {\n\
        // Start out with all bits zero\n\
        if((*bitcount) == 0)\n\
            bytes[*index] = 0;\n\
\n\
        // imagine that bitcount is currently 2, i.e. the 2 left most bits\n\
        // have already been encoded. In that case we want to encode 6 more\n\
        // bits in the current byte.\n\
        bitstomove = 8 - (*bitcount);\n\
\n\
        // shift value to the correct alignment.\n\
        if(bitstomove >= numbits)\n\
        {\n\
            // In this case all the bits in value will fit in the current byte\n\
            bytes[*index] |= (uint8_t)(value << (bitstomove - numbits));\n\
            (*bitcount) += numbits;\n\
            numbits = 0;\n\
        }\n\
        else\n\
        {\n\
            // In this case we have too many bits in value to fit in the\n\
            // current byte, encode the most significant bits that fit\n\
            bytes[*index] |= (uint8_t)(value >> (numbits - bitstomove));\n\
            (*bitcount) += bitstomove;\n\
            numbits -= bitstomove;\n\
        }\n\
\n\
        // Check if we have moved to the next byte\n\
        if((*bitcount) >= 8)\n\
        {\n\
            (*index)++;\n\
            (*bitcount) = 0;\n\
        }\n\
\n\
    }// while still bits to encode\n\
\n\
}// encodeBitfield\n\
\n\
\n\
/*! Decode a bit field from a byte stream.\n\
 * \\param bytes is the byte stream from where the bitfields are taken\n\
 * \\param index is the current byte stream index, which will be incremented as\n\
 *        needed.\n\
 * \\param bitcount is the current bit counter index in the current byte, which\n\
 *        will be incremented as needed.\n\
 * \\param numbits is the number of bits to pull from the byte stream\n\
 * \\return the decoded unsigned bitfield integer\n\
 */\n\
unsigned int decodeBitfield(const uint8_t* bytes, int* index, int* bitcount, int numbits)\n\
{\n\
    unsigned int value = 0;\n\
    uint8_t  byte;\n\
\n\
    int bitstomove;\n\
    while(numbits > 0)\n\
    {\n\
        // The current byte we are operating on\n\
        byte = bytes[*index];\n\
\n\
        // Remove any left most bits that we have already decoded\n\
        byte = byte << (*bitcount);\n\
\n\
        // Put the remaining bytes back in least significant position\n\
        byte = byte >> (*bitcount);\n\
\n\
        // Number of bits in the current byte that we could move\n\
        bitstomove = 8 - (*bitcount);\n\
\n\
        // It may be that some of the right most bits are not for\n\
        // this bit field, we can tell by looking at numbits\n\
        if(bitstomove >= numbits)\n\
        {\n\
            byte = byte >> (bitstomove - numbits);\n\
            bitstomove = numbits;\n\
            value |= byte;\n\
        }\n\
        else\n\
        {\n\
            // OR these bytes into position in value. The position is given by\n\
            // numbits, which identifies the bit position of the most significant\n\
            // bit.\n\
            value |= ((unsigned int)byte) << (numbits - bitstomove);\n\
        }\n\
\n\
        // Count the bits\n\
        numbits -= bitstomove;\n\
        (*bitcount) += bitstomove;\n\
\n\
        // Check if we have moved to the next byte\n\
        if((*bitcount) >= 8)\n\
        {\n\
            (*index)++;\n\
            (*bitcount) = 0;\n\
        }\n\
\n\
    }// while still bits to encode\n\
\n\
    return value;\n\
\n\
}// decodeBitfield\n");
source.write("\n");

if(support.float64)
{
source.write("\n\
/*!\n\
 * Scale a float64 to the base integer type used for bitfield\n\
 * \\param value is the number to scale.\n\
 * \\param min is the minimum value that can be encoded.\n\
 * \\param scaler is multiplied by value to create the integer.\n\
 * \\return (value-min)*scaler.\n\
 */\n\
unsigned int float64ScaledToBitfield(double value, double min, double scaler)\n\
{\n\
    // scale the number\n\
    value = (value - min)*scaler;\n\
\n\
    // account for fractional truncation\n\
    return (unsigned int)(value + 0.5);\n\
}\n\
\n");
}

source.write("\n\
/*!\n\
 * Scale a float32 to the base integer type used for bitfield\n\
 * \\param value is the number to scale.\n\
 * \\param min is the minimum value that can be encoded.\n\
 * \\param scaler is multiplied by value to create the integer.\n]\
 * \\return (value-min)*scaler.\n\
 */\n\
unsigned int float32ScaledToBitfield(float value, float min, float scaler)\n\
{\n\
    // scale the number\n\
    value = (value - min)*scaler;\n\
\n\
    // account for fractional truncation\n\
    return (unsigned int)(value + 0.5f);\n\
}\n\
\n");

if(support.float64)
{
source.write("\n\
/*!\n\
 * Inverse scale the bitfield base integer type to a float64\n\
 * \\param value is the integer number to inverse scale\n\
 * \\param min is the minimum value that can be represented.\n\
 * \\param invscaler is multiplied by the integer to create the return value.\n\
 *        invscaler should be the inverse of the scaler given to the scaling function.\n\
 * \\return the correctly scaled decoded value. return = min + value*invscaler.\n\
 */\n\
double float64ScaledFromBitfield(unsigned int value, double min, double invscaler)\n\
{\n\
    return (double)(min + invscaler*value);\n\
}\n\
\n");
}

source.write("\n\
/*!\n\
 * Inverse scale the bitfield base integer type to a float32\n\
 * \\param value is the integer number to inverse scale\n\
 * \\param min is the minimum value that can be represented.\n\
 * \\param invscaler is multiplied by the integer to create the return value.\n\
 *        invscaler should be the inverse of the scaler given to the scaling function.\n\
 * \\return the correctly scaled decoded value. return = min + value*invscaler.\n\
 */\n\
float float32ScaledFromBitfield(unsigned int value, float min, float invscaler)\n\
{\n\
    return (float)(min + invscaler*value);\n\
}\n");
    source.write("\n");

    if(support.longbitfield)
        generateLongSource();

    if(support.bitfieldtest)
        generateTest();

    source.write("\n");
    source.flush();

}// ProtocolBitfield::generateSource


/*!
 * Generate the test code in the source file for bitfield test support
 */
void ProtocolBitfield::generateTest(void)
{
source.write("\
/*!\n\
 * Test the bit field encode decode logic\n\
 * \\return 1 if the test passes, else 0\n\
 */\n\
int testBitfield(void)\n\
{\n\
    struct\n\
    {\n\
        unsigned int test1;     // :1;  1\n\
        unsigned int test2;     // :2;  3\n\
        unsigned int test3;     // :3;  6\n\
        unsigned int test12;    // :12; 18\n\
        unsigned int testa;     // :1;  19\n\
        unsigned int testb;     // :2;  20\n\
        unsigned int testc;     // :4;  24\n");
    if(support.longbitfield)
        source.write("        uint64_t     testd;     // :36;  0xC87654321\n    }test_t = {1, 2, 5, 0xABC, 0, 3, 4, 0xC87654321};\n");
    else
        source.write("    }test_t = {1, 2, 5, 0xABC, 0, 3, 4};\n");

source.write("\
\n\
    uint8_t data[20];\n\
    int index = 0;\n\
    int bitcount = 0;\n\
\n\
    encodeBitfield(test_t.test1, data, &index, &bitcount, 1);\n\
    encodeBitfield(test_t.test2, data, &index, &bitcount, 2);\n\
    encodeBitfield(test_t.test3, data, &index, &bitcount, 3);\n\
    encodeBitfield(test_t.test12, data, &index, &bitcount, 12);\n\
    encodeBitfield(test_t.testa, data, &index, &bitcount, 1);\n\
    encodeBitfield(test_t.testb, data, &index, &bitcount, 2);\n\
    encodeBitfield(test_t.testc, data, &index, &bitcount, 4);\n");
    if(support.longbitfield)
        source.write("    encodeLongBitfield(test_t.testd, data, &index, &bitcount, 36);\n");

source.write("\n\
\n\
    index = bitcount = 0;\n\
\n\
    memset(&test_t, 0, sizeof(test_t));\n\
\n\
    test_t.test1 = decodeBitfield(data, &index, &bitcount, 1);\n\
    test_t.test2 = decodeBitfield(data, &index, &bitcount, 2);\n\
    test_t.test3 = decodeBitfield(data, &index, &bitcount, 3);\n\
    test_t.test12 = decodeBitfield(data, &index, &bitcount, 12);\n\
    test_t.testa = decodeBitfield(data, &index, &bitcount, 1);\n\
    test_t.testb = decodeBitfield(data, &index, &bitcount, 2);\n\
    test_t.testc = decodeBitfield(data, &index, &bitcount, 4);\n");
    if(support.longbitfield)
        source.write("    test_t.testd = decodeLongBitfield(data, &index, &bitcount, 36);\n");

source.write("\n\
    if(test_t.test1 != 1)\n\
        return 0;\n\
    else if(test_t.test2 != 2)\n\
        return 0;\n\
    else if(test_t.test3 != 5)\n\
        return 0;\n\
    else if(test_t.test12 != 0xABC)\n\
        return 0;\n\
    else if(test_t.testa != 0)\n\
        return 0;\n\
    else if(test_t.testb != 3)\n\
        return 0;\n\
    else if(test_t.testc != 4)\n\
        return 0;\n");
    if(support.longbitfield)
        source.write("    else if(test_t.testd != 0xC87654321)\n        return 0;\n");

source.write("\
    else\n\
        return 1;\n\
\n\
}// testBitfield\n");

}// ProtocolBitfield::generateTest


/*!
* Generate the source code for long bitfield support
*/
void ProtocolBitfield::generateLongSource(void)
{
source.write("\n\
/*!\n\
 * Add a long bit field to a byte stream.\n\
 * \\param value is the unsigned integer to encode, which can vary from 1 to 32\n\
 *        bits in length. The bits encoded are the least significant (right\n\
 *        most) bits of value\n\
 * \\param bytes is the byte stream to receive the bits\n\
 * \\param index is the current byte stream index, which will be incremented as\n\
 *        needed.\n\
 * \\param bitcount is the current bit counter index in the current byte, which\n\
 *        will be incremented as needed.\n\
 * \\param numbits is the number of bits in value to encode\n\
 */\n\
void encodeLongBitfield(uint64_t value, uint8_t* bytes, int* index, int* bitcount, int numbits)\n\
{\n\
    int bitstomove;\n\
    while(numbits > 0)\n\
    {\n\
        // Start out with all bits zero\n\
        if((*bitcount) == 0)\n\
            bytes[*index] = 0;\n\
\n\
        // imagine that bitcount is currently 2, i.e. the 2 left most bits\n\
        // have already been encoded. In that case we want to encode 6 more\n\
        // bits in the current byte.\n\
        bitstomove = 8 - (*bitcount);\n\
\n\
        // shift value to the correct alignment.\n\
        if(bitstomove >= numbits)\n\
        {\n\
            // In this case all the bits in value will fit in the current byte\n\
            bytes[*index] |= (uint8_t)(value << (bitstomove - numbits));\n\
            (*bitcount) += numbits;\n\
            numbits = 0;\n\
        }\n\
        else\n\
        {\n\
            // In this case we have too many bits in value to fit in the\n\
            // current byte, encode the most significant bits that fit\n\
            bytes[*index] |= (uint8_t)(value >> (numbits - bitstomove));\n\
            (*bitcount) += bitstomove;\n\
            numbits -= bitstomove;\n\
        }\n\
\n\
        // Check if we have moved to the next byte\n\
        if((*bitcount) >= 8)\n\
        {\n\
            (*index)++;\n\
            (*bitcount) = 0;\n\
        }\n\
\n\
    }// while still bits to encode\n\
\n\
}// encodeLongBitfield\n\
\n\
\n\
/*! Decode a long bit field from a byte stream.\n\
 * \\param bytes is the byte stream from where the bitfields are taken\n\
 * \\param index is the current byte stream index, which will be incremented as\n\
 *        needed.\n\
 * \\param bitcount is the current bit counter index in the current byte, which\n\
 *        will be incremented as needed.\n\
 * \\param numbits is the number of bits to pull from the byte stream\n\
 * \\return the decoded unsigned bitfield integer\n\
 */\n\
uint64_t decodeLongBitfield(const uint8_t* bytes, int* index, int* bitcount, int numbits)\n\
{\n\
    uint64_t value = 0;\n\
    uint8_t  byte;\n\
\n\
    int bitstomove;\n\
    while(numbits > 0)\n\
    {\n\
        // The current byte we are operating on\n\
        byte = bytes[*index];\n\
\n\
        // Remove any left most bits that we have already decoded\n\
        byte = byte << (*bitcount);\n\
\n\
        // Put the remaining bytes back in least significant position\n\
        byte = byte >> (*bitcount);\n\
\n\
        // Number of bits in the current byte that we could move\n\
        bitstomove = 8 - (*bitcount);\n\
\n\
        // It may be that some of the right most bits are not for\n\
        // this bit field, we can tell by looking at numbits\n\
        if(bitstomove >= numbits)\n\
        {\n\
            byte = byte >> (bitstomove - numbits);\n\
            bitstomove = numbits;\n\
            value |= byte;\n\
        }\n\
        else\n\
        {\n\
            // OR these bytes into position in value. The position is given by\n\
            // numbits, which identifies the bit position of the most significant\n\
            // bit.\n\
            value |= ((uint64_t)byte) << (numbits - bitstomove);\n\
        }\n\
\n\
        // Count the bits\n\
        numbits -= bitstomove;\n\
        (*bitcount) += bitstomove;\n\
\n\
        // Check if we have moved to the next byte\n\
        if((*bitcount) >= 8)\n\
        {\n\
            (*index)++;\n\
            (*bitcount) = 0;\n\
        }\n\
\n\
    }// while still bits to encode\n\
\n\
    return value;\n\
\n\
}// decodeLongBitfield\n");
source.write("\n");

if(support.float64)
{
source.write("\n\
/*!\n\
 * Scale a float64 to the long integer type used for bitfield\n\
 * \\param value is the number to scale.\n\
 * \\param min is the minimum value that can be encoded.\n\
 * \\param scaler is multiplied by value to create the integer: return = (value-min)*scaler.\n\
 */\n\
uint64_t float64ScaledToLongBitfield(double value, double min, double scaler)\n\
{\n\
    // scale the number\n\
    value = (value - min)*scaler;\n\
\n\
    // account for fractional truncation\n\
    return (unsigned int)(value + 0.5);\n\
}\n\
\n");
}

source.write("\n\
/*!\n\
 * Scale a float32 to the long integer type used for bitfield\n\
 * \\param value is the number to scale.\n\
 * \\param min is the minimum value that can be encoded.\n\
 * \\param scaler is multiplied by value to create the integer: return = (value-min)*scaler.\n\
 */\n\
uint64_t float32ScaledToLongBitfield(float value, float min, float scaler)\n\
{\n\
    // scale the number\n\
    value = (value - min)*scaler;\n\
\n\
    // account for fractional truncation\n\
    return (unsigned int)(value + 0.5f);\n\
}\n\
\n");

if(support.float64)
{
source.write("\n\
/*!\n\
 * Inverse scale the bitfield long integer type to a float64\n\
 * \\param value is the integer number to inverse scale\n\
 * \\param min is the minimum value that can be represented.\n\
 * \\param invscaler is multiplied by the integer to create the return value.\n\
 *        invscaler should be the inverse of the scaler given to the scaling function.\n\
 * \\return the correctly scaled decoded value. return = min + value*invscaler.\n\
 */\n\
double float64ScaledFromLongBitfield(uint64_t value, double min, double invscaler)\n\
{\n\
    return (double)(min + invscaler*value);\n\
}\n\
\n");
}

source.write("\n\
/*!\n\
 * Inverse scale the bitfield long integer type to a float32\n\
 * \\param value is the integer number to inverse scale\n\
 * \\param min is the minimum value that can be represented.\n\
 * \\param invscaler is multiplied by the integer to create the return value.\n\
 *        invscaler should be the inverse of the scaler given to the scaling function.\n\
 * \\return the correctly scaled decoded value. return = min + value*invscaler.\n\
 */\n\
float float32ScaledFromLongBitfield(uint64_t value, float min, float invscaler)\n\
{\n\
    return (float)(min + invscaler*value);\n\
}\n");
    source.write("\n");

}// ProtocolBitfield::generate
