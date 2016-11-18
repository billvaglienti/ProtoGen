#include "protocolstructuremodule.h"
#include "protocolparser.h"
#include <iostream>

/*!
 * Construct the object that parses structure descriptions
 * \param parse points to the global protocol parser that owns everything
 * \param protocolName is the name of the protocol
 * \param supported gives the supported features of the protocol
 * \param protocolApi is the API string of the protocol
 * \param protocolVersion is the version string of the protocol
 */
ProtocolStructureModule::ProtocolStructureModule(ProtocolParser* parse, const QString& protocolName, ProtocolSupport supported, const QString& protocolApi, const QString& protocolVersion) :
    ProtocolStructure(parse, protocolName, protocolName, supported),
    api(protocolApi),
    version(protocolVersion),
    encode(true),
    decode(true)
{
    // These are attributes on top of the normal structure that we support
    attriblist << "encode" << "decode" << "file";
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
    encode = decode = true;

    // Note that data set during constructor are not changed

}


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
    encode = !ProtocolParser::isFieldClear(ProtocolParser::getAttribute("encode", map));
    decode = !ProtocolParser::isFieldClear(ProtocolParser::getAttribute("decode", map));

    // Look for any other attributes that we don't recognize
    for(int i = 0; i < map.count(); i++)
    {
        QDomAttr attr = map.item(i).toAttr();
        if(attr.isNull())
            continue;

        if((attriblist.contains(attr.name(), Qt::CaseInsensitive) == false) && (support.disableunrecognized == false))
            emitWarning("Unrecognized attribute " + attr.name());
    }

    if(isArray())
    {
        emitWarning("top level structure cannot be an array");
        array.clear();
        variableArray.clear();
        array2d.clear();
        variable2dArray.clear();
    }

    if(!dependsOn.isEmpty())
    {
        emitWarning("dependsOn makes no sense for a top level structure");
        dependsOn.clear();
    }

    // The file directive tells us if we are creating a separate file, or if we are appending an existing one
    if(moduleName.isEmpty())
        moduleName = support.globalFileName;

    if(moduleName.isEmpty())
    {
        // The file names
        header.setModuleNameAndPath(support.prefix + name, support.outputpath);
        source.setModuleNameAndPath(support.prefix + name, support.outputpath);

        // Comment block at the top of the header file
        header.write("/*!\n");
        header.write(" * \\file\n");
        header.write(" * \\brief " + header.fileName() + " defines the interface for the " + typeName + " structure of the " + protoName + " protocol stack\n");

        // A potentially long comment that should be wrapped at 80 characters
        if(!comment.isEmpty())
        {
            header.write(" *\n");
            header.write(ProtocolParser::outputLongComment(" *", comment) + "\n");
        }

        // Finish the top comment block
        header.write(" */\n");
        header.write("\n");

        // Include the protocol top level module
        header.writeIncludeDirective(protoName + "Protocol.h");
    }
    else
    {
        // The file names
        header.setModuleNameAndPath(moduleName, support.outputpath);
        source.setModuleNameAndPath(moduleName, support.outputpath);

        // Two options here: we may be appending an a-priori existing file, or we may be starting one fresh.
        if(!header.isAppending())
        {
            // Comment block at the top of the header file
            header.write("/*!\n");
            header.write(" * \\file\n");
            header.write(" * " + header.fileName() + " is part of the " + protoName + " protocol stack\n");

            // Finish the top comment block
            header.write(" */\n");
            header.write("\n");

            // Include the protocol top level module
            header.writeIncludeDirective(protoName + "Protocol.h");
        }
        else
            header.makeLineSeparator();

    }

    // Add other includes specific to this structure
    parser->outputIncludes(getHierarchicalName(), header, e);

    // Include directives that may be needed for our children
    QStringList list;
    for(int i = 0; i < encodables.length(); i++)
        encodables[i]->getIncludeDirectives(list);
    header.writeIncludeDirectives(list);

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

    // White space is good
    header.makeLineSeparator();

    // Create the structure definition in the header.
    // This includes any sub-structures as well
    header.write(getStructureDeclaration(true));

    // White space is good
    header.makeLineSeparator();

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
    source.flush();

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
