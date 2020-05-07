#include "protocolstructuremodule.h"
#include "protocolparser.h"
#include <iostream>

/*!
 * Construct the object that parses structure descriptions
 * \param parse points to the global protocol parser that owns everything
 * \param supported gives the supported features of the protocol
 * \param protocolApi is the API string of the protocol
 * \param protocolVersion is the version string of the protocol
 */
ProtocolStructureModule::ProtocolStructureModule(ProtocolParser* parse, ProtocolSupport supported, const QString& protocolApi, const QString& protocolVersion) :
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
    mapHeader(nullptr),
    api(protocolApi),
    version(protocolVersion)
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
    attriblist << "encode" << "decode" << "file" << "deffile" << "verifyfile" << "comparefile" << "printfile" << "mapfile" << "redefine" << "compare" << "print" << "map";
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
void ProtocolStructureModule::issueWarnings(const QDomNamedNodeMap& map)
{
    Q_UNUSED(map);

    if(isArray())
    {
        emitWarning("top level object cannot be an array");
        array.clear();
        variableArray.clear();
        array2d.clear();
        variable2dArray.clear();
    }

    if(!dependsOn.isEmpty())
    {
        emitWarning("dependsOn makes no sense for a top level object");
        dependsOn.clear();
    }

    if(!dependsOnValue.isEmpty())
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

    // Me and all my children, which may themselves be structures
    ProtocolStructure::parse();

    QDomNamedNodeMap map = e.attributes();

    QString moduleName = ProtocolParser::getAttribute("file", map);
    QString defheadermodulename = ProtocolParser::getAttribute("deffile", map);
    QString verifymodulename = ProtocolParser::getAttribute("verifyfile", map);
    QString comparemodulename = ProtocolParser::getAttribute("comparefile", map);
    QString printmodulename = ProtocolParser::getAttribute("printfile", map);
    QString mapmodulename = ProtocolParser::getAttribute("mapfile", map);

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

    QString redefinename = ProtocolParser::getAttribute("redefine", map);

    // Warnings for users
    issueWarnings(map);

    if(!redefinename.isEmpty())
    {
        if(redefinename == name)
            emitWarning("Redefine must be different from name");
        else
        {
            redefines = parser->lookUpStructure(support.prefix + redefinename + "_t");
            if(redefines == NULL)
                emitWarning("Could not find structure to redefine");
        }

        if(redefines != NULL)
            structName = support.prefix + redefinename + "_t";
    }

    // Do the bulk of the file creation and setup
    setupFiles(moduleName, defheadermodulename, verifymodulename, comparemodulename, printmodulename, mapmodulename, true, true);

    // The functions to encoding and ecoding
    createStructureFunctions();

    // Write to disk, note that duplicate flush() calls are OK
    header.flush();    
    structHeader->flush();

    // We don't write the source to disk if we are not encoding or decoding anything
    if(encode || decode)
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
void ProtocolStructureModule::setupFiles(QString moduleName,
                                         QString defheadermodulename,
                                         QString verifymodulename,
                                         QString comparemodulename,
                                         QString printmodulename,
                                         QString mapmodulename,
                                         bool forceStructureDeclaration, bool outputUtilities)
{
    // User can provide compare flag, or the file name, or set the global flag
    if(!comparemodulename.isEmpty() || !support.globalCompareName.isEmpty() || support.compare)
        compare = true;

    // User can provide print flag, or the file name, or set the global flag
    if(!printmodulename.isEmpty() || !support.globalPrintName.isEmpty() || support.print)
        print = true;

    // User can provide map flag, or the file name, or set the global flag
    if(!mapmodulename.isEmpty() || !support.globalMapName.isEmpty() || support.mapEncode)
        mapEncode = true;

    // In order to do compare, print, map, verify or init we must actually have some parameters
    if((getNumberOfEncodeParameters() <= 0) && (getNumberOfDecodeParameters() <= 0))
        compare = print = mapEncode = hasverify = hasinit = false;

    // We need to reflect the compare, print, and mapEncode flags to our child structures
    for(int i = 0; i < encodables.length(); i++)
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
    if(moduleName.isEmpty())
        moduleName = support.globalFileName;

    // The file names
    if(moduleName.isEmpty())
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

    if(verifymodulename.isEmpty())
        verifymodulename = support.globalVerifyName;

    if(verifymodulename.isEmpty())
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
        if(comparemodulename.isEmpty())
            comparemodulename = support.globalCompareName;

        if(comparemodulename.isEmpty() && (support.language == ProtocolSupport::c_language))
            comparemodulename = support.prefix + name + "_compare";

        if(comparemodulename.isEmpty())
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
    }

    if(mapEncode)
    {
        if(mapmodulename.isEmpty())
            mapmodulename = support.globalMapName;

        // In C the map outputs cannot be in the main code files, because they are c++
        if(mapmodulename.isEmpty() && (support.language == ProtocolSupport::c_language))
            mapmodulename = support.prefix + name + "_map";

        if(mapmodulename.isEmpty())
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
        if(printmodulename.isEmpty())
            printmodulename = support.globalPrintName;

        // In C the print outputs cannot be in the main code files, because they are c++
        if(printmodulename.isEmpty() && (support.language == ProtocolSupport::c_language))
            printmodulename = support.prefix + name + "_print";

        if(printmodulename.isEmpty())
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

        // Make sure to provide the extractText function
        printSource->makeLineSeparator();
        printSource->writeOnce(getExtractTextFunction());
        printSource->makeLineSeparator();

    }

    // Include the protocol top level module. This module may already be included, but in that case it won't be included twice
    header.writeIncludeDirective(support.protoName + "Protocol.h");

    // If we are using someone elses definition then we can't have a separate definition file
    if(redefines != NULL)
    {
        QStringList list;
        redefines->getIncludeDirectives(list);
        header.writeIncludeDirectives(list);
    }
    else if(!defheadermodulename.isEmpty())
    {
        // Handle the idea that the structure might be defined in a different file
        _structHeader.setModuleNameAndPath(defheadermodulename, support.outputpath, support.language);
        structHeader = &_structHeader;

        if(support.supportbool && (support.language == ProtocolSupport::c_language))
            structHeader->writeIncludeDirective("stdbool.h", "", true);

        // The structHeader might need stdint.h. It's an open question if this is
        // the best answer, or if we should just include the main protocol file
        structHeader->writeIncludeDirective("stdint.h", QString(), true);

        // In this instance we know that the normal header file needs to include
        // the file with the structure definition
        header.writeIncludeDirective(structHeader->fileName());
    }

    QStringList list;
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
        compareHeader->writeIncludeDirective("string", QString(), true, false);
        compareSource->writeIncludeDirective("sstream", QString(), true, false);
        compareSource->writeIncludeDirective("iomanip", QString(), true, false);

        if(support.language == ProtocolSupport::cpp_language)
        {
            // In C++ these function declarations are in the class declaration
            structHeader->writeIncludeDirective("string", QString(), true, false);
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
        printHeader->writeIncludeDirective("string", QString(), true, false);
        printSource->writeIncludeDirective("fieldencode.h", QString(), false, false);
        printSource->writeIncludeDirective("sstream", QString(), true, false);
        printSource->writeIncludeDirective("iomanip", QString(), true, false);

        if(support.language == ProtocolSupport::cpp_language)
        {
            // In C++ these function declarations are in the class declaration
            structHeader->writeIncludeDirective("string", QString(), true, false);
        }

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
        mapHeader->writeIncludeDirective("QVariant", QString(), true, false);
        mapHeader->writeIncludeDirective("QString", QString(), true, false);

        if(support.language == ProtocolSupport::cpp_language)
        {
            // In C++ these function declarations are in the class declaration
            structHeader->writeIncludeDirective("QString", QString(), true, false);
            structHeader->writeIncludeDirective("QVariant", QString(), true, false);
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
        for(int i = 0; i < encodables.length(); i++)
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
    for(int i = 0; i < enumList.count(); i++)
    {
        QString enumoutput = enumList.at(i)->getSourceOutput();
        if(!enumoutput.isEmpty())
        {
            source.makeLineSeparator();
            source.write(enumoutput);
        }
    }

    // White space is good
    header.makeLineSeparator();

    // The encoded size of this structure as a macro that others can access
    if(((encode != false) || (decode != false)) && outputUtilities)
    {
        header.write("//! return the minimum encoded length for the " + typeName + " structure\n");
        header.write("#define getMinLengthOf" + typeName + "() ");
        if(encodedLength.minEncodedLength.isEmpty())
            header.write("(0)\n");
        else
            header.write("(" + encodedLength.minEncodedLength + ")\n");

        // White space is good
        header.makeLineSeparator();
    }

}// ProtocolStructureModule::setupFiles


/*!
 * Return the include directives needed for this encodable
 * \param list is appended with any directives this encodable requires.
 */
void ProtocolStructureModule::getIncludeDirectives(QStringList& list) const
{
    // Our header
    list.append(structHeader->fileName());
    list.append(header.fileName());

    // And any of our children's headers
    ProtocolStructure::getIncludeDirectives(list);

    list.removeDuplicates();
}


/*!
 * Return the include directives that go into source code for this encodable
 * \param list is appended with any directives for source code.
 */
void ProtocolStructureModule::getSourceIncludeDirectives(QStringList& list) const
{
    if(support.specialFloat)
        list.append("floatspecial.h");

    list.append("fielddecode.h");
    list.append("fieldencode.h");
    list.append("scaleddecode.h");
    list.append("scaledencode.h");

    // And any of our children's headers
    ProtocolStructure::getSourceIncludeDirectives(list);

    list.removeDuplicates();
}


/*!
 * Return the include directives needed for this encodable's init and verify functions
 * \param list is appended with any directives this encodable requires.
 */
void ProtocolStructureModule::getInitAndVerifyIncludeDirectives(QStringList& list) const
{
    // Our header
    if(verifyHeader != nullptr)
        list.append(verifyHeader->fileName());

    // And any of our children's headers
    ProtocolStructure::getInitAndVerifyIncludeDirectives(list);

    list.removeDuplicates();
}


/*!
 * Return the include directives needed for this encodable's map functions
 * \param list is appended with any directives this encodable requires.
 */
void ProtocolStructureModule::getMapIncludeDirectives(QStringList& list) const
{
    // Our header
    if(mapHeader != nullptr)
        list.append(mapHeader->fileName());

    // And any of our children's headers
    ProtocolStructure::getMapIncludeDirectives(list);

    list.removeDuplicates();
}


/*!
 * Return the include directives needed for this encodable's compare functions
 * \param list is appended with any directives this encodable requires.
 */
void ProtocolStructureModule::getCompareIncludeDirectives(QStringList& list) const
{
    // Our header
    if(compareHeader != nullptr)
        list.append(compareHeader->fileName());

    // And any of our children's headers
    ProtocolStructure::getCompareIncludeDirectives(list);

    list.removeDuplicates();
}


/*!
 * Return the include directives needed for this encodable's print functions
 * \param list is appended with any directives this encodable requires.
 */
void ProtocolStructureModule::getPrintIncludeDirectives(QStringList& list) const
{
    // Our header
    if(printHeader != nullptr)
        list.append(printHeader->fileName());

    // And any of our children's headers
    ProtocolStructure::getPrintIncludeDirectives(list);

    list.removeDuplicates();
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
    for(int i = 0; i < encodables.size(); i++)
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
            verifyHeader->write(getSetToInitialValueFunctionPrototype(QString(), false));
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
            header.write(getEncodeFunctionPrototype(QString(), false));
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
            header.write(getDecodeFunctionPrototype(QString(), false));
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
            verifyHeader->write(getVerifyFunctionPrototype(QString(), false));
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
            compareHeader->write(getComparisonFunctionPrototype(QString(), false));
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
            printHeader->write(getTextPrintFunctionPrototype(QString(), false));
            printHeader->makeLineSeparator();
            printHeader->write(getTextReadFunctionPrototype(QString(), false));
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
            mapHeader->write(getMapEncodeFunctionPrototype(QString(), false));
            mapHeader->makeLineSeparator();
            mapHeader->write(getMapDecodeFunctionPrototype(QString(), false));
            mapHeader->makeLineSeparator();
        }

        mapSource->makeLineSeparator();
        mapSource->write(getMapEncodeFunctionBody(false));
        mapSource->makeLineSeparator();
        mapSource->write(getMapDecodeFunctionBody(false));
        mapSource->makeLineSeparator();
    }

}// ProtocolStructureModule::createTopLevelStructureFunctions


