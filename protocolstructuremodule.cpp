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
    api(protocolApi),
    version(protocolVersion),
    encode(true),
    decode(true)
{
    // These are attributes on top of the normal structure that we support
    attriblist << "encode" << "decode" << "file" << "deffile";
}


ProtocolStructureModule::~ProtocolStructureModule(void)
{
    clear();
}

//! Get the name of the header file that encompasses this structure definition
QString ProtocolStructureModule::getDefinitionFileName(void) const
{
    if(defheader.moduleName().isEmpty())
        return header.fileName();
    else
        return defheader.fileName();
}



//! Get the path of the header file that encompasses this structure definition
QString ProtocolStructureModule::getDefinitionFilePath(void) const
{
    if(defheader.moduleName().isEmpty())
        return header.filePath();
    else
        return defheader.filePath();
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
    encode = !ProtocolParser::isFieldClear(ProtocolParser::getAttribute("encode", map));
    decode = !ProtocolParser::isFieldClear(ProtocolParser::getAttribute("decode", map));

    // Warnings for users
    issueWarnings(map);

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
        header.write(" * \\brief " + header.fileName() + " defines the interface for the " + typeName + " structure of the " + support.protoName + " protocol stack\n");

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

    // Add other includes specific to this structure
    parser->outputIncludes(getHierarchicalName(), *structfile, e);

    // Include directives that may be needed for our children
    QStringList list;
    for(int i = 0; i < encodables.length(); i++)
        encodables[i]->getIncludeDirectives(list);
    structfile->writeIncludeDirectives(list);

    // White space is good
    structfile->makeLineSeparator();

    // Create the structure definition in the header.
    // This includes any sub-structures as well
    structfile->write(getStructureDeclaration(true));

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

    // The functions to encoding and ecoding
    createStructureFunctions();

    // White space is good
    header.makeLineSeparator();

    // The encoded size of this structure as a macro that others can access
    if((encode != false) || (decode != false))
    {
        header.write("#define getMinLengthOf" + typeName + "() ");
        if(encodedLength.minEncodedLength.isEmpty())
            header.write("(0)\n");
        else
            header.write("(" + encodedLength.minEncodedLength + ")\n");

        // White space is good
        header.makeLineSeparator();
    }

    // Write to disk
    header.flush();

    if(encode || decode)
        source.flush();
    else
        source.clear();

    // This may be one of the files above, in which case this will do nothing
    structfile->flush();

}// ProtocolStructureModule::parse


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
        if(encodables[i]->isPrimitive())
            continue;

        if(encode)
        {
            source.makeLineSeparator();
            source.write(encodables[i]->getPrototypeEncodeString(support.bigendian));
            source.makeLineSeparator();
            source.write(encodables[i]->getFunctionEncodeString(support.bigendian));
        }

        if(decode)
        {
            source.makeLineSeparator();
            source.write(encodables[i]->getPrototypeDecodeString(support.bigendian));
            source.makeLineSeparator();
            source.write(encodables[i]->getFunctionDecodeString(support.bigendian));
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

    header.makeLineSeparator();
    source.makeLineSeparator();

}// ProtocolStructureModule::createTopLevelStructureFunctions
