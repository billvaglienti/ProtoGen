#include "protocolpacket.h"
#include "protocolfield.h"
#include "enumcreator.h"
#include "protocolfield.h"
#include "protocolstructure.h"
#include "protocolparser.h"
#include "protocoldocumentation.h"
#include "shuntingyard.h"
#include <QDateTime>
#include <QStringList>
#include <iostream>

/*
 * Constant string defines that are re-used often
 * Encoding them as constant values prevents typo mistakes
 * and copying errors
 */

const QString VOID_ENCODE = "void encode";
const QString INT_DECODE = "int decode";

const QString TAB_IN = "    ";

/*!
 * Construct the object that parses packet descriptions
 * \param parse points to the global protocol parser that owns everything
 * \param supported gives the supported features of the protocol
 * \param protocolApi is the API string of the protocol
 * \param protocolVersion is the version string of the protocol
 * \param bigendian should be true to encode multi-byte fields with the most
 *        significant byte first.
 */
ProtocolPacket::ProtocolPacket(ProtocolParser* parse, ProtocolSupport supported, const QString& protocolApi, const QString& protocolVersion) :
    ProtocolStructureModule(parse, supported, protocolApi, protocolVersion)
{
    // These are attributes on top of the normal structureModule that we support
    attriblist << "structureInterface" << "parameterInterface" << "ID" << "useInOtherPackets";
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
    ids.clear();

    // Delete all the objects in the list
    for(int i = 0; i < documentList.count(); i++)
    {
        if(documentList[i] != NULL)
        {
            delete documentList[i];
            documentList[i] = NULL;
        }
    }

    // clear the list
    documentList.clear();

    // Note that data set during constructor are not changed

}


/*!
 * Create the source and header files that represent a packet
 */
