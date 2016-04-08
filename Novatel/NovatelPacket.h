#ifndef _NOVATELPACKET_H
#define _NOVATELPACKET_H

// C++ compilers: don't mangle us
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

//! Maximum size of a novatel packet that we can process.
#define MAX_NOV_PKT_SIZE 1024

typedef struct
{
    uint8_t data[MAX_NOV_PKT_SIZE];  //!< The actual packet data including header, body, and crc
    uint32_t totalSize; //!< The number of bytes to transmit or receive including header, body, and crc
    uint32_t rxstate;   //!< The state of the receive state machine
}NovatelPkt_t;

//! Look for a Novatel packet in a sequence of bytes
int lookForNovatelPacketInByte(uint8_t byte, NovatelPkt_t* pkt);

#ifdef __cplusplus
}
#endif
#endif
