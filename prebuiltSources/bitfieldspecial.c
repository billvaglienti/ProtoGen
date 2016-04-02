#include "bitfieldspecial.h"
#include <string.h>


/*!
 * Add a bit field to a byte stream.
 * \param value is the unsigned integer to encode, which can vary from 1 to 32
 *        bits in length. The bits encoded are the least significant (right
 *        most) bits of value
 * \param bytes is the byte stream to receive the bits
 * \param index is the current byte stream index, which will be incremented as
 *        needed.
 * \param bitcount is the current bit counter index in the current byte, which
 *        will be incremented as needed.
 * \param numbits is the number of bits in value to encode
 */
void encodeBitfield(unsigned int value, uint8_t* bytes, int* index, int* bitcount, int numbits)
{
    int bitstomove;
    while(numbits > 0)
    {
        // Start out with all bits zero
        if((*bitcount) == 0)
            bytes[*index] = 0;

        // imagine that bitcount is currently 2, i.e. the 2 left most bits
        // have already been encoded. In that case we want to encode 6 more
        // bits in the current byte.
        bitstomove = 8 - (*bitcount);

        // shift value to the correct alignment.
        if(bitstomove >= numbits)
        {
            // In this case all the bits in value will fit in the current byte
            bytes[*index] |= (uint8_t)(value << (bitstomove - numbits));
            (*bitcount) += numbits;
            numbits = 0;
        }
        else
        {
            // In this case we have too many bits in value to fit in the
            // current byte, encode the most significant bits that fit
            bytes[*index] |= (uint8_t)(value >> (numbits - bitstomove));
            (*bitcount) += bitstomove;
            numbits -= bitstomove;
        }

        // Check if we have moved to the next byte
        if((*bitcount) >= 8)
        {
            (*index)++;
            (*bitcount) = 0;
        }

    }// while still bits to encode

}// encodeBitfield


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
    uint8_t  byte;

    int bitstomove;
    while(numbits > 0)
    {
        // The current byte we are operating on
        byte = bytes[*index];

        // Remove any left most bits that we have already decoded
        byte = byte << (*bitcount);

        // Put the remaining bytes back in least significant position
        byte = byte >> (*bitcount);

        // Number of bits in the current byte that we could move
        bitstomove = 8 - (*bitcount);

        // It may be that some of the right most bits are not for
        // this bit field, we can tell by looking at numbits
        if(bitstomove >= numbits)
        {
            byte = byte >> (bitstomove - numbits);
            bitstomove = numbits;
            value |= byte;
        }
        else
        {
            // OR these bytes into position in value. The position is given by
            // numbits, which identifies the bit position of the most significant
            // bit.
            value |= ((unsigned int)byte) << (numbits - bitstomove);
        }

        // Count the bits
        numbits -= bitstomove;
        (*bitcount) += bitstomove;

        // Check if we have moved to the next byte
        if((*bitcount) >= 8)
        {
            (*index)++;
            (*bitcount) = 0;
        }

    }// while still bits to encode

    return value;

}// decodeBitfield


/*!
 * Scale a float64 to the base integer type used for bitfield
 * \param value is the number to scale.
 * \param min is the minimum value that can be encoded.
 * \param scaler is multiplied by value to create the integer: return = (value-min)*scaler.
 */
unsigned int float64ScaledToBitfield(double value, double min, double scaler)
{
    // scale the number
    value = (value - min)*scaler;

    // account for fractional truncation
    return (unsigned int)(value + 0.5);
}


/*!
 * Scale a float32 to the base integer type used for bitfield
 * \param value is the number to scale.
 * \param min is the minimum value that can be encoded.
 * \param scaler is multiplied by value to create the integer: return = (value-min)*scaler.
 */
unsigned int float32ScaledToBitfield(float value, float min, float scaler)
{
    // scale the number
    value = (value - min)*scaler;

    // account for fractional truncation
    return (unsigned int)(value + 0.5f);
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
 * Test the bit field encode decode logic
 * \return 1 if the test passes, else 0
 */
int testBitfield(void)
{
    struct
    {
        uint32_t test1;     // :1;  1
        uint32_t test2;     // :2;  3
        uint32_t test3;     // :3;  6
        uint32_t test12;    // :12; 18
        uint32_t testa;     // :1;  19
        uint32_t testb;     // :2;  20
        uint32_t testc;     // :4;  24

    }test_t = {1, 2, 5, 0xABC, 0, 3, 4};

    uint8_t data[10];
    int index = 0;
    int bitcount = 0;

    encodeBitfield(test_t.test1, data, &index, &bitcount, 1);
    encodeBitfield(test_t.test2, data, &index, &bitcount, 2);
    encodeBitfield(test_t.test3, data, &index, &bitcount, 3);
    encodeBitfield(test_t.test12, data, &index, &bitcount, 12);
    encodeBitfield(test_t.testa, data, &index, &bitcount, 1);
    encodeBitfield(test_t.testb, data, &index, &bitcount, 2);
    encodeBitfield(test_t.testc, data, &index, &bitcount, 4);

    index = bitcount = 0;

    memset(&test_t, 0, sizeof(test_t));

    test_t.test1 = decodeBitfield(data, &index, &bitcount, 1);
    test_t.test2 = decodeBitfield(data, &index, &bitcount, 2);
    test_t.test3 = decodeBitfield(data, &index, &bitcount, 3);
    test_t.test12 = decodeBitfield(data, &index, &bitcount, 12);
    test_t.testa = decodeBitfield(data, &index, &bitcount, 1);
    test_t.testb = decodeBitfield(data, &index, &bitcount, 2);
    test_t.testc = decodeBitfield(data, &index, &bitcount, 4);

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
    else
        return 1;

}// testBitfield
