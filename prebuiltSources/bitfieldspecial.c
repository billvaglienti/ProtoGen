// bitfieldspecial.c was hand-coded for protogen, which copied it here

#include "bitfieldspecial.h"
#include <string.h>
#include <limits.h>

/*!
 * Add a bit field to a byte stream.
 * \param value is the unsigned integer to encode, which can vary from 1 to
 *        the number of bits in an int in length. The bits encoded are the least
 *        significant (right most) bits of value
 * \param bytes is the byte stream that receives the bits
 * \param index is the current byte stream index, which will be incremented as
 *        needed.
 * \param bitcount is the current bit counter index in the current byte, which
 *        will be incremented as needed.
 * \param numbits is the number of bits in value to encode
 */
void encodeBitfield(unsigned int value, uint8_t* bytes, int* index, int* bitcount, int numbits)
{
    // The maximum value that can be stored in a unsigned int
    unsigned int max = UINT_MAX;

    // The maximum value that can be stored in numbits
    max = max >> (CHAR_BIT*sizeof(unsigned int) - numbits);

    // Enforce maximum value
    if(value > max)
        value = max;

    encodeBitfieldUnchecked(value, bytes, index, bitcount, numbits);
}


/*!
 * Add a bit field to a byte stream.
 * \param value is the unsigned integer to encode, which can vary from 1 to
 *        the number of bits in an int in length. The bits encoded are the least
 *        significant (right most) bits of value
 * \param bytes is the byte stream that receives the bits
 * \param index is the current byte stream index, which will be incremented as
 *        needed.
 * \param bitcount is the current bit counter index in the current byte, which
 *        will be incremented as needed.
 * \param numbits is the number of bits in value to encode
 */
void encodeBitfieldUnchecked(unsigned int value, uint8_t* bytes, int* index, int* bitcount, int numbits)
{
    // Bits are encoded left-to-right from most-significant to least-significant.
    // The least significant bits are moved first, as that allows us to keep the
    // shifts to 8 bits or less, which some compilers need.

    // The value of the bit count after moving all the bits
    int bitoffset = (*bitcount) + numbits;

    // The byte offset of the least significant block of 8 bits to move
    int byteoffset = (bitoffset-1) >> 3;

    // The remainder bits (modulo 8) which are the least significant bits to move
    int remainder = bitoffset & 0x07;

    // If these are the first bits going in this byte, make sure the byte is zero
    if((*bitcount) == 0)
        bytes[(*index)] = 0;

    // Zero the last byte, as we may not be writing a full 8 bits there
    if(byteoffset > 0)
        bytes[(*index) + byteoffset] = 0;

    // The index of the right most byte to write
    byteoffset += (*index);

    // The value of index to return to the caller
    (*index) = byteoffset;

    // Handle any least significant remainder bits
    if(remainder)
    {
        // The least significant bits of value, encoded in the most
        // significant bits of the last byte we are going to use.
        bytes[byteoffset--] |= (uint8_t)(value << (8 - remainder));

        // Discard these bits, we have encoded them
        numbits -= remainder;
        value = value >> remainder;

        // The new value of bitcount to return to the caller
        // for subsequent bitfield encodings
        (*bitcount) = remainder;
    }
    else
    {
        (*bitcount) = 0; // no remainder bits, aligned on byte boundary
        (*index)++;      // This byte will be completed
    }

    // Now aligned on a byte boundary, move full bytes
    while(numbits >= 8)
    {
        bytes[byteoffset--] = (uint8_t)value;
        value = value >> 8;
        numbits -= 8;

    }// while still bits to encode

    // Finally finish any remaining most significant bits. There are
    // some valid bits in the most signficant bit locations.
    if(numbits > 0)
        bytes[byteoffset] |= (uint8_t)value;

}// encodeBitfield

/*!
 * Scale a float32 to the base integer type used for bitfield
 * \param value is the number to scale.
 * \param min is the minimum value that can be encoded.
 * \param scaler is multiplied by value to create the integer.
 * \return (value-min)*scaler.
 */
unsigned int float32ScaledToBitfield(float value, float min, float scaler)
{
    // scale the number
    value = (value - min)*scaler;

    // account for fractional truncation
    return (unsigned int)(value + 0.5f);
}

/*!
 * Scale a float64 to the base integer type used for bitfield
 * \param value is the number to scale.
 * \param min is the minimum value that can be encoded.
 * \param scaler is multiplied by value to create the integer.
 * \return (value-min)*scaler.
 */
