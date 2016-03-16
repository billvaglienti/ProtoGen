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
    isBigEndian(bigendian)
{

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
    QString moduleName = e.attribute("file");

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

    // Output enumerations specific to this structure
    for(int i = 0; i < enumList.size(); i++)
    {
        header.makeLineSeparator();
        header.write(enumList.at(i)->getOutput());
    }

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
    header.write("#define getMinLengthOf" + typeName + "() (" + encodedLength.minEncodedLength + ")\n");

    // White space is good
    header.makeLineSeparator();

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
    // The encoding and decoding prototypes of my children, if any. I want these to appear before me, because I'm going to call them
    for(int i = 0; i < encodables.length(); i++)
    {
        if(!encodables[i]->isPrimitive())
        {
            source.makeLineSeparator();
            source.write(encodables[i]->getPrototypeEncodeString(isBigEndian));
            source.makeLineSeparator();
            source.write(encodables[i]->getPrototypeDecodeString(isBigEndian));
        }

    }

    // Write these to the source file
    source.makeLineSeparator();

    // Now build the top level function
    createTopLevelStructureFunctions();

}// ProtocolStructureModule::createStructureFunctions


/*!
 * Write data to the source and header files to encode and decode this structure
 * but not its children. This will add to the length strings, but not reset them
 */
void ProtocolStructureModule::createTopLevelStructureFunctions(void)
{
    QString output;

    if(getNumberOfEncodes() <= 0)
        return;

    int numNonConstEncodes = getNumberOfNonConstEncodes();

    header.makeLineSeparator();

    // My encoding and decoding prototypes in the header file
    output += "//! Encode a " + typeName + " structure into a byte array\n";
    if(numNonConstEncodes > 0)
        output += "int encode" + typeName + "(uint8_t* data, int byteCount, const " + typeName + "* user);\n";
    else
        output += "int encode" + typeName + "(uint8_t* data, int byteCount);\n";
    output += "\n";
    output += "//! Decode a " + typeName + " structure from a byte array\n";
    output += "int decode" + typeName + "(const uint8_t* data, int byteCount, " + typeName + "* user);\n";
    header.write(output);
    output.clear();

    source.makeLineSeparator();

    // My encoding function
    output += "/*!\n";
    output += " * \\brief Encode a " + typeName + " structure into a byte array\n";
    output += " *\n";
    output += ProtocolParser::outputLongComment(" *", comment) + "\n";
    output += " * \\param data points to the byte array to add encoded data to\n";
    output += " * \\param byteindex is the starting location in the byte array\n";
    if(numNonConstEncodes > 0)
        output += " * \\param user is the data to encode in the byte array\n";
    output += " * \\return the location for the next data to be encoded in the byte array\n";
    output += " */\n";
    if(numNonConstEncodes > 0)
        output += "int encode" + typeName + "(uint8_t* data, int byteindex, const " + typeName + "* user)\n";
    else
        output += "int encode" + typeName + "(uint8_t* data, int byteindex)\n";
    output += "{\n";

    if(bitfields)
        output += "    int bitcount = 0;\n";

    if(needsEncodeIterator)
        output += "    int i = 0;\n";

    int bitcount = 0;
    for(int i = 0; i < encodables.length(); i++)
    {
        ProtocolFile::makeLineSeparator(output);
        output += encodables[i]->getEncodeString(isBigEndian, &bitcount, true);
    }

    ProtocolFile::makeLineSeparator(output);
    output += "    return byteindex;\n";
    output += "}\n";


    // My decoding function
    output += "\n";
    output += "/*!\n";
    output += " * \\brief Decode a " + typeName + " structure from a byte array\n";
    output += " *\n";
    output += ProtocolParser::outputLongComment(" *", comment) + "\n";
    output += " * \\param data points to the byte array to decoded data from\n";
    output += " * \\param byteindex is the starting location in the byte array\n";
    output += " * \\param user is the data to decode from the byte array\n";
    output += " * \\return the location for the next data to be decoded in the byte array\n";
    output += " */\n";
    output += "int decode" + typeName + "(const uint8_t* data, int byteindex, " + typeName + "* user)\n";
    output += "{\n";

    if(bitfields)
        output += "    int bitcount = 0;\n";

    if(needsDecodeIterator)
        output += "    int i = 0;\n";

    bitcount = 0;
    for(int i = 0; i < encodables.length(); i++)
    {
        ProtocolFile::makeLineSeparator(output);
        output += encodables[i]->getDecodeString(isBigEndian, &bitcount, true);
    }

    ProtocolFile::makeLineSeparator(output);
    output += "    return byteindex;\n";
    output += "}\n";

    // Write to the source file
    source.write(output);
    output.clear();

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
