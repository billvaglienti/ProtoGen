#include "protocolpacket.h"
#include "protocolfield.h"
#include "enumcreator.h"
#include "protocolfield.h"
#include "protocolstructure.h"
#include "protocolparser.h"
#include <QDateTime>
#include <QStringList>
#include <iostream>


/*!
 * Construct the object that parses packet descriptions
 * \param protocolName is the name of the protocol
 * \param protocolPrefix is the prefix string to use
 * \param supported gives the supported features of the protocol
 * \param protocolApi is the API string of the protocol
 * \param protocolVersion is the version string of the protocol
 * \param bigendian should be true to encode multi-byte fields with the most
 *        significant byte first.
 */
ProtocolPacket::ProtocolPacket(const QString& protocolName, const QString& protocolPrefix, ProtocolSupport supported, const QString& protocolApi, const QString& protocolVersion, bool bigendian) :
    ProtocolStructureModule(protocolName, protocolPrefix, supported, protocolApi, protocolVersion, bigendian)
{
}


ProtocolPacket::~ProtocolPacket(void)
{
    clear();
}


/*!
 * Clear out any data, resetting for next packet parse operation
 */
void ProtocolPacket::clear(void)
{
    ProtocolStructureModule::clear();
    id.clear();

    // Note that data set during constructor are not changed

}


/*!
 * Create the source and header files that represent a packet
 * \param packet is DOM element that defines the packet
 */
void ProtocolPacket::parse(const QDomElement& e)
{
    // Initialize metadata
    clear();

    // Me and all my children, which may themselves be structures
    ProtocolStructure::parse(e);

    if(ProtocolParser::isFieldClear(e, "encode"))
        encode = false;

    if(ProtocolParser::isFieldClear(e, "decode"))
        decode = false;

    if(isArray())
    {
        std::cout << name.toStdString() << ": packets cannot be an array" << std::endl;
        array.clear();
        variableArray.clear();
    }

    if(!dependsOn.isEmpty())
    {
        std::cout << name.toStdString() << ": dependsOn makes no sense for a packet" << std::endl;
        dependsOn.clear();
    }

    // The file directive allows us to override the file name
    QString moduleName = e.attribute("file").trimmed();

    if(moduleName.isEmpty())
        moduleName = support.globalFileName;

    if(moduleName.isEmpty())
    {
        // The file names
        header.setModuleName(prefix + name + "Packet");
        source.setModuleName(prefix + name + "Packet");
    }
    else
    {
        // The file names
        header.setModuleName(moduleName);
        source.setModuleName(moduleName);
    }

    // We may be appending data to an already existing file
    header.prepareToAppend();
    source.prepareToAppend();

    if(!header.isAppending())
    {
        // Comment block at the top of the header file
        header.write("/*!\n");
        header.write(" * \\file\n");
        header.write(" * \\brief " + header.fileName() + " defines the interface for the " + name + " packet of the " + protoName + " protocol stack\n");

        // A potentially long comment that should be wrapped at 80 characters
        if(!comment.isEmpty())
        {
            header.write(" *\n");
            header.write(ProtocolParser::outputLongComment(" *", comment) + "\n");
        }

        // Finish the top comment block
        header.write(" */\n");

        // White space is good
        header.makeLineSeparator();

        // Include the protocol top level module
        header.writeIncludeDirective(protoName + "Protocol.h");
    }
    else
        header.makeLineSeparator();

    // Add other includes specific to this packet
    ProtocolParser::outputIncludes(header, e);

    // Include directives that may be needed for our children
    for(int i = 0; i < encodables.length(); i++)
        header.writeIncludeDirective(encodables[i]->getIncludeDirective());

    // White space is good
    header.makeLineSeparator();

    bool structureFunctions = ProtocolParser::isFieldSet(e, "structureInterface");
    bool parameterFunctions = ProtocolParser::isFieldSet(e, "parameterInterface");

    if((structureFunctions == false) && (parameterFunctions == false))
    {
        // If the user gave us no guidance (or turned both off, which is the
        // same as no guidance), make a choice based on the size of the
        // encodable list. If we only have 1 parameter, there is no sense in
        // wrapping it in a structure
        if((getNumberOfEncodeParameters() > 1) && (getNumberOfDecodeParameters() > 1))
            structureFunctions = true;
        else
            parameterFunctions = true;
    }

    // Create the structure definition in the header.
    // This includes any sub-structures as well
    header.write(getStructureDeclaration(structureFunctions));

    // White space is good
    header.makeLineSeparator();

    // Include the helper files in the source, but only do this once
    if(!source.isAppending())
    {
        // White space is good
        source.makeLineSeparator();

        if(support.specialFloat)
            source.writeIncludeDirective("floatspecial.h");

        if(support.bitfield)
            source.writeIncludeDirective("bitfieldspecial.h");

        source.writeIncludeDirective("fielddecode.h");
        source.writeIncludeDirective("fieldencode.h");
        source.writeIncludeDirective("scaleddecode.h");
        source.writeIncludeDirective("scaledencode.h");
    }

    // The functions that include structures which are children of this
    // packet. These need to be declared before the main functions
    createSubStructureFunctions();

    // The functions that encode and decode the packet from a structure.
    if(structureFunctions)
        createStructurePacketFunctions();

    // The functions that encode and decode the packet from parameters
    if(parameterFunctions)
        createPacketFunctions();

    // Utility functions for ID, length, etc.
    createUtilityFunctions(e);

    // White space is good
    header.makeLineSeparator();

    // Write to disk
    header.flush();
    source.flush();

    // Make sure these are empty for next time around
    header.clear();
    source.clear();

}// ProtocolPacket::parse


