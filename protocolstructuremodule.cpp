#include "protocolstructuremodule.h"
#include "protocolparser.h"
#include <iostream>

/*!
 * Construct the object that parses structure descriptions
 * \param parse points to the global protocol parser that owns everything
 * \param supported gives the supported features of the protocol
 */
ProtocolStructureModule::ProtocolStructureModule(ProtocolParser* parse, ProtocolSupport supported) :
    ProtocolStructure(parse, supported.protoName, supported),
    source(supported),
    header(supported),
    _structHeader(supported),
    _verifySource(supported),
    _verifyHeader(supported),
    _compareSource(supported),
    _compareHeader(supported),
    _printSource(supported),
    _printHeader(supported),
    _mapSource(supported),
    _mapHeader(supported),
    structHeader(&header),
    verifySource(&source),
    verifyHeader(&header),
    compareSource(nullptr),
    compareHeader(nullptr),
    printSource(nullptr),
    printHeader(nullptr),
    mapSource(nullptr),
    mapHeader(nullptr)
{
    // In the C language these files must have their modules, because they use
    // c++ features, in c++ they can output to the source and header files
    if(support.language != ProtocolSupport::c_language)
    {
        compareSource = &source;
        compareHeader = &header;
        printSource = &source;
        printHeader = &header;
        mapSource = &source;
        mapHeader = &header;
    }

    // These are attributes on top of the normal structure that we support
    std::vector<std::string> newattribs({"encode", "decode", "file", "deffile", "verifyfile", "comparefile", "printfile", "mapfile", "redefine", "compare", "print", "map"});

    // Now append the new attributes onto our old list
    attriblist.insert(attriblist.end(), newattribs.begin(), newattribs.end());
}


ProtocolStructureModule::~ProtocolStructureModule(void)
{
}


/*!
 * Clear out any data, resetting for next packet parse operation
 */
void ProtocolStructureModule::clear(void)
{
    ProtocolStructure::clear();
    source.clear();
    header.clear();
    _structHeader.clear();
    _compareHeader.clear();
    _compareSource.clear();
    _printHeader.clear();
    _printSource.clear();
    _mapSource.clear();
    _mapHeader.clear();
    structHeader = &header;
    verifyHeader = &header;
    verifySource = &source;

    // In the C language these files must have their modules, because they use .cpp features
    if(support.language == ProtocolSupport::c_language)
    {
        compareSource = nullptr;
        compareHeader = nullptr;
        printSource = nullptr;
        printHeader = nullptr;
        mapSource = nullptr;
        mapHeader = nullptr;
    }
    else
    {
        compareSource = &source;
        compareHeader = &header;
        printSource = &source;
        printHeader = &header;
        mapSource = &source;
        mapHeader = &header;
    }

    // Note that api, version, and support are not changed

}


/*!
 * Issue warnings for the structure module. This should be called after the
 * attributes have been parsed.
 */
void ProtocolStructureModule::issueWarnings(const XMLAttribute* map)
{
    (void)map;

    if(isArray())
    {
        emitWarning("top level object cannot be an array");
        array.clear();
        variableArray.clear();
        array2d.clear();
        variable2dArray.clear();
    }

    if(!dependsOn.empty())
    {
        emitWarning("dependsOn makes no sense for a top level object");
        dependsOn.clear();
    }

    if(!dependsOnValue.empty())
    {
        emitWarning("dependsOnValue makes no sense for a top level object");
        dependsOnValue.clear();
    }

}// ProtocolStructureModule::issueWarnings


/*!
 * Create the source and header files that represent a packet
 */
