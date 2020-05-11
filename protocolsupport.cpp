#include "protocolsupport.h"
#include "protocolparser.h"

#include <QStringList>

ProtocolSupport::ProtocolSupport() :
    language(c_language),
    maxdatasize(0),
    int64(true),
    float64(true),
    specialFloat(true),
    bitfield(true),
    longbitfield(false),
    bitfieldtest(false),
    disableunrecognized(false),
    bigendian(true),
    supportbool(false),
    limitonencode(false),
    compare(false),
    print(false),
    mapEncode(false),
    packetStructureSuffix("PacketStructure"),
    packetParameterSuffix("Packet"),
    enablelanguageoverride(false)
{
}


//! Return the list of attributes understood by ProtocolSupport
QStringList ProtocolSupport::getAttriblist(void) const
{
    QStringList attribs;

    attribs << "maxSize"
            << "supportInt64"
            << "supportFloat64"
            << "supportSpecialFloat"
            << "supportBitfield"
            << "supportLongBitfield"
            << "bitfieldTest"
            << "file"
            << "verifyfile"
            << "comparefile"
            << "printfile"
            << "mapfile"
            << "prefix"
            << "packetStructureSuffix"
            << "packetParameterSuffix"
            << "endian"
            << "pointer"
            << "supportBool"
            << "limitOnEncode"
            << "C"
            << "CPP"
            << "compare"
            << "print"
            << "map";

    return attribs;
}


/*!
 * Parse the attributes for this support object from the DOM map
 * \param map is the DOM map
 */
void ProtocolSupport::parse(const XMLAttribute* map)
{
    if(!enablelanguageoverride)
    {
        // The type of language, C and C++ are the only supported options right now
        if(ProtocolParser::isFieldSet(ProtocolParser::getAttribute("CPP", map)))
            language = cpp_language;
        else
            language = c_language;

        // Don't let any copies of us override the language setting. We need this to be the same for all support objects.
        enablelanguageoverride = true;
    }

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
    if(int64 && ProtocolParser::isFieldSet("supportLongBitfield", map))
        longbitfield = true;

    // bitfield test support can be turned on
    if(ProtocolParser::isFieldSet("bitfieldTest", map))
        bitfieldtest = true;

    // bool support default is based on language type
    if(language == c_language)
        supportbool = false;
    else
        supportbool = true;

    // bool support can be turned on or off
    if(ProtocolParser::isFieldSet("supportBool", map))
        supportbool = true;
    else if(ProtocolParser::isFieldClear("supportBool", map))
        supportbool = false;

    // Limit on encode can be turned on
    if(ProtocolParser::isFieldSet("limitOnEncode", map))
        limitonencode = true;

    // Global flags to force output for compare, print, and map functions
    compare = ProtocolParser::isFieldSet(ProtocolParser::getAttribute("compare", map));
    print = ProtocolParser::isFieldSet(ProtocolParser::getAttribute("print", map));
    mapEncode = ProtocolParser::isFieldSet(ProtocolParser::getAttribute("map", map));

    // The global file names
    parseFileNames(map);

    // Prefix is not required
    prefix = ProtocolParser::getAttribute("prefix", map);

    // Packet pointer type (default is 'void')
    pointerType = ProtocolParser::getAttribute("pointer", map, "void*");

    // Must be a pointer type
    if(!pointerType.endsWith("*"))
        pointerType += "*";

    // Packet name post fixes
    packetStructureSuffix = ProtocolParser::getAttribute("packetStructureSuffix", map, packetStructureSuffix);
    packetParameterSuffix = ProtocolParser::getAttribute("packetParameterSuffix", map, packetParameterSuffix);

    if(ProtocolParser::getAttribute("endian", map).contains("little", Qt::CaseInsensitive))
        bigendian = false;

}// ProtocolSupport::parse


/*!
 * Parse the global file names used for this support object from the DOM map
 * \param map is the DOM map
 */
void ProtocolSupport::parseFileNames(const XMLAttribute* map)
{
    // Global file names can be specified, but cannot have a "." in it
    globalFileName = ProtocolParser::getAttribute("file", map);
    globalFileName = globalFileName.left(globalFileName.indexOf("."));
    globalVerifyName = ProtocolParser::getAttribute("verifyfile", map);
    globalVerifyName = globalVerifyName.left(globalVerifyName.indexOf("."));
    globalCompareName = ProtocolParser::getAttribute("comparefile", map);
    globalCompareName = globalCompareName.left(globalCompareName.indexOf("."));
    globalPrintName = ProtocolParser::getAttribute("printfile", map);
    globalPrintName = globalPrintName.left(globalPrintName.indexOf("."));
    globalMapName = ProtocolParser::getAttribute("mapfile", map);
    globalMapName = globalMapName.left(globalMapName.indexOf("."));

}// ProtocolSupport::parseFileNames
