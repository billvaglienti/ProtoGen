#include "packetinterface.h"

//! Determine if a demolink packet is valid based on its checksum
static int validateDemolinkPacket(const testPacket_c* pkt);

//! Compute the Fletcher 16 checksum on a hunk of bytes
static uint16_t fletcher16( uint8_t const *data, int bytes );

//! \return the packet data pointer from the packet
uint8_t* getDemolinkPacketData(testPacket_c* pkt)
{
    return ((testPacket_c*)pkt)->data;
}

//! \return the packet data pointer from the packet
const uint8_t* getDemolinkPacketDataConst(const testPacket_c* pkt)
{
    return ((testPacket_c*)pkt)->data;
}

//! Complete a packet after the data have been encoded
void finishDemolinkPacket(testPacket_c* pkt, int size, uint32_t packetID)
{
    uint16_t check;

    pkt->sync0 = TEST_PKT_SYNC_BYTE0;
    pkt->sync1 = TEST_PKT_SYNC_BYTE1;
    pkt->pkttype = (uint8_t)packetID;
    pkt->length = (uint8_t)size;

    // Compute and apply the checksum
    check = fletcher16((uint8_t*)pkt, size+4);
    pkt->data[size] = (uint8_t)(check>>8);
    pkt->data[size+1] = (uint8_t)(check);

}

//! \return the size of a packet from the packet header
int getDemolinkPacketSize(const testPacket_c* pkt)
{
    return pkt->length;
}

//! \return the ID of a packet from the packet header
uint32_t getDemolinkPacketID(const testPacket_c* pkt)
{
    return pkt->pkttype;
}


/*!
 * Look for a demolink packet in a series of bytes provided one at a time.
 * \param pkt receives the packet. pkt *must* persist between calls
 *        to this function
 * \param byte is the next byte in the series to evaluate
 * \return 1 if a packet with a valid checksum is found, else 0.
 */
int lookForDemolinkPacket(testPacket_c* pkt, uint8_t byte)
{
    // Protect against packet bounds, this should never happen because of code
    // below, but in case pkt is uninitialized we include this.
    if(pkt->rxstate >= (TEST_PKT_MAX_DATA + TEST_PKT_OVERHEAD))
        pkt->rxstate = 0;

    // record byte in packet, this is header and data
    ((uint8_t*)pkt)[pkt->rxstate] = byte;

    // Look for packet synchronization bytes
    switch(pkt->rxstate)
    {
    case 0:
        pkt->length = 0;
        if(byte == TEST_PKT_SYNC_BYTE0)
            pkt->rxstate++;
        break;

    case 1:
        if(byte == TEST_PKT_SYNC_BYTE1)
            pkt->rxstate++;
        else if(byte == TEST_PKT_SYNC_BYTE0)
        {
            // Previous byte was a false positive packet start
            pkt->sync0 = TEST_PKT_SYNC_BYTE0;
            pkt->rxstate = 1;
        }
        else
            pkt->rxstate = 0;
        break;

    // If we get past the synchronization  bytes,
    // then simply count the number of bytes
    default:
        pkt->rxstate++;

        // check to see if the packet is valid
        if(pkt->rxstate >= pkt->length + TEST_PKT_OVERHEAD)
        {
            // Starting over no matter what
            pkt->rxstate = 0;

            // Check computed versus transmitted checksum
            if(validateDemolinkPacket(pkt))
                return 1;
            else
                return 0;

        }// if we have all the data

        break;

    }// switch on receive state

    return 0;

}// lookForDemolinkPacket


/*!
 * Check a received packet for correct checksum.
 * \param pkt is the packet to check.
 * \return 1 if the packet checksum is correct, else 0.
 */
int validateDemolinkPacket(const testPacket_c* pkt)
{
    uint16_t check = (pkt->data[pkt->length] << 8) | pkt->data[pkt->length+1];

    // Compute and apply the checksum
    if(fletcher16((uint8_t*)pkt, pkt->length+4) == check)
        return 1;
    else
        return 0;
}


/*!
 * Compute the Fletcher 16 on a hunk of bytes.
 * \param data are the data bytes to compute the checksum for.
 * \param bytes is the number of bytes to use in the computation.
 * \return the 16-bit Fletcher's checksum of data.
 */
uint16_t fletcher16( uint8_t const *data, int bytes )
{
    uint16_t sum1 = 0xff, sum2 = 0xff;
    int tlen;

    while (bytes)
    {
        // 20 is a magic number that guarantees no overflow in the worst case
        tlen = bytes >= 20 ? 20 : bytes;
        bytes -= tlen;
        do
        {
            sum2 += sum1 += *data++;
        } while (--tlen);

        sum1 = (sum1 & 0xff) + (sum1 >> 8);
        sum2 = (sum2 & 0xff) + (sum2 >> 8);
    }
    /* Second reduction step to reduce sums to 8 bits */
    sum1 = (sum1 & 0xff) + (sum1 >> 8);
    sum2 = (sum2 & 0xff) + (sum2 >> 8);
    return sum2 << 8 | sum1;
}
