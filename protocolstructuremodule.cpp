#include "protocolstructuremodule.h"
#include "protocolparser.h"
#include <iostream>

/*!
 * Construct the object that parses structure descriptions
 * \param protocolName is the name of the protocol
 * \param protocolPrefix is the prefix string to use
 * \param supported gives the supported features of the protocol
 * \param protocolApi is the API string of the protocol
 * \param protocolVersion is the version string of the protocol
 * \param bigendian should be true to encode multi-byte fields with the most
 *        significant byte first.
 */
ProtocolStructureModule::ProtocolStructureModule(const QString& protocolName, const QString& protocolPrefix, ProtocolSupport supported, const QString& protocolApi, const QString& protocolVersion, bool bigendian) :
    ProtocolStructure(protocolName, protocolPrefix, supported),
    api(protocolApi),
    version(protocolVersion),
    isBigEndian(bigendian),
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
 * \param packet is DOM element that defines the packet
 */
void ProtocolStructureModule::parse(const QDomElement& e)
{
    // Initialize metadata
    clear();

    // Me and all my children, which may themselves be structures
    ProtocolStructure::parse(e);

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
            std::cout << "Unrecognized attribute of top level Structure: " << name.toStdString() << " : " << attr.name().toStdString() << std::endl;
    }

    if(isArray())
    {
        std::cout << name.toStdString() + ": top level structure cannot be an array" << std::endl;
        array.clear();
        variableArray.clear();
    }

    if(!dependsOn.isEmpty())
    {
        std::cout << name.toStdString() << ": dependsOn makes no sense for a top level structure" << std::endl;
        dependsOn.clear();
    }

    // The file directive tells us if we are creating a separate file, or if we are appending an existing one
    if(moduleName.isEmpty())
        moduleName = support.globalFileName;

    if(moduleName.isEmpty())
    {
        // The file names
        header.setModuleName(prefix + name);
        source.setModuleName(prefix + name);

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
        header.setModuleName(moduleName);
        source.setModuleName(moduleName);

        // We may be appending data to an already existing file
        header.prepareToAppend();
        source.prepareToAppend();

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
    ProtocolParser::outputIncludes(header, e);

    // Include directives that may be needed for our children
    for(int i = 0; i < encodables.length(); i++)
        header.writeIncludeDirective(encodables[i]->getIncludeDirective());

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
        header.write("#define getMinLengthOf" + typeName + "() (" + encodedLength.minEncodedLength + ")\n");

        // White space is good
        header.makeLineSeparator();
    }

    // Write to disk
    header.flush();
    source.flush();

    // Make sure these are empty for next time around
    header.clear();
    source.clear();

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
            source.write(encodables[i]->getPrototypeEncodeString(isBigEndian));
            source.makeLineSeparator();
            source.write(encodables[i]->getFunctionEncodeString(isBigEndian));
        }

        if(decode)
        {
            source.makeLineSeparator();
            source.write(encodables[i]->getPrototypeDecodeString(isBigEndian));
            source.makeLineSeparator();
            source.write(encodables[i]->getFunctionDecodeString(isBigEndian));
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
        header.write(getPrototypeEncodeString(isBigEndian, false));
        source.makeLineSeparator();
        source.write(getFunctionEncodeString(isBigEndian, false));
    }

    if(decode)
    {
        header.makeLineSeparator();
        header.write(getPrototypeDecodeString(isBigEndian, false));
        source.makeLineSeparator();
        source.write(getFunctionDecodeString(isBigEndian, false));
    }

    header.makeLineSeparator();
    source.makeLineSeparator();

}// ProtocolStructureModule::createTopLevelStructureFunctions


/*!
 * Output the top level markdown documentation for the this structure and its children
 * \param outline gives the outline number for this heading
 * \return the markdown documentation for this structure and its children
 */
QString ProtocolStructureModule::getTopLevelMarkdown(QString outline) const
{
    /*
    QString output;
    int paragraph = 1;

    output += "## " + outline + ")" + name + "\n";
    output += "\n";

    if(!comment.isEmpty())
    {
        output += comment + "\n";
        output += "\n";
    }

    if(enumList.size() > 0)
    {
        output += "\n";
        output += "### " + outline + "." + QString().setNum(paragraph++) + ") " + name + " enumerations\n";
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
        output += "### " + outline + "." + QString().setNum(paragraph++) + ") " + name + " encoding\n";
        output += "\n";

        QStringList names, encodings, repeats, comments;

        // The column headings
        names.append("Name");
        encodings.append("Encoding");
        repeats.append("Repeat");
        comments.append("Description");

        int nameColumn = 0, encodingColumn = 0, repeatColumn = 0, commentColumn = 0;

        // Get all the details that are going to end up in the table
        for(int i = 0; i < encodables.length(); i++)
        {
            if(encodables.at(i) == NULL)
                continue;

            encodables.at(i)->getDocumentationDetails("", names, encodings, repeats, comments);
        }

        // Figure out the column widths, note that we assume all the lists are the same length
        for(int i = 0; i < names.length(); i++)
        {
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
        output += "[Encoding for structure " + name + "]\n";

        // Table header, notice the column markers lead and follow. We have to do this for merged cells
        output +=  "| ";
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

            output += spacedString(names.at(i), nameColumn);

            // We support the idea that repeats and or encodings could be empty, causing cells to be merged
            if(encodings.at(i).isEmpty() && repeats.at(i).isEmpty())
            {
                spacedString("", encodingColumn + repeatColumn);
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
    */
    return QString();
}
