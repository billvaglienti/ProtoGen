#ifndef _NOVATELSHIM_H
#define _NOVATELSHIM_H

/*!
 * \file
 * \brief Special support functions for ProtoGen Novatel messages
 *
 * This module handles any message encodings or decodings that ProtoGen could
 * not handle. Chief among these are messages that use bitfields. Novatel
 * handles bitfields in the strangest way possible. This is particularly
 * evident when decoding the RangeCmp or RangeCmp2 messages.
 *
 * This module was hand-coded to be very similar to typical ProtoGen code. The
 * Novatel app note APN-031: Decoding RANGECMP and RANGECMP2
 * (http://www.novatel.com/assets/Documents/Bulletins/apn031.pdf) was
 * particularly helpful.
 */

// C++ compilers: don't mangle us
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "NovatelPackets.h"
#include "NovatelProtocol.h"
#include "NovatelStructures.h"

//! Decode the RangeCmp packet
int decodeRangeCmpPacketStructure(const void* pkt, Range_t* user);

//! return the packet ID for the RangeCmp packet
uint32_t getRangeCmpPacketID(void);

//! return the minimum data length for the RangeCmp packet
int getRangeCmpMinDataLength(void);

//! Verify correct range cmp decoding
int testRangeCmp(void);

#ifdef __cplusplus
}
#endif
#endif