/*!
 * Create utility functions for packet ID and lengths. The structure must
 * already have been parsed to give the lengths
 * \param e is the top level DOM element that represents the packet.
 */
void ProtocolPacket::createUtilityFunctions(const QDomElement& e)
{
    id = e.attribute("ID");

    // If no ID is supplied then use the packet name in upper case,
    // assuming that the user will define it elsewhere
    if(id.isEmpty())
        id = name.toUpper();

    // The prototype for the packet ID
    header.makeLineSeparator();
    header.write("//! return the packet ID for the " + prefix + name + " packet\n");
    header.write("uint32_t get" + prefix + name + "PacketID(void);\n");

    // And the source code
    source.makeLineSeparator();
    source.write("/*!\n");
    source.write(" * \\return the packet ID for the " + prefix + name + " packet\n");
    source.write(" */\n");
    source.write("uint32_t get" + prefix + name + "PacketID(void)\n");
    source.write("{\n");
    source.write("    return " + id + ";\n");
    source.write("}\n");

    // The prototype for the minimum packet length
    header.makeLineSeparator();
    header.write("//! return the minimum data length for the " + prefix + name + " packet\n");
    header.write("int get" + prefix + name + "MinDataLength(void);\n");

    // And the source code
    source.makeLineSeparator();
    source.write("/*!\n");
    source.write(" * \\return the minimum data length in bytes for the " + prefix + name + " packet\n");
    source.write(" */\n");
    source.write("int get" + prefix + name + "MinDataLength(void)\n");
    source.write("{\n");
    if(encodedLength.minEncodedLength.isEmpty())
        source.write("    return 0;\n");
    else
        source.write("    return " + encodedLength.minEncodedLength + ";\n");
    source.write("}\n");

}// ProtocolPacket::createUtilityFunctions


/*!
 * Create the functions for encoding and decoding the packet to/from a structure
 */
