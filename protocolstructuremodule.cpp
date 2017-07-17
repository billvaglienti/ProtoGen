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
    decode(true)
{
    // These are attributes on top of the normal structure that we support
    attriblist << "encode" << "decode" << "file" << "deffile" << "verifyfile";
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
    encode = decode = true;
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
    encode = !ProtocolParser::isFieldClear(ProtocolParser::getAttribute("encode", map));
    decode = !ProtocolParser::isFieldClear(ProtocolParser::getAttribute("decode", map));

    // Warnings for users
    issueWarnings(map);

    // Do the bulk of the file creation and setup
    setupFiles(moduleName, defheadermodulename, verifymodulename);

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
 * \param verifymodulename is the verify module name from the attibutes
 * \param forceStructureDeclaration should be true to force the declaration of the structure, even if it only has one member
 * \param outputUtilties should be true to output the helper macros
 */
void ProtocolStructureModule::setupFiles(QString moduleName, QString defheadermodulename, QString verifymodulename, bool forceStructureDeclaration, bool outputUtilities)
{
    // The file directive tells us if we are creating a separate file, or if we are appending an existing one
    if(moduleName.isEmpty())
        moduleName = support.globalFileName;

    header.setLicenseText(support.licenseText);
    source.setLicenseText(support.licenseText);

    verifyHeader.setLicenseText(support.licenseText);
    verifySource.setLicenseText(support.licenseText);

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

    // Two options here: we may be appending an a-priori existing file, or we may be starting one fresh.
    if(header.isAppending())
    {
        header.makeLineSeparator();
    }
    else
    {
        // Comment block at the top of the header file
        header.write("/*!\n");
        header.write(" * \\file\n");

        // A potentially long comment that should be wrapped at 80 characters
        if(!comment.isEmpty())
        {
            header.write(" *\n");
            header.write(ProtocolParser::outputLongComment(" *", comment) + "\n");
        }

        // Finish the top comment block
        header.write(" */\n");
        header.write("\n");

    }

    // Include the protocol top level module. This module may already be included, but in that case it won't be included twice
    header.writeIncludeDirective(support.protoName + "Protocol.h");

    // Handle the idea that the structure might be defined in a different file
    if(!defheadermodulename.isEmpty())
    {
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

    // The verify file needs access to the struct file
    verifyheaderfile->writeIncludeDirective(structfile->fileName());

    // The verification details may be spread across multiple files
    QStringList list;
    for(int i = 0; i < encodables.length(); i++)
        encodables[i]->getInitAndVerifyIncludeDirectives(list);
    verifyheaderfile->writeIncludeDirectives(list);

    // Add other includes specific to this structure
    parser->outputIncludes(getHierarchicalName(), *structfile, e);

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
void ProtocolStructureModule::createSubStructureFunctions(void)
{
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
    }

    source.makeLineSeparator();

}// ProtocolStructureModule::createSubStructureFunctions


/*!
 * Write data to the source and header files to encode and decode this structure
 * but not its children. This will add to the length strings, but not reset them
 */
void ProtocolStructureModule::createTopLevelStructureFunctions(void)
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

    header.makeLineSeparator();
    source.makeLineSeparator();

}// ProtocolStructureModule::createTopLevelStructureFunctions
