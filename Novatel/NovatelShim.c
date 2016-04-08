#include <string.h>
#include "NovatelPackets.h"
#include "NovatelProtocol.h"
#include "NovatelStructures.h"
#include "floatspecial.h"
#include "fielddecode.h"
#include "fieldencode.h"
#include "scaleddecode.h"
#include "scaledencode.h"

//! Decode the rangecmp record (24 bytes)
static int decodeRangeCmpRecord(const uint8_t* data, int* bytecount, range_t* user);

//! Use the data in a range record to correct the accumulated doppler range
static void performAdrCorrection(range_t* range);

//! return the packet ID for the RangeCmp packet
uint32_t getRangeCmpPacketID(void)
{
    return RANGECMP;
}

//! return the minimum data length for the RangeCmp packet
int getRangeCmpMinDataLength(void)
{
    // A single observation takes a header, a numObs, and one record
    return 28+4+24;
}

/*!
 * \brief Decode the RangeCmp packet
 *
 * The raw data.
 * \param pkt points to the packet being decoded by this function
 * \param user receives the data decoded from the packet
 * \return 0 is returned if the packet ID or size is wrong, else 1
 */
int decodeRangeCmpPacketStructure(const void* pkt, Range_t* user)
{
    int numBytes;
    int byteindex = 0;
    const uint8_t* data;
    int i = 0;

    // Verify the packet identifier
    if(getNovatelPacketID(pkt) != getRangeCmpPacketID())
        return 0;

    // Verify the packet size
    numBytes = getNovatelPacketSize(pkt);
    if(numBytes < getRangeCmpMinDataLength())
        return 0;

    // The raw data from the packet
    data = getNovatelPacketDataConst(pkt);

    // Header information for this packet.
    if(decodeHeader_t(data, &byteindex, &user->header) == 0)
        return 0;

    // Account for extended Novatel headers
    byteindex += (user->header.headerLength - MinHeaderLength);

    // Number of satellite observations with information to follow
    user->numObs = (uint32_t)uint32FromLeBytes(data, &byteindex);

    // Make sure all observations are blanked
    memset(user->range, 0, sizeof(user->range));

    // The raw measurements from each channel
    for(i = 0; i < (int)user->numObs && i < 24; i++)
    {
        if(decodeRangeCmpRecord(data, &byteindex, &user->range[i]) == 0)
            return 0;
    }

    // Used variable length arrays or dependent fields, check actual length
    if(numBytes < byteindex)
        return 0;

    return 1;

}// decodeRangeCmpPacketStructure


/*!
 * Decode a series of bytes that represent the 24 bytes of rangecmp. This
 * decoding is enormously weird, see APN-031: Decoding RANGECMP and RANGECMP2
 * \param data points to the byte array to decoded data from
 * \param bytecount points to the starting location in the byte array, and will be incremented by the number of bytes decoded
 * \param user is the data to decode from the byte array
 * \return 1 if the data are decoded, else 0. If 0 is returned bytecount will not be updated.
 */
int decodeRangeCmpRecord(const uint8_t* data, int* bytecount, range_t* user)
{
    // Lookup table to convert pseudo range deviation code into meters
    static const float psrDevs[16] = {0.050f, 0.075f, 0.113f, 0.169f, 0.253f, 0.380f, 0.570f, 0.854f, 1.281f, 2.375f, 4.75f, 9.5f, 19.0f, 38.0f, 76.0f, 152.0f};

    int byteindex = *bytecount;

    int trackcount = 0;
    uint8_t temp8;
    uint16_t temp16;
    uint32_t temp32;
    uint64_t temp64;

    // Tracking status bytes, in swapped order
    user->trackingStatusBytes[0] = data[byteindex + 3];
    user->trackingStatusBytes[1] = data[byteindex + 2];
    user->trackingStatusBytes[2] = data[byteindex + 1];
    user->trackingStatusBytes[3] = data[byteindex];
    byteindex += 4;

    // Channel tracking status bits
    if(decodeTrackingStatus_t(user->trackingStatusBytes, &trackcount, &user->trackingStatus) == 0)
        return 0;

    // Decode from little endian byte order to effect swap, byteindex incremented by 4
    temp32 = uint32FromLeBytes(data, &byteindex);

    // discard 4 most significant bits to get to 28 bits
    temp32 &= 0x0FFFFFFF;

    // reverse byte index by 1, because we want those 4 bits back for the next field
    byteindex--;

    // Each bit is worth 1/256 of a Hertz
    user->dopplerFrequency = temp32*(1.0f/256.0f);

    // Decode from litle endian byte order to effect swap, byteindex incremented by 5
    temp64 = uint40FromLeBytes(data, &byteindex);

    // shift 4 bits right to discard least significant bits to get to 36 bits
    temp64 = temp64 >> 4;

    // Each bit is worth 1/128 of a Hertz
    user->pseudoRange = temp64*(1.0/128.0);

    // Accumulated doppler (carrier phase) in 1/256 of a cycle. This value still needs correction
    user->accumulatedDoppler = float64ScaledFrom4SignedLeBytes(data, &byteindex, 1.0/256.0);

    // Perform correction using psuedo range information
    performAdrCorrection(user);

    // The pseudo range standard deviation, using 4 bit lookup
    user->psrDeviation = psrDevs[data[byteindex] & 0x0F];

    // The ADR standard deviation, 4 upper bits of this byte
    temp8 = data[byteindex++] >> 4;

    // ADR standard deviation in cycles
    user->adrDeviation = (temp8 + 1)/512.0f;

    // The PRN/SLOT number for this satellite
    user->prn = data[byteindex++];

    // Decode from little endian byte order to effect swap, byteindex incremented by 3
    temp32 = uint24FromLeBytes(data, &byteindex);

    // Mask off upper bits to get 21 bits
    temp32 = temp32 & 0x001FFFFF;

    // reverse byte index by 1, because we want those 3 bits back for the next field
    byteindex--;

    // Lock time in 1/32 of a second
    user->lockTime = temp32*(1.0f/32.0f);

    // Decode from little endian byte order to effect swap, byteindex incremented by 2
    temp16 = uint16FromLeBytes(data, &byteindex);

    // Mask off 6 upper bits
    temp16 &= 0x03FF;

    // Shift 5 bits to the right
    temp16 = temp16 >> 5;

    // Compute CNo
    user->cno = temp16 + 20.0f;

    // Reserved bits at the end
    byteindex += 2;

    *bytecount = byteindex;

    return 1;

}// decodeRangeCmpRecord


