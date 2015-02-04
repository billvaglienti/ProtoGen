#include "bitfieldspecial.h"
#include <string.h>

/*!
 * Add a bit field to a byte stream. It is recommended that the bit field be no
 * more than 8 bits in length or problems decoding the byte stream could occur
 * if the decoder is different endian than the encoder
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
void encodeBitfield(uint32_t value, uint8_t* bytes, int* index, int* bitcount, int numbits)
{
    int bitstomove;
    while(numbits > 0)
    {
        // Start out with all bits zero
        if(*bitcount == 0)
            bytes[*index] = 0;

        // imagine that bitcount is currently 2, i.e. the 2 left most bits
        // have already been encoded. In that case we want to encode 6 more
        // bits in the current byte.
        bitstomove = 8 - *bitcount;

        // shift value to the correct alignment.
        if(bitstomove >= numbits)
        {
            // In this case all the bits in value will fit in the current byte
            bytes[*index] |= (uint8_t)(value << (bitstomove - numbits));
            *bitcount += numbits;
            numbits = 0;
        }
        else
        {
            // In this case we have too many bits in value to fit in the
            // current byte, encode the most significant bits that fit
            bytes[*index] |= (uint8_t)(value >> (numbits - bitstomove));
            *bitcount += bitstomove;
            numbits -= bitstomove;
        }

        // Check if we have moved to the next byte
        if(*bitcount >= 8)
        {
            (*index)++;
            *bitcount = 0;
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
uint32_t decodeBitfield(const uint8_t* bytes, int* index, int* bitcount, int numbits)
{
    uint32_t value = 0;
    uint32_t byte;

    int bitstomove;
    while(numbits > 0)
    {
        // The current byte we are operating on
        byte = bytes[*index];

        // Remove any left most bits that we have already decoded
        byte = byte << *bitcount;

        // Put the remaining bytes back in least significant position
        byte = byte >> *bitcount;

        // Number of bits in the current byte that we could move
        bitstomove = 8 - *bitcount;

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
            value |= byte << (numbits - bitstomove);
        }

        // Count the bits
        numbits -= bitstomove;
        *bitcount += bitstomove;

        // Check if we have moved to the next byte
        if(*bitcount >= 8)
        {
            (*index)++;
            *bitcount = 0;
        }

    }// while still bits to encode

    return value;

}// decodeBitfield



/*!
 * Test the bit field encode decode logic
 * \return 1 if the test passes, else 0
 */
int testBitfield(void)
{
    struct
    {
        uint32_t test1 : 1;
        uint32_t test2 : 2;
        uint32_t test3 : 3;
        uint32_t test12 : 12;
        uint32_t testa : 1;
        uint32_t testb : 2;

    }test_t = {1, 2, 5, 0xABC, 0, 3};

    uint8_t data[10];
    int index = 0;
    int bitcount = 0;

    encodeBitfield(test_t.test1, data, &index, &bitcount, 1);
    encodeBitfield(test_t.test2, data, &index, &bitcount, 2);
    encodeBitfield(test_t.test3, data, &index, &bitcount, 3);
    encodeBitfield(test_t.test12, data, &index, &bitcount, 12);
    encodeBitfield(test_t.testa, data, &index, &bitcount, 1);
    encodeBitfield(test_t.testb, data, &index, &bitcount, 2);

    index = bitcount = 0;

    memset(&test_t, 0, sizeof(test_t));

    test_t.test1 = decodeBitfield(data, &index, &bitcount, 1);
    test_t.test2 = decodeBitfield(data, &index, &bitcount, 2);
    test_t.test3 = decodeBitfield(data, &index, &bitcount, 3);
    test_t.test12 = decodeBitfield(data, &index, &bitcount, 12);
    test_t.testa = decodeBitfield(data, &index, &bitcount, 1);
    test_t.testb = decodeBitfield(data, &index, &bitcount, 2);

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
    else
        return 1;

}// testBitfield