void ProtocolStructureModule::parse(void)
{
    // Initialize metadata
    clear();

    if(e == nullptr)
        return;

    // Me and all my children, which may themselves be structures
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

    std::string redefinename = ProtocolParser::getAttribute("redefine", map);

    // Warnings for users
    issueWarnings(map);

    if(!redefinename.empty())
    {
        if(redefinename == name)
            emitWarning("Redefine must be different from name");
        else
        {
            redefines = parser->lookUpStructure(support.prefix + redefinename + support.typeSuffix);
            if(redefines == NULL)
                emitWarning("Could not find structure to redefine");
        }

        if(redefines != NULL)
            structName = support.prefix + redefinename + support.typeSuffix;
    }

    // Don't output if hidden and we are omitting hidden items
    if(isHidden() && !neverOmit && support.omitIfHidden)
    {
        std::cout << "Skipping code output for hidden global structure " << getHierarchicalName() << std::endl;
        return;
    }

    // Do the bulk of the file creation and setup
    setupFiles(moduleName, defheadermodulename, verifymodulename, comparemodulename, printmodulename, mapmodulename, true, true);

    // The functions to encoding and ecoding
    createStructureFunctions();

    // Write to disk, note that duplicate flush() calls are OK
    header.flush();    
    structHeader->flush();
    source.flush();

    // Only write the compare if we have compare functions to output
    if(compare)
    {
        compareSource->flush();
        compareHeader->flush();
    }

    // Only write the print if we have print functions to output
    if(print)
    {
        printSource->flush();
        printHeader->flush();
    }

    // Only write the map functions if we have map functions to support
    if(mapEncode)
    {
        mapSource->flush();
        mapHeader->flush();
    }

    // We don't write the verify files to disk if we are not initializing or verifying anything
    if(hasInit() || hasVerify())
    {
        verifyHeader->flush();
        verifySource->flush();
    }

}// ProtocolStructureModule::parse


/*!
 * Setup the files, which accounts for all the ways the fils can be organized for this structure.
 * \param moduleName is the module name from the attributes
 * \param defheadermodulename is the structure header file name from the attributes
 * \param verifymodulename is the verify module name from the attributes
 * \param comparemodulename is the comparison module name from the attributes
 * \param printmodulename is the print module name from the attributes
 * \param forceStructureDeclaration should be true to force the declaration of the structure, even if it only has one member
 * \param outputUtilties should be true to output the helper macros
 */