void ProtocolPacket::parse(void)
{
    // Initialize metadata
    clear();

    // Get any documentation for this packet
    ProtocolDocumentation::getChildDocuments(parser, getHierarchicalName(), support, e, documentList);

    // Me and all my children, which may themselves be structures
    ProtocolStructure::parse();
    QDomNamedNodeMap map = e.attributes();

    QString moduleName = ProtocolParser::getAttribute("file", map);
    QString defheadermodulename = ProtocolParser::getAttribute("deffile", map);
    encode = !ProtocolParser::isFieldClear(ProtocolParser::getAttribute("encode", map));
    decode = !ProtocolParser::isFieldClear(ProtocolParser::getAttribute("decode", map));
    bool outputTopLevelStructureCode = ProtocolParser::isFieldSet(ProtocolParser::getAttribute("useInOtherPackets", map));

    // Typically "parameterInterface" and "structureInterface" are only ever set to "true".
    // However we do handle the case where someone uses "false"
    bool parameterFunctions = ProtocolParser::isFieldSet(ProtocolParser::getAttribute("parameterInterface", map));
    bool structureFunctions = ProtocolParser::isFieldSet(ProtocolParser::getAttribute("structureInterface", map));

    if(ProtocolParser::isFieldClear(ProtocolParser::getAttribute("parameterInterface", map)))
    {
        parameterFunctions = false;
        structureFunctions = true;
    }

    if(ProtocolParser::isFieldClear(ProtocolParser::getAttribute("structureInterface", map)))
    {
        parameterFunctions = true;
        structureFunctions = false;
    }

    // Its possible to have multiple ID attributes which are separated by white space
    ids = ProtocolParser::getAttribute("ID", map).split(QRegExp("[,;:\\s]+"), QString::SkipEmptyParts);

    // Warnings common to structures and packets
    issueWarnings(map);

    // Warning about maximum data size, only applies to packets
    if(support.maxdatasize > 0)
    {
        // The length strings, which may include enumerated identiers such as "N3D"
        QString maxLength = encodedLength.maxEncodedLength;

        // Replace any defined enumerations with their actual value
        parser->replaceEnumerationNameWithValue(maxLength);

        // maxdatasize will be zero if the length string cannot be computed
        int maxdatasize = (int)(ShuntingYard::computeInfix(maxLength) + 0.5);

        // Warn the user if the packet might be too big
        if(maxdatasize > support.maxdatasize)
            emitWarning("Maximum packet size of " + QString::number(maxdatasize) + " bytes exceeds limit of " + QString::number(support.maxdatasize) + " bytes");
    }

    // Warnings about C keywords
    for(int i = 0; i < ids.count(); i++)
    {
        if(keywords.contains(ids.at(i)))
        {
            emitWarning(ids.at(i) + " matches C keyword, changed to _" + ids.at(i));
            ids[i] = "_" + ids.at(i);
        }
    }

    // The file directive allows us to override the file name
    if(moduleName.isEmpty())
        moduleName = support.globalFileName;

    if(moduleName.isEmpty())
    {
        // The file names
        header.setModuleNameAndPath(support.prefix, name + support.packetParameterSuffix, support.outputpath);
        source.setModuleNameAndPath(support.prefix, name + support.packetParameterSuffix, support.outputpath);
    }
    else
    {
        // The file names
        header.setModuleNameAndPath(moduleName, support.outputpath);
        source.setModuleNameAndPath(moduleName, support.outputpath);
    }


    if(header.isAppending())
    {
        header.makeLineSeparator();
    }
    else
    {
        // Comment block at the top of the header file
        header.write("/*!\n");
        header.write(" * \\file\n");
        header.write(" * \\brief " + header.fileName() + " defines the interface for the " + name + " packet of the " + support.protoName + " protocol stack\n");

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
    }

    // Include the protocol top level module. This module may already be included, but in that case it won't be included twice
    header.writeIncludeDirective(support.protoName + "Protocol.h");

    // Handle the idea that the structure might be defined in a different file
    ProtocolHeaderFile* structfile = &header;
    if(!defheadermodulename.isEmpty())
    {
        defheader.setModuleNameAndPath(defheadermodulename, support.outputpath);
        if(defheader.isAppending())
            defheader.makeLineSeparator();

        structfile = &defheader;

        // In this instance we know that the normal header file needs to include
        // the file with the structure definition
        header.writeIncludeDirective(structfile->fileName());
    }

    // Add other includes specific to this packet
    parser->outputIncludes(getHierarchicalName(), *structfile, e);

    // Include directives that may be needed for our children
    QStringList list;
    for(int i = 0; i < encodables.length(); i++)
        encodables[i]->getIncludeDirectives(list);
    structfile->writeIncludeDirectives(list);

    // White space is good
    structfile->makeLineSeparator();

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
    structfile->write(getStructureDeclaration(structureFunctions));

    // White space is good
    structfile->makeLineSeparator();

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

    // The functions that include structures which are children of this
    // packet. These need to be declared before the main functions
    createSubStructureFunctions();

    // Outputs for the enumerations in source file, if any
    for(int i = 0; i < enumList.count(); i++)
    {
        QString enumoutput = enumList.at(i)->getSourceOutput();
        if(!enumoutput.isEmpty())
        {
            source.makeLineSeparator();
            source.write(enumoutput);
        }
    }

    // For referencing this packet as a structure
    if(outputTopLevelStructureCode)
        createTopLevelStructureFunctions();

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

    if(encode || decode)
        source.flush();
    else
        source.clear();

    // This may be one of the files above, in which case this will do nothing
    structfile->flush();

}// ProtocolPacket::parse


/*!
 * Create utility functions for packet ID and lengths. The structure must
 * already have been parsed to give the lengths
 * \param e is the top level DOM element that represents the packet.
 */
