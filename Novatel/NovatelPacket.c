#include "NovatelPacket.h"
#include "NovatelProtocol.h"
#include "NovatelStructures.h"
#include "fielddecode.h"
#include "fieldencode.h"

//! Generate CRC from a single word of data
static unsigned long CRC32Value(int i);

//! Generate CRC from a block of data
static unsigned long CalculateBlockCRC32(unsigned long ulCount, unsigned char *ucBuffer );

/*!
 * Look for a Novatel packet in a series of bytes provided one at a time.
 * \param byte is the next byte in the series to evaluate
 * \param pkt receives the Novatel packet. pkt *must* persist between calls
 *        to this function
 * \return 1 if a packet with a valid CRC is found, else 0.
 */
int lookForNovatelPacketInByte(uint8_t byte, NovatelPkt_t* pkt)
{
    // Protect against array bounds
    if(pkt->rxstate >= MAX_NOV_PKT_SIZE)
        pkt->rxstate = 0;

    // record byte in packet
    pkt->data[pkt->rxstate] = byte;

    // Look for packet synchronization bytes
    switch(pkt->rxstate)
    {
    case 0:
        if(byte == Sync0)
            pkt->rxstate++;
        break;

    case 1:
        if(byte == Sync1)
            pkt->rxstate++;
        else if(byte == Sync0)
            pkt->rxstate = 1;
        else
            pkt->rxstate = 0;
        break;

    case 2:
        if(byte == Sync2)
            pkt->rxstate++;
        else if(byte == Sync0)
            pkt->rxstate = 1;
        else
            pkt->rxstate = 0;
        break;

    // If we get past the synchronization bytes,
    // then simply count the number of bytes
    default:
        pkt->rxstate++;
        break;

    }// switch on receive state

    if(pkt->rxstate >= MinHeaderLength+4)
    {
        // Size of the CRC
        pkt->totalSize = 4;

        // Size of the message body, 16 bits little endian
        pkt->totalSize += 256*pkt->data[9];
        pkt->totalSize += pkt->data[8];

        // Size of the header
        pkt->totalSize += pkt->data[3];

        // Check to make sure the size is OK
        if(pkt->totalSize > MAX_NOV_PKT_SIZE)
            pkt->rxstate = 0;

        // If we have the bytes, check the crc
        if(pkt->rxstate >= pkt->totalSize)
        {
            // The byte index where the crc starts
            int byteindex = pkt->totalSize - 4;

            // The crc that was transmitted in the data stream
            uint32_t crc = uint32FromLeBytes(pkt->data, &byteindex);

            // Win or lose, we are starting over with this packet
            pkt->rxstate = 0;

            // Compare the crc against the computed value
            if(crc == CalculateBlockCRC32(pkt->totalSize - 4, pkt->data))
                return 1;

        }// if we have all the bytes we expect

    }// if we have the minimum bytes for a packet

    return 0;

}// lookForNovatelPacketInByte


/*!
 * Return the packet data pointer. Since ProtoGen knows about the packet header
 * in the Novatel case, the data pointer is the same as the packet pointer.
 * \param pkt is the packet pointer
 * \return the packet data pointer from the packet
 */
uint8_t* getNovatelPacketData(void* pkt)
{
    return (uint8_t*)pkt;
}


/*!
 * Return the packet const data pointer. Since ProtoGen knows about the packet header
 * in the Novatel case, the data pointer is the same as the packet pointer.
 * \param pkt is the const packet pointer
 * \return the packet data pointer from the packet, const
 */
const uint8_t* getNovatelPacketDataConst(const void* pkt)
{
    return (const uint8_t*)pkt;
}


/*!
 * Complete a packet after the data have been encoded. This will set the length
 * of the data body, the message ID, and the crc.
 * \param pkt is the packet data pointer.
 * \param size is the total size of the packet, which is the sum of the header
 *        length and data body lenght, excluding the crc.
 * \param packetID is the message ID to encode in the header.
 */
void finishNovatelPacket(void* pkt, int size, uint32_t packetID)
{
    int byteindex;
    uint32_t crc;
    uint8_t* data = (uint8_t*)pkt;

    // This is the Message ID number of the log.
    byteindex = 4;
    uint16ToLeBytes((uint16_t)packetID, data, &byteindex);

    // Set the length of the data body
    byteindex = 8;
    uint16ToLeBytes((uint16_t)(size - data[3]), data, &byteindex);

    // Compute the crc on the entire message, including header
    crc = CalculateBlockCRC32(size, data);

    // Append the crc to the packet
    byteindex = size;
    uint32ToLeBytes((uint32_t)crc, data, &byteindex);
}


/*!
 * Return the size of a packet. In the Novatel case this is the header length
 * plus the data body length.
 * \param pkt is the packet data pointer
 * \return the size of the packet including the header, but excluding the crc.
 */
int getNovatelPacketSize(const void* pkt)
{
    int byteindex = 8;
    uint8_t* data = (uint8_t*)pkt;

    // Length of the header
    int length = data[3];

    // Plus length of the message, not including the header or CRC.
    length += uint16FromLeBytes(data, &byteindex);

    return length;
}


/*!
 * Return the message ID from a Novatel packet
 * \param pkt is the packet data pointer
 * \return the ID of the packet
 */
uint32_t getNovatelPacketID(const void* pkt)
{
    int byteindex = 4;
    uint8_t* data = (uint8_t*)pkt;

    // This is the Message ID number of the log.
    return uint16FromLeBytes(data, &byteindex);
}

/*
 * Code below here was taken directly from the Novatle manual.
 */

#define CRC32_POLYNOMIAL 0xEDB88320L
/* --------------------------------------------------------------------------
Calculate a CRC value to be used by CRC calculation functions.
-------------------------------------------------------------------------- */
unsigned long CRC32Value(int i)
{
    int j;
    unsigned long ulCRC;
    ulCRC = i;

    for ( j = 8 ; j > 0; j-- )
    {
        if ( ulCRC & 1 )
           ulCRC = ( ulCRC >> 1 ) ^ CRC32_POLYNOMIAL;
        else
            ulCRC >>= 1;
    }

    return ulCRC;
}


/* --------------------------------------------------------------------------
Calculates the CRC-32 of a block of data all at once
-------------------------------------------------------------------------- */
unsigned long CalculateBlockCRC32(unsigned long ulCount, unsigned char *ucBuffer )
{
    unsigned long ulTemp1;
    unsigned long ulTemp2;
    unsigned long ulCRC = 0;

    while ( ulCount-- != 0 )
    {
        ulTemp1 = ( ulCRC >> 8 ) & 0x00FFFFFFL;
        ulTemp2 = CRC32Value( ((int) ulCRC ^ *ucBuffer++ ) & 0xff );
        ulCRC = ulTemp1 ^ ulTemp2;
    }

    return( ulCRC );
}