void ProtocolStructureModule::setupFiles(std::string moduleName,
                                         std::string defheadermodulename,
                                         std::string verifymodulename,
                                         std::string comparemodulename,
                                         std::string printmodulename,
                                         std::string mapmodulename,
                                         bool forceStructureDeclaration, bool outputUtilities)
{
    // User can provide compare flag, or the file name, or set the global flag
    if(!comparemodulename.empty() || !support.globalCompareName.empty() || support.compare)
        compare = true;

    // User can provide print flag, or the file name, or set the global flag
    if(!printmodulename.empty() || !support.globalPrintName.empty() || support.print)
        print = true;

    // User can provide map flag, or the file name, or set the global flag
    if(!mapmodulename.empty() || !support.globalMapName.empty() || support.mapEncode)
        mapEncode = true;

    // In order to do compare, print, map, verify or init we must actually have some parameters
    if((getNumberOfEncodeParameters() <= 0) && (getNumberOfDecodeParameters() <= 0))
        compare = print = mapEncode = hasverify = hasinit = false;

    // We need to reflect the compare, print, and mapEncode flags to our child structures
    for(std::size_t i = 0; i < encodables.size(); i++)
    {
        // Is this encodable a structure?
        ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

        if(structure == nullptr)
            continue;

        if(compare)
            structure->setCompare(true);

        if(print)
            structure->setPrint(true);

        if(mapEncode)
            structure->setMapEncode(true);
    }

    // Must have a structure definition to do any of these operations
    if(compare || print || mapEncode || hasverify || hasinit)
        forceStructureDeclaration = true;

    // The file directive tells us if we are creating a separate file, or if we are appending an existing one
    if(moduleName.empty())
        moduleName = support.globalFileName;

    // The file names
    if(moduleName.empty())
    {
        header.setModuleNameAndPath(support.prefix, name, support.outputpath);
        source.setModuleNameAndPath(support.prefix, name, support.outputpath);
    }
    else
    {
        header.setModuleNameAndPath(moduleName, support.outputpath);
        source.setModuleNameAndPath(moduleName, support.outputpath);
    }

    if(support.supportbool && (support.language == ProtocolSupport::c_language))
        header.writeIncludeDirective("stdbool.h", "", true);

    if(verifymodulename.empty())
        verifymodulename = support.globalVerifyName;

    if(verifymodulename.empty())
    {
        // We can do this in C or C++ because the verify and init functions are all C-based
        verifyHeader = &header;
        verifySource = &source;
    }
    else if(hasInit() || hasVerify())
    {
        _verifyHeader.setModuleNameAndPath(verifymodulename, support.outputpath);
        _verifySource.setModuleNameAndPath(verifymodulename, support.outputpath);
        verifyHeader = &_verifyHeader;
        verifySource = &_verifySource;
    }

    if(compare)
    {
        if(comparemodulename.empty())
            comparemodulename = support.globalCompareName;

        if(comparemodulename.empty() && (support.language == ProtocolSupport::c_language))
            comparemodulename = support.prefix + name + "_compare";

        if(comparemodulename.empty())
        {
            compareHeader = &header;
            compareSource = &source;
        }
        else
        {
            _compareHeader.setModuleNameAndPath(comparemodulename, support.outputpath, ProtocolSupport::cpp_language);
            _compareSource.setModuleNameAndPath(comparemodulename, support.outputpath, ProtocolSupport::cpp_language);
            compareHeader = &_compareHeader;
            compareSource = &_compareSource;
        }

        // Make sure to provide the helper functions
        compareSource->makeLineSeparator();
        compareSource->writeOnce(getToFormattedStringFunction());
        compareSource->makeLineSeparator();
    }

    if(mapEncode)
    {
        if(mapmodulename.empty())
            mapmodulename = support.globalMapName;

        // In C the map outputs cannot be in the main code files, because they are c++
        if(mapmodulename.empty() && (support.language == ProtocolSupport::c_language))
            mapmodulename = support.prefix + name + "_map";

        if(mapmodulename.empty())
        {
            mapHeader = &header;
            mapSource = &source;
        }
        else
        {
            _mapHeader.setModuleNameAndPath(mapmodulename, support.outputpath, ProtocolSupport::cpp_language);
            _mapSource.setModuleNameAndPath(mapmodulename, support.outputpath, ProtocolSupport::cpp_language);
            mapHeader = &_mapHeader;
            mapSource = &_mapSource;
        }
    }

    if(print)
    {
        if(printmodulename.empty())
            printmodulename = support.globalPrintName;

        // In C the print outputs cannot be in the main code files, because they are c++
        if(printmodulename.empty() && (support.language == ProtocolSupport::c_language))
            printmodulename = support.prefix + name + "_print";

        if(printmodulename.empty())
        {
            printHeader = &header;
            printSource = &source;
        }
        else
        {
            _printHeader.setModuleNameAndPath(printmodulename, support.outputpath, ProtocolSupport::cpp_language);
            _printSource.setModuleNameAndPath(printmodulename, support.outputpath, ProtocolSupport::cpp_language);
            printHeader = &_printHeader;
            printSource = &_printSource;
        }

        // Make sure to provide the helper functions
        printSource->makeLineSeparator();
        printSource->writeOnce(getToFormattedStringFunction());
        printSource->makeLineSeparator();
        printSource->writeOnce(getExtractTextFunction());
        printSource->makeLineSeparator();

    }

    // Include the protocol top level module. This module may already be included, but in that case it won't be included twice
    header.writeIncludeDirective(support.protoName + "Protocol");

    // If we are using someone elses definition then we can't have a separate definition file
    if(redefines != NULL)
    {
        std::vector<std::string> list;
        redefines->getIncludeDirectives(list);
        header.writeIncludeDirectives(list);
    }
    else if(!defheadermodulename.empty())
    {
        // Handle the idea that the structure might be defined in a different file
        _structHeader.setModuleNameAndPath(defheadermodulename, support.outputpath, support.language);
        structHeader = &_structHeader;

        if(support.supportbool && (support.language == ProtocolSupport::c_language))
            structHeader->writeIncludeDirective("stdbool.h", "", true);

        // The structHeader might need stdint.h. It's an open question if this is
        // the best answer, or if we should just include the main protocol file
        structHeader->writeIncludeDirective("stdint.h", std::string(), true);

        // In this instance we know that the normal header file needs to include
        // the file with the structure definition
        header.writeIncludeDirective(structHeader->fileName());
    }

    std::vector<std::string> list;
    if(hasVerify() || hasInit())
    {
        verifyHeader->writeIncludeDirective(structHeader->fileName());
        verifyHeader->writeIncludeDirective(header.fileName());

        // The verification details may be spread across multiple files
        list.clear();
        getInitAndVerifyIncludeDirectives(list);
        verifyHeader->writeIncludeDirectives(list);
        verifyHeader->makeLineSeparator();
        verifyHeader->write(getInitialAndVerifyDefines());
        verifyHeader->makeLineSeparator();
    }

    // The compare details may be spread across multiple files
    if(compare)
    {
        compareHeader->writeIncludeDirective(structHeader->fileName());
        compareHeader->writeIncludeDirective(header.fileName());
        compareHeader->writeIncludeDirective("string", std::string(), true, false);
        compareSource->writeIncludeDirective("sstream", std::string(), true, false);
        compareSource->writeIncludeDirective("iomanip", std::string(), true, false);
        compareSource->writeIncludeDirective("cstring", std::string(), true, false);

        if(support.language == ProtocolSupport::cpp_language)
        {
            // In C++ these function declarations are in the class declaration
            structHeader->writeIncludeDirective("string", std::string(), true, false);
        }

        list.clear();
        getCompareIncludeDirectives(list);
        compareHeader->writeIncludeDirectives(list);
        compareHeader->makeLineSeparator();
    }

    // The print details may be spread across multiple files
    if(print)
    {
        printHeader->writeIncludeDirective(structHeader->fileName());
        printHeader->writeIncludeDirective(header.fileName());        
        printHeader->writeIncludeDirective("string", std::string(), true, false);
        printSource->writeIncludeDirective("sstream", std::string(), true, false);
        printSource->writeIncludeDirective("iomanip", std::string(), true, false);
        printSource->writeIncludeDirective("cstring", std::string(), true, false);

        if(support.language == ProtocolSupport::cpp_language)
        {
            // In C++ these function declarations are in the class declaration
            structHeader->writeIncludeDirective("string", std::string(), true, false);
            printSource->writeIncludeDirective("fieldencode.hpp", std::string(), false);
        }
        else
            printSource->writeIncludeDirective("fieldencode.h", std::string(), false);

        list.clear();
        getPrintIncludeDirectives(list);
        printHeader->writeIncludeDirectives(list);
        printHeader->makeLineSeparator();
    }

    // The map details may be spread across multiple files
    if(mapEncode)
    {
        mapHeader->writeIncludeDirective(structHeader->fileName());
        mapHeader->writeIncludeDirective(header.fileName());
        mapHeader->writeIncludeDirective("QVariant", std::string(), true, false);
        mapHeader->writeIncludeDirective("QString", std::string(), true, false);

        if(support.language == ProtocolSupport::cpp_language)
        {
            // In C++ these function declarations are in the class declaration
            structHeader->writeIncludeDirective("QString", std::string(), true, false);
            structHeader->writeIncludeDirective("QVariant", std::string(), true, false);
        }

        list.clear();
        getMapIncludeDirectives(list);
        mapHeader->writeIncludeDirectives(list);
        mapHeader->makeLineSeparator();
    }

    // Add other includes specific to this structure
    parser->outputIncludes(getHierarchicalName(), *structHeader, e);

    // If we are using someones elses definition we don't need to output our structure or add any of its includes
    if(redefines == NULL)
    {
        // Include directives that may be needed for our children
        list.clear();
        for(std::size_t i = 0; i < encodables.size(); i++)
            encodables[i]->getIncludeDirectives(list);

        structHeader->writeIncludeDirectives(list);
    }

    // White space is good
    structHeader->makeLineSeparator();

    // Create the structure/class definition, this includes any sub-structures as well
    structHeader->write(getStructureDeclaration(forceStructureDeclaration));

    // White space is good
    structHeader->makeLineSeparator();

    // White space is good
    source.makeLineSeparator();

    list.clear();
    getSourceIncludeDirectives(list);
    source.writeIncludeDirectives(list);

    // Outputs for the enumerations in source file, if any
    for(std::size_t i = 0; i < enumList.size(); i++)
    {
        std::string enumoutput = enumList.at(i)->getSourceOutput();
        if(!enumoutput.empty())
        {
            source.makeLineSeparator();
            source.write(enumoutput);
        }
    }

    // White space is good
    header.makeLineSeparator();

    // The encoded size of this structure as a macro that others can access
    if(((encode != false) || (decode != false)) && outputUtilities && (support.language == ProtocolSupport::c_language))
    {        
        // White space is good
        header.makeLineSeparator();

        // The utility functions
        header.write(createUtilityFunctions(std::string()));

        // White space is good
        header.makeLineSeparator();
    }

}// ProtocolStructureModule::setupFiles