void ProtocolPacket::createUtilityFunctions(const QDomElement& e)
{
    // If no ID is supplied then use the packet name in upper case,
    // assuming that the user will define it elsewhere
    if(ids.count() <= 0)
        ids.append(name.toUpper());

    for(int i = 0; i < ids.count(); i++)
    {
        // The ID may be a value defined somewhere else
        QString include = parser->lookUpIncludeName(ids.at(i));
        if(!include.isEmpty())
            header.writeIncludeDirective(include);
    }

    // The macro for the packet ID, we only emit this if the packet has a single ID, which is the normal case
    if(ids.count() == 1)
    {
        header.makeLineSeparator();
        header.write("//! return the packet ID for the " + support.prefix + name + " packet\n");
        header.write("#define get" + support.prefix + name + support.packetParameterSuffix + "ID() (" + ids.at(0) + ")\n");
    }

    // The macro for the minimum packet length
    header.makeLineSeparator();
    header.write("//! return the minimum data length for the " + support.prefix + name + " packet\n");
    header.write("#define get" + support.prefix + name + "MinDataLength() ");
    if(encodedLength.minEncodedLength.isEmpty())
        header.write("0\n");
    else
        header.write("("+encodedLength.minEncodedLength + ")\n");

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
        {
            if(ids.count() <= 1)
                header.write(VOID_ENCODE + extendedName() + "(" + support.pointerType + " pkt, const " + typeName + "* user);\n");
            else
                header.write(VOID_ENCODE + extendedName() + "(" + support.pointerType + " pkt, const " + typeName + "* user, uint32_t id);\n");
        }
        else
        {
            if(ids.count() <= 1)
                header.write(VOID_ENCODE + extendedName() + "(" + support.pointerType + " pkt);\n");
            else
                header.write(VOID_ENCODE + extendedName() + "(" + support.pointerType + " pkt, uint32_t id);\n");
        }
    }

    if(decode)
    {
        // The prototype for the packet decode function
        header.makeLineSeparator();
        header.write("//! " + getPacketDecodeBriefComment() + "\n");

        if(numDecodes > 0)
            header.write(INT_DECODE + extendedName() + "(const " + support.pointerType + " pkt, " + typeName + "* user);\n");
        else
            header.write(INT_DECODE + extendedName() + "(const " + support.pointerType + " pkt);\n");
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
            if(ids.count() <= 1)
            {
                source.write(" */\n");
                source.write(VOID_ENCODE + extendedName() + "(" + support.pointerType + " pkt, const " + typeName + "* user)\n");
            }
            else
            {
                source.write(" * \\param id is the packet identifier for pkt\n");
                source.write(" */\n");
                source.write(VOID_ENCODE + extendedName() + "(" + support.pointerType + " pkt, const " + typeName + "* user, uint32_t id)\n");
            }
            source.write("{\n");
        }
        else
        {
            if(ids.count() <= 1)
            {
                source.write(" */\n");
                source.write(VOID_ENCODE + extendedName() + "(" + support.pointerType + " pkt)\n");
            }
            else
            {
                source.write(" * \\param id is the packet identifier for pkt\n");
                source.write(" */\n");
                source.write(VOID_ENCODE + extendedName() + "(" + support.pointerType + " pkt, uint32_t id)\n");
            }
            source.write("{\n");
        }

        source.write(TAB_IN + "uint8_t* data = get" + support.protoName + "PacketData(pkt);\n");
        source.write(TAB_IN + "int byteindex = 0;\n");

        if(bitfields)
            source.write(TAB_IN + "int bitcount = 0;\n");

        if(numbitfieldgroupbytes > 0)
        {
            source.write(TAB_IN + "int bitfieldindex = 0;\n");
            source.write(TAB_IN + "uint8_t bitfieldbytes[" + QString::number(numbitfieldgroupbytes) + "];\n");
        }

        if(needsEncodeIterator)
            source.write(TAB_IN + "int i = 0;\n");

        if(needs2ndEncodeIterator)
            source.write(TAB_IN + "int j = 0;\n");

        int bitcount = 0;
        for(int i = 0; i < encodables.length(); i++)
        {
            source.makeLineSeparator();
            source.write(encodables[i]->getEncodeString(support.bigendian, &bitcount, true));
        }

        source.makeLineSeparator();
        source.write(TAB_IN + "// complete the process of creating the packet\n");

        if(ids.count() <= 1)
            source.write(TAB_IN + "finish" + support.protoName + "Packet(pkt, byteindex, get" + support.prefix + name + support.packetParameterSuffix + "ID());\n");
        else
            source.write(TAB_IN + "finish" + support.protoName + "Packet(pkt, byteindex, id);\n");

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
                source.write(INT_DECODE + extendedName() + "(const " + support.pointerType + " pkt pkt, " + typeName + "* user)\n");
            else
                source.write(INT_DECODE + extendedName() + "(const " + support.pointerType + " pkt)\n");
            source.write("{\n");
            source.write(TAB_IN + "int numBytes;\n");
            source.write(TAB_IN + "int byteindex = 0;\n");
            source.write(TAB_IN + "const uint8_t* data;\n");
            if(bitfields)
                source.write(TAB_IN + "int bitcount = 0;\n");

            if(numbitfieldgroupbytes > 0)
            {
                source.write(TAB_IN + "int bitfieldindex = 0;\n");
                source.write(TAB_IN + "uint8_t bitfieldbytes[" + QString::number(numbitfieldgroupbytes) + "];\n");
            }

            if(needsDecodeIterator)
                source.write(TAB_IN + "int i = 0;\n");
            if(needs2ndDecodeIterator)
                source.write(TAB_IN + "int j = 0;\n");
            source.write("\n");

            if(ids.count() <= 1)
            {
                source.write(TAB_IN + "// Verify the packet identifier\n");
                source.write(TAB_IN + "if(get"+ support.protoName + "PacketID(pkt) != get" + support.prefix + name + support.packetParameterSuffix + "ID())\n");
            }
            else
            {
                source.write(TAB_IN + "// Verify the packet identifier, multiple options exist\n");
                source.write(TAB_IN + "uint32_t packetid = get"+ support.protoName + "PacketID(pkt);\n");
                source.write(TAB_IN + "if( packetid != " + ids.at(0));
                for(int i = 1; i < ids.count(); i++)
                    source.write(" &&\n        packetid != " + ids.at(i));
                source.write(" )\n");
            }

            source.write(TAB_IN + TAB_IN + "return 0;\n");
            source.write("\n");
            source.write(TAB_IN + "// Verify the packet size\n");
            source.write(TAB_IN + "numBytes = get" + support.protoName + "PacketSize(pkt);\n");
            source.write(TAB_IN + "if(numBytes < get" + support.prefix + name + "MinDataLength())\n");
            source.write(TAB_IN + "    return 0;\n");
            source.write("\n");
            source.write(TAB_IN + "// The raw data from the packet\n");
            source.write(TAB_IN + "data = get" + support.protoName + "PacketDataConst(pkt);\n");
            source.makeLineSeparator();
            if(defaults)
            {
                source.write(TAB_IN + "// this packet has default fields, make sure they are set\n");

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

                source.write(encodables[i]->getDecodeString(support.bigendian, &bitcount, true, true));
            }

            // Before we write out the decodes for default fields we need to check
            // packet size in the event that we were using variable length arrays
            // or dependent fields
            if((encodedLength.minEncodedLength != encodedLength.nonDefaultEncodedLength) && (i > 0))
            {
                source.makeLineSeparator();
                source.write(TAB_IN + "// Used variable length arrays or dependent fields, check actual length\n");
                source.write(TAB_IN + "if(numBytes < byteindex)\n");
                source.write(TAB_IN + "    return 0;\n");
            }

            // Now finish the fields (if any defaults)
            for(; i < encodables.length(); i++)
            {
                source.makeLineSeparator();
                source.write(encodables[i]->getDecodeString(support.bigendian, &bitcount, true, true));
            }

            source.makeLineSeparator();
            source.write(TAB_IN + "return 1;\n");
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
            source.write(INT_DECODE + extendedName() + "(const " + support.pointerType + " pkt)\n");
            source.write("{\n");
            if(ids.count() <= 1)
            {
                source.write(TAB_IN + "// Verify the packet identifier\n");
                source.write(TAB_IN + "if(get"+ support.protoName + "PacketID(pkt) != get" + support.prefix + name + support.packetParameterSuffix + "ID())\n");
            }
            else
            {
                source.write(TAB_IN + "// Verify the packet identifier, multiple options exist\n");
                source.write(TAB_IN + "uint32_t packetid = get"+ support.protoName + "PacketID(pkt);\n");
                source.write(TAB_IN + "if( packetid != " + ids.at(0));
                for(int i = 1; i < ids.count(); i++)
                    source.write(" &&\n        packetid != " + ids.at(i));
                source.write(" )\n");
            }
            source.write(TAB_IN + TAB_IN + "return 0;\n");
            source.write(TAB_IN + "else\n");
            source.write(TAB_IN + "    return 1;\n");
            source.write("}\n");

        }// else if no fields to decode

    }// if decode function enabled

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

        if(ids.count() > 1)
            source.write(" * \\param id is the packet identifier for pkt\n");

        source.write(" */\n");
        source.write(getPacketEncodeSignature() + "\n");
        source.write("{\n");

        if(!encodedLength.isZeroLength())
        {
            source.write(TAB_IN + "uint8_t* data = get"+ support.protoName + "PacketData(pkt);\n");
            source.write(TAB_IN + "int byteindex = 0;\n");

            if(bitfields)
                source.write(TAB_IN + "int bitcount = 0;\n");

            if(numbitfieldgroupbytes > 0)
            {
                source.write(TAB_IN + "int bitfieldindex = 0;\n");
                source.write(TAB_IN + "uint8_t bitfieldbytes[" + QString::number(numbitfieldgroupbytes) + "];\n");
            }

            if(needsEncodeIterator)
                source.write(TAB_IN + "int i = 0;\n");

            if(needs2ndEncodeIterator)
                source.write(TAB_IN + "int j = 0;\n");

            // Keep our own track of the bitcount so we know what to do when we close the bitfield
            for(i = 0; i < encodables.length(); i++)
            {
                source.makeLineSeparator();
                source.write(encodables[i]->getEncodeString(support.bigendian, &bitcount, false));
            }

            source.makeLineSeparator();
            source.write(TAB_IN + "// complete the process of creating the packet\n");
            if(ids.count() <= 1)
                source.write(TAB_IN + "finish" + support.protoName + "Packet(pkt, byteindex, get" + support.prefix + name + support.packetParameterSuffix + "ID());\n");
            else
                source.write(TAB_IN + "finish" + support.protoName + "Packet(pkt, byteindex, id);\n");
        }
        else
        {
            source.write(TAB_IN + "// Zero length packet, no data encoded\n");
            if(ids.count() <= 1)
                source.write(TAB_IN + "finish" + support.protoName + "Packet(pkt, 0, get" + support.prefix + name + support.packetParameterSuffix + "ID());\n");
            else
                source.write(TAB_IN + "finish" + support.protoName + "Packet(pkt, 0, id);\n");
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
                source.write(TAB_IN + "int bitcount = 0;\n");

            if(numbitfieldgroupbytes > 0)
            {
                source.write(TAB_IN + "int bitfieldindex = 0;\n");
                source.write(TAB_IN + "uint8_t bitfieldbytes[" + QString::number(numbitfieldgroupbytes) + "];\n");
            }

            if(needsDecodeIterator)
                source.write(TAB_IN + "int i = 0;\n");
            if(needs2ndDecodeIterator)
                source.write(TAB_IN + "int j = 0;\n");
            source.write(TAB_IN + "int byteindex = 0;\n");
            source.write(TAB_IN + "const uint8_t* data = get" + support.protoName + "PacketDataConst(pkt);\n");
            source.write(TAB_IN + "int numBytes = get" + support.protoName + "PacketSize(pkt);\n");
            source.write("\n");

            if(ids.count() <= 1)
            {
                source.write(TAB_IN + "// Verify the packet identifier\n");
                source.write(TAB_IN + "if(get"+ support.protoName + "PacketID(pkt) != get" + support.prefix + name + support.packetParameterSuffix + "ID())\n");
            }
            else
            {
                source.write(TAB_IN + "// Verify the packet identifier, multiple options exist\n");
                source.write(TAB_IN + "uint32_t packetid = get"+ support.protoName + "PacketID(pkt);\n");
                source.write(TAB_IN + "if( packetid != " + ids.at(0));
                for(int i = 1; i < ids.count(); i++)
                    source.write(" &&\n        packetid != " + ids.at(i));
                source.write(" )\n");
            }
            source.write(TAB_IN + TAB_IN + "return 0;\n");

            source.write("\n");
            source.write(TAB_IN + "if(numBytes < get" + support.prefix + name + "MinDataLength())\n");
            source.write(TAB_IN + TAB_IN + "return 0;\n");
            if(defaults)
            {
                source.write("\n");
                source.write(TAB_IN + "// this packet has default fields, make sure they are set\n");

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

                source.write(encodables[i]->getDecodeString(support.bigendian, &bitcount, false, true));
            }

            // Before we write out the decodes for default fields we need to check
            // packet size in the event that we were using variable length arrays
            // or dependent fields
            if((encodedLength.minEncodedLength != encodedLength.nonDefaultEncodedLength) && (i > 0))
            {
                source.makeLineSeparator();
                source.write(TAB_IN + "// Used variable length arrays or dependent fields, check actual length\n");
                source.write(TAB_IN + "if(numBytes < byteindex)\n");
                source.write(TAB_IN + TAB_IN + "return 0;\n");
            }

            // Now finish the fields (if any defaults)
            for(; i < encodables.length(); i++)
            {
                source.makeLineSeparator();
                source.write(encodables[i]->getDecodeString(support.bigendian, &bitcount, false, true));
            }

            source.makeLineSeparator();
            source.write(TAB_IN + "return 1;\n");

        }// if some fields to decode
        else
        {
            if(ids.count() <= 1)
            {
                source.write(TAB_IN + "// Verify the packet identifier\n");
                source.write(TAB_IN + "if(get"+ support.protoName + "PacketID(pkt) != get" + support.prefix + name + support.packetParameterSuffix + "ID())\n");
            }
            else
            {
                source.write(TAB_IN + "// Verify the packet identifier, multiple options exist\n");
                source.write(TAB_IN + "uint32_t packetid = get"+ support.protoName + "PacketID(pkt);\n");
                source.write(TAB_IN + "if( packetid != " + ids.at(0));
                for(int i = 1; i < ids.count(); i++)
                    source.write(" &&\n        packetid != " + ids.at(i));
                source.write(" )\n");
            }
            source.write(TAB_IN + TAB_IN + "return 0;\n");
            source.write(TAB_IN + "else\n");
            source.write(TAB_IN + TAB_IN + "return 1;\n");

        }// If no fields to decode

        source.write("}\n");

    }// if decode function enabled

}// createPacketFunctions