//! Get the text used to extract text for text read functions
QString ProtocolStructureModule::getExtractTextFunction(void)
{
    std::string output = "\
//! Extract text that is identified by a key\n\
static std::string extractText(const std::string& key, const std::string& source, int* fieldcount);\n\
\n\
/*!\n\
 * Extract text that is identified by a key\n\
 * \\param key is the key, the text to extract follows the key and is on the same line\n\
 * \\param source is the source information to find the key in\n\
 * \\param fieldcount is incremented whenever the key is found in the source\n\
 * \\return the extracted text, which may be empty\n\
 */\n\
std::string extractText(const std::string& key, const std::string& source, int* fieldcount)\n\
{\n\
    std::string text;\n\
\n\
    std::string::size_type index = source.find(key);\n\
\n\
    if(index < source.length())\n\
    {\n\
        // This is the location of the first character after the key\n\
        std::string::size_type first = index + key.length();\n\
\n\
        // The location of the next linefeed after the key\n\
        std::string::size_type linefeed = source.find(\"\\n\", first);\n\
\n\
        // This is how many characters until we get to the linefeed\n\
        if((linefeed > first) && (linefeed < source.length()))\n\
        {\n\
            // This is the number of characters to remove\n\
            std::string::size_type length = linefeed - first;\n\
\n\
            // Increment our field count\n\
            (*fieldcount)++;\n\
\n\
            // Extract the text between the key and the linefeed\n\
            text = source.substr(first, length);\n\
\n\
            // Remove the first \" '\" from the string\n\
            if((text.length() > 1) && (text.at(0) == ' ') && (text.at(1) == '\\''))\n\
                text.erase(0, 2);\n\
\n\
            // Remove the last \"'\" from the string\n\
            if((text.length() > 0) && (text.back() == '\\''))\n\
                text.erase(text.length()-1, 1);\n\
        }\n\
    }\n\
\n\
    return text;\n\
\n\
}// extractText\n";

    return QString::fromStdString(output);

}// ProtocolStructureModule::getExtractTextFunction
