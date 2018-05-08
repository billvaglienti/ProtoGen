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
    structfile(&header),
    verifyheaderfile(&header),
    verifysourcefile(&source),
    api(protocolApi),
    version(protocolVersion),
    encode(true),
    decode(true),
    compare(false),
    print(false),
    mapEncode(false)
{
    // These are attributes on top of the normal structure that we support
    attriblist << "encode" << "decode" << "file" << "deffile" << "verifyfile" << "comparefile" << "printfile" << "mapfile" << "redefine" << "map";

    compareSource.setCpp(true);
    compareHeader.setCpp(true);
    printSource.setCpp(true);
    printHeader.setCpp(true);
    mapSource.setCpp(true);
    mapHeader.setCpp(true);

}


ProtocolStructureModule::~ProtocolStructureModule(void)
{
    clear();
}


/*!
 * Clear out any data, resetting for next packet parse operation
 */
void ProtocolStructureModule::clear(void)
{
    ProtocolStructure::clear();
    source.clear();
    header.clear();
    defheader.clear();
    compareHeader.clear();
    compareSource.clear();
    printHeader.clear();
    printSource.clear();
    mapSource.clear();
    mapHeader.clear();
    encode = decode = true;
    print = compare = mapEncode = false;
    structfile = &header;
    verifyheaderfile = &header;
    verifysourcefile = &source;

    // Note that data set during constructor are not changed

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
    compare = ProtocolParser::isFieldSet(ProtocolParser::getAttribute("compare", map));
    print = ProtocolParser::isFieldSet(ProtocolParser::getAttribute("print", map));
    mapEncode = ProtocolParser::isFieldSet(ProtocolParser::getAttribute("map", map));

    QString redefinename = ProtocolParser::getAttribute("redefine", map);

    // Warnings for users
    issueWarnings(map);

    const ProtocolStructureModule* redefines = NULL;
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
    setupFiles(moduleName, defheadermodulename, verifymodulename, comparemodulename, printmodulename, mapmodulename, true, true, redefines);

    // The functions to encoding and ecoding
    createStructureFunctions();

    // Write to disk, note that duplicate flush() calls are OK
    header.flush();    
    structfile->flush();

    // We don't write the source to disk if we are not encoding or decoding anything
    if(encode || decode)
        source.flush();
    else
        source.clear();

    // Only write the compare if we have compare functions to output
    if(compare)
    {
        compareSource.flush();
        compareHeader.flush();
    }

    // Only write the print if we have print functions to output
    if(print)
    {
        printSource.flush();
        printHeader.flush();
    }

    // Only write the map functions if we have map functions to support
    if(mapEncode)
    {
        mapSource.flush();
        mapHeader.flush();
    }

    // We don't write the verify files to disk if we are not initializing or verifying anything
    if(hasInit() || hasVerify())
    {
        verifyheaderfile->flush();
        verifysourcefile->flush();
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
                                         bool forceStructureDeclaration, bool outputUtilities, const ProtocolStructureModule* redefines)
{
    // User can provide compare flag, or the file name
    if(!comparemodulename.isEmpty() || !support.globalCompareName.isEmpty())
        compare = true;

    // User can provide print flag, or the file name
    if(!printmodulename.isEmpty() || !support.globalPrintName.isEmpty())
        print = true;

    if(!mapmodulename.isEmpty() || !support.globalMapName.isEmpty())
        mapEncode = true;

    // In order to do compare, print, map, verify or init we must actually have some parameters
    if((getNumberOfEncodeParameters() <= 0) && (getNumberOfDecodeParameters() <= 0))
        compare = print = mapEncode = hasverify = hasinit = false;

    // Must have a structure definition to do any of these operations
    if(compare || print || hasverify || hasinit)
        forceStructureDeclaration = true;

    // The file directive tells us if we are creating a separate file, or if we are appending an existing one
    if(moduleName.isEmpty())
        moduleName = support.globalFileName;

    header.setLicenseText(support.licenseText);
    source.setLicenseText(support.licenseText);

    verifyHeader.setLicenseText(support.licenseText);
    verifySource.setLicenseText(support.licenseText);

    compareHeader.setLicenseText(support.licenseText);
    compareSource.setLicenseText(support.licenseText);

    mapHeader.setLicenseText(support.licenseText);
    mapSource.setLicenseText(support.licenseText);

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

    if(verifymodulename.isEmpty())
        verifymodulename = support.globalVerifyName;

    if(!verifymodulename.isEmpty() && (hasInit() || hasVerify()))
    {
        verifyHeader.setModuleNameAndPath(verifymodulename, support.outputpath);
        verifySource.setModuleNameAndPath(verifymodulename, support.outputpath);

        verifyheaderfile = &verifyHeader;
        verifysourcefile = &verifySource;
    }

    if(compare)
    {
        if(comparemodulename.isEmpty())
            comparemodulename = support.globalCompareName;

        if(comparemodulename.isEmpty())
        {
            compareHeader.setModuleNameAndPath(support.prefix, name + "_compare", support.outputpath);
            compareSource.setModuleNameAndPath(support.prefix, name + "_compare", support.outputpath);
        }
        else
        {
            compareHeader.setModuleNameAndPath(comparemodulename, support.outputpath);
            compareSource.setModuleNameAndPath(comparemodulename, support.outputpath);
        }

        compareHeader.makeLineSeparator();

        // We may be appending an a-priori existing file, or we may be starting one fresh.
        if(!compareHeader.isAppending())
        {
            // Comment block at the top of the header file needed so doxygen will document the file
            compareHeader.write("/*!\n");
            compareHeader.write(" * \\file\n");
            compareHeader.write(" */\n");
            compareHeader.write("\n");
        }

    }

    if(print)
    {
        if(printmodulename.isEmpty())
            printmodulename = support.globalPrintName;

        if(printmodulename.isEmpty())
        {
            printHeader.setModuleNameAndPath(support.prefix, name + "_print", support.outputpath);
            printSource.setModuleNameAndPath(support.prefix, name + "_print", support.outputpath);
        }
        else
        {
            printHeader.setModuleNameAndPath(printmodulename, support.outputpath);
            printSource.setModuleNameAndPath(printmodulename, support.outputpath);
        }

        printHeader.makeLineSeparator();

        // We may be appending an a-priori existing file, or we may be starting one fresh.
        if(!printHeader.isAppending())
        {
            // Comment block at the top of the header file needed so doxygen will document the file
            printHeader.write("/*!\n");
            printHeader.write(" * \\file\n");
            printHeader.write(" */\n");
            printHeader.write("\n");
        }

        if(!printSource.isAppending())
        {
            // Make sure to provide the extractText function
            printSource.makeLineSeparator();
            printSource.write(getExtractTextFunction());
            printSource.makeLineSeparator();
        }
    }

    if(mapEncode)
    {
        if(mapmodulename.isEmpty())
            mapmodulename = support.globalMapName;

        if(mapmodulename.isEmpty())
        {
            mapHeader.setModuleNameAndPath(support.prefix, name + "_map", support.outputpath);
            mapSource.setModuleNameAndPath(support.prefix, name + "_map", support.outputpath);
        }
        else
        {
            mapHeader.setModuleNameAndPath(mapmodulename, support.outputpath);
            mapSource.setModuleNameAndPath(mapmodulename, support.outputpath);
        }

        mapHeader.makeLineSeparator();

        if(!mapHeader.isAppending())
        {
            mapHeader.write("/*!\n");
            mapHeader.write(" * \\file\n");
            mapHeader.write(" */\n");
            mapHeader.write("\n");
        }

        if(!printSource.isAppending())
        {
            mapSource.makeLineSeparator();
        }
    }

    // Two options here: we may be appending an a-priori existing file, or we may be starting one fresh.
    if(header.isAppending())
    {
        header.makeLineSeparator();
    }
    else
    {
        // Comment block at the top of the header file needed so doxygen will document the file
        header.write("/*!\n");
        header.write(" * \\file\n");
        header.write(" */\n");
        header.write("\n");
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

        defheader.setLicenseText(support.licenseText);
        defheader.setModuleNameAndPath(defheadermodulename, support.outputpath);

        if(defheader.isAppending())
            defheader.makeLineSeparator();

        structfile = &defheader;

        // The structfile might need stdint.h. It's an open question if this is
        // the best answer, or if we should just include the main protocol file
        structfile->writeIncludeDirective("stdint.h", QString(), true);

        // In this instance we know that the normal header file needs to include
        // the file with the structure definition
        header.writeIncludeDirective(structfile->fileName());
    }

    // The verify, comparison, print, and map files needs access to the struct file
    verifyheaderfile->writeIncludeDirective(structfile->fileName());
    compareHeader.writeIncludeDirective(structfile->fileName());
    compareHeader.writeIncludeDirective(header.fileName());
    compareHeader.writeIncludeDirective("QString", QString(), true, false);
    printHeader.writeIncludeDirective(structfile->fileName());
    printHeader.writeIncludeDirective(header.fileName());
    printHeader.writeIncludeDirective("QString", QString(), true, false);
    mapHeader.writeIncludeDirective(header.fileName());
    mapHeader.writeIncludeDirective("QVariant", QString(), true, false);
    mapHeader.writeIncludeDirective("string", QString(), true, false);

    // The verification details may be spread across multiple files
    QStringList list;
    for(int i = 0; i < encodables.length(); i++)
        encodables[i]->getInitAndVerifyIncludeDirectives(list);
    verifyheaderfile->writeIncludeDirectives(list);

    // Add other includes specific to this structure
    parser->outputIncludes(getHierarchicalName(), *structfile, e);

    // If we are using someones elses definition we don't need to output our structure or add any of its includes
    if(redefines == NULL)
    {
        // Include directives that may be needed for our children
        list.clear();
        for(int i = 0; i < encodables.length(); i++)
            encodables[i]->getIncludeDirectives(list);
        structfile->writeIncludeDirectives(list);

        // White space is good
        structfile->makeLineSeparator();

        // Create the structure definition in the header.
        // This includes any sub-structures as well
        structfile->write(getStructureDeclaration(forceStructureDeclaration));

        // White space is good
        structfile->makeLineSeparator();
    }

    // White space is good
    source.makeLineSeparator();

    if(hasVerify() || hasInit())
    {
        verifyheaderfile->makeLineSeparator();
        verifyheaderfile->write(getInitialAndVerifyDefines());
        verifyheaderfile->makeLineSeparator();
    }

    if(support.specialFloat)
        source.writeIncludeDirective("floatspecial.h");

    source.writeIncludeDirective("fielddecode.h");
    source.writeIncludeDirective("fieldencode.h");
    source.writeIncludeDirective("scaleddecode.h");
    source.writeIncludeDirective("scaledencode.h");

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
    list.append(structfile->fileName());
    list.append(header.fileName());

    ProtocolStructure::getIncludeDirectives(list);

}


/*!
 * Return the include directives needed for this encodable's init and verify functions
 * \param list is appended with any directives this encodable requires.
 */
void ProtocolStructureModule::getInitAndVerifyIncludeDirectives(QStringList& list) const
{
    // Our verify header
    list.append(verifyheaderfile->fileName());

    // And any of our children's headers
    ProtocolStructure::getInitAndVerifyIncludeDirectives(list);

    list.removeDuplicates();
}


/*!
 * Write data to the source and header files to encode and decode this structure
 * and all its children. This will reset the length strings.
 */
void ProtocolStructureModule::createStructureFunctions(const ProtocolStructureModule* redefines)
{
    // The encoding and decoding prototypes of my children, if any.
    // I want these to appear before me, because I'm going to call them
    createSubStructureFunctions(redefines);

    // Now build the top level function
    createTopLevelStructureFunctions(redefines);

}// ProtocolStructureModule::createStructureFunctions


/*!
 * Create the functions that encode/decode sub stuctures.
 * These functions are local to the source module
 */
void ProtocolStructureModule::createSubStructureFunctions(const ProtocolStructureModule* redefines)
{
    if(redefines != NULL)
        return;

    // The embedded structures functions
    for(int i = 0; i < encodables.size(); i++)
    {
        ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

        if(!structure)
            continue;

        if(encode)
        {
            source.makeLineSeparator();
            source.write(structure->getPrototypeEncodeString(support.bigendian));
            source.makeLineSeparator();
            source.write(structure->getFunctionEncodeString(support.bigendian));
        }

        if(decode)
        {
            source.makeLineSeparator();
            source.write(structure->getPrototypeDecodeString(support.bigendian));
            source.makeLineSeparator();
            source.write(structure->getFunctionDecodeString(support.bigendian));
        }

        if(hasInit())
        {
            verifysourcefile->makeLineSeparator();
            verifysourcefile->write(structure->getSetToInitialValueFunctionPrototype());
            verifysourcefile->makeLineSeparator();
            verifysourcefile->write(structure->getSetToInitialValueFunctionString());
            verifysourcefile->makeLineSeparator();
        }

        if(hasVerify())
        {
            verifysourcefile->makeLineSeparator();
            verifysourcefile->write(structure->getVerifyFunctionPrototype());
            verifysourcefile->makeLineSeparator();
            verifysourcefile->write(structure->getVerifyFunctionString());
            verifysourcefile->makeLineSeparator();
        }

        if(compare)
        {
            compareSource.makeLineSeparator();
            compareSource.write(structure->getComparisonFunctionPrototype());
            compareSource.makeLineSeparator();
            compareSource.write(structure->getComparisonFunctionString());
            compareSource.makeLineSeparator();
        }

        if(print)
        {
            printSource.makeLineSeparator();
            printSource.write(structure->getTextPrintFunctionPrototype());
            printSource.makeLineSeparator();
            printSource.write(structure->getTextPrintFunctionString());
            printSource.makeLineSeparator();
            printSource.write(structure->getTextReadFunctionPrototype());
            printSource.makeLineSeparator();
            printSource.write(structure->getTextReadFunctionString());
            printSource.makeLineSeparator();
        }

        if(mapEncode)
        {
            mapSource.makeLineSeparator();
            mapSource.write(structure->getMapEncodeFunctionPrototype());
            mapSource.makeLineSeparator();
            mapSource.write(structure->getMapEncodeFunctionString());
            mapSource.makeLineSeparator();
            mapSource.makeLineSeparator();
            mapSource.write(structure->getMapDecodeFunctionPrototype());
            mapSource.makeLineSeparator();
            mapSource.write(structure->getMapDecodeFunctionString());
            mapSource.makeLineSeparator();
        }

    }

    source.makeLineSeparator();

}// ProtocolStructureModule::createSubStructureFunctions


/*!
 * Write data to the source and header files to encode and decode this structure
 * but not its children. This will add to the length strings, but not reset them
 */
void ProtocolStructureModule::createTopLevelStructureFunctions(const ProtocolStructureModule* redefines)
{
    if(encode)
    {
        header.makeLineSeparator();
        header.write(getPrototypeEncodeString(support.bigendian, false));
        source.makeLineSeparator();
        source.write(getFunctionEncodeString(support.bigendian, false));
    }

    if(decode)
    {
        header.makeLineSeparator();
        header.write(getPrototypeDecodeString(support.bigendian, false));
        source.makeLineSeparator();
        source.write(getFunctionDecodeString(support.bigendian, false));
    }

    header.makeLineSeparator();
    source.makeLineSeparator();

    if(redefines != NULL)
        return;

    if(hasInit())
    {
        verifyheaderfile->makeLineSeparator();
        verifyheaderfile->write(getSetToInitialValueFunctionPrototype(false));
        verifyheaderfile->makeLineSeparator();

        verifysourcefile->makeLineSeparator();
        verifysourcefile->write(getSetToInitialValueFunctionString(false));
        verifysourcefile->makeLineSeparator();
    }

    if(hasVerify())
    {
        verifyheaderfile->makeLineSeparator();
        verifyheaderfile->write(getVerifyFunctionPrototype(false));
        verifyheaderfile->makeLineSeparator();

        verifysourcefile->makeLineSeparator();
        verifysourcefile->write(getVerifyFunctionString(false));
        verifysourcefile->makeLineSeparator();
    }


    if(compare)
    {
        compareHeader.makeLineSeparator();
        compareHeader.write(getComparisonFunctionPrototype(false));
        compareHeader.makeLineSeparator();

        compareSource.makeLineSeparator();
        compareSource.write(getComparisonFunctionString(false));
        compareSource.makeLineSeparator();
    }


    if(print)
    {
        printHeader.makeLineSeparator();
        printHeader.write(getTextPrintFunctionPrototype(false));
        printHeader.makeLineSeparator();
        printHeader.write(getTextReadFunctionPrototype(false));
        printHeader.makeLineSeparator();

        printSource.makeLineSeparator();
        printSource.write(getTextPrintFunctionString(false));
        printSource.makeLineSeparator();
        printSource.write(getTextReadFunctionString(false));
        printSource.makeLineSeparator();
    }

    if (mapEncode)
    {
        mapHeader.makeLineSeparator();
        mapHeader.write(getMapEncodeFunctionPrototype(false));
        mapHeader.makeLineSeparator();
        mapHeader.write(getMapDecodeFunctionPrototype(false));
        mapHeader.makeLineSeparator();

        mapSource.makeLineSeparator();
        mapSource.write(getMapEncodeFunctionString(false));
        mapSource.makeLineSeparator();
        mapSource.write(getMapDecodeFunctionString(false));
        mapSource.makeLineSeparator();
    }

}// ProtocolStructureModule::createTopLevelStructureFunctions


//! Get the text used to extract text for text read functions
QString ProtocolStructureModule::getExtractTextFunction(void)
{
    return QString("\
//! Extract text that is identified by a key\n\
QString extractText(const QString& key, const QString& source);\n\
\n\
/*!\n\
 * Extract text that is identified by a key\n\
 * \\param key is the key, the text to extract follows the key and is on the same line\n\
 * \\param source is the source information to find the key in\n\
 * \\param fieldcount is incremented whenever the key is found in the source\n\
 * \\return the extracted text, which may be empty\n\
 */\n\
QString extractText(const QString& key, const QString& source, int* fieldcount)\n\
{\n\
    QString text;\n\
\n\
    int index = source.indexOf(key);\n\
\n\
    if(index >= 0)\n\
    {\n\
        // This is the location of the first character after the key\n\
        int first = index + key.length();\n\
\n\
        // This is how many characters until we get to the linefeed\n\
        int length = source.indexOf(\"\\n\", first) - first;\n\
\n\
        if(length > 0)\n\
        {\n\
            // Increment our field count\n\
            (*fieldcount)++;\n\
\n\
            // Extract the text between the key and the linefeed\n\
            text = source.mid(first, length);\n\
\n\
            // Remove the first \" '\" from the string\n\
            if((text.length() > 1) && (text.at(0) == QChar(' ')) && (text.at(1) == QChar('\\'')))\n\
                text.remove(0, 2);\n\
\n\
            // Remove the last \"'\" from the string\n\
            if((text.length() > 0) && (text.at(text.length()-1) == QChar('\\'')))\n\
                text.remove(text.length()-1, 1);\n\
        }\n\
    }\n\
\n\
    return text;\n\
\n\
}// extractText\n\n");

}// ProtocolStructureModule::getExtractTextFunction