/*!
 * \return The signature of the packet encode function, without semicolon or comments or line feed
 */
QString ProtocolPacket::getPacketEncodeSignature(void) const
{
    QString output = VOID_ENCODE + support.prefix + name + support.packetParameterSuffix + "(" + support.pointerType + " pkt";

    output += getDataEncodeParameterList();

    if(ids.count() <= 1)
        output += ")";
    else
        output += ", uint32_t id)";

    return output;
}


/*!
 * \return The signature of the packet decode function, without semicolon or comments or line feed
 */
QString ProtocolPacket::getPacketDecodeSignature(void) const
{
    QString output = INT_DECODE + support.prefix + name + support.packetParameterSuffix + "(const " + support.pointerType + " pkt";

    output += getDataDecodeParameterList() + ")";

    return output;
}


/*!
 * \return The brief comment of the packet encode function, without doxygen decorations or line feed
 */
QString ProtocolPacket::getPacketEncodeBriefComment(void) const
{
    return QString("Create the " + support.prefix + name + " packet");
}


/*!
 * \return The brief comment of the packet decode function, without doxygen decorations or line feed
 */
QString ProtocolPacket::getPacketDecodeBriefComment(void) const
{
    return QString("Decode the " + support.prefix + name + " packet");
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
    return QString("Encode the data from the " + support.protoName + " " + name + " structure");
}


