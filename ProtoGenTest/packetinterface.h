#ifndef PACKETINTERFACE_H
#define PACKETINTERFACE_H

#include "DemolinkProtocol.h"

//! Look for an incoming packet in a sequence of bytes
int lookForDemolinkPacket(testPacket_t* pkt, uint8_t byte);

#endif // PACKETINTERFACE_H