/*!
 * Create utility functions for structure lengths. The structure must
 * already have been parsed to give the lengths.
 * \param spacing sets the amount of space to put before each line.
 * \return the string which goes in the header or class definition, depending
 *         on the language being output
 */
std::string ProtocolStructureModule::createUtilityFunctions(const std::string& spacing) const
{
    std::string output;

    if(support.language == ProtocolSupport::c_language)
    {
        // The macro for the minimum packet length
        output += spacing + "//! return the minimum encoded length for the " + typeName + " structure\n";
        output += spacing + "#define getMinLengthOf" + typeName + "() ";
        if(encodedLength.minEncodedLength.empty())
            output += "0\n";
        else
            output += "("+encodedLength.minEncodedLength + ")\n";

        // The macro for the maximum packet length
        output += "\n";
        output += spacing + "//! return the maximum encoded length for the " + typeName + " structure\n";
        output += spacing + "#define getMaxLengthOf" + typeName + "() ";
        if(encodedLength.maxEncodedLength.empty())
            output += "0\n";
        else
            output += "("+encodedLength.maxEncodedLength + ")\n";
    }
    else
    {
        // The minimum encoded length
        output += spacing + "//! \\return the minimum encoded length for the structure\n";
        output += spacing + "static int minLength(void) { return ";
        if(encodedLength.minEncodedLength.empty())
            output += "0;}\n";
        else
            output += "("+encodedLength.minEncodedLength + ");}\n";

        // The maximum encoded length
        output += "\n";
        output += spacing + "//! \\return the maximum encoded length for the structure\n";
        output += spacing + "static int maxLength(void) { return ";
        if(encodedLength.maxEncodedLength.empty())
            output += "0;}\n";
        else
            output += "("+encodedLength.maxEncodedLength + ");}\n";
    }

    return output;

}// ProtocolStructureModule::createUtilityFunctions


