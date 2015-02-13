#include "DemolinkProtocol.h"
#include "packetinterface.h"

//! \return the packet data pointer from the packet
uint8_t* getDemolinkPacketData(void* pkt)
{
    return ((testPacket_t*)pkt)->data;
}

//! \return the packet data pointer from the packet
const uint8_t* getDemolinkPacketDataConst(const void* pkt)
{
    return ((testPacket_t*)pkt)->data;
}

//! Complete a packet after the data have been encoded
void finishDemolinkPacket(void* pkt, int size, uint32_t packetID)
{
    ((testPacket_t*)pkt)->pkttype = (uint8_t)packetID;
    ((testPacket_t*)pkt)->length = (uint8_t)size;
}

//! \return the size of a packet from the packet header
int getDemolinkPacketSize(const void* pkt)
{
    return ((testPacket_t*)pkt)->length;
}

//! \return the ID of a packet from the packet header
uint32_t getDemolinkPacketID(const void* pkt)
{
    return ((testPacket_t*)pkt)->pkttype;
}