void ProtocolPacket::createStructurePacketFunctions(void)
{
    int numEncodes = getNumberOfEncodeParameters();
    int numDecodes = getNumberOfDecodeParameters();

    if(encode)
    {
        // The prototype for the packet encode function
        header.makeLineSeparator();
        header.write("//! " + getPacketEncodeBriefComment() + "\n");
        if(numEncodes > 0)
            header.write("void encode" + prefix + name + "PacketStructure(void* pkt, const " + typeName + "* user);\n");
        else
            header.write("void encode" + prefix + name + "PacketStructure(void* pkt);\n");
    }

    if(decode)
    {
        // The prototype for the packet decode function
        header.makeLineSeparator();
        header.write("//! " + getPacketDecodeBriefComment() + "\n");

        if(numDecodes > 0)
            header.write("int decode" + prefix + name + "PacketStructure(const void* pkt, " + typeName + "* user);\n");
        else
            header.write("int decode" + prefix + name + "PacketStructure(const void* pkt);\n");
    }

    if(encode)
    {
        // The source function for the encode function
        source.makeLineSeparator();
        source.write("/*!\n");
        source.write(" * \\brief " + getPacketEncodeBriefComment() + "\n");
        source.write(" *\n");
        source.write(ProtocolParser::outputLongComment(" *", comment) + "\n");
        source.write(" * \\param pkt points to the packet which will be created by this function\n");

        if(numEncodes > 0)
        {
            source.write(" * \\param user points to the user data that will be encoded in pkt\n");
            source.write(" */\n");
            source.write("void encode" + prefix + name + "PacketStructure(void* pkt, const " + typeName + "* user)\n");
            source.write("{\n");
        }
        else
        {
            source.write(" */\n");
            source.write("void encode" + prefix + name + "PacketStructure(void* pkt)\n");
            source.write("{\n");
        }

        source.write("    uint8_t* data = get" + protoName + "PacketData(pkt);\n");
        source.write("    int byteindex = 0;\n");

        if(bitfields)
            source.write("    int bitcount = 0;\n");

        if(needsEncodeIterator)
            source.write("    int i = 0;\n");

        int bitcount = 0;
        for(int i = 0; i < encodables.length(); i++)
        {
            source.makeLineSeparator();
            source.write(encodables[i]->getEncodeString(isBigEndian, &bitcount, true));
        }

        source.makeLineSeparator();
        source.write("    // complete the process of creating the packet\n");
        source.write("    finish" + protoName + "Packet(pkt, byteindex, get" + prefix + name + "PacketID());\n");
        source.write("}\n");
    }

    if(decode)
    {
        if(getNumberOfEncodes() > 0)
        {
            // The source function for the decode function. The decode function is
            // much more complex because we support default fields here
            source.makeLineSeparator();
            source.write("/*!\n");
            source.write(" * \\brief " + getPacketDecodeBriefComment() + "\n");
            source.write(" *\n");
            source.write(ProtocolParser::outputLongComment(" *", comment) + "\n");
            source.write(" * \\param pkt points to the packet being decoded by this function\n");
            if(numDecodes > 0)
                source.write(" * \\param user receives the data decoded from the packet\n");
            source.write(" * \\return 0 is returned if the packet ID or size is wrong, else 1\n");
            source.write(" */\n");
            if(numDecodes > 0)
                source.write("int decode" + prefix + name + "PacketStructure(const void* pkt, " + typeName + "* user)\n");
            else
                source.write("int decode" + prefix + name + "PacketStructure(const void* pkt)\n");
            source.write("{\n");
            source.write("    int numBytes;\n");
            source.write("    int byteindex = 0;\n");
            source.write("    const uint8_t* data;\n");
            if(bitfields)
                source.write("    int bitcount = 0;\n");

            if(needsDecodeIterator)
                source.write("    int i = 0;\n");
            source.write("\n");
            source.write("    // Verify the packet identifier\n");
            source.write("    if(get"+ protoName + "PacketID(pkt) != get" + prefix + name + "PacketID())\n");
            source.write("        return 0;\n");
            source.write("\n");
            source.write("    // Verify the packet size\n");
            source.write("    numBytes = get" + protoName + "PacketSize(pkt);\n");
            source.write("    if(numBytes < get" + prefix + name + "MinDataLength())\n");
            source.write("        return 0;\n");
            source.write("\n");
            source.write("    // The raw data from the packet\n");
            source.write("    data = get" + protoName + "PacketDataConst(pkt);\n");
            source.makeLineSeparator();
            if(defaults)
            {
                source.write("    // this packet has default fields, make sure they are set\n");

                for(int i = 0; i < encodables.size(); i++)
                    source.write(encodables[i]->getSetToDefaultsString(true));

            }// if defaults are used in this packet

            source.makeLineSeparator();

            // Keep our own track of the bitcount so we know what to do when we close the bitfield
            int bitcount = 0;
            int i;
            for(i = 0; i < encodables.length(); i++)
            {
                source.makeLineSeparator();

                // Encode just the nondefaults here
                if(encodables[i]->isDefault())
                    break;

                source.write(encodables[i]->getDecodeString(isBigEndian, &bitcount, true, true));
            }

            // Before we write out the decodes for default fields we need to check
            // packet size in the event that we were using variable length arrays
            // or dependent fields
            if((encodedLength.minEncodedLength != encodedLength.nonDefaultEncodedLength) && (i > 0))
            {
                source.makeLineSeparator();
                source.write("    // Used variable length arrays or dependent fields, check actual length\n");
                source.write("    if(numBytes < byteindex)\n");
                source.write("        return 0;\n");
            }

            // Now finish the fields (if any defaults)
            for(; i < encodables.length(); i++)
            {
                source.makeLineSeparator();
                source.write(encodables[i]->getDecodeString(isBigEndian, &bitcount, true, true));
            }

            source.makeLineSeparator();
            source.write("    return 1;\n");
            source.write("}\n");

        }// if fields to decode
        else
        {
            source.makeLineSeparator();
            source.write("/*!\n");
            source.write(" * \\brief " + getPacketDecodeBriefComment() + "\n");
            source.write(" *\n");
            source.write(ProtocolParser::outputLongComment(" *", comment) + "\n");
            source.write(" * \\param pkt points to the packet being decoded by this function\n");
            source.write(" * \\return 0 is returned if the packet ID is wrong, else 1\n");
            source.write(" */\n");
            source.write("int decode" + prefix + name + "PacketStructure(const void* pkt)\n");
            source.write("{\n");
            source.write("    if(get"+ protoName + "PacketID(pkt) != get" + prefix + name + "PacketID())\n");
            source.write("        return 0;\n");
            source.write("    else\n");
            source.write("        return 1;\n");
            source.write("}\n");

        }// else if no fields to decode

    }

}// createStructurePacketFunctions


