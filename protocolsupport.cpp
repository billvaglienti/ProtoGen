#include "protocolsupport.h"
#include "protocolparser.h"

ProtocolSupport::ProtocolSupport() :
    maxdatasize(0),
    int64(true),
    float64(true),
    specialFloat(true),
    bitfield(true),
    longbitfield(false),
    bitfieldtest(false),
    disableunrecognized(false),
    bigendian(true),
    packetStructureSuffix("PacketStructure"),
    packetParameterSuffix("Packet")
{
}


//! Return the list of attributes understood by ProtocolSupport
QStringList ProtocolSupport::getAttriblist(void) const
{
    QStringList attribs;

    attribs << "maxSize" << "supportInt64" << "supportFloat64" << "supportSpecialFloat" << "supportBitfield" << "supportLongBitfield" << "bitfieldTest" << "file" << "prefix" << "packetStructureSuffix" << "packetParameterSuffix" << "endian";

    return attribs;
}


void ProtocolSupport::parse(const QDomNamedNodeMap& map)
{
    // Maximum bytes of data in a packet.
    maxdatasize = ProtocolParser::getAttribute("maxSize", map, "0").toInt();

    // 64-bit support can be turned off
    if(ProtocolParser::isFieldClear(ProtocolParser::getAttribute("supportInt64", map)))
        int64 = false;

    // double support can be turned off
    if(ProtocolParser::isFieldClear(ProtocolParser::getAttribute("supportFloat64", map)))
        float64 = false;

    // special float support can be turned off
    if(ProtocolParser::isFieldClear(ProtocolParser::getAttribute("supportSpecialFloat", map)))
        specialFloat = false;

    // bitfield support can be turned off
    if(ProtocolParser::isFieldClear(ProtocolParser::getAttribute("supportBitfield", map)))
        bitfield = false;

    // long bitfield support can be turned on
    if(int64 && ProtocolParser::isFieldSet(ProtocolParser::getAttribute("supportLongBitfield", map)))
        longbitfield = true;

    // bitfield test support can be turned on
    if(ProtocolParser::isFieldSet(ProtocolParser::getAttribute("bitfieldTest", map)))
        bitfieldtest = true;

    // Global file names can be specified, but cannot have a "." in it
    globalFileName = ProtocolParser::getAttribute("file", map);
    globalFileName = globalFileName.left(globalFileName.indexOf("."));

    // Prefix is not required
    prefix = ProtocolParser::getAttribute("prefix", map);

    // Packet name post fixes
    packetStructureSuffix = ProtocolParser::getAttribute("packetStructureSuffix", map, packetStructureSuffix);
    packetParameterSuffix = ProtocolParser::getAttribute("packetParameterSuffix", map, packetParameterSuffix);

    if(ProtocolParser::getAttribute("endian", map).contains("little", Qt::CaseInsensitive))
        bigendian = false;

}// ProtocolSupport::parse