/*!
 * \return The brief comment of the structure decode function, without doxygen decorations or line feed
 */
QString ProtocolPacket::getDataDecodeBriefComment(void) const
{
    return QString("Decode the data from the " + support.protoName + " " + name + " structure");
}


QString ProtocolPacket::getTopLevelMarkdown(bool global, const QStringList& packetids) const
{
    QString output;

    if(ids.count() <= 1)
    {
        QString id = ids.at(0);
        QString idvalue = id;

        // Put an anchor in the identifier line which is the same as the ID. We'll link to it if we can
        output += "## <a name=\"" + id + "\"></a>" + name + " packet\n";
        output += "\n";

        if(!comment.isEmpty())
        {
            output += comment + "\n";
            output += "\n";
        }

        if(!ids.at(0).isEmpty())
        {
            // In case the packet identifer is an enumeration we know
            parser->replaceEnumerationNameWithValue(idvalue);

            if(id.compare(idvalue) == 0)
                output += "- packet identifier: `" + id + "`\n";
            else
                output += "- packet identifier: `" + id + "` : " + idvalue + "\n";
        }

    }// if a single identifier
    else
    {
        // Packet name heading
        output += "## " + name + " packet\n";
        output += "\n";

        if(!comment.isEmpty())
        {
            output += comment + "\n";
            output += "\n";
        }

        output += "This packet supports multiple identifiers\n";
        output += "\n";
        for(int i= 0; i < ids.count(); i++)
        {
            QString id = ids.at(i);
            QString idvalue = id;

            // In case the packet identifer is an enumeration we know
            parser->replaceEnumerationNameWithValue(idvalue);

            // Put the link here in this case
            if(id.compare(idvalue) == 0)
                output += "- packet identifier: <a name=\"" + id + "\"></a>`" + id + "`\n";
            else
                output += "- packet identifier: <a name=\"" + id + "\"></a>`" + id + "` : " + idvalue + "\n";

        }// for all identifers

    }// else if multiple identifiers


    if(encodedLength.minEncodedLength.compare(encodedLength.maxEncodedLength) == 0)
    {
        // The length strings, which may include enumerated identiers such as "N3D"
        QString minLength = EncodedLength::collapseLengthString(encodedLength.minEncodedLength, true).replace("1*", "");

        // Replace any defined enumerations with their actual value
        parser->replaceEnumerationNameWithValue(minLength);

        // Re-collapse, perhaps we can solve it now
        minLength = EncodedLength::collapseLengthString(minLength, true);

        // Output the length, replacing the multiply asterisk with a times symbol
        output += "- data length: " + minLength.replace("*", "&times;") + "\n";
    }
    else
    {
        // The length strings, which may include enumerated identiers such as "N3D"
        QString maxLength = EncodedLength::collapseLengthString(encodedLength.maxEncodedLength, true).replace("1*", "");
        QString minLength = EncodedLength::collapseLengthString(encodedLength.minEncodedLength, true).replace("1*", "");

        // Replace any defined enumerations with their actual value
        parser->replaceEnumerationNameWithValue(maxLength);
        parser->replaceEnumerationNameWithValue(minLength);

        // Re-collapse, perhaps we can solve it now
        maxLength = EncodedLength::collapseLengthString(maxLength, true);
        minLength = EncodedLength::collapseLengthString(minLength, true);

        // Output the length, replacing the multiply asterisk with a times symbol
        output += "- minimum data length: " + minLength.replace("*", "&times;") + "\n";

        // Output the length, replacing the multiply asterisk with a times symbol
        output += "- maximum data length: " + maxLength.replace("*", "&times;") + "\n";
    }

    // Output any documetation data
    output += "\n";
    for(int i = 0; i < documentList.count(); i++)
        output += documentList.at(i)->getTopLevelMarkdown();

    for(int i = 0; i < enumList.length(); i++)
    {
        if(enumList[i] == NULL)
            continue;

        output += enumList[i]->getTopLevelMarkdown();
        output += "\n";
        output += "\n";
    }

    if(encodables.size() > 0)
    {
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
            // is clearer. Also replace "*" with the html times symbol. This
            // looks better and does not cause markdown to emphasize the text
            // if there are multiple "*".
            bytes[i].replace("1*", "").replace("*", "&times;");
            repeats[i].replace("*", "&times;");

            if(bytes.at(i).length() > byteColumn)
                byteColumn = bytes.at(i).length();

            if(names.at(i).length() > nameColumn)
                nameColumn = names.at(i).length();

            if(encodings.at(i).length() > encodingColumn)
                encodingColumn = encodings.at(i).length();

            if(repeats.at(i).length() > repeatColumn)
                repeatColumn = repeats.at(i).length();

            if(comments.at(i).length() > commentColumn)
                commentColumn = comments.at(i).length();
        }


        output += "\n";

        // Table caption
        output += "[" + name + " packet bytes]\n";

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