/*!
 * Create the functions for encoding and decoding the packet to/from parameters
 */
void ProtocolPacket::createPacketFunctions(void)
{    
    if(encode)
    {
        // The prototype for the packet encode function
        header.makeLineSeparator();
        header.write("//! " + getPacketEncodeBriefComment() + "\n");
        header.write(getPacketEncodeSignature() + ";\n");
    }

    if(decode)
    {
        // The prototype for the packet decode function
        header.makeLineSeparator();
        header.write("//! " + getPacketDecodeBriefComment() + "\n");
        header.write(getPacketDecodeSignature() + ";\n");
    }

    int i;
    int bitcount = 0;

    if(encode)
    {
        source.makeLineSeparator();
        source.write("/*!\n");
        source.write(" * \\brief " + getPacketEncodeBriefComment() + "\n");
        source.write(" *\n");
        source.write(ProtocolParser::outputLongComment(" *", comment) + "\n");
        source.write(" * \\param pkt points to the packet which will be created by this function\n");
        for(i = 0; i < encodables.length(); i++)
            source.write(encodables.at(i)->getEncodeParameterComment());
        source.write(" */\n");
        source.write(getPacketEncodeSignature() + "\n");
        source.write("{\n");

        if(!encodedLength.isZeroLength())
        {
            source.write("    uint8_t* data = get"+ protoName + "PacketData(pkt);\n");
            source.write("    int byteindex = 0;\n");

            if(bitfields)
                source.write("    int bitcount = 0;\n");

            if(needsEncodeIterator)
                source.write("    int i = 0;\n");

            // Keep our own track of the bitcount so we know what to do when we close the bitfield
            for(i = 0; i < encodables.length(); i++)
            {
                source.makeLineSeparator();
                source.write(encodables[i]->getEncodeString(isBigEndian, &bitcount, false));
            }

            source.makeLineSeparator();
            source.write("    // complete the process of creating the packet\n");
            source.write("    finish" + protoName + "Packet(pkt, byteindex, get" + prefix + name + "PacketID());\n");
        }
        else
        {
            source.write("    // Zero length packet, no data encoded\n");
            source.write("    finish" + protoName + "Packet(pkt, 0, get" + prefix + name + "PacketID());\n");
        }

        source.write("}\n");
    }

    if(decode)
    {
        // Now the decode function
        source.write("\n");
        source.write("/*!\n");
        source.write(" * \\brief " + getPacketDecodeBriefComment() + "\n");
        source.write(" *\n");
        source.write(ProtocolParser::outputLongComment(" *", comment) + "\n");
        source.write(" * \\param pkt points to the packet being decoded by this function\n");
        for(i = 0; i < encodables.length(); i++)
            source.write(encodables.at(i)->getDecodeParameterComment());
        source.write(" * \\return 0 is returned if the packet ID or size is wrong, else 1\n");
        source.write(" */\n");
        source.write(getPacketDecodeSignature() + "\n");
        source.write("{\n");

        if(!encodedLength.isZeroLength())
        {
            if(bitfields)
                source.write("    int bitcount = 0;\n");
            if(needsDecodeIterator)
                source.write("    int i = 0;\n");
            source.write("    int byteindex = 0;\n");
            source.write("    const uint8_t* data = get" + protoName + "PacketDataConst(pkt);\n");
            source.write("    int numBytes = get" + protoName + "PacketSize(pkt);\n");
            source.write("\n");
            source.write("    if(get"+ protoName + "PacketID(pkt) != get" + prefix + name + "PacketID())\n");
            source.write("        return 0;\n");
            source.write("\n");
            source.write("    if(numBytes < get" + prefix + name + "MinDataLength())\n");
            source.write("        return 0;\n");
            if(defaults)
            {
                source.write("\n");
                source.write("    // this packet has default fields, make sure they are set\n");

                for(int i = 0; i < encodables.size(); i++)
                    source.write(encodables[i]->getSetToDefaultsString(false));

            }// if defaults are used in this packet

            // Keep our own track of the bitcount so we know what to do when we close the bitfield
            bitcount = 0;
            for(i = 0; i < encodables.length(); i++)
            {
                source.makeLineSeparator();

                // Encode just the nondefaults here
                if(encodables[i]->isDefault())
                    break;

                source.write(encodables[i]->getDecodeString(isBigEndian, &bitcount, false, true));
            }

            // Before we write out the decodes for default fields we need to check
            // packet size in the event that we were using variable length arrays
            // or dependent fields
            if((encodedLength.minEncodedLength != encodedLength.nonDefaultEncodedLength) && (i > 0))
            {
                source.makeLineSeparator();
                source.write("    // Used variable length arrays or dependent fields, check actual length\n");
                source.write("    if(numBytes < byteindex)\n");
                source.write("        return 0;\n");
            }

            // Now finish the fields (if any defaults)
            for(; i < encodables.length(); i++)
            {
                source.makeLineSeparator();
                source.write(encodables[i]->getDecodeString(isBigEndian, &bitcount, false, true));
            }

            source.makeLineSeparator();
            source.write("    return 1;\n");
        }
        else
        {
            source.write("    if(get"+ protoName + "PacketID(pkt) != get" + prefix + name + "PacketID())\n");
            source.write("        return 0;\n");
            source.write("    else\n");
            source.write("        return 1;\n");
        }

        source.write("}\n");
    }

}// createPacketFunctions