/*!
 * Return the include directives needed for this encodable
 * \param list is appended with any directives this encodable requires.
 */
void ProtocolStructureModule::getIncludeDirectives(std::vector<std::string>& list) const
{
    // Our header
    list.push_back(structHeader->fileName());
    list.push_back(header.fileName());

    // And any of our children's headers
    ProtocolStructure::getIncludeDirectives(list);

    removeDuplicates(list);
}


/*!
 * Return the include directives that go into source code for this encodable
 * \param list is appended with any directives for source code.
 */
void ProtocolStructureModule::getSourceIncludeDirectives(std::vector<std::string>& list) const
{
    if(support.specialFloat)
        list.push_back("floatspecial");

    list.push_back("fielddecode");
    list.push_back("fieldencode");
    list.push_back("scaleddecode");
    list.push_back("scaledencode");

    // And any of our children's headers
    ProtocolStructure::getSourceIncludeDirectives(list);

    removeDuplicates(list);
}


/*!
 * Return the include directives needed for this encodable's init and verify functions
 * \param list is appended with any directives this encodable requires.
 */
void ProtocolStructureModule::getInitAndVerifyIncludeDirectives(std::vector<std::string>& list) const
{
    // Our header
    if(verifyHeader != nullptr)
        list.push_back(verifyHeader->fileName());

    // And any of our children's headers
    ProtocolStructure::getInitAndVerifyIncludeDirectives(list);

    removeDuplicates(list);
}