unsigned int float64ScaledToBitfield(double value, double min, double scaler)
{
    // scale the number
    value = (value - min)*scaler;

    // account for fractional truncation
    return (unsigned int)(value + 0.5);
}

/*! Decode a bit field from a byte stream.
 * \param bytes is the byte stream from where the bitfields are taken
 * \param index is the current byte stream index, which will be incremented as
 *        needed.
 * \param bitcount is the current bit counter index in the current byte, which
 *        will be incremented as needed.
 * \param numbits is the number of bits to pull from the byte stream
 * \return the decoded unsigned bitfield integer
 */
unsigned int decodeBitfield(const uint8_t* bytes, int* index, int* bitcount, int numbits)
{
    unsigned int value = 0;
    int bitstomove;
    int count = (*bitcount);
    uint8_t byte;

    // Handle any leading bits
    if(count > 0)
    {
        // The current byte we are operating on
        byte = bytes[*index];

        // Remove any left most bits that we have already decoded
        byte = byte << count;

        // Put the remaining bits back in least significant position
        byte = byte >> count;

        // Number of bits in the current byte that we could move
        bitstomove = 8 - count;

        if(bitstomove > numbits)
        {
            // Using only some of the remaining bits
            value = byte >> (bitstomove - numbits);

            // Keep track of bit location for caller
            (*bitcount) = count + numbits;

            // Nothing more to do, (*index) not incremented
            return value;
        }
        else
        {
            // Using all the remaining bits
            value = byte;

            // Keep track of bytes
            (*index)++;

            // bitcount has reached byte boundary
            (*bitcount) = 0;

            // Keep track of the bits we have decoded
            numbits -= bitstomove;
        }

    }// If we do not start on a byte boundary

    // On a byte boundary ((*bitcount) == 0), move whole bytes
    while(numbits >= 8)
    {
        // Previous bits are shifted up to make room for new bits
        value = value << 8;

        // New bits in least significant position
        value |= bytes[(*index)++];

        // Keep track of the bits we have decoded
        numbits -= 8;

    }// while moving whole bytes

    // Move any residual (less than whole byte) bits
    if(numbits > 0)
    {
        // Previous bits are shifted up
        value = value << numbits;

        // The next byte which has some of the bits we want
        byte = bytes[(*index)];

        // We keep the most significant bits
        value |= (byte >> (8 - numbits));

        // Keep track of bit location for caller
        (*bitcount) += numbits;
    }

    return value;

}// decodeBitfield

/*!
 * Inverse scale the bitfield base integer type to a float32
 * \param value is the integer number to inverse scale
 * \param min is the minimum value that can be represented.
 * \param invscaler is multiplied by the integer to create the return value.
 *        invscaler should be the inverse of the scaler given to the scaling function.
 * \return the correctly scaled decoded value. return = min + value*invscaler.
 */
float float32ScaledFromBitfield(unsigned int value, float min, float invscaler)
{
    return (float)(min + invscaler*value);
}

/*!
 * Inverse scale the bitfield base integer type to a float64
 * \param value is the integer number to inverse scale
 * \param min is the minimum value that can be represented.
 * \param invscaler is multiplied by the integer to create the return value.
 *        invscaler should be the inverse of the scaler given to the scaling function.
 * \return the correctly scaled decoded value. return = min + value*invscaler.
 */
double float64ScaledFromBitfield(unsigned int value, double min, double invscaler)
{
    return (double)(min + invscaler*value);
}

#ifdef UINT64_MAX

/*!
 * Add a bit field to a byte stream.
 * \param value is the unsigned integer to encode, which can vary from 1 to
 *        64 bits in length. The bits encoded are the least
 *        significant (right most) bits of value
 * \param bytes is the byte stream that receives the bits
 * \param index is the current byte stream index, which will be incremented as
 *        needed.
 * \param bitcount is the current bit counter index in the current byte, which
 *        will be incremented as needed.
 * \param numbits is the number of bits in value to encode
 */
void encodeLongBitfield(uint64_t value, uint8_t* bytes, int* index, int* bitcount, int numbits)
{
    // The maximum value that can be stored in a uint64_t
    uint64_t max = UINT64_MAX;

    // The maximum value that can be stored in numbits
    max = max >> (CHAR_BIT*sizeof(uint64_t) - numbits);

    // Enforce maximum value
    if(value > max)
        value = max;

    encodeLongBitfieldUnchecked(value, bytes, index, bitcount, numbits);
}