/*!
 * \return The signature of the packet encode function, without semicolon or comments or line feed
 */
QString ProtocolPacket::getPacketEncodeSignature(void) const
{
    QString output = "void encode" + prefix + name + "Packet(void* pkt";

    output += getDataEncodeParameterList() + ")";

    return output;
}


/*!
 * \return The signature of the packet decode function, without semicolon or comments or line feed
 */
QString ProtocolPacket::getPacketDecodeSignature(void) const
{
    QString output = "int decode" + prefix + name + "Packet(const void* pkt";

    output += getDataDecodeParameterList() + ")";

    return output;
}


/*!
 * \return The brief comment of the packet encode function, without doxygen decorations or line feed
 */
QString ProtocolPacket::getPacketEncodeBriefComment(void) const
{
    return QString("Create the " + prefix + name + " packet");
}


/*!
 * \return The brief comment of the packet decode function, without doxygen decorations or line feed
 */
QString ProtocolPacket::getPacketDecodeBriefComment(void) const
{
    return QString("Decode the " + prefix + name + " packet");
}


/*!
 * \return The parameter list part of a encode signature like ", type1 name1, type2 name2 ... "
 */
QString ProtocolPacket::getDataEncodeParameterList(void) const
{
    QString output;

    for(int i = 0; i < encodables.length(); i++)
        output += encodables[i]->getEncodeSignature();

    return output;
}


/*!
 * \return The parameter list part of a decode signature like ", type1* name1, type2 name2[3] ... "
 */
QString ProtocolPacket::getDataDecodeParameterList(void) const
{
    QString output;

    for(int i = 0; i < encodables.length(); i++)
        output += encodables[i]->getDecodeSignature();

    return output;
}


