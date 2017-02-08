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
    header.setModuleNameAndPath("bitfieldspecial", support.outputpath);

    // Top level comment
    header.write(
"/*!\n\
 * \\file\n\
 * Routines for encoding and decoding bitfields into and out of byte arrays\n\
 */\n");

    header.write("\n");
    header.write("#define __STDC_CONSTANT_MACROS\n");
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
        header.write("#ifdef UINT64_MAX\n");
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
        header.write("\n");
        header.write("#endif // UINT64_MAX\n");
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
    source.setModuleNameAndPath("bitfieldspecial", support.outputpath);

    if(support.bitfieldtest)
        source.write("#include <string.h>\n");

    source.write("#include <limits.h>\n");

    source.makeLineSeparator();
    generateEncodeBitfield();

    source.makeLineSeparator();
    generateDecodeBitfield();

    if(support.longbitfield)
    {
        source.makeLineSeparator();
        source.write("#ifdef UINT64_MAX\n");
        source.makeLineSeparator();
        generateEncodeLongBitfield();
        source.makeLineSeparator();
        generateDecodeLongBitfield();
        source.makeLineSeparator();
        source.write("#endif // UINT64_MAX\n");
    }

    if(support.bitfieldtest)
    {
        source.makeLineSeparator();
        generateTest();
    }

    source.makeLineSeparator();
    source.flush();

}// ProtocolBitfield::generateSource