/*!
 * Add a bit field to a byte stream.
 * \param value is the unsigned integer to encode, which can vary from 1 to
 *        64 bits in length. The bits encoded are the least
 *        significant (right most) bits of value
 * \param bytes is the byte stream that receives the bits
 * \param index is the current byte stream index, which will be incremented as
 *        needed.
 * \param bitcount is the current bit counter index in the current byte, which
 *        will be incremented as needed.
 * \param numbits is the number of bits in value to encode
 */
void encodeLongBitfieldUnchecked(uint64_t value, uint8_t* bytes, int* index, int* bitcount, int numbits)
{
    // Bits are encoded left-to-right from most-significant to least-significant.
    // The least signficiant bits are moved first, as that allows us to keep the
    // shifts to 8 bits or less, which some compilers need.

    // The value of the bit count after moving all the bits
    int bitoffset = (*bitcount) + numbits;

    // The byte offset of the least significant block of 8 bits to move
    int byteoffset = (bitoffset-1) >> 3;

    // The remainder bits (modulo 8) which are the least significant bits to move
    int remainder = bitoffset & 0x07;

    // If these are the first bits going in this byte, make sure the byte is zero
    if((*bitcount) == 0)
        bytes[(*index)] = 0;

    // Zero the last byte, as we may not be writing a full 8 bits there
    if(byteoffset > 0)
        bytes[(*index) + byteoffset] = 0;

    // The index of the right most byte to write
    byteoffset += (*index);

    // The value of index to return to the caller
    (*index) = byteoffset;

    // Handle any least significant remainder bits
    if(remainder)
    {
        // The least significant bits of value, encoded in the most
        // significant bits of the last byte we are going to use.
        bytes[byteoffset--] |= (uint8_t)(value << (8 - remainder));

        // Discard these bits, we have encoded them
        numbits -= remainder;
        value = value >> remainder;

        // The new value of bitcount to return to the caller
        // for subsequent bitfield encodings
        (*bitcount) = remainder;
    }
    else
    {
        (*bitcount) = 0; // no remainder bits, aligned on byte boundary
        (*index)++;      // This byte will be completed
    }

    // Now aligned on a byte boundary, move full bytes
    while(numbits >= 8)
    {
        bytes[byteoffset--] = (uint8_t)value;
        value = value >> 8;
        numbits -= 8;

    }// while still bits to encode

    // Finally finish any remaining most significant bits. There are
    // some valid bits in the most signficant bit locations.
    if(numbits > 0)
        bytes[byteoffset] |= (uint8_t)value;

}// encodeLongBitfield

/*!
 * Scale a float32 to the base integer type used for long bitfields
 * \param value is the number to scale.
 * \param min is the minimum value that can be encoded.
 * \param scaler is multiplied by value to create the integer.
 * \return (value-min)*scaler.
 */
uint64_t float32ScaledToLongBitfield(float value, float min, float scaler)
{
    // scale the number
    value = (value - min)*scaler;

    // account for fractional truncation
    return (uint64_t)(value + 0.5f);
}

/*!
 * Scale a float64 to the base integer type used for long bitfields
 * \param value is the number to scale.
 * \param min is the minimum value that can be encoded.
 * \param scaler is multiplied by value to create the integer.
 * \return (value-min)*scaler.
 */
uint64_t float64ScaledToLongBitfield(double value, double min, double scaler)
{
    // scale the number
    value = (value - min)*scaler;

    // account for fractional truncation
    return (uint64_t)(value + 0.5);
}

/*! Decode a long bit field from a byte stream.
 * \param bytes is the byte stream from where the bitfields are taken
 * \param index is the current byte stream index, which will be incremented as
 *        needed.
 * \param bitcount is the current bit counter index in the current byte, which
 *        will be incremented as needed.
 * \param numbits is the number of bits to pull from the byte stream
 * \return the decoded unsigned bitfield integer
 */