/*!
 * Return the include directives needed for this encodable's map functions
 * \param list is appended with any directives this encodable requires.
 */
void ProtocolStructureModule::getMapIncludeDirectives(std::vector<std::string>& list) const
{
    // Our header
    if(mapHeader != nullptr)
        list.push_back(mapHeader->fileName());

    // And any of our children's headers
    ProtocolStructure::getMapIncludeDirectives(list);

    removeDuplicates(list);
}


/*!
 * Return the include directives needed for this encodable's compare functions
 * \param list is appended with any directives this encodable requires.
 */
void ProtocolStructureModule::getCompareIncludeDirectives(std::vector<std::string>& list) const
{
    // Our header
    if(compareHeader != nullptr)
        list.push_back(compareHeader->fileName());

    // And any of our children's headers
    ProtocolStructure::getCompareIncludeDirectives(list);

    removeDuplicates(list);
}


/*!
 * Return the include directives needed for this encodable's print functions
 * \param list is appended with any directives this encodable requires.
 */
void ProtocolStructureModule::getPrintIncludeDirectives(std::vector<std::string>& list) const
{
    // Our header
    if(printHeader != nullptr)
        list.push_back(printHeader->fileName());

    // And any of our children's headers
    ProtocolStructure::getPrintIncludeDirectives(list);

    removeDuplicates(list);
}


/*!
 * Write data to the source and header files to encode and decode this structure
 * and all its children.
 */
void ProtocolStructureModule::createStructureFunctions(void)
{
    // The encoding and decoding prototypes of my children, if any.
    // I want these to appear before me, because I'm going to call them
    createSubStructureFunctions();

    // Now build the top level function
    createTopLevelStructureFunctions();

}// ProtocolStructureModule::createStructureFunctions


/*!
 * Create the functions that encode/decode sub stuctures.
 * These functions are local to the source module
 */
void ProtocolStructureModule::createSubStructureFunctions()
{
    // If we are redefining someone else, then their children are already defined
    if(redefines != nullptr)
        return;

    // The embedded structures functions
    for(std::size_t i = 0; i < encodables.size(); i++)
    {
        ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

        if(!structure)
            continue;

        if(support.language == ProtocolSupport::cpp_language)
        {
            // In this case the initialization function is the constructor,
            // which always exists if we have any parameters. It always goes
            // in the source file. The constructor prototype is already in the
            // class definition in the header file.
            if(structure->getNumberInMemory() > 0)
            {
                source.makeLineSeparator();
                source.write(structure->getSetToInitialValueFunctionBody());
            }
        }
        else if(hasInit() && (verifyHeader != nullptr) && (verifySource != nullptr))
        {
            verifyHeader->makeLineSeparator();
            verifyHeader->write(structure->getSetToInitialValueFunctionPrototype());

            verifySource->makeLineSeparator();
            verifySource->write(structure->getSetToInitialValueFunctionBody());
        }

        if(encode)
        {
            // In C++ this is part of the class declaration
            if(support.language == ProtocolSupport::c_language)
            {
                header.makeLineSeparator();
                header.write(structure->getEncodeFunctionPrototype());
            }

            source.makeLineSeparator();
            source.write(structure->getEncodeFunctionBody(support.bigendian));
        }

        if(decode)
        {
            // In C++ this is part of the class declaration
            if(support.language == ProtocolSupport::c_language)
            {
                header.makeLineSeparator();
                header.write(structure->getDecodeFunctionPrototype());
            }

            source.makeLineSeparator();
            source.write(structure->getDecodeFunctionBody(support.bigendian));
        }

        if(hasVerify() && (verifyHeader != nullptr) && (verifySource != nullptr))
        {
            // In C++ this is part of the class declaration
            if(support.language == ProtocolSupport::c_language)
            {
                verifyHeader->makeLineSeparator();
                verifyHeader->write(structure->getVerifyFunctionPrototype());
            }

            verifySource->makeLineSeparator();
            verifySource->write(structure->getVerifyFunctionBody());
        }

        if(compare && (compareSource != nullptr))
        {
            // In C++ this is part of the class declaration
            if((support.language == ProtocolSupport::c_language) && (compareHeader != nullptr))
            {
                compareHeader->makeLineSeparator();
                compareHeader->write(structure->getComparisonFunctionPrototype());
            }

            compareSource->makeLineSeparator();
            compareSource->write(structure->getComparisonFunctionBody());
        }

        if(print && (printSource != nullptr))
        {
            // In C++ this is part of the class declaration
            if((support.language == ProtocolSupport::c_language) && (printHeader != nullptr))
            {
                printHeader->makeLineSeparator();
                printHeader->write(structure->getTextPrintFunctionPrototype());
                printHeader->makeLineSeparator();
                printHeader->write(structure->getTextReadFunctionPrototype());
            }

            printSource->makeLineSeparator();
            printSource->write(structure->getTextPrintFunctionBody());
            printSource->makeLineSeparator();
            printSource->write(structure->getTextReadFunctionBody());
        }

        if(mapEncode && (mapSource != nullptr))
        {
            // In C++ this is part of the class declaration
            if((support.language == ProtocolSupport::c_language) && (mapHeader != nullptr))
            {
                mapHeader->makeLineSeparator();
                mapHeader->write(structure->getMapEncodeFunctionPrototype());
                mapHeader->makeLineSeparator();
                mapHeader->write(structure->getMapDecodeFunctionPrototype());
            }

            mapSource->makeLineSeparator();
            mapSource->write(structure->getMapEncodeFunctionBody());
            mapSource->makeLineSeparator();
            mapSource->write(structure->getMapDecodeFunctionBody());
        }

    }// for all of our structure children

    source.makeLineSeparator();

}// ProtocolStructureModule::createSubStructureFunctions