void ProtocolBitfield::generateEncodeBitfield(void)
{
source.write("\
/*!\n\
 * Add a bit field to a byte stream.\n\
 * \\param value is the unsigned integer to encode, which can vary from 1 to\n\
 *        the number of bits in an int in length. The bits encoded are the least\n\
 *        significant (right most) bits of value\n\
 * \\param bytes is the byte stream that receives the bits\n\
 * \\param index is the current byte stream index, which will be incremented as\n\
 *        needed.\n\
 * \\param bitcount is the current bit counter index in the current byte, which\n\
 *        will be incremented as needed.\n\
 * \\param numbits is the number of bits in value to encode\n\
 */\n\
void encodeBitfield(unsigned int value, uint8_t* bytes, int* index, int* bitcount, int numbits)\n\
{\n\
    // Bits are encoded left-to-right from most-significant to least-significant.\n\
    // The least significant bits are moved first, as that allows us to keep the\n\
    // shifts to 8 bits or less, which some compilers need.\n\
\n\
    // The value of the bit count after moving all the bits\n\
    int bitoffset = (*bitcount) + numbits;\n\
\n\
    // The byte offset of the least significant block of 8 bits to move\n\
    int byteoffset = (bitoffset-1) >> 3;\n\
\n\
    // The remainder bits (modulo 8) which are the least significant bits to move\n\
    int remainder = bitoffset & 0x07;\n\
\n\
    // The maximum value that can be stored in a unsigned int\n\
    unsigned int max = UINT_MAX;\n\
\n\
    // The maximum value that can be stored in numbits\n\
    max = max >> (CHAR_BIT*sizeof(unsigned int) - numbits);\n\
\n\
    // Enforce maximum value\n\
    if(value > max)\n\
        value = max;\n\
\n\
    // If these are the first bits going in this byte, make sure the byte is zero\n\
    if((*bitcount) == 0)\n\
        bytes[(*index)] = 0;\n\
\n\
    // Zero the last byte, as we may not be writing a full 8 bits there\n\
    if(byteoffset > 0)\n\
        bytes[(*index) + byteoffset] = 0;\n\
\n\
    // The index of the right most byte to write\n\
    byteoffset += (*index);\n\
\n\
    // The value of index to return to the caller\n\
    (*index) = byteoffset;\n\
\n\
    // Handle any least significant remainder bits\n\
    if(remainder)\n\
    {\n\
        // The least significant bits of value, encoded in the most\n\
        // significant bits of the last byte we are going to use.\n\
        bytes[byteoffset--] |= (uint8_t)(value << (8 - remainder));\n\
\n\
        // Discard these bits, we have encoded them\n\
        numbits -= remainder;\n\
        value = value >> remainder;\n\
\n\
        // The new value of bitcount to return to the caller\n\
        // for subsequent bitfield encodings\n\
        (*bitcount) = remainder;\n\
    }\n\
    else\n\
    {\n\
        (*bitcount) = 0; // no remainder bits, aligned on byte boundary\n\
        (*index)++;      // This byte will be completed\n\
    }\n\
\n\
    // Now aligned on a byte boundary, move full bytes\n\
    while(numbits >= 8)\n\
    {\n\
        bytes[byteoffset--] = (uint8_t)value;\n\
        value = value >> 8;\n\
        numbits -= 8;\n\
\n\
    }// while still bits to encode\n\
\n\
    // Finally finish any remaining most significant bits. There are\n\
    // some valid bits in the most signficant bit locations.\n\
    if(numbits > 0)\n\
        bytes[byteoffset] |= (uint8_t)value;\n\
\n\
}// encodeBitfield\n");

source.makeLineSeparator();
source.write("\
/*!\n\
 * Scale a float32 to the base integer type used for bitfield\n\
 * \\param value is the number to scale.\n\
 * \\param min is the minimum value that can be encoded.\n\
 * \\param scaler is multiplied by value to create the integer.\n\
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
source.makeLineSeparator();
source.write("\
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
}// if float64 support

}// ProtocolBitfield::generateEncodeBitfield


void ProtocolBitfield::generateDecodeBitfield(void)
{
source.write("\
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
    int bitstomove;\n\
    int count = (*bitcount);\n\
    uint8_t byte;\n\
\n\
    // Handle any leading bits\n\
    if(count > 0)\n\
    {\n\
        // The current byte we are operating on\n\
        byte = bytes[*index];\n\
\n\
        // Remove any left most bits that we have already decoded\n\
        byte = byte << count;\n\
\n\
        // Put the remaining bits back in least significant position\n\
        byte = byte >> count;\n\
\n\
        // Number of bits in the current byte that we could move\n\
        bitstomove = 8 - count;\n\
\n\
        if(bitstomove > numbits)\n\
        {\n\
            // Using only some of the remaining bits\n\
            value = byte >> (bitstomove - numbits);\n\
\n\
            // Keep track of bit location for caller\n\
            (*bitcount) = count + numbits;\n\
\n\
            // Nothing more to do, (*index) not incremented\n\
            return value;\n\
        }\n\
        else\n\
        {\n\
            // Using all the remaining bits\n\
            value = byte;\n\
\n\
            // Keep track of bytes\n\
            (*index)++;\n\
\n\
            // bitcount has reached byte boundary\n\
            (*bitcount) = 0;\n\
\n\
            // Keep track of the bits we have decoded\n\
            numbits -= bitstomove;\n\
        }\n\
\n\
    }// If we do not start on a byte boundary\n\
\n\
    // On a byte boundary ((*bitcount) == 0), move whole bytes\n\
    while(numbits >= 8)\n\
    {\n\
        // Previous bits are shifted up to make room for new bits\n\
        value = value << 8;\n\
\n\
        // New bits in least significant position\n\
        value |= bytes[(*index)++];\n\
\n\
        // Keep track of the bits we have decoded\n\
        numbits -= 8;\n\
\n\
    }// while moving whole bytes\n\
\n\
    // Move any residual (less than whole byte) bits\n\
    if(numbits > 0)\n\
    {\n\
        // Previous bits are shifted up\n\
        value = value << numbits;\n\
\n\
        // The next byte which has some of the bits we want\n\
        byte = bytes[(*index)];\n\
\n\
        // We keep the most significant bits\n\
        value |= (byte >> (8 - numbits));\n\
\n\
        // Keep track of bit location for caller\n\
        (*bitcount) += numbits;\n\
    }\n\
\n\
    return value;\n\
\n\
}// decodeBitfield\n");

source.makeLineSeparator();
source.write("\
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

if(support.float64)
{
source.makeLineSeparator();
source.write("\
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
}// if support 64 bit floats

}// ProtocolBitfield::generateDecodeBitfield


void ProtocolBitfield::generateEncodeLongBitfield(void)
{
source.write("\
/*!\n\
 * Add a bit field to a byte stream.\n\
 * \\param value is the unsigned integer to encode, which can vary from 1 to\n\
 *        64 bits in length. The bits encoded are the least\n\
 *        significant (right most) bits of value\n\
 * \\param bytes is the byte stream that receives the bits\n\
 * \\param index is the current byte stream index, which will be incremented as\n\
 *        needed.\n\
 * \\param bitcount is the current bit counter index in the current byte, which\n\
 *        will be incremented as needed.\n\
 * \\param numbits is the number of bits in value to encode\n\
 */\n\
void encodeLongBitfield(uint64_t value, uint8_t* bytes, int* index, int* bitcount, int numbits)\n\
{\n\
    // Bits are encoded left-to-right from most-significant to least-significant.\n\
    // The least signficiant bits are moved first, as that allows us to keep the\n\
    // shifts to 8 bits or less, which some compilers need.\n\
\n\
    // The value of the bit count after moving all the bits\n\
    int bitoffset = (*bitcount) + numbits;\n\
\n\
    // The byte offset of the least significant block of 8 bits to move\n\
    int byteoffset = (bitoffset-1) >> 3;\n\
\n\
    // The remainder bits (modulo 8) which are the least significant bits to move\n\
    int remainder = bitoffset & 0x07;\n\
\n\
    // The maximum value that can be stored in a uint64_t\n\
    uint64_t max = UINT64_MAX;\n\
\n\
    // The maximum value that can be stored in numbits\n\
    max = max >> (CHAR_BIT*sizeof(uint64_t) - numbits);\n\
\n\
    // Enforce maximum value\n\
    if(value > max)\n\
        value = max;\n\
\n\
    // If these are the first bits going in this byte, make sure the byte is zero\n\
    if((*bitcount) == 0)\n\
        bytes[(*index)] = 0;\n\
\n\
    // Zero the last byte, as we may not be writing a full 8 bits there\n\
    if(byteoffset > 0)\n\
        bytes[(*index) + byteoffset] = 0;\n\
\n\
    // The index of the right most byte to write\n\
    byteoffset += (*index);\n\
\n\
    // The value of index to return to the caller\n\
    (*index) = byteoffset;\n\
\n\
    // Handle any least significant remainder bits\n\
    if(remainder)\n\
    {\n\
        // The least significant bits of value, encoded in the most\n\
        // significant bits of the last byte we are going to use.\n\
        bytes[byteoffset--] |= (uint8_t)(value << (8 - remainder));\n\
\n\
        // Discard these bits, we have encoded them\n\
        numbits -= remainder;\n\
        value = value >> remainder;\n\
\n\
        // The new value of bitcount to return to the caller\n\
        // for subsequent bitfield encodings\n\
        (*bitcount) = remainder;\n\
    }\n\
    else\n\
    {\n\
        (*bitcount) = 0; // no remainder bits, aligned on byte boundary\n\
        (*index)++;      // This byte will be completed\n\
    }\n\
\n\
    // Now aligned on a byte boundary, move full bytes\n\
    while(numbits >= 8)\n\
    {\n\
        bytes[byteoffset--] = (uint8_t)value;\n\
        value = value >> 8;\n\
        numbits -= 8;\n\
\n\
    }// while still bits to encode\n\
\n\
    // Finally finish any remaining most significant bits. There are\n\
    // some valid bits in the most signficant bit locations.\n\
    if(numbits > 0)\n\
        bytes[byteoffset] |= (uint8_t)value;\n\
\n\
}// encodeLongBitfield\n");

source.makeLineSeparator();
source.write("\
/*!\n\
 * Scale a float32 to the base integer type used for long bitfields\n\
 * \\param value is the number to scale.\n\
 * \\param min is the minimum value that can be encoded.\n\
 * \\param scaler is multiplied by value to create the integer.\n\
 * \\return (value-min)*scaler.\n\
 */\n\
uint64_t float32ScaledToLongBitfield(float value, float min, float scaler)\n\
{\n\
    // scale the number\n\
    value = (value - min)*scaler;\n\
\n\
    // account for fractional truncation\n\
    return (uint64_t)(value + 0.5f);\n\
}\n\
\n");

if(support.float64)
{
source.makeLineSeparator();
source.write("\
/*!\n\
 * Scale a float64 to the base integer type used for long bitfields\n\
 * \\param value is the number to scale.\n\
 * \\param min is the minimum value that can be encoded.\n\
 * \\param scaler is multiplied by value to create the integer.\n\
 * \\return (value-min)*scaler.\n\
 */\n\
uint64_t float64ScaledToLongBitfield(double value, double min, double scaler)\n\
{\n\
    // scale the number\n\
    value = (value - min)*scaler;\n\
\n\
    // account for fractional truncation\n\
    return (uint64_t)(value + 0.5);\n\
}\n\
\n");
}// if float64 support

}// ProtocolBitfield::generateEncodeLongBitfield


void ProtocolBitfield::generateDecodeLongBitfield(void)
{
source.write("\
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
    int bitstomove;\n\
    int count = (*bitcount);\n\
    uint8_t byte;\n\
\n\
    // Handle any leading bits\n\
    if(count > 0)\n\
    {\n\
        // The current byte we are operating on\n\
        byte = bytes[*index];\n\
\n\
        // Remove any left most bits that we have already decoded\n\
        byte = byte << count;\n\
\n\
        // Put the remaining bytes back in least significant position\n\
        byte = byte >> count;\n\
\n\
        // Number of bits in the current byte that we could move\n\
        bitstomove = 8 - count;\n\
\n\
        if(bitstomove > numbits)\n\
        {\n\
            // Using only some of the remaining bits\n\
            value = byte >> (bitstomove - numbits);\n\
\n\
            // Keep track of bit location for caller\n\
            (*bitcount) = count + numbits;\n\
\n\
            // Nothing more to do, (*index) not incremented\n\
            return value;\n\
        }\n\
        else\n\
        {\n\
            // Using all the remaining bits\n\
            value = byte;\n\
\n\
            // Keep track of bytes\n\
            (*index)++;\n\
\n\
            // bitcount has reached byte boundary\n\
            (*bitcount) = 0;\n\
\n\
            // Keep track of the bits we have decoded\n\
            numbits -= bitstomove;\n\
        }\n\
\n\
    }// If we do not start on a byte boundary\n\
\n\
    // On a byte boundary ((*bitcount) == 0), move whole bytes\n\
    while(numbits >= 8)\n\
    {\n\
        // Previous bits are shifted up to make room for new bits\n\
        value = value << 8;\n\
\n\
        // New bits in least significant position\n\
        value |= bytes[(*index)++];\n\
\n\
        // Keep track of the bits we have decoded\n\
        numbits -= 8;\n\
\n\
    }// while moving whole bytes\n\
\n\
    // Move any residual (less than whole byte) bits\n\
    if(numbits > 0)\n\
    {\n\
        // Previous bits are shifted up\n\
        value = value << numbits;\n\
\n\
        // The next byte which has some of the bits we want\n\
        byte = bytes[(*index)];\n\
\n\
        // We keep the most significant bits\n\
        value |= (byte >> (8 - numbits));\n\
\n\
        // Keep track of bit location for caller\n\
        (*bitcount) += numbits;\n\
    }\n\
\n\
    return value;\n\
\n\
}// decodeLongBitfield\n");

source.makeLineSeparator();
source.write("\
/*!\n\
 * Inverse scale the long bitfield base integer type to a float32\n\
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

if(support.float64)
{
source.makeLineSeparator();
source.write("\
/*!\n\
 * Inverse scale the long bitfield base integer type to a float64\n\
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
}// if support 64 bit floats

}// ProtocolBitfield::generateDecodeLongBitfield


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
        unsigned int testb;     // :2;  21\n\
        unsigned int testc;     // :4;  25\n");
    if(support.longbitfield)
        source.write("        uint64_t     testd;     // :36; 61\n    }test_t = {1, 2, 5, 0xABC, 0, 3, 4, 0xC87654321};\n");
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