uint64_t decodeLongBitfield(const uint8_t* bytes, int* index, int* bitcount, int numbits)
{
    uint64_t value = 0;
    int bitstomove;
    int count = (*bitcount);
    uint8_t byte;

    // Handle any leading bits
    if(count > 0)
    {
        // The current byte we are operating on
        byte = bytes[*index];

        // Remove any left most bits that we have already decoded
        byte = byte << count;

        // Put the remaining bytes back in least significant position
        byte = byte >> count;

        // Number of bits in the current byte that we could move
        bitstomove = 8 - count;

        if(bitstomove > numbits)
        {
            // Using only some of the remaining bits
            value = byte >> (bitstomove - numbits);

            // Keep track of bit location for caller
            (*bitcount) = count + numbits;

            // Nothing more to do, (*index) not incremented
            return value;
        }
        else
        {
            // Using all the remaining bits
            value = byte;

            // Keep track of bytes
            (*index)++;

            // bitcount has reached byte boundary
            (*bitcount) = 0;

            // Keep track of the bits we have decoded
            numbits -= bitstomove;
        }

    }// If we do not start on a byte boundary

    // On a byte boundary ((*bitcount) == 0), move whole bytes
    while(numbits >= 8)
    {
        // Previous bits are shifted up to make room for new bits
        value = value << 8;

        // New bits in least significant position
        value |= bytes[(*index)++];

        // Keep track of the bits we have decoded
        numbits -= 8;

    }// while moving whole bytes

    // Move any residual (less than whole byte) bits
    if(numbits > 0)
    {
        // Previous bits are shifted up
        value = value << numbits;

        // The next byte which has some of the bits we want
        byte = bytes[(*index)];

        // We keep the most significant bits
        value |= (byte >> (8 - numbits));

        // Keep track of bit location for caller
        (*bitcount) += numbits;
    }

    return value;

}// decodeLongBitfield

/*!
 * Inverse scale the long bitfield base integer type to a float32
 * \param value is the integer number to inverse scale
 * \param min is the minimum value that can be represented.
 * \param invscaler is multiplied by the integer to create the return value.
 *        invscaler should be the inverse of the scaler given to the scaling function.
 * \return the correctly scaled decoded value. return = min + value*invscaler.
 */
float float32ScaledFromLongBitfield(uint64_t value, float min, float invscaler)
{
    return (float)(min + invscaler*value);
}

/*!
 * Inverse scale the long bitfield base integer type to a float64
 * \param value is the integer number to inverse scale
 * \param min is the minimum value that can be represented.
 * \param invscaler is multiplied by the integer to create the return value.
 *        invscaler should be the inverse of the scaler given to the scaling function.
 * \return the correctly scaled decoded value. return = min + value*invscaler.
 */
double float64ScaledFromLongBitfield(uint64_t value, double min, double invscaler)
{
    return (double)(min + invscaler*value);
}

#endif // UINT64_MAX

/*!
 * Test the bit field encode decode logic
 * \return 1 if the test passes, else 0
 */
int testBitfield(void)
{
    struct
    {
        unsigned int test1;     // :1;  1
        unsigned int test2;     // :2;  3
        unsigned int test3;     // :3;  6
        unsigned int test12;    // :12; 18
        unsigned int testa;     // :1;  19
        unsigned int testb;     // :2;  21
        unsigned int testc;     // :4;  25
        uint64_t     testd;     // :36; 61
    }test_t = {1, 2, 5, 0xABC, 0, 3, 4, 0xC87654321};

    uint8_t data[20];
    int index = 0;
    int bitcount = 0;

    encodeBitfield(test_t.test1, data, &index, &bitcount, 1);
    encodeBitfield(test_t.test2, data, &index, &bitcount, 2);
    encodeBitfield(test_t.test3, data, &index, &bitcount, 3);
    encodeBitfield(test_t.test12, data, &index, &bitcount, 12);
    encodeBitfield(test_t.testa, data, &index, &bitcount, 1);
    encodeBitfield(test_t.testb, data, &index, &bitcount, 2);
    encodeBitfield(test_t.testc, data, &index, &bitcount, 4);
    encodeLongBitfield(test_t.testd, data, &index, &bitcount, 36);


    index = bitcount = 0;

    memset(&test_t, 0, sizeof(test_t));

    test_t.test1 = decodeBitfield(data, &index, &bitcount, 1);
    test_t.test2 = decodeBitfield(data, &index, &bitcount, 2);
    test_t.test3 = decodeBitfield(data, &index, &bitcount, 3);
    test_t.test12 = decodeBitfield(data, &index, &bitcount, 12);
    test_t.testa = decodeBitfield(data, &index, &bitcount, 1);
    test_t.testb = decodeBitfield(data, &index, &bitcount, 2);
    test_t.testc = decodeBitfield(data, &index, &bitcount, 4);
    test_t.testd = decodeLongBitfield(data, &index, &bitcount, 36);

    if(test_t.test1 != 1)
        return 0;
    else if(test_t.test2 != 2)
        return 0;
    else if(test_t.test3 != 5)
        return 0;
    else if(test_t.test12 != 0xABC)
        return 0;
    else if(test_t.testa != 0)
        return 0;
    else if(test_t.testb != 3)
        return 0;
    else if(test_t.testc != 4)
        return 0;
    else if(test_t.testd != 0xC87654321)
        return 0;
    else
        return 1;

}// testBitfield

// end of bitfieldspecial.c