/*!
 * Write data to the source and header files to encode and decode this structure
 * but not its children. This is all functions for the structure, including
 * constructor, encode, decode, verify, print, and map functions.
 */
void ProtocolStructureModule::createTopLevelStructureFunctions()
{
    // Output the constructor first
    if(hasInit() && (verifySource != nullptr) && (redefines == nullptr))
    {
        // In C++ this is part of the class declaration
        if((support.language == ProtocolSupport::c_language) && (verifyHeader != nullptr))
        {
            verifyHeader->makeLineSeparator();
            verifyHeader->write(getSetToInitialValueFunctionPrototype(std::string(), false));
            verifyHeader->makeLineSeparator();
        }

        verifySource->makeLineSeparator();
        verifySource->write(getSetToInitialValueFunctionBody(false));
        verifySource->makeLineSeparator();
    }

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

    // The helper functions, which are verify, print, and map
    createTopLevelStructureHelperFunctions();

}// ProtocolStructureModule::createTopLevelStructureFunctions


/*!
 * Write data to the source and header files to the helper functions for this
 * structure, but not its children. This is for verify, print, and map functions.
 */
void ProtocolStructureModule::createTopLevelStructureHelperFunctions(void)
{
    // The rest of the functions are not output if we are redefining another class
    if(redefines != nullptr)
        return;

    if(hasVerify() && (verifySource != nullptr))
    {
        // In C++ this is part of the class declaration
        if((support.language == ProtocolSupport::c_language) && (verifyHeader != nullptr))
        {
            verifyHeader->makeLineSeparator();
            verifyHeader->write(getVerifyFunctionPrototype(std::string(), false));
            verifyHeader->makeLineSeparator();
        }

        verifySource->makeLineSeparator();
        verifySource->write(getVerifyFunctionBody(false));
        verifySource->makeLineSeparator();
    }


    if(compare && (compareSource != nullptr))
    {
        // In C++ this is part of the class declaration
        if((support.language == ProtocolSupport::c_language) && (compareHeader != nullptr))
        {
            compareHeader->makeLineSeparator();
            compareHeader->write(getComparisonFunctionPrototype(std::string(), false));
            compareHeader->makeLineSeparator();
        }

        compareSource->makeLineSeparator();
        compareSource->write(getComparisonFunctionBody(false));
        compareSource->makeLineSeparator();
    }


    if(print && (printSource != nullptr))
    {
        // In C++ this is part of the class declaration
        if((support.language == ProtocolSupport::c_language) && (printHeader != nullptr))
        {
            printHeader->makeLineSeparator();
            printHeader->write(getTextPrintFunctionPrototype(std::string(), false));
            printHeader->makeLineSeparator();
            printHeader->write(getTextReadFunctionPrototype(std::string(), false));
            printHeader->makeLineSeparator();
        }

        printSource->makeLineSeparator();
        printSource->write(getTextPrintFunctionBody(false));
        printSource->makeLineSeparator();
        printSource->write(getTextReadFunctionBody(false));
        printSource->makeLineSeparator();
    }

    if(mapEncode && (mapSource != nullptr))
    {
        // In C++ this is part of the class declaration
        if((support.language == ProtocolSupport::c_language) && (mapHeader != nullptr))
        {
            mapHeader->makeLineSeparator();
            mapHeader->write(getMapEncodeFunctionPrototype(std::string(), false));
            mapHeader->makeLineSeparator();
            mapHeader->write(getMapDecodeFunctionPrototype(std::string(), false));
            mapHeader->makeLineSeparator();
        }

        mapSource->makeLineSeparator();
        mapSource->write(getMapEncodeFunctionBody(false));
        mapSource->makeLineSeparator();
        mapSource->write(getMapDecodeFunctionBody(false));
        mapSource->makeLineSeparator();
    }

}// ProtocolStructureModule::createTopLevelStructureFunctions