/*!
 * \return The brief comment of the structure encode function, without doxygen decorations or line feed
 */
QString ProtocolPacket::getDataEncodeBriefComment(void) const
{
    return QString("Encode the data from the " + protoName + " " + name + " structure");
}


/*!
 * \return The brief comment of the structure decode function, without doxygen decorations or line feed
 */
QString ProtocolPacket::getDataDecodeBriefComment(void) const
{
    return QString("Decode the data from the " + protoName + " " + name + " structure");
}

QString ProtocolPacket::getTopLevelMarkdown(QString outline) const
{
    QString output;
    QString idvalue = id;

    // Put an anchor in the identifier line which is the same as the ID. We'll link to it if we can
    output += "## <a name=\"" + id + "\"></a>" + name + "\n";
    output += "\n";

    if(!comment.isEmpty())
    {
        output += comment + "\n";
        output += "\n";
    }

    // In case the packet identifer is an enumeration we know
    ProtocolParser::replaceEnumerationNameWithValue(idvalue);

    if(id.compare(idvalue) == 0)
        output += "- packet identifier: `" + id + "`\n";
    else
        output += "- packet identifier: `" + id + "` : " + idvalue + "\n";

    if(encodedLength.minEncodedLength.compare(encodedLength.maxEncodedLength) == 0)
    {
        // The length strings, which may include enumerated identiers such as "N3D"
        QString minLength = EncodedLength::collapseLengthString(encodedLength.minEncodedLength, true).replace("1*", "");

        // The same quantities, but here we'll try and evaluate to a fixed number
        QString minLengthValue = minLength;

        // Replace any defined enumerations with their actual value
        ProtocolParser::replaceEnumerationNameWithValue(minLengthValue);

        // Re-collapse, perhaps we can solve it now
        minLengthValue = EncodedLength::collapseLengthString(minLengthValue, true);

        output += "- data length: " + minLength.replace("*", "&times;");

        // See if we made it any simpler, if we did then provide both versions,
        // this way the reader can see the numeric result, and how we got there.
        if(minLengthValue.compare(minLength) != 0)
            output += " : " + minLengthValue.replace("*", "&times;");

        output += "\n";
    }
    else
    {
        // The length strings, which may include enumerated identiers such as "N3D"
        QString maxLength = EncodedLength::collapseLengthString(encodedLength.maxEncodedLength, true).replace("1*", "");
        QString minLength = EncodedLength::collapseLengthString(encodedLength.minEncodedLength, true).replace("1*", "");

        // The same quantities, but here we'll try and evaluate to a fixed number
        QString maxLengthValue = maxLength;
        QString minLengthValue = minLength;

        // Replace any defined enumerations with their actual value
        ProtocolParser::replaceEnumerationNameWithValue(maxLengthValue);
        ProtocolParser::replaceEnumerationNameWithValue(minLengthValue);

        // Re-collapse, perhaps we can solve it now
        maxLengthValue = EncodedLength::collapseLengthString(maxLengthValue, true);
        minLengthValue = EncodedLength::collapseLengthString(minLengthValue, true);

        output += "- minimum data length: " + minLength.replace("*", "&times;");

        // See if we made it any simpler, if we did then provide both versions,
        // this way the reader can see the numeric result, and how we got there.
        if(minLengthValue.compare(minLength) != 0)
            output += " : " + minLengthValue.replace("*", "&times;");

        output += "\n";

        output += "- maximum data length: " + maxLength.replace("*", "&times;");

        // See if we made it any simpler, if we did then provide both versions,
        // this way the reader can see the numeric result, and how we got there.
        if(maxLengthValue.compare(maxLength) != 0)
            output += " : " + maxLengthValue.replace("*", "&times;");

        output += "\n";

    }

    if(enumList.size() > 0)
    {
        output += "\n";
        output += "### " + name + " enumerations\n";
        output += "\n";

        for(int i = 0; i < enumList.length(); i++)
        {
            if(enumList[i] == NULL)
                continue;

            output += enumList[i]->getMarkdown("");
            output += "\n";        
        }

        output += "\n";

    }

    if(encodables.size() > 0)
    {
        output += "\n";
        output += "### " + name + " encoding\n";
        output += "\n";

        QStringList bytes, names, encodings, repeats, comments;
        QString startByte = "0";

        // The column headings
        bytes.append("Bytes");
        names.append("Name");
        encodings.append("[Enc](#Enc)");
        repeats.append("Repeat");
        comments.append("Description");

        int byteColumn = 0, nameColumn = 0, encodingColumn = 0, repeatColumn = 0, commentColumn = 0;

        // Get all the details that are going to end up in the table
        QList<int>prefix;
        for(int i = 0, j = 0; i < encodables.length(); i++)
        {
            if(encodables.at(i) == NULL)
                continue;

            if(encodables.at(i)->isNotEncoded())
                continue;

            if(!encodables.at(i)->hasDocumentation())
                continue;

            // Prefix is the outline marker for the names in the table
            prefix.append(j++);
            encodables.at(i)->getDocumentationDetails(prefix, startByte, bytes, names, encodings, repeats, comments);
            prefix.clear();
        }

        // Figure out the column widths, note that we assume all the lists are the same length
        for(int i = 0; i < names.length(); i++)
        {
            // replace a "1*" with nothing, since that won't change the value but
            // is clearer. Also replace "*" with the html time symbol. This
            // looks better and does not cause markdown to emphasize the text
            // if there are multiple "*".
            bytes[i].replace("1*", "").replace("*", "&times;");

            if(bytes.at(i).length() > byteColumn)
                byteColumn = bytes.at(i).length();

            // Place a soft hypen after the ")" so the text can wrap in the table
            // names[i].replace(")", ")&shy;");

            if(names.at(i).length() > nameColumn)
                nameColumn = names.at(i).length();

            if(encodings.at(i).length() > encodingColumn)
                encodingColumn = encodings.at(i).length();

            if(repeats.at(i).length() > repeatColumn)
                repeatColumn = repeats.at(i).length();

            if(comments.at(i).length() > commentColumn)
                commentColumn = comments.at(i).length();
        }

        // Output the header
        output += "\n";

        // Table caption
        output += "[Encoding for packet " + name + "]\n";

        // Table header, notice the column markers lead and follow. We have to do this for merged cells
        output +=  "| ";
        output += spacedString(bytes.at(0), byteColumn);
        output += " | ";
        output += spacedString(names.at(0), nameColumn);
        output += " | ";
        output += spacedString(encodings.at(0), encodingColumn);
        output += " | ";
        output += spacedString(repeats.at(0), repeatColumn);
        output += " | ";
        output += spacedString(comments.at(0), commentColumn);
        output += " |\n";

        // Underscore the header
        output +=  "| ";
        for(int i = 0; i < byteColumn; i++)
            output += "-";

        output +=  " | ";
        for(int i = 0; i < nameColumn; i++)
            output += "-";

        // Encoding column is centered
        output += " | :";
        for(int i = 1; i < encodingColumn-1; i++)
            output += "-";

        // Repeat column is centered
        output += ": | :";
        for(int i = 1; i < repeatColumn-1; i++)
            output += "-";

        output += ": | ";
        for(int i = 0; i < commentColumn; i++)
            output += "-";
        output += " |\n";

        // Now write out the outputs
        for(int i = 1; i < names.length(); i++)
        {
            // Open the line
            output +=  "| ";
            output += spacedString(bytes.at(i), byteColumn);
            output += " | ";
            output += spacedString(names.at(i), nameColumn);

            // We support the idea that repeats and or encodings could be empty, causing cells to be merged
            if(encodings.at(i).isEmpty() && repeats.at(i).isEmpty())
            {
                output += spacedString("", encodingColumn + repeatColumn);
                output += "     ||| ";
            }
            else if(encodings.at(i).isEmpty())
            {
                output += spacedString(encodings.at(i), encodingColumn);
                output += "   || ";
                output += spacedString(repeats.at(i), repeatColumn);
                output += " | ";
            }
            else if(repeats.at(i).isEmpty())
            {
                output += " | ";
                output += spacedString(encodings.at(i), encodingColumn);
                output += spacedString(repeats.at(i), repeatColumn);
                output += "   || ";

            }
            else
            {
                output += " | ";
                output += spacedString(encodings.at(i), encodingColumn);
                output += " | ";
                output += spacedString(repeats.at(i), repeatColumn);
                output += " | ";
            }

            output += spacedString(comments.at(i), commentColumn);
            output += " |\n";
        }

        output += "\n";

    }

    return output;
}