/*!
 * Use the data in a range record to correct the accumulated doppler range,
 * assuming the current ADR value came from the RangeCmp message
 * \param range is the range record whose ADR needs correcting
 */
void performAdrCorrection(range_t* range)
{
    double wavelength;
    double adrrolls;
    int32_t adrollsint;

    /// TODO: add non-GPS support? add L5 support?

    if(range->trackingStatus.signalType == L1CA_SIGNAL)
        wavelength = 0.1902936727984;
    else
        wavelength = 0.2442102134246;

    // Compute the number of times ADR has rolled over
    adrrolls = (range->pseudoRange/wavelength + range->accumulatedDoppler)*(1.0/8388608.0);

    if(adrrolls > 0.0)
        adrollsint = (int32_t)(adrrolls + 0.5);
    else
        adrollsint = (int32_t)(adrrolls - 0.5);

    // Now perform the correction
    range->accumulatedDoppler = range->accumulatedDoppler - (adrollsint*8388608.0);

}// performAdrCorrection


/*!
 * Perform a test of the rangecmp2 decode function. This uses test data from
 * Novatel's document: APN-031: Decoding RANGECMP and RANGECMP2
 * \return 1 if the test passed, else 0
 */
int testRangeCmp(void)
{
    // test string from Novatel app note
    uint8_t data[] = {0x24, 0x9C, 0x10, 0x08, 0x0e, 0x63, 0x06, 0x20, 0x6A, 0xBA, 0xF7, 0x0B, 0x29, 0x7A, 0xE7, 0xF9, 0x40, 0x1B, 0x81, 0x8E, 0x01, 0x03, 0x00, 0x00};

    range_t range = {0};
    int byteindex = 0;

    decodeRangeCmpRecord(data, &byteindex, &range);

    if(range.trackingStatus.channelAssignmentForced != 0)
        return 0;

    if(range.trackingStatus.prnLockFlag != 0)
        return 0;

    if(range.trackingStatus.halfCycleAdded != 0)
        return 0;

    if(range.trackingStatus.primaryL1Channel != 1)
        return 0;

    if(range.trackingStatus.signalType != L1CA_SIGNAL)
        return 0;

    if(range.trackingStatus.grouping != 1)
        return 0;

    if(range.trackingStatus.satelliteSystem != GPS_SAT)
        return 0;

    if(range.trackingStatus.correlator != pulseApertureCorrelator)
        return 0;

    if(range.trackingStatus.codeLocked != 1)
        return 0;

    if(range.trackingStatus.parityKnown != 1)
        return 0;

    if(range.trackingStatus.phaseLocked != 1)
        return 0;

    if(range.trackingStatus.channel != 1)
        return 0;

    if(range.trackingStatus.trackingState != PLL_TRACKING)
        return 0;

    if(range.dopplerFrequency != 1635.05469f)
        return 0;

    if(range.pseudoRange != 25098061.265625)
        return 0;

    if(range.accumulatedDoppler != -134617221.83984375)
        return 0;

    if(range.psrDeviation != 0.05f)
        return 0;

    if(range.adrDeviation != 0.009765625f)
        return 0;

    if(range.prn != 27)
        return 0;

    if(range.lockTime != 3188.03125f)
        return 0;

    if(range.cno != 44.0f)
        return 0;

    return 1;

}