//! Get the text used to print a formatted string function
std::string ProtocolStructureModule::getToFormattedStringFunction(void)
{
    return R"(//! Create a numeric string with a specific number of decimal places
static std::string to_formatted_string(double number, int precision);

/*!
 * Create a numeric string with a specific number of decimal places
 * \param number is the number to convert to string
 * \param precision is the number of decimal places to output
 * \return the number as a string
 */
std::string to_formatted_string(double number, int precision)
{
    // This function exists becuase of a bug in GCC which prevents this from working correctly:
    // string = (std::stringstream() << std::setprecision(7) << _pg_user1->indices[_pg_i]).str()

    std::stringstream stream;
    stream << std::setprecision(precision);
    stream << number;
    return stream.str();

}// to_formatted_string)";

}// ProtocolStructureModule::getToFormattedStringFunction


//! Get the text used to extract text for text read functions
std::string ProtocolStructureModule::getExtractTextFunction(void)
{
    return R"(//! Extract text that is identified by a key
static std::string extractText(const std::string& key, const std::string& source, int* fieldcount);

/*!
 * Extract text that is identified by a key
 * \param key is the key, the text to extract follows the key and is on the same line
 * \param source is the source information to find the key in
 * \param fieldcount is incremented whenever the key is found in the source
 * \return the extracted text, which may be empty
 */
std::string extractText(const std::string& key, const std::string& source, int* fieldcount)
{
    std::string text;

    // All fields follow the key with " '". Use that as part of the search; to
    // prevent detecting shorter keys that are repeated within longer keys
    std::string::size_type index = source.find(key + " '");

    // This is the location of the first character after the key
    std::string::size_type first = index + key.size() + 2;

    if(first < source.size())
    {
        // The location of the next linefeed after the key
        std::string::size_type linefeed = source.find("\n", first);

        // This is how many characters until we get to the linefeed
        if((linefeed > first) && (linefeed < source.size()))
        {
            // This is the number of characters to remove
            std::string::size_type length = linefeed - first;

            // Increment our field count
            (*fieldcount)++;

            // Extract the text between the key and the linefeed
            text = source.substr(first, length);

            // Remove the last "'" from the string
            if((text.size() > 0) && (text.back() == '\''))
                text.erase(text.size()-1, 1);
        }
    }

    return text;

}// extractText)";

}// ProtocolStructureModule::getExtractTextFunction
