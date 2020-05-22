#include "protocolpacket.h"
#include "protocolfield.h"
#include "enumcreator.h"
#include "protocolfield.h"
#include "protocolstructure.h"
#include "protocolparser.h"
#include "protocoldocumentation.h"
#include "shuntingyard.h"
#include <iostream>


/*!
 * Construct the object that parses packet descriptions
 * \param parse points to the global protocol parser that owns everything
 * \param supported gives the supported features of the protocol
 * \param protocolApi is the API string of the protocol
 * \param protocolVersion is the version string of the protocol
 * \param bigendian should be true to encode multi-byte fields with the most
 *        significant byte first.
 */
ProtocolPacket::ProtocolPacket(ProtocolParser* parse, ProtocolSupport supported, const std::string& protocolApi, const std::string& protocolVersion) :
    ProtocolStructureModule(parse, supported, protocolApi, protocolVersion),
    useInOtherPackets(false),
    parameterFunctions(false),
    structureFunctions(true)
{
    // These are attributes on top of the normal structureModule that we support
    std::vector<std::string> newattribs({"structureInterface", "parameterInterface", "ID", "useInOtherPackets"});

    // Now append the new attributes onto our old list
    // Now append the new attributes onto our old list
    attriblist.insert(attriblist.end(), newattribs.begin(), newattribs.end());
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
    useInOtherPackets = false;
    parameterFunctions = false;
    structureFunctions = true;

    // Delete all the objects in the list
    for(std::size_t i = 0; i < documentList.size(); i++)
    {
        if(documentList[i] != nullptr)
        {
            delete documentList[i];
            documentList[i] = nullptr;
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

    if(e == nullptr)
        return;

    // Get any documentation for this packet
    ProtocolDocumentation::getChildDocuments(parser, getHierarchicalName(), support, e, documentList);

    // Me and all my children, which may themselves be structures, notice we
    // are not parsing ProtocolStructureModule. This class is basically a
    // re-implementation of ProtocolStructureModule with different rules.
    ProtocolStructure::parse();

    const XMLAttribute* map = e->FirstAttribute();

    std::string moduleName = ProtocolParser::getAttribute("file", map);
    std::string defheadermodulename = ProtocolParser::getAttribute("deffile", map);
    std::string verifymodulename = ProtocolParser::getAttribute("verifyfile", map);
    std::string comparemodulename = ProtocolParser::getAttribute("comparefile", map);
    std::string printmodulename = ProtocolParser::getAttribute("printfile", map);
    std::string mapmodulename = ProtocolParser::getAttribute("mapfile", map);

    encode = !ProtocolParser::isFieldClear(ProtocolParser::getAttribute("encode", map));
    decode = !ProtocolParser::isFieldClear(ProtocolParser::getAttribute("decode", map));

    // It is possible to suppress the globally specified compare output
    if(ProtocolParser::isFieldClear(ProtocolParser::getAttribute("compare", map)))
    {
        support.compare = compare = false;
        comparemodulename.clear();
        support.globalCompareName.clear();
    }
    else if(ProtocolParser::isFieldSet(ProtocolParser::getAttribute("compare", map)))
        compare = true;

    // It is possible to suppress the globally specified print output
    if(ProtocolParser::isFieldClear(ProtocolParser::getAttribute("print", map)))
    {
        support.print = print = false;
        printmodulename.clear();
        support.globalPrintName.clear();
    }
    else if(ProtocolParser::isFieldSet(ProtocolParser::getAttribute("print", map)))
        print = true;

    // It is possible to suppress the globally specified map output
    if(ProtocolParser::isFieldClear(ProtocolParser::getAttribute("map", map)))
    {
        support.mapEncode = mapEncode = false;
        mapmodulename.clear();
        support.globalMapName.clear();
    }
    else if(ProtocolParser::isFieldSet(ProtocolParser::getAttribute("map", map)))
        mapEncode = true;

    useInOtherPackets = ProtocolParser::isFieldSet("useInOtherPackets", map);
    std::string redefinename = ProtocolParser::getAttribute("redefine", map);

    // Typically "parameterInterface" and "structureInterface" are only ever set to "true".
    // However we do handle the case where someone uses "false"
    parameterFunctions = ProtocolParser::isFieldSet("parameterInterface", map);
    structureFunctions = ProtocolParser::isFieldSet("structureInterface", map);

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

    // Its possible to have multiple ID attributes which are separated by white space or commas, etc.
    ids = splitanyof(ProtocolParser::getAttribute("ID", map), " ,;:\t\n\r");

    // In case the user didn't provide a comment, see if we use the comment for the ID
    if(comment.empty() && (ids.size() > 0))
        comment = parser->getEnumerationValueComment(ids.at(0));

    // Warnings common to structures and packets
    issueWarnings(map);

    // Warning about maximum data size, only applies to packets
    if(support.maxdatasize > 0)
    {
        // maxdatasize will be zero if the length string cannot be computed
        int maxdatasize = (int)(ShuntingYard::computeInfix(parser->replaceEnumerationNameWithValue(encodedLength.maxEncodedLength)) + 0.5);

        // Warn the user if the packet might be too big
        if(maxdatasize > support.maxdatasize)
            emitWarning("Maximum packet size of " + std::to_string(maxdatasize) + " bytes exceeds limit of " + std::to_string(support.maxdatasize) + " bytes");
    }

    // Warnings about C keywords
    for(std::size_t i = 0; i < ids.size(); i++)
    {
        if(contains(keywords, ids.at(i), true))
        {
            emitWarning(ids.at(i) + " matches C keyword, changed to _" + ids.at(i));
            ids[i] = "_" + ids.at(i);
        }
    }

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

    if(!redefinename.empty())
    {
        if(redefinename == name)
            emitWarning("Redefine must be different from name");
        else
        {
            redefines = parser->lookUpStructure(support.prefix + redefinename + "_t");
            if(redefines == nullptr)
            {
                redefinename.clear();
                emitWarning("Could not find structure to redefine");
            }
        }

        if(redefines != nullptr)
            structName = support.prefix + redefinename + "_t";
    }

    // If no ID is supplied use the packet name in upper case,
    // assuming that the user will define it elsewhere
    if(ids.size() <= 0)
        ids.push_back(toUpper(name));

    // Most of the file setup work. This will also declare the structure if
    // warranted (note the details of the structure declaration will reflect
    // back to this class via virtual functions).
    setupFiles(moduleName, defheadermodulename, verifymodulename, comparemodulename, printmodulename, mapmodulename, structureFunctions, false);

    // The functions that include structures which are children of this
    // packet. These need to be declared before the main functions
    createSubStructureFunctions();

    // This is the constructor output, we want it to be the first function for this packet
    createTopLevelInitializeFunction();

    for(std::size_t i = 0; i < ids.size(); i++)
    {
        // The ID may be a value defined somewhere else
        std::string include = parser->lookUpIncludeName(ids.at(i));
        if(!include.empty())
            header.writeIncludeDirective(include);
    }

    // The functions that encode and decode the packet from a structure.
    if(structureFunctions)
        createStructurePacketFunctions();

    // The functions that encode and decode the packet from parameters
    if(parameterFunctions)
        createPacketFunctions();

    // Now that the packet functions are out, do the non-packet functions
    createTopLevelStructureFunctions();

    // In the C language the utility functions are macros, defined just below the functions.
    if(support.language == ProtocolSupport::c_language)
    {
        // White space is good
        header.makeLineSeparator();

        // Utility functions for ID, length, etc.
        header.write(createUtilityFunctions(std::string()));
    }

    // White space is good
    header.makeLineSeparator();

    // Write to disk, note that duplicate flush() calls are OK
    header.flush();
    if(structHeader != nullptr)
        structHeader->flush();

    // We don't write the source to disk if we are not encoding or decoding anything
    if(encode || decode)
        source.flush();
    else
        source.clear();

    // We don't write the verify files to disk if we are not initializing or verifying anything
    if(hasInit() || hasVerify())
    {
        if(verifyHeader != nullptr)
            verifyHeader->flush();

        if(verifySource != nullptr)
            verifySource->flush();
    }

    if(compare)
    {
        if(compareHeader != nullptr)
            compareHeader->flush();

        if(compareSource != nullptr)
            compareSource->flush();
    }

    if(print)
    {
        if(printHeader != nullptr)
            printHeader->flush();

        if(printSource != nullptr)
            printSource->flush();
    }

    if(mapEncode)
    {
        if(mapHeader != nullptr)
            mapHeader->flush();

        if(mapSource != nullptr)
            mapSource->flush();
    }

}// ProtocolPacket::parse


/*!
 * Get the class declaration, for this structure only (not its children) for the C++ language
 * \return The string that gives the class declaration
 */
std::string ProtocolPacket::getClassDeclaration_CPP(void) const
{
    std::string output;
    std::string structure;

    // The top level comment for the class definition
    if(!comment.empty())
    {
        output += "/*!\n";
        output += ProtocolParser::outputLongComment(" * ", comment) + "\n";
        output += " */\n";
    }

    // The opening to the class.
    // In the context of C++ redefining means inheriting from a base class,
    // and adding a new encode or decode function. All the other members and
    // methods come from the base class
    if(redefines == nullptr)
        output += "class " + typeName + "\n";
    else
        output += "class " + typeName + " : public " + redefines->typeName + "\n";

        output += "{\n";

    // All members of the structure are public.
    output += "public:\n";

    // Function prototypes, in C++ these are part of the class definition.
    // Notice that if we are not outputting structure functions, and this
    // class won't be used by others, we will not have any data members and
    // do not need a constructor.
    if((redefines == nullptr) && (getNumberInMemory() > 0) && (useInOtherPackets || structureFunctions))
    {
        ProtocolFile::makeLineSeparator(output);
        output += getSetToInitialValueFunctionPrototype(TAB_IN, false);
        ProtocolFile::makeLineSeparator(output);
    }

    // Utility functions for ID, length, etc.
    ProtocolFile::makeLineSeparator(output);
    output += createUtilityFunctions(TAB_IN);
    ProtocolFile::makeLineSeparator(output);

    // The parameter functions encode parameters to/from packets
    if(parameterFunctions)
    {
        if(encode)
        {
            ProtocolFile::makeLineSeparator(output);
            output += getParameterPacketEncodePrototype(TAB_IN);
            ProtocolFile::makeLineSeparator(output);
        }

        if(decode)
        {
            ProtocolFile::makeLineSeparator(output);
            output += getParameterPacketDecodePrototype(TAB_IN);
            ProtocolFile::makeLineSeparator(output);
        }

    }// if parameter packet functions

    // The structure functions encode members of this class directly to/from packets
    if(structureFunctions)
    {
        // In the event that there are no parameters, the parameter function
        // is the same as the structure function - so don't output both
        if(encode && ((getNumberOfEncodeParameters() > 0) || !parameterFunctions))
        {
            ProtocolFile::makeLineSeparator(output);
            output += getStructurePacketEncodePrototype(TAB_IN);
            ProtocolFile::makeLineSeparator(output);
        }

        // In the event that there are no parameters, the parameter function
        // is the same as the structure function - so don't output both
        if(decode && ((getNumberOfDecodeParameters() > 0) || !parameterFunctions))
        {
            ProtocolFile::makeLineSeparator(output);
            output += getStructurePacketDecodePrototype(TAB_IN);
            ProtocolFile::makeLineSeparator(output);
        }

    }// if structure packet functions

    // Packet version of compare function
    if(compare)
    {
        ProtocolFile::makeLineSeparator(output);
        output += TAB_IN + "//! Compare two " + support.prefix + name + " packets and generate a report\n";
        output += TAB_IN + "static std::string compare(std::string prename, const " + support.pointerType + " pkt1, const " + support.pointerType + " pkt2);\n";
        ProtocolFile::makeLineSeparator(output);
    }

    // Packet version of print function
    if(print)
    {
        ProtocolFile::makeLineSeparator(output);
        output += TAB_IN + "//! Generate a string that describes the contents of a " + name + " packet\n";
        output += TAB_IN + "static std::string textPrint(std::string prename, const " + support.pointerType + " pkt);\n";
        ProtocolFile::makeLineSeparator(output);
    }

    // For use in other packets we need the ability to encode to a byte
    // stream, which is what ProtocolStructure gives us
    if(useInOtherPackets)
    {
        if(encode)
        {
            ProtocolFile::makeLineSeparator(output);
            output += ProtocolStructure::getEncodeFunctionPrototype(TAB_IN, false);
            ProtocolFile::makeLineSeparator(output);
        }

        if(decode)
        {
            ProtocolFile::makeLineSeparator(output);
            output += ProtocolStructure::getDecodeFunctionPrototype(TAB_IN, false);
            ProtocolFile::makeLineSeparator(output);
        }

    }// If structure functions that will be accessed by others

    // There are cool utility functions: verify, print, read, mapencde,
    // mapdecode, and compare. All of these have forms that come from
    // ProtocolStructure. Thse functions are only output for the base class,
    // inherited (redefined) classes do not output them, because they would be
    // the same. We also do not output data members for redefined classes,
    // they come from the base class
    if((useInOtherPackets || structureFunctions) && (redefines == nullptr))
    {
        if(compare)
        {
            ProtocolFile::makeLineSeparator(output);
            output += ProtocolStructure::getComparisonFunctionPrototype(TAB_IN, false);
            ProtocolFile::makeLineSeparator(output);
        }

        if(print)
        {
            ProtocolFile::makeLineSeparator(output);
            output += ProtocolStructure::getTextPrintFunctionPrototype(TAB_IN, false);
            ProtocolFile::makeLineSeparator(output);
            output += ProtocolStructure::getTextReadFunctionPrototype(TAB_IN, false);
            ProtocolFile::makeLineSeparator(output);
        }

        if(hasVerify())
        {
            ProtocolFile::makeLineSeparator(output);
            output += ProtocolStructure::getVerifyFunctionPrototype(TAB_IN, false);
            ProtocolFile::makeLineSeparator(output);
        }

        if(mapEncode)
        {
            ProtocolFile::makeLineSeparator(output);
            output += ProtocolStructure::getMapEncodeFunctionPrototype(TAB_IN, false);
            ProtocolFile::makeLineSeparator(output);
            output += ProtocolStructure::getMapDecodeFunctionPrototype(TAB_IN, false);
            ProtocolFile::makeLineSeparator(output);
        }

        ProtocolFile::makeLineSeparator(output);

        // Finally the local members of this class. Notice that if we only have
        // parameter functions then we do not output these (and the class is
        // esentially static). The same is true if we are redefining another class,
        // in which case we use the members from the base class
        if(getNumberInMemory() > 0)
        {
            // Now declare the members of this class
            for(std::size_t i = 0; i < encodables.size(); i++)
                structure += encodables[i]->getDeclaration();

            // Make classes pretty with alignment goodness
            output += alignStructureData(structure);
        }

        ProtocolFile::makeLineSeparator(output);

    }// If outputting structure functions

    // Close out the class
    output += "}; // " + typeName + "\n";

    return output;

}// ProtocolStructure::getClassDeclaration_CPP


/*!
 * Create utility functions for packet ID and lengths. The structure must
 * already have been parsed to give the lengths.
 * \param spacing sets the amount of space to put before each line.
 * \return the string which goes in the header or class definition, depending
 *         on the language being output
 */
std::string ProtocolPacket::createUtilityFunctions(const std::string& spacing) const
{
    std::string output;

    if(support.language == ProtocolSupport::c_language)
    {
        // The macro for the packet ID, we only emit this if the packet has a single ID, which is the normal case
        if(ids.size() == 1)
        {
            output += spacing + "//! return the packet ID for the " + support.prefix + name + " packet\n";
            output += spacing + "#define get" + support.prefix + name + support.packetParameterSuffix + "ID() (" + ids.at(0) + ")\n";
            output += "\n";
        }

        // The macro for the minimum packet length
        output += spacing + "//! return the minimum encoded length for the " + support.prefix + name + " packet\n";
        output += spacing + "#define get" + support.prefix + name + "MinDataLength() ";
        if(encodedLength.minEncodedLength.empty())
            output += "0\n";
        else
            output += "("+encodedLength.minEncodedLength + ")\n";

        // The macro for the maximum packet length
        output += "\n";
        output += spacing + "//! return the maximum encoded length for the " + support.prefix + name + " packet\n";
        output += spacing + "#define get" + support.prefix + name + "MaxDataLength() ";
        if(encodedLength.maxEncodedLength.empty())
            output += "0\n";
        else
            output += "("+encodedLength.maxEncodedLength + ")\n";
    }
    else
    {
        if(ids.size() == 1)
        {
            output += spacing + "//! \\return the identifier for the packet\n";
            output += spacing + "static uint32_t id(void) { return " + ids.at(0) + ";}\n";
            output += "\n";
        }

        // The minimum packet length
        output += spacing + "//! \\return the minimum encoded length for the packet\n";
        output += spacing + "static int minLength(void) { return ";
        if(encodedLength.minEncodedLength.empty())
            output += "0;}\n";
        else
            output += "("+encodedLength.minEncodedLength + ");}\n";

        // The maximum packet length
        output += "\n";
        output += spacing + "//! \\return the maximum encoded length for the packet\n";
        output += spacing + "static int maxLength(void) { return ";
        if(encodedLength.maxEncodedLength.empty())
            output += "0;}\n";
        else
            output += "("+encodedLength.maxEncodedLength + ");}\n";
    }

    return output;

}// ProtocolPacket::createUtilityFunctions


/*!
 * Write the initializer / constructor function for this packet only
 */
void ProtocolPacket::createTopLevelInitializeFunction(void)
{
    if(hasInit() && (verifySource != nullptr) && (redefines == nullptr))
    {
        // In C++ this is part of the class declaration
        if((support.language == ProtocolSupport::c_language) && (verifyHeader != nullptr))
        {
            verifyHeader->makeLineSeparator();
            verifyHeader->write(getSetToInitialValueFunctionPrototype(std::string(), false));
            verifyHeader->makeLineSeparator();
        }

        // In C++ we do not output the constructor if we have no class members
        // Notice that if we are not outputting structure functions, and this
        // class won't be used by others, we will not have any data members and
        // do not need a constructor.
        if((support.language == ProtocolSupport::c_language) || ((getNumberInMemory() > 0) && (useInOtherPackets || structureFunctions)))
        {
            verifySource->makeLineSeparator();
            verifySource->write(getSetToInitialValueFunctionBody(false));
            verifySource->makeLineSeparator();
        }
    }

}// ProtocolPacket::createTopLevelInitializeFunction


/*!
 * Write data to the source and header files to encode and decode structure
 * functions that do not use a packet. For this structure only, not its children
 */
void ProtocolPacket::createTopLevelStructureFunctions(void)
{
    // If we are using this structure in other packets, we need the structure
    // functions that come from ProtocolStructureModule
    if(useInOtherPackets)
    {
        if(encode)
        {
            // In C++ this is part of the class declaration
            if(support.language == ProtocolSupport::c_language)
            {
                header.makeLineSeparator();
                header.write(getEncodeFunctionPrototype(std::string(), false));
            }

            source.makeLineSeparator();
            source.write(getEncodeFunctionBody(support.bigendian, false));
        }

        if(decode)
        {
            // In C++ this is part of the class declaration
            if(support.language == ProtocolSupport::c_language)
            {
                header.makeLineSeparator();
                header.write(getDecodeFunctionPrototype(std::string(), false));
            }

            source.makeLineSeparator();
            source.write(getDecodeFunctionBody(support.bigendian, false));
        }

        header.makeLineSeparator();
        source.makeLineSeparator();

    }// if we are not suppressing the encode and decode functions

    createTopLevelStructureHelperFunctions();

}// ProtocolPacket::createTopLevelStructureFunctions


/*!
 * Create the functions for encoding and decoding the packet to/from a structure
 */
void ProtocolPacket::createStructurePacketFunctions(void)
{
    int numDecodes = getNumberOfDecodeParameters();

    // The prototypes in the header file are only needed for C,
    // in C++ these prototypes are part of the class declaration.
    if(support.language == ProtocolSupport::c_language)
    {
        // In the event that there are no parameters, the parameter function
        // is the same as the structure function - so don't output both
        if(encode && ((getNumberOfEncodeParameters() > 0) || !parameterFunctions))
        {
            // The prototype for the structure packet encode function
            header.makeLineSeparator();
            header.write(getStructurePacketEncodePrototype(std::string()));
        }

        // In the event that there are no parameters, the parameter function
        // is the same as the structure function - so don't output both
        if(decode && ((numDecodes > 0) || !parameterFunctions))
        {
            // The prototype for the structure packet decode function
            header.makeLineSeparator();
            header.write(getStructurePacketDecodePrototype(std::string()));
        }

        if(compare && compareHeader != nullptr)
        {
            compareHeader->makeLineSeparator();
            compareHeader->write("//! Compare two " + support.prefix + name + " packets and generate a report\n");
            compareHeader->write("std::string compare" + support.prefix + name + support.packetParameterSuffix + "(std::string prename, const " + support.pointerType + " pkt1, const " + support.pointerType + " pkt2);\n");
            compareHeader->makeLineSeparator();
        }

        if(print && printHeader != nullptr)
        {
            printHeader->makeLineSeparator();
            printHeader->write("//! Generate a string that describes the contents of a " + name + " packet\n");
            printHeader->write("std::string textPrint" + (support.prefix + name + support.packetParameterSuffix) + "(std::string prename, const " + support.pointerType + " pkt);\n");
            printHeader->makeLineSeparator();
        }

    }// If C language

    // In the event that there are no parameters, the parameter function
    // is the same as the structure function - so don't output both
    if(encode && ((getNumberOfEncodeParameters() > 0) || !parameterFunctions))
    {
        // The source function for the encode function
        source.makeLineSeparator();
        source.write(getStructurePacketEncodeBody());
    }

    // In the event that there are no parameters, the parameter function
    // is the same as the structure function - so don't output both
    if(decode && ((numDecodes > 0) || !parameterFunctions))
    {
        // The source function for the decode function
        source.makeLineSeparator();
        source.write(getStructurePacketDecodeBody());
    }

    if(compare && (compareSource != nullptr))
    {
        compareSource->makeLineSeparator();
        compareSource->write("/*!\n");
        compareSource->write(" * Compare two " + name + " packets and generate a report of any differences.\n");
        compareSource->write(" * \\param _pg_prename is prepended to the name of the data field in the comparison report\n");
        compareSource->write(" * \\param _pg_pkt1 is the first data to compare\n");
        compareSource->write(" * \\param _pg_pkt2 is the second data to compare\n");
        compareSource->write(" * \\return a string describing any differences between pk1 and pkt2. The string will be empty if there are no differences\n");
        compareSource->write(" */\n");

        if(support.language == ProtocolSupport::c_language)
            compareSource->write("std::string compare" + (support.prefix + name + support.packetParameterSuffix) + "(std::string _pg_prename, const " + support.pointerType + " _pg_pkt1, const " + support.pointerType + " _pg_pkt2)\n");
        else
            compareSource->write("std::string " + typeName + "::compare(std::string _pg_prename, const " + support.pointerType + " _pg_pkt1, const " + support.pointerType + " _pg_pkt2)\n");

        compareSource->write("{\n");
        compareSource->write(TAB_IN + "std::string _pg_report;\n");

        if(numDecodes > 0)
        {
            compareSource->makeLineSeparator();
            compareSource->write(TAB_IN + "// Structures to decode into\n");
            compareSource->write(TAB_IN + structName + " _pg_struct1, _pg_struct2;\n");

            compareSource->makeLineSeparator();
            compareSource->write(TAB_IN + "if(_pg_prename.empty())\n");
            compareSource->write(TAB_IN + TAB_IN + "_pg_prename = \"" + name + "\";\n");

            if(support.language == ProtocolSupport::c_language)
            {
                // In C we need explicity initializers
                compareSource->makeLineSeparator();
                compareSource->write(TAB_IN + "// All zeroes before decoding\n");
                compareSource->write(TAB_IN + "memset(&_pg_struct1, 0, sizeof(_pg_struct1));\n");
                compareSource->write(TAB_IN + "memset(&_pg_struct2, 0, sizeof(_pg_struct2));\n");

                compareSource->makeLineSeparator();
                compareSource->write(TAB_IN + "// Decode each packet\n");
                compareSource->write(TAB_IN + "if(!decode" + extendedName() + "(_pg_pkt1, &_pg_struct1) || !decode" + extendedName() + "(_pg_pkt2, &_pg_struct2))\n");
            }
            else
            {
                compareSource->makeLineSeparator();
                compareSource->write(TAB_IN + "// Decode each packet\n");
                compareSource->write(TAB_IN + "if(!_pg_struct1.decode(_pg_pkt1) || !_pg_struct2.decode(_pg_pkt2))\n");
            }

            compareSource->write(TAB_IN + "{\n");
            compareSource->write(TAB_IN + TAB_IN + "_pg_report = _pg_prename + \" packets failed to decode\\n\";\n");
            compareSource->write(TAB_IN + TAB_IN + "return _pg_report;\n");
            compareSource->write(TAB_IN + "}\n");
        }
        else
        {
            compareSource->makeLineSeparator();
            compareSource->write(TAB_IN + "if(_pg_prename.empty())\n");
            compareSource->write(TAB_IN + TAB_IN + "_pg_prename = \"" + name + "\";\n");

            compareSource->makeLineSeparator();
            compareSource->write(TAB_IN + "// Check packet types\n");
            compareSource->write(TAB_IN + "if((get" + support.protoName + "PacketID(_pg_pkt1) != get" + support.protoName + "PacketID(_pg_pkt1)) || (get"+ support.protoName + "PacketID(_pg_pkt2) != get" + (support.prefix + name + support.packetParameterSuffix) + "ID()))\n");
            compareSource->write(TAB_IN + "{\n");
            compareSource->write(TAB_IN + TAB_IN + "_pg_report += _pg_prename + \" packet IDs are different\\n\";\n");
            compareSource->write(TAB_IN + TAB_IN + "return _pg_report;\n");
            compareSource->write(TAB_IN + "}\n");
        }

        compareSource->makeLineSeparator();
        compareSource->write(TAB_IN + "// Check packet sizes. Even if sizes are different the packets may contain the same result\n");
        compareSource->write(TAB_IN + "if(get" + support.protoName + "PacketSize(_pg_pkt1) != get" + support.protoName + "PacketSize(_pg_pkt2))\n");
        compareSource->write(TAB_IN + TAB_IN + "_pg_report += _pg_prename + \" packet sizes are different\\n\";\n");

        if(numDecodes > 0)
        {
            compareSource->makeLineSeparator();

            if(support.language == ProtocolSupport::c_language)
                compareSource->write(TAB_IN + "_pg_report += compare" + structName + "(_pg_prename, &_pg_struct1, &_pg_struct2);\n");
            else
                compareSource->write(TAB_IN + "_pg_report += _pg_struct1.compare(_pg_prename, &_pg_struct2);\n");
        }

        compareSource->makeLineSeparator();
        compareSource->write(TAB_IN + "return _pg_report;\n");
        compareSource->write("\n");

        if(support.language == ProtocolSupport::c_language)
            compareSource->write("}// compare" + support.prefix + name + support.packetParameterSuffix + "\n");
        else
            compareSource->write("}// " + typeName + "::compare\n");

    }// if we need to generate compare functions

    if(print && (printSource != nullptr))
    {
        printSource->makeLineSeparator();
        printSource->write("/*!\n");
        printSource->write(" * Generate a string that describes the contents of a " + name + " packet\n");
        printSource->write(" * \\param _pg_prename is prepended to the name of the data field in the report\n");
        printSource->write(" * \\param _pg_pkt is the data to print\n");
        printSource->write(" * \\return a string describing the contents of _pg_pkt\n");
        printSource->write(" */\n");
        if(support.language == ProtocolSupport::c_language)
            printSource->write("std::string textPrint" + (support.prefix + name + support.packetParameterSuffix) + "(std::string _pg_prename, const " + support.pointerType + " _pg_pkt)\n");
        else
            printSource->write("std::string " + typeName + "::textPrint(std::string _pg_prename, const " + support.pointerType + " _pg_pkt)\n");
        printSource->write("{\n");
        printSource->write(TAB_IN + "std::string _pg_report;\n");

        if(numDecodes > 0)
        {
            printSource->makeLineSeparator();
            printSource->write(TAB_IN + "// Structure to decode into\n");
            printSource->write(TAB_IN + structName + " _pg_user;\n");

            printSource->makeLineSeparator();
            printSource->write(TAB_IN + "if(_pg_prename.empty())\n");
            printSource->write(TAB_IN + TAB_IN + "_pg_prename = \"" + name + "\";\n");

            if(support.language == ProtocolSupport::c_language)
            {
                // In C we need explicity initializers
                printSource->makeLineSeparator();
                printSource->write(TAB_IN + "// All zeroes before decoding\n");
                printSource->write(TAB_IN + "memset(&_pg_user, 0, sizeof(_pg_user));\n");

                printSource->makeLineSeparator();
                printSource->write(TAB_IN + "// Decode packet\n");
                printSource->write(TAB_IN + "if(!decode" + extendedName() + "(_pg_pkt, &_pg_user))\n");
            }
            else
            {
                printSource->makeLineSeparator();
                printSource->write(TAB_IN + "// Decode packet\n");
                printSource->write(TAB_IN + "if(!_pg_user.decode(_pg_pkt))\n");

            }

            printSource->write(TAB_IN + "{\n");
            printSource->write(TAB_IN + TAB_IN + "_pg_report = _pg_prename + \" packet failed to decode\\n\";\n");
            printSource->write(TAB_IN + TAB_IN + "return _pg_report;\n");
            printSource->write(TAB_IN + "}\n");
        }
        else
        {
            printSource->makeLineSeparator();
            printSource->write(TAB_IN + "if(_pg_prename.empty())\n");
            printSource->write(TAB_IN + TAB_IN + "_pg_prename = \"" + name + "\";\n");

            printSource->makeLineSeparator();
            printSource->write(TAB_IN + "// Check packet type\n");
            printSource->write(TAB_IN + "if(get"+ support.protoName + "PacketID(_pg_pkt) != get" + (support.prefix + name + support.packetParameterSuffix) + "ID())\n");
            printSource->write(TAB_IN + "{\n");
            printSource->write(TAB_IN + TAB_IN + "_pg_report += _pg_prename + \" packet ID is incorrect\\n\";\n");
            printSource->write(TAB_IN + TAB_IN + "return _pg_report;\n");
            printSource->write(TAB_IN + "}\n");
        }

        printSource->makeLineSeparator();
        printSource->write(TAB_IN + "// Print the packet size\n");
        printSource->write(TAB_IN + "_pg_report += _pg_prename + \" packet size is \" + std::to_string(get" + support.protoName + "PacketSize(_pg_pkt)) + \"\\n\";\n");

        if(numDecodes > 0)
        {
            printSource->makeLineSeparator();

            if(support.language == ProtocolSupport::c_language)
                printSource->write(TAB_IN + "_pg_report += textPrint" + structName + "(_pg_prename, &_pg_user);\n");
            else
                printSource->write(TAB_IN + "_pg_report += _pg_user.textPrint(_pg_prename);\n");
        }

        printSource->makeLineSeparator();
        printSource->write(TAB_IN + "return _pg_report;\n");
        printSource->write("\n");
        if(support.language == ProtocolSupport::c_language)
            printSource->write("}// textPrint" + (support.prefix + name + support.packetParameterSuffix) + "\n");
        else
            printSource->write("}// " + typeName + "::textPrint\n");

    }// if we need to generate print functions

}// createStructurePacketFunctions


/*!
 * Get the signature of the packet structure encode function, without semicolon or
 * comments or line feed, for the prototype or actual function.
 * \param insource should be true to indicate this signature is in source code
 *        (i.e. not a prototype) which determines if the "_pg_" decoration is
 *        used as well as c++ access specifiers.
 * \return the encode signature
 */
std::string ProtocolPacket::getStructurePacketEncodeSignature(bool insource) const
{
    std::string output;
    std::string pg;
    int numEncodes = getNumberOfEncodeParameters();

    if(insource)
        pg = "_pg_";

    if(support.language == ProtocolSupport::c_language)
    {
        output = "void encode" + support.prefix + name + support.packetStructureSuffix + "(" + support.pointerType + " " + pg + "pkt";

        if(numEncodes > 0)
            output += ", const " + structName + "* " + pg + "user";
    }
    else
    {
        // C++ class member, this function should be "const" as it does not modify
        // any class members, unless it has no encode parameters, in which case it
        // should be "static".
        if(!insource && (numEncodes <= 0))
            output += "static ";

        output += "void ";

        // In the source the function needs the class scope
        if(insource)
            output += typeName + "::";

        output += "encode(" + support.pointerType + " " + pg + "pkt";
    }

    if(ids.size() <= 1)
        output += ")";
    else
        output += ", uint32_t " + pg + "id)";

    if((support.language == ProtocolSupport::cpp_language) && (numEncodes > 0))
        output += " const";

    return output;

}// ProtocolPacket::getStructurePacketEncodeSignature


/*!
 * Get the prototype for the structure packet encode function
 * \param spacing is the offset for each line
 * \return the prototype including semicolon and line fees
 */
std::string ProtocolPacket::getStructurePacketEncodePrototype(const std::string& spacing) const
{
    std::string output;

    if(!encode)
        return output;

    output += spacing + "//! " + getPacketEncodeBriefComment() + "\n";
    output += spacing + getStructurePacketEncodeSignature(false) + ";\n";;

    return output;
}


/*!
 * Get the body for the structure packet encode function
 * \return The body of the function that encodes this packet from a structure or class.
 */
std::string ProtocolPacket::getStructurePacketEncodeBody(void) const
{
    std::string output;

    if(!encode)
        return output;

    int numEncodes = getNumberOfEncodeParameters();

    // The source function for the encode function
    output += "/*!\n";
    output += " * \\brief " + getPacketEncodeBriefComment() + "\n";
    output += " *\n";
    output += ProtocolParser::outputLongComment(" * ", comment) + "\n";
    output += " * \\param _pg_pkt points to the packet which will be created by this function\n";
    if((numEncodes > 0) && (support.language == ProtocolSupport::c_language))
        output += " * \\param _pg_user points to the user data that will be encoded in _pg_pkt\n";
    if(ids.size() > 1)
        output += " * \\param _pg_id is the packet identifier for _pg_pkt\n";
    output += " */\n";
    output += getStructurePacketEncodeSignature(true) + "\n";
    output += "{\n";

    if(getNumberOfEncodes() > 0)
        output += TAB_IN + "uint8_t* _pg_data = get" + support.protoName + "PacketData(_pg_pkt);\n";

    output += TAB_IN + "int _pg_byteindex = 0;\n";

    if(usestempencodebitfields)
        output += TAB_IN + "unsigned int _pg_tempbitfield = 0;\n";

    if(usestempencodelongbitfields)
        output += TAB_IN + "uint64_t _pg_templongbitfield = 0;\n";

    if(numbitfieldgroupbytes > 0)
    {
        output += TAB_IN + "int _pg_bitfieldindex = 0;\n";
        output += TAB_IN + "uint8_t _pg_bitfieldbytes[" + std::to_string(numbitfieldgroupbytes) + "];\n";
    }

    if(needsEncodeIterator)
        output += TAB_IN + "unsigned _pg_i = 0;\n";

    if(needs2ndEncodeIterator)
        output += TAB_IN + "unsigned _pg_j = 0;\n";

    int bitcount = 0;
    for(std::size_t i = 0; i < encodables.size(); i++)
    {
        output += "\n";
        output += encodables[i]->getEncodeString(support.bigendian, &bitcount, true);
    }

    std::string id;
    if(ids.size() > 1)
        id = "_pg_id";
    else if(support.language == ProtocolSupport::c_language)
        id = "get" + support.prefix + name + support.packetParameterSuffix + "ID()";
    else
        id = "id()";

    ProtocolFile::makeLineSeparator(output);
    output += TAB_IN + "// complete the process of creating the packet\n";
    output += TAB_IN + "finish" + support.protoName + "Packet(_pg_pkt, _pg_byteindex, " + id + ");\n";

    ProtocolFile::makeLineSeparator(output);
    if(support.language == ProtocolSupport::c_language)
        output += "}// encode" + support.prefix + name + support.packetStructureSuffix + "\n";
    else
        output += "}// " + typeName + "::encode\n";

    return output;

}// ProtocolPacket::getStructurePacketEncodeBody


/*!
 * Get the signature of the packet structure decode function, without semicolon or
 * comments or line feed, for the prototype or actual function.
 * \param insource should be true to indicate this signature is in source code
 *        (i.e. not a prototype) which determines if the "_pg_" decoration is
 *        used as well as c++ access specifiers.
 * \return the decode signature
 */
std::string ProtocolPacket::getStructurePacketDecodeSignature(bool insource) const
{
    std::string output;
    std::string pg;

    int numDecodes = getNumberOfDecodeParameters();

    if(insource)
        pg = "_pg_";

    if(support.language == ProtocolSupport::c_language)
    {
        output = "int decode" + support.prefix + name + support.packetStructureSuffix + "(const " + support.pointerType + " " + pg + "pkt";

        if(numDecodes > 0)
            output += ", " + structName + "* " + pg + "user";
    }
    else
    {
        // C++ class member should be static if there are no decodes, because
        // nothing will be modified, we are simply checking of the packet is good
        if(!insource && (numDecodes <= 0))
            output += "static ";

        output += "bool ";

        // In the source the function needs the class scope
        if(insource)
            output += typeName + "::";

        output += "decode(const " + support.pointerType + " " + pg + "pkt";
    }

    output += + ")";

    return output;

}//ProtocolPacket::getStructurePacketDecodeSignature


/*!
 * Get the prototype for the structure packet decode function
 * \param spacing is the offset for each line
 * \return the prototype including semicolon and line fees
 */
std::string ProtocolPacket::getStructurePacketDecodePrototype(const std::string& spacing) const
{
    std::string output;

    if(!decode)
        return output;

    output += spacing + "//! " + getPacketDecodeBriefComment() + "\n";
    output += spacing + getStructurePacketDecodeSignature(false) + ";\n";;

    return output;
}


/*!
 * Get the body for the structure packet decode function
 * \return The body of the function that decodes this packet from a structure or class.
 */
std::string ProtocolPacket::getStructurePacketDecodeBody(void) const
{
    std::string output;

    if(!decode)
        return output;

    // The string that gets the identifier for the packet, if there is only one
    std::string id;
    if(ids.size() <= 1)
    {
        if(support.language == ProtocolSupport::c_language)
            id = "get" + support.prefix + name + support.packetParameterSuffix + "ID()";
        else
            id = "id()";
    }

    // Check if there is anything that is encoded, if not, we use a different form of the function
    if(getNumberOfEncodes() > 0)
    {
        output += "/*!\n";
        output += " * \\brief " + getPacketDecodeBriefComment() + "\n";
        output += " *\n";
        output += ProtocolParser::outputLongComment(" * ", comment) + "\n";
        output += " * \\param _pg_pkt points to the packet being decoded by this function\n";
        if((getNumberOfDecodeParameters() > 0) && (support.language == ProtocolSupport::c_language))
            output += " * \\param _pg_user receives the data decoded from the packet\n";
        output += " * \\return " + getReturnCode(false) + " is returned if the packet ID or size is wrong, else " + getReturnCode(true) + "\n";
        output += " */\n";
        output += getStructurePacketDecodeSignature(true) + "\n";
        output += "{\n";
        output += TAB_IN + "int _pg_numbytes;\n";
        output += TAB_IN + "int _pg_byteindex = 0;\n";
        output += TAB_IN + "const uint8_t* _pg_data;\n";

        if(usestempdecodebitfields)
            output += TAB_IN + "unsigned int _pg_tempbitfield = 0;\n";

        if(usestempdecodelongbitfields)
            output += TAB_IN + "uint64_t _pg_templongbitfield = 0;\n";

        if(numbitfieldgroupbytes > 0)
        {
            output += TAB_IN + "int _pg_bitfieldindex = 0;\n";
            output += TAB_IN + "uint8_t _pg_bitfieldbytes[" + std::to_string(numbitfieldgroupbytes) + "];\n";
        }

        if(needsDecodeIterator)
            output += TAB_IN + "unsigned _pg_i = 0;\n";
        if(needs2ndDecodeIterator)
            output += TAB_IN + "unsigned _pg_j = 0;\n";
        output += "\n";

        if(ids.size() <= 1)
        {
            output += TAB_IN + "// Verify the packet identifier\n";
            output += TAB_IN + "if(get"+ support.protoName + "PacketID(_pg_pkt) != " + id + ")\n";
        }
        else
        {
            output += TAB_IN + "// Verify the packet identifier, multiple options exist\n";
            output += TAB_IN + "uint32_t _pg_packetid = get"+ support.protoName + "PacketID(_pg_pkt);\n";
            output += TAB_IN + "if( _pg_packetid != " + ids.at(0);
            for(std::size_t i = 1; i < ids.size(); i++)
                output += " &&\n" + TAB_IN + TAB_IN + "_pg_packetid != " + ids.at(i);
            output += " )\n";
        }
        output += TAB_IN + TAB_IN + "return " + getReturnCode(false) + ";\n";
        output += "\n";
        output += TAB_IN + "// Verify the packet size\n";
        output += TAB_IN + "_pg_numbytes = get" + support.protoName + "PacketSize(_pg_pkt);\n";
        if(support.language == ProtocolSupport::c_language)
            output += TAB_IN + "if(_pg_numbytes < get" + support.prefix + name + "MinDataLength())\n";
        else
            output += TAB_IN + "if(_pg_numbytes < minLength())\n";
        output += TAB_IN + TAB_IN + "return " + getReturnCode(false) + ";\n";
        output += "\n";
        output += TAB_IN + "// The raw data from the packet\n";
        output += TAB_IN + "_pg_data = get" + support.protoName + "PacketDataConst(_pg_pkt);\n";
        output += "\n";
        if(defaults)
        {
            output += TAB_IN + "// this packet has default fields, make sure they are set\n";

            for(std::size_t i = 0; i < encodables.size(); i++)
                output += encodables[i]->getSetToDefaultsString(true);

        }// if defaults are used in this packet

        ProtocolFile::makeLineSeparator(output);

        // Keep our own track of the bitcount so we know what to do when we close the bitfield
        int bitcount = 0;
        std::size_t i;
        for(i = 0; i < encodables.size(); i++)
        {
            ProtocolFile::makeLineSeparator(output);

            // Encode just the nondefaults here
            if(encodables[i]->isDefault())
                break;

            output += encodables[i]->getDecodeString(support.bigendian, &bitcount, true, true);
        }

        // Before we write out the decodes for default fields we need to check
        // packet size in the event that we were using variable length arrays
        // or dependent fields
        if((encodedLength.minEncodedLength != encodedLength.nonDefaultEncodedLength) && (i > 0))
        {
            ProtocolFile::makeLineSeparator(output);
            output += TAB_IN + "// Used variable length arrays or dependent fields, check actual length\n";
            output += TAB_IN + "if(_pg_numbytes < _pg_byteindex)\n";
            output += TAB_IN + TAB_IN + "return " + getReturnCode(false) + ";\n";
        }

        // Now finish the fields (if any defaults)
        for(; i < encodables.size(); i++)
        {
            ProtocolFile::makeLineSeparator(output);
            output += encodables[i]->getDecodeString(support.bigendian, &bitcount, true, true);
        }

        ProtocolFile::makeLineSeparator(output);
        output += TAB_IN + "return " + getReturnCode(true) + ";\n";

    }// if fields to decode
    else
    {
        output += "/*!\n";
        output += " * \\brief " + getPacketDecodeBriefComment() + "\n";
        output += " *\n";
        output += ProtocolParser::outputLongComment(" * ", comment) + "\n";
        output += " * \\param _pg_pkt points to the packet being decoded by this function\n";
        output += " * \\return " + getReturnCode(false) + " is returned if the packet ID is wrong, else " + getReturnCode(true) + "\n";
        output += " */\n";
        output += getStructurePacketDecodeSignature(true) + "\n";
        output += "{\n";
        if(ids.size() <= 1)
        {
            output += TAB_IN + "// Verify the packet identifier\n";
            output += TAB_IN + "if(get"+ support.protoName + "PacketID(_pg_pkt) != " + id + ")\n";
        }
        else
        {
            output += TAB_IN + "// Verify the packet identifier, multiple options exist\n";
            output += TAB_IN + "uint32_t _pg_packetid = get"+ support.protoName + "PacketID(_pg_pkt);\n";
            output += TAB_IN + "if( _pg_packetid != " + ids.at(0);
            for(std::size_t i = 1; i < ids.size(); i++)
                output += " &&\n" + TAB_IN + TAB_IN + "_pg_packetid != " + ids.at(i);
            output += " )\n";
        }
        output += TAB_IN + TAB_IN + "return " + getReturnCode(false) + ";\n";
        output += TAB_IN + "else\n";
        output += TAB_IN + TAB_IN + "return " + getReturnCode(true) + ";\n";

    }// else if no fields to decode

    ProtocolFile::makeLineSeparator(output);
    if(support.language == ProtocolSupport::c_language)
        output += "}// decode" + support.prefix + name + support.packetStructureSuffix + "\n";
    else
        output += "}// " + typeName + "::decode\n";

    return output;

}// ProtocolPacket::getStructurePacketDecodeBody


/*!
 * Create the functions for encoding and decoding the packet to/from parameters
 */
void ProtocolPacket::createPacketFunctions(void)
{    
    // The prototypes in the header file are only needed for C,
    // in C++ these prototypes are part of the class declaration.
    if(support.language == ProtocolSupport::c_language)
    {
        if(encode)
        {
            // The prototype for the packet encode function
            header.makeLineSeparator();
            header.write(getParameterPacketEncodePrototype(std::string()));
        }

        if(decode)
        {
            // The prototype for the packet decode function
            header.makeLineSeparator();
            header.write(getParameterPacketDecodePrototype(std::string()));
        }
    }

    if(encode)
    {
        source.makeLineSeparator();
        source.write(getParameterPacketEncodeBody());
    }

    if(decode)
    {
        source.makeLineSeparator();
        source.write(getParameterPacketDecodeBody());

    }// if decode function enabled

}// createPacketFunctions


/*!
 * Get the signature of the packet encode function, without semicolon or
 * comments or line feed, for the prototype or actual function.
 * \param insource should be true to indicate this signature is in source code
 *        (i.e. not a prototype) which determines if the "_pg_" decoration is
 *        used as well as c++ access specifiers.
 * \return the encode signature
 */
std::string ProtocolPacket::getParameterPacketEncodeSignature(bool insource) const
{
    std::string output;
    std::string pg;

    if(insource)
        pg = "_pg_";

    if(support.language == ProtocolSupport::c_language)
    {
        output = "void encode" + support.prefix + name + support.packetParameterSuffix + "(" + support.pointerType + " " + pg + "pkt";
    }
    else
    {
        // C++ class member, this function should be "static" as it does not depend on any variables in the class
        if(!insource)
            output += "static ";

        output += "void ";

        // In the source the function needs the class scope
        if(insource)
            output += typeName + "::";

        output += "encode(" + support.pointerType + " " + pg + "pkt";
    }

    output += getDataEncodeParameterList();

    if(ids.size() <= 1)
        output += ")";
    else
        output += ", uint32_t " + pg + "id)";

    return output;
}


/*!
 * Get the prototype for the parameter packet encode function
 * \param spacing is the offset for each line
 * \return the prototype including semicolon and line fees
 */
std::string ProtocolPacket::getParameterPacketEncodePrototype(const std::string& spacing) const
{
    std::string output;

    if(!encode)
        return output;

    output += spacing + "//! " + getDataEncodeBriefComment() + "\n";
    output += spacing + getParameterPacketEncodeSignature(false) + ";\n";

    return output;
}


//! Get the prototype for the parameter packet encode function
std::string ProtocolPacket::getParameterPacketEncodeBody(void) const
{
    std::string output;

    std::size_t i;
    int bitcount = 0;

    if(!encode)
        return output;

    // The string that gets the identifier for the packet
    std::string id;
    if(ids.size() > 1)
        id = "_pg_id";
    else if(support.language == ProtocolSupport::c_language)
        id = "get" + support.prefix + name + support.packetParameterSuffix + "ID()";
    else
        id = "id()";

    output += "/*!\n";
    output += " * \\brief " + getPacketEncodeBriefComment() + "\n";
    output += " *\n";
    output += ProtocolParser::outputLongComment(" * ", comment) + "\n";
    output += " * \\param _pg_pkt points to the packet which will be created by this function\n";
    for(i = 0; i < encodables.size(); i++)
        output += encodables.at(i)->getEncodeParameterComment();

    if(ids.size() > 1)
        output += " * \\param id is the packet identifier for _pg_pkt\n";

    output += " */\n";
    output += getParameterPacketEncodeSignature(true) + "\n";
    output += "{\n";

    if(!encodedLength.isZeroLength())
    {
        output += TAB_IN + "uint8_t* _pg_data = get"+ support.protoName + "PacketData(_pg_pkt);\n";
        output += TAB_IN + "int _pg_byteindex = 0;\n";

        if(usestempencodebitfields)
            output += TAB_IN + "unsigned int _pg_tempbitfield = 0;\n";

        if(usestempencodelongbitfields)
            output += TAB_IN + "uint64_t _pg_templongbitfield = 0;\n";

        if(numbitfieldgroupbytes > 0)
        {
            output += TAB_IN + "int _pg_bitfieldindex = 0;\n";
            output += TAB_IN + "uint8_t _pg_bitfieldbytes[" + std::to_string(numbitfieldgroupbytes) + "];\n";
        }

        if(needsEncodeIterator)
            output += TAB_IN + "unsigned _pg_i = 0;\n";

        if(needs2ndEncodeIterator)
            output += TAB_IN + "unsigned _pg_j = 0;\n";

        // Keep our own track of the bitcount so we know what to do when we close the bitfield
        for(i = 0; i < encodables.size(); i++)
        {
            ProtocolFile::makeLineSeparator(output);
            output += encodables[i]->getEncodeString(support.bigendian, &bitcount, false);
        }

        ProtocolFile::makeLineSeparator(output);
        output += TAB_IN + "// complete the process of creating the packet\n";
        output += TAB_IN + "finish" + support.protoName + "Packet(_pg_pkt, _pg_byteindex, " + id + ");\n";
    }
    else
    {
        ProtocolFile::makeLineSeparator(output);
        output += TAB_IN + "// Zero length packet, no data encoded\n";
        output += TAB_IN + "finish" + support.protoName + "Packet(_pg_pkt, 0, " + id + ");\n";
    }

    ProtocolFile::makeLineSeparator(output);
    if(support.language == ProtocolSupport::c_language)
        output += "}// encode" + support.prefix + name + support.packetParameterSuffix + "\n";
    else
        output += "}// " + typeName + "::encode\n";

    return output;

}// ProtocolPacket::getParameterPacketEncodeBody


/*!
 * Get the signature of the packet decode function, without semicolon or
 * comments or line feed, for the prototype or actual function.
 * \param insource should be true to indicate this signature is in source code
 *        (i.e. not a prototype) which determines if the "_pg_" decoration is
 *        used as well as c++ access specifiers.
 * \return the decode signature
 */
std::string ProtocolPacket::getParameterPacketDecodeSignature(bool insource) const
{
    std::string output;
    std::string pg;

    if(insource)
        pg = "_pg_";

    if(support.language == ProtocolSupport::c_language)
    {
        output = "int decode" + support.prefix + name + support.packetParameterSuffix + "(const " + support.pointerType + " " + pg + "pkt";
    }
    else
    {
        // C++ class member, this function should be "static" as it does not depend on any variables in the class
        if(!insource)
            output += "static ";

        output += "bool ";

        // In the source the function needs the class scope
        if(insource)
            output += typeName + "::";

        output += "decode(const " + support.pointerType + " " + pg + "pkt";
    }

    output += getDataDecodeParameterList() + ")";

    return output;

}// ProtocolPacket::getParameterPacketDecodeSignature


/*!
 * Get the prototype for the parameter packet decode function
 * \param spacing is the offset for each line
 * \return the prototype including semicolon and line fees
 */
std::string ProtocolPacket::getParameterPacketDecodePrototype(const std::string& spacing) const
{
    std::string output;

    if(!decode)
        return output;

    output += spacing + "//! " + getDataDecodeBriefComment() + "\n";
    output += spacing + getParameterPacketDecodeSignature(false) + ";\n";

    return output;
}


//! Get the prototype for the parameter packet decode function
std::string ProtocolPacket::getParameterPacketDecodeBody(void) const
{
    std::string output;

    std::size_t i;
    int bitcount = 0;

    if(!decode)
        return output;

    // The string that gets the identifier for the packet, if there is only one
    std::string id;
    if(ids.size() <= 1)
    {
        if(support.language == ProtocolSupport::c_language)
            id = "get" + support.prefix + name + support.packetParameterSuffix + "ID()";
        else
            id = "id()";
    }

    output += "/*!\n";
    output += " * \\brief " + getPacketDecodeBriefComment() + "\n";
    output += " *\n";
    output += ProtocolParser::outputLongComment(" * ", comment) + "\n";
    output += " * \\param _pg_pkt points to the packet being decoded by this function\n";
    for(i = 0; i < encodables.size(); i++)
        output += encodables.at(i)->getDecodeParameterComment();

    if(support.language == ProtocolSupport::c_language)
        output += " * \\return 0 is returned if the packet ID or size is wrong, else 1\n";
    else
        output += " * \\return false is returned if the packet ID or size is wrong, else true\n";

    output += " */\n";
    output += getParameterPacketDecodeSignature(true) + "\n";
    output += "{\n";

    if(!encodedLength.isZeroLength())
    {
        if(usestempdecodebitfields)
            output += TAB_IN + "unsigned int _pg_tempbitfield = 0;\n";

        if(usestempdecodelongbitfields)
            output += TAB_IN + "uint64_t _pg_templongbitfield = 0;\n";

        if(numbitfieldgroupbytes > 0)
        {
            output += TAB_IN + "int _pg_bitfieldindex = 0;\n";
            output += TAB_IN + "uint8_t _pg_bitfieldbytes[" + std::to_string(numbitfieldgroupbytes) + "];\n";
        }

        if(needsDecodeIterator)
            output += TAB_IN + "unsigned _pg_i = 0;\n";
        if(needs2ndDecodeIterator)
            output += TAB_IN + "unsigned _pg_j = 0;\n";
        output += TAB_IN + "int _pg_byteindex = 0;\n";
        output += TAB_IN + "const uint8_t* _pg_data = get" + support.protoName + "PacketDataConst(_pg_pkt);\n";
        output += TAB_IN + "int _pg_numbytes = get" + support.protoName + "PacketSize(_pg_pkt);\n";
        output += "\n";

        if(ids.size() <= 1)
        {
            output += TAB_IN + "// Verify the packet identifier\n";
            output += TAB_IN + "if(get"+ support.protoName + "PacketID(_pg_pkt) != " + id + ")\n";
        }
        else
        {
            output += TAB_IN + "// Verify the packet identifier, multiple options exist\n";
            output += TAB_IN + "uint32_t packetid = get"+ support.protoName + "PacketID(_pg_pkt);\n";
            output += TAB_IN + "if( packetid != " + ids.at(0);
            for(i = 1; i < ids.size(); i++)
                output += " &&\n        packetid != " + ids.at(i);
            output += " )\n";
        }
        output += TAB_IN + TAB_IN + "return 0;\n";

        output += "\n";
        if(support.language == ProtocolSupport::c_language)
            output += TAB_IN + "if(_pg_numbytes < get" + support.prefix + name + "MinDataLength())\n";
        else
            output += TAB_IN + "if(_pg_numbytes < minLength())\n";
        output += TAB_IN + TAB_IN + "return 0;\n";
        if(defaults)
        {
            output += "\n";
            output += TAB_IN + "// this packet has default fields, make sure they are set\n";

            for(i = 0; i < encodables.size(); i++)
                output += encodables[i]->getSetToDefaultsString(false);

        }// if defaults are used in this packet

        // Keep our own track of the bitcount so we know what to do when we close the bitfield
        bitcount = 0;
        for(i = 0; i < encodables.size(); i++)
        {
            ProtocolFile::makeLineSeparator(output);

            // Encode just the nondefaults here
            if(encodables[i]->isDefault())
                break;

            output += encodables[i]->getDecodeString(support.bigendian, &bitcount, false, true);
        }

        // Before we write out the decodes for default fields we need to check
        // packet size in the event that we were using variable length arrays
        // or dependent fields
        if((encodedLength.minEncodedLength != encodedLength.nonDefaultEncodedLength) && (i > 0))
        {
            ProtocolFile::makeLineSeparator(output);
            output += TAB_IN + "// Used variable length arrays or dependent fields, check actual length\n";
            output += TAB_IN + "if(_pg_numbytes < _pg_byteindex)\n";
            output += TAB_IN + TAB_IN + "return " + getReturnCode(false) + ";\n";
        }

        // Now finish the fields (if any defaults)
        for(; i < encodables.size(); i++)
        {
            ProtocolFile::makeLineSeparator(output);
            output += encodables[i]->getDecodeString(support.bigendian, &bitcount, false, true);
        }

        ProtocolFile::makeLineSeparator(output);
        output += TAB_IN + "return 1;\n";

    }// if some fields to decode
    else
    {
        if(ids.size() <= 1)
        {
            output += TAB_IN + "// Verify the packet identifier\n";
            output += TAB_IN + "if(get"+ support.protoName + "PacketID(_pg_pkt) != " + id + ")\n";
        }
        else
        {
            output += TAB_IN + "// Verify the packet identifier, multiple options exist\n";
            output += TAB_IN + "uint32_t packetid = get"+ support.protoName + "PacketID(_pg_pkt);\n";
            output += TAB_IN + "if( packetid != " + ids.at(0);
            for(i = 1; i < ids.size(); i++)
                output += " &&\n        packetid != " + ids.at(i);
            output += " )\n";
        }
        output += TAB_IN + TAB_IN + "return " + getReturnCode(false) + ";\n";
        output += TAB_IN + "else\n";
        output += TAB_IN + TAB_IN + "return " + getReturnCode(true) + ";\n";

    }// If no fields to decode

    ProtocolFile::makeLineSeparator(output);
    if(support.language == ProtocolSupport::c_language)
        output += "}// decode" + support.prefix + name + support.packetParameterSuffix + "\n";
    else
        output += "}// " + typeName + "::decode\n";

    return output;

}// ProtocolPacket::getParameterPacketDecodeBody


/*!
 * \return The brief comment of the packet encode function, without doxygen decorations or line feed
 */
std::string ProtocolPacket::getPacketEncodeBriefComment(void) const
{
    return std::string("Create the " + support.prefix + name + " packet");
}


/*!
 * \return The brief comment of the packet decode function, without doxygen decorations or line feed
 */
std::string ProtocolPacket::getPacketDecodeBriefComment(void) const
{
    return std::string("Decode the " + support.prefix + name + " packet");
}


/*!
 * \return The parameter list part of a encode signature like ", type1 name1, type2 name2 ... "
 */
std::string ProtocolPacket::getDataEncodeParameterList(void) const
{
    std::string output;

    for(std::size_t i = 0; i < encodables.size(); i++)
        output += encodables[i]->getEncodeSignature();

    return output;
}


/*!
 * \return The parameter list part of a decode signature like ", type1* name1, type2 name2[3] ... "
 */
std::string ProtocolPacket::getDataDecodeParameterList(void) const
{
    std::string output;

    for(std::size_t i = 0; i < encodables.size(); i++)
        output += encodables[i]->getDecodeSignature();

    return output;
}


/*!
 * \return The brief comment of the structure encode function, without doxygen decorations or line feed
 */
std::string ProtocolPacket::getDataEncodeBriefComment(void) const
{
    return std::string("Create the " + support.prefix + name + " packet from parameters");
}


/*!
 * \return The brief comment of the structure decode function, without doxygen decorations or line feed
 */
std::string ProtocolPacket::getDataDecodeBriefComment(void) const
{
    return std::string("Decode the " + support.prefix + name + " packet to parameters");
}


/*!
 * Get the markdown documentation for this packet
 * \param global should be true to include a paragraph number for this heading (not used by this function)
 * \param packetids is the list of packet identifiers, used to determine if a link should be added (not used by this function)
 * \return the markdown output string
 */
std::string ProtocolPacket::getTopLevelMarkdown(bool global, const std::vector<std::string>& packetids) const
{
    (void)global;
    (void)packetids;

    std::string output;

    if(ids.size() <= 1)
    {
        std::string id = ids.at(0);
        std::string idvalue = id;

        // Put an anchor in the identifier line which is the same as the ID. We'll link to it if we can
        if(title == name)
            output += "## <a name=\"" + id + "\"></a>" + name + " packet\n\n";
        else
            output += "## <a name=\"" + id + "\"></a>" + title + "\n\n";

        if(!comment.empty())
        {
            output += comment + "\n";
            output += "\n";
        }

        if(!ids.at(0).empty())
        {
            // In case the packet identifer is an enumeration we know
            idvalue = parser->replaceEnumerationNameWithValue(idvalue);

            if(id.compare(idvalue) == 0)
                output += "- packet identifier: `" + id + "`\n";
            else
                output += "- packet identifier: `" + id + "` : " + idvalue + "\n";
        }

    }// if a single identifier
    else
    {
        // Packet name heading
        if(title == name)
            output += "## " + name + " packet\n\n";
        else
            output += "## " + title + "\n\n";

        if(!comment.empty())
        {
            output += comment + "\n";
            output += "\n";
        }

        output += "This packet supports multiple identifiers.\n";
        output += "\n";
        for(std::size_t i= 0; i < ids.size(); i++)
        {
            std::string id = ids.at(i);
            std::string idvalue = id;

            // In case the packet identifer is an enumeration we know
            idvalue = parser->replaceEnumerationNameWithValue(idvalue);

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
        std::string minLength = replace(EncodedLength::collapseLengthString(encodedLength.minEncodedLength, true), "1*", "");

        // Replace any defined enumerations with their actual value
        minLength = parser->replaceEnumerationNameWithValue(minLength);

        // Re-collapse, perhaps we can solve it now
        minLength = EncodedLength::collapseLengthString(minLength, true);

        // Output the length, replacing the multiply asterisk with a times symbol
        // We put spaces around the multiply symbol, so that the html tables can
        // better reflow the resulting text
        output += "- data length: " + replace(minLength, "*", " &times; ") + "\n";
    }
    else
    {
        // The length strings, which may include enumerated identiers such as "N3D"
        std::string maxLength = replace(EncodedLength::collapseLengthString(encodedLength.maxEncodedLength, true), "1*", "");
        std::string minLength = replace(EncodedLength::collapseLengthString(encodedLength.minEncodedLength, true), "1*", "");

        // Replace any defined enumerations with their actual value
        maxLength = parser->replaceEnumerationNameWithValue(maxLength);
        minLength = parser->replaceEnumerationNameWithValue(minLength);

        // Re-collapse, perhaps we can solve it now
        maxLength = EncodedLength::collapseLengthString(maxLength, true);
        minLength = EncodedLength::collapseLengthString(minLength, true);

        // Output the length, replacing the multiply asterisk with a times symbol
        // We put spaces around the multiply symbol, so that the html tables can
        // better reflow the resulting text
        output += "- minimum data length: " + replace(minLength, "*", " &times; ") + "\n";

        // Output the length, replacing the multiply asterisk with a times symbol
        // We put spaces around the multiply symbol, so that the html tables can
        // better reflow the resulting text
        output += "- maximum data length: " + replace(maxLength, "*", " &times; ") + "\n";
    }

    // Output any documetation data
    output += "\n";
    for(std::size_t i = 0; i < documentList.size(); i++)
        output += documentList.at(i)->getTopLevelMarkdown();

    for(std::size_t i = 0; i < enumList.size(); i++)
    {
        if(enumList[i] == nullptr)
            continue;

        output += enumList[i]->getTopLevelMarkdown();
        output += "\n";
        output += "\n";
    }

    if(encodables.size() > 0)
    {
        std::vector<std::string> bytes, names, encodings, repeats, comments;
        std::string startByte = "0";

        // The column headings
        bytes.push_back("Bytes");
        names.push_back("Name");

        if (parser->hasAboutSection())
        {
            encodings.push_back("[Enc](#Enc)");
        }
        else
        {
            // Disable linking if there's nothing to link to
            encodings.push_back("Enc");
        }

        repeats.push_back("Repeat");
        comments.push_back("Description");

        std::size_t byteColumn = 0, nameColumn = 0, encodingColumn = 0, repeatColumn = 0, commentColumn = 0;

        // Get all the details that are going to end up in the table
        std::vector<int>prefix;
        for(std::size_t i = 0, j = 0; i < encodables.size(); i++)
        {
            if(encodables.at(i) == nullptr)
                continue;

            if(encodables.at(i)->isNotEncoded())
                continue;

            if(!encodables.at(i)->hasDocumentation())
                continue;

            // Prefix is the outline marker for the names in the table
            prefix.push_back((int)(j++));
            encodables.at(i)->getDocumentationDetails(prefix, startByte, bytes, names, encodings, repeats, comments);
            prefix.clear();
        }

        // Figure out the column widths, note that we assume all the lists are the same length
        for(std::size_t i = 0; i < names.size(); i++)
        {
            // replace a "1*" with nothing, since that won't change the value but
            // is clearer. Also replace "*" with the html times symbol. This
            // looks better and does not cause markdown to emphasize the text
            // if there are multiple "*".
            // We put spaces around the multiply symbol, so that the html tables can
            // better reflow the resulting text
            replaceinplace(replaceinplace(bytes[i], "1*", ""), "*", " &times; ");
            replaceinplace(repeats[i], "*", " &times; ");

            if(bytes.at(i).size() > byteColumn)
                byteColumn = bytes.at(i).size();

            if(names.at(i).size() > nameColumn)
                nameColumn = names.at(i).size();

            if(encodings.at(i).size() > encodingColumn)
                encodingColumn = encodings.at(i).size();

            if(repeats.at(i).size() > repeatColumn)
                repeatColumn = repeats.at(i).size();

            if(comments.at(i).size() > commentColumn)
                commentColumn = comments.at(i).size();
        }


        output += "\n";

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
        for(std::size_t i = 0; i < byteColumn; i++)
            output += "-";

        output +=  " | ";
        for(std::size_t i = 0; i < nameColumn; i++)
            output += "-";

        // Encoding column is centered
        output += " | :";
        for(std::size_t i = 1; i < encodingColumn-1; i++)
            output += "-";

        // Repeat column is centered
        output += ": | :";
        for(std::size_t i = 1; i < repeatColumn-1; i++)
            output += "-";

        output += ": | ";
        for(std::size_t i = 0; i < commentColumn; i++)
            output += "-";
        output += " |\n";

        // Now write out the outputs
        for(std::size_t i = 1; i < names.size(); i++)
        {
            // Open the line
            output +=  "| ";
            output += spacedString(bytes.at(i), byteColumn);
            output += " | ";
            output += spacedString(names.at(i), nameColumn);

            // We support the idea that repeats and or encodings could be empty, causing cells to be merged
            if(encodings.at(i).empty() && repeats.at(i).empty())
            {
                output += spacedString("", encodingColumn + repeatColumn);
                output += TAB_IN + " ||| ";
            }
            else if(encodings.at(i).empty())
            {
                output += spacedString(encodings.at(i), encodingColumn);
                output += "   || ";
                output += spacedString(repeats.at(i), repeatColumn);
                output += " | ";
            }
            else if(repeats.at(i).empty())
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

        // Table caption
        output += "[" + title + " packet bytes]\n";

        output += "\n";

    }

    return output;

}// ProtocolPacket::getTopLevelMarkdown

