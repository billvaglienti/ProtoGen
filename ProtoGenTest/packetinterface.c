#include "DemolinkProtocol.h"



//! \return the packet data pointer from the packet
uint8_t* getDemolinkPacketData(void* pkt)
{
    return pkt;
}

//! \return the packet data pointer from the packet
const uint8_t* getDemolinkPacketDataConst(const void* pkt)
{
    return pkt;
}

//! Complete a packet after the data have been encoded
void finishDemolinkPacket(void* pkt, int size, uint32_t packetID)
{

}

//! \return the size of a packet from the packet header
int getDemolinkPacketSize(const void* pkt)
{
    return 0;
}

//! \return the ID of a packet from the packet header
uint32_t getDemolinkPacketID(const void* pkt)
{
    return 0;
}
