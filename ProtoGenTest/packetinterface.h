#ifndef PACKETINTERFACE_H
#define PACKETINTERFACE_H

/*!
 * This is a simple packet layout which is used only for testing ProtoGen. A
 * real packet layout would likely have synchronization bytes and crcs
 */
typedef struct
{
    uint8_t pkttype;    // The packet type information
    uint8_t length;     // The length of the data in bytes
    uint8_t data[255];  // The data of the packet

}testPacket_t;

#endif // PACKETINTERFACE_H
