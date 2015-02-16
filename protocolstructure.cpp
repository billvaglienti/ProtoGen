#include "protocolstructure.h"
#include "protocolparser.h"
#include <QDomNodeList>
#include <QStringList>
#include <iostream>


/*!
 * Construct a protocol structure
 * \param protocolName is the name of the protocol
 * \param protocolPrefix is the type name prefix
 */
ProtocolStructure::ProtocolStructure(const QString& protocolName, const QString& protocolPrefix, ProtocolSupport supported) :
    Encodable(protocolName, protocolPrefix, supported),
    bitfields(false),
    needsIterator(false),
    defaults(false)
{

}

/*!
 * Construct a protocol structure from a DOM field
 * \param protocolName is the name of the protocol
 * \param protocolPrefix is the type name prefix
 * \param field is a DOM field whose data and children define this structure
 */
ProtocolStructure::ProtocolStructure(const QString& protocolName, const QString& protocolPrefix, ProtocolSupport supported, const QDomElement& field) :
    Encodable(protocolName, protocolPrefix, supported),
    bitfields(false),
    needsIterator(false),
    defaults(false)
{
    parse(field);

}// ProtocolStructure::ProtocolStructure


/*!
 * Destroy the protocol structure and all its children
 */
ProtocolStructure::~ProtocolStructure()
{
    clear();

}// ProtocolStructure::~ProtocolStructure


/*!
 * Reset all data to defaults
 */
void ProtocolStructure::clear(void)
{
    Encodable::clear();

    // Delete any encodable objects in our list
    for(int i = 0; i < encodables.length(); i++)
    {
        if(encodables[i])
        {
            delete encodables[i];
            encodables[i] = NULL;
        }
    }

    // Empty the list itself
    encodables.clear();

    // Objects in this list are owned by others, we just clear it, don't delete the objects
    enumList.clear();

    // The rest of the metadata
    bitfields = false;
    needsIterator = false;
    defaults = false;

}// ProtocolStructure::clear


/*!
 * Parse the DOM data for this structure
 * \param field is the DOM data for this structure
 * \param typePrefix is prefix information for the type data
 */
void ProtocolStructure::parse(const QDomElement& field)
{
    name = field.attribute("name");

    if(name.isEmpty())
        name = "_unknown";

    // for now the typename is derived from the name
    typeName = prefix + name + "_t";

    // The data that describe this structure
    array = field.attribute("array");

    // Is the array variable length
    variableArray = field.attribute("variableArray");

    // We can't have a variable array length without an array
    if(array.isEmpty() && !variableArray.isEmpty())
    {
        std::cout << name.toStdString() << ": must specify array length to specify variable array length" << std::endl;
        variableArray.clear();
    }

    // String for depending on something else
    dependsOn = field.attribute("dependsOn");

    if(!dependsOn.isEmpty() && !variableArray.isEmpty())
    {
        std::cout << name.toStdString() << ": variable length arrays cannot also use dependsOn" << std::endl;
        dependsOn.clear();
    }

    // Any user comment about this
    comment = ProtocolParser::getComment(field);

    // Get any enumerations
    parseEnumerations(field);

    // At this point a structure cannot be default, null, or reserved.
    parseChildren(field);

}// ProtocolStructure::parse


/*!
 * Parse and output all enumerations which are direct children of a DomNode
 * \param node is parent node
 */
void ProtocolStructure::parseEnumerations(const QDomNode& node)
{
    // Build the top level enumerations
    QList<QDomNode>list = ProtocolParser::childElementsByTagName(node, "Enum");

    for(int i = 0; i < list.size(); i++)
    {
        enumList.append(ProtocolParser::parseEnumeration(list.at(i).toElement()));

    }// for all my enum tags

}// ProtocolStructure::parseEnumerations


/*!
 * Parse the DOM data for the children of this structure
 * \param field is the DOM data for this structure
 */
void ProtocolStructure::parseChildren(const QDomElement& field)
{
    // All the direct children, which may themselves be structures or primitive fields
    QDomNodeList children = field.childNodes();

    // Make encodables out of them, and add to our list
    for (int i = 0; i < children.size(); i++)
    {
        Encodable* encodable = generateEncodable(protoName, prefix, support, children.at(i).toElement());
        if(encodable != NULL)
        {
            // If the encodable is null, then none of the metadata
            // matters, its not going to end up in the output
            if(!encodable->isNotEncoded())
            {
                if(encodable->isPrimitive())
                {
                    // Track our metadata
                    if(encodable->usesBitfields())
                        bitfields = true;

                    if(encodable->usesIterator())
                        needsIterator = true;

                    if(encodable->usesDefaults())
                        defaults = true;
                    else if(defaults)
                    {
                        // Check defaults. If a previous field was defaulted but this
                        // field is not, then we have to terminate the previous default,
                        // only the last fields can have defaults
                        for(int j = 0; j < encodables.size(); j++)
                        {
                            encodables[j]->clearDefaults();
                            std::cout << name.toStdString() << ": " << encodables[j]->name.toStdString() << ": default value ignored, field is followed by non-default" << std::endl;
                        }

                        defaults = false;
                    }

                }// if this encodable is a primitive
                else
                {
                    // Structures can be arrays as well.
                    if(encodable->isArray())
                        needsIterator = true;
                }


                // Handle the variable array case. We have to make sure that the referenced variable exists
                if(!encodable->variableArray.isEmpty())
                {
                    int prev;
                    for(prev = 0; prev < encodables.size(); prev++)
                    {
                       Encodable* prevEncodable = encodables.at(prev);
                       if(prevEncodable == NULL)
                           continue;

                       // It has to be a named variable that is both in memory and encoded
                       if(prevEncodable->isNotEncoded() || prevEncodable->isNotInMemory())
                           continue;

                       // It has to be a primitive, and it cannot be an array itself
                       if(!prevEncodable->isPrimitive() && !prevEncodable->isArray())
                           continue;

                       // Now check to see if this previously defined encodable is our variable
                       if(prevEncodable->name == encodable->variableArray)
                           break;
                    }

                    if(prev >= encodables.size())
                    {
                       std::cout << name.toStdString() << ": " << encodable->name.toStdString() << ": variable length array ignored, failed to find length variable" << std::endl;
                       encodable->variableArray.clear();
                    }

                }// if this is a variable length array

                // Handle the dependsOn case. We have to make sure that the referenced variable exists
                if(!encodable->dependsOn.isEmpty())
                {
                    if(encodable->isBitfield())
                    {
                        std::cout << name.toStdString() << ": " << encodable->name.toStdString() << ": bitfields cannot use dependsOn" << std::endl;
                        encodable->dependsOn.clear();
                    }
                    else
                    {
                        int prev;
                        for(prev = 0; prev < encodables.size(); prev++)
                        {
                           Encodable* prevEncodable = encodables.at(prev);
                           if(prevEncodable == NULL)
                               continue;

                           // It has to be a named variable that is both in memory and encoded
                           if(prevEncodable->isNotEncoded() || prevEncodable->isNotInMemory())
                               continue;

                           // It has to be a primitive, and it cannot be an array itself
                           if(!prevEncodable->isPrimitive() && !prevEncodable->isArray())
                               continue;

                           // Now check to see if this previously defined encodable is our variable
                           if(prevEncodable->name == encodable->dependsOn)
                               break;
                        }

                        if(prev >= encodables.size())
                        {
                           std::cout << name.toStdString() << ": " << encodable->name.toStdString() << ": dependsOn ignored, failed to find dependsOn variable" << std::endl;
                           encodable->dependsOn.clear();
                        }
                    }

                }// if this field depends on another

            }// if not null

            // Remember this encodable
            encodables.push_back(encodable);

        }// if the child was an encodable

    }// for all children

}// ProtocolStructure::parseChildren


/*!
 * Get the number of encoded fields. This is not the same as the length of the
 * encodables list, because some or all of them could be isNotEncoded()
 * \return the number of encodables that appear in the packet.
 */
int ProtocolStructure::getNumberOfEncodes(void) const
{
    // Its is possible that all of our encodes are in fact not encoded
    int numEncodes = 0;
    for(int i = 0; i < encodables.length(); i++)
    {
        if(encodables.at(i)->isNotEncoded())
            continue;
        else
            numEncodes++;
    }

    return numEncodes;
}


/*!
 * Get the number of encoded fields whose value is set by the user. This is
 * not the same as the length of the encodables list, because some or all of
 * them could be isNotEncoded(), isNotInMemory(), or isConstant()
 * \return the number of user set encodables that appear in the packet.
 */
int ProtocolStructure::getNumberOfNonConstEncodes(void) const
{
    // Its is possible that all of our encodes are in fact not set by the user
    int numEncodes = 0;
    for(int i = 0; i < encodables.length(); i++)
    {
        if(encodables.at(i)->isNotEncoded() || encodables.at(i)->isNotInMemory() || encodables.at(i)->isConstant())
            continue;
        else
            numEncodes++;
    }

    return numEncodes;
}


/*!
 * Return the string used to declare this encodable as part of a structure.
 * This includes the spacing, typename, name, semicolon, comment, and linefeed
 * \return the declaration string for this encodable
 */
QString ProtocolStructure::getDeclaration(void) const
{
    QString output = "    " + typeName + " " + name;

    if(array.isEmpty())
        output += ";";
    else
        output += "[" + array + "];";

    if(!comment.isEmpty())
        output += " //!< " + comment;

    output += "\n";

    return output;

}// ProtocolStructure::getDeclaration


/*!
 * Get the declaration that goes in the header which declares this structure
 * and all its children.
 * \param alwaysCreate should be true to force creation of the structure, even if there is only one member
 * \return the string that represents the structure declaration
 */
QString ProtocolStructure::getStructureDeclaration(bool alwaysCreate) const
{
    QString output;
    QString structure;

    if(encodables.length() > 0)
    {
        // Declare our childrens structures first
        for(int i = 0; i < encodables.length(); i++)
        {
            if(!encodables[i]->isPrimitive())
            {
                output += encodables[i]->getStructureDeclaration(true);
                output += "\n";
            }

        }// for all children

        // We don't generate the structure if there is only one element, whats
        // the point? Unless the the caller tells us to always create it
        if((encodables.length() > 1) || alwaysCreate)
        {
            // The top level comment for the structure definition
            if(!comment.isEmpty())
            {
                output += "/*!\n";
                output += ProtocolParser::outputLongComment(" *", comment) + "\n";
                output += " */\n";
            }

            // The opening to the structure
            output += "typedef struct\n";
            output += "{\n";
            for(int i = 0; i < encodables.length(); i++)
                structure += encodables[i]->getDeclaration();

            // Make structures pretty with alignment goodness
            output += alignStructureData(structure);

            // Close out the structure
            output += "}" + typeName + ";\n";
        }

    }// if we have some data to encode

    return output;

}// ProtocolStructure::getStructureDeclaration


/*!
 * Make a structure output be prettily aligned
 * \param structure is the input structure string
 * \return a new string that is the aligned structure string
 */
QString ProtocolStructure::alignStructureData(const QString& structure) const
{
    int i, index;

    // The strings as a list separated by line feeds
    QStringList list = structure.split("\n", QString::SkipEmptyParts);

    // The space separates the typeName from the name,
    // but skip the indent spaces
    int max = 0;
    for(i = 0; i < list.size(); i++)
    {
        int index = list[i].indexOf(" ", 4);
        if(index > max)
            max = index;
    }

    for(int i = 0; i < list.size(); i++)
    {
        // Insert spaces until we have reached the max
        index = list[i].indexOf(" ", 4);
        for(; index < max; index++)
            list[i].insert(index, " ");
    }

    // The first semicolon we find separates the name from the comment
    max = 0;
    for(i = 0; i < list.size(); i++)
    {
        // we want the character after the semicolon
        index = list[i].indexOf(";") + 1;
        if(index > max)
            max = index;
    }

    for(i = 0; i < list.size(); i++)
    {
        // Insert spaces until we have reached the max
        index = list[i].indexOf(";") + 1;
        for(; index < max; index++)
            list[i].insert(index, " ");
    }

    // Re-assemble the output, put the line feeds back on
    QString output;
    for(i = 0; i < list.size(); i++)
        output += list[i] + "\n";

    return output;

}// ProtocolStructure::alignStructureData


/*!
 * Return the string that gives the prototype of the functions used to encode
 * the structure. The encoding is to a simple byte array.
 * \param isBigEndian should be true for big endian encoding.
 * \param encLength is appended for length information of this field.
 * \return The string including the comments and prototypes with linefeeds and semicolons
 */
QString ProtocolStructure::getPrototypeEncodeString(bool isBigEndian, EncodedLength* encLength)
{
    QString output;

    // Start with no length
    encodedLength.clear();

    if(encodables.length() > 0)
    {
        // The encoding prototypes of my children, if any. I want these to appear before me, because I'm going to call them
        for(int i = 0; i < encodables.length(); i++)
        {
            if(!encodables[i]->isPrimitive())
            {
                output += encodables[i]->getPrototypeEncodeString(isBigEndian, &encodedLength);
                ProtocolFile::makeLineSeparator(output);
            }

        }

        ProtocolFile::makeLineSeparator(output);

        // My encoding prototype and function
        output += "/*!\n";
        output += " * \\brief Encode a " + typeName + " structure into a byte array\n";
        output += " *\n";
        output += ProtocolParser::outputLongComment(" *", comment) + "\n";
        output += " * \\param data points to the byte array to add encoded data to\n";
        output += " * \\param byteindex is the starting location in the byte array\n";
        output += " * \\param user is the data to encode in the byte array\n";
        output += " * \\return the location for the next data to be encoded in the byte array\n";
        output += " */\n";
        output += "static int encode" + typeName + "(uint8_t* data, int byteCount, const " + typeName + "* user);\n";
        output += "\n";
        output += "int encode" + typeName + "(uint8_t* data, int byteindex, const " + typeName + "* user)\n";
        output += "{\n";

        if(bitfields)
            output += "    int bitcount = 0;\n";

        if(needsIterator)
            output += "    int i = 0;\n";

        int bitcount = 0;
        for(int i = 0; i < encodables.length(); i++)
        {
            ProtocolFile::makeLineSeparator(output);
            output += encodables[i]->getEncodeString(isBigEndian, encodedLength, &bitcount, true);
        }

        if(bitcount != 0)
        {
            ProtocolFile::makeLineSeparator(output);
            output += Encodable::getCloseBitfieldString(&bitcount, &encodedLength);
        }

        ProtocolFile::makeLineSeparator(output);
        output += "    return byteindex;\n";
        output += "}\n";

    }

    // Add our length to our parent's length
    EncodedLength::add(encLength, encodedLength);

    return output;

}// ProtocolStructure::getPrototypeEncodeString


/*!
 * Return the string that gives the prototype of the functions used to decode
 * the structure. The encoding is to a simple byte array.
 * \param isBigEndian should be true for big endian encoding.
 * \return The string including the comments and prototypes with linefeeds and semicolons
 */
QString ProtocolStructure::getPrototypeDecodeString(bool isBigEndian) const
{
    QString output;

    if(encodables.length() > 0)
    {
        // The decoding prototypes of my children, if any. I want these to appear before me, because I'm going to call them
        for(int i = 0; i < encodables.length(); i++)
        {
            if(!encodables[i]->isPrimitive())
            {
                output += encodables[i]->getPrototypeDecodeString(isBigEndian);
                ProtocolFile::makeLineSeparator(output);
            }

        }

        ProtocolFile::makeLineSeparator(output);

        // My decoding prototype and function
        output += "/*!\n";
        output += " * \\brief Decode a " + typeName + " structure from a byte array\n";
        output += " *\n";
        output += ProtocolParser::outputLongComment(" *", comment) + "\n";
        output += " * \\param data points to the byte array to decoded data from\n";
        output += " * \\param byteindex is the starting location in the byte array\n";
        output += " * \\param user is the data to decode from the byte array\n";
        output += " * \\return the location for the next data to be decoded in the byte array\n";
        output += " */\n";
        output += "static int decode" + typeName + "(const uint8_t* data, int byteCount, " + typeName + "* user);\n";
        output += "\n";
        output += "int decode" + typeName + "(const uint8_t* data, int byteindex, " + typeName + "* user)\n";
        output += "{\n";

        if(bitfields)
            output += "    int bitcount = 0;\n";

        if(needsIterator)
            output += "    int i = 0;\n";

        int bitcount = 0;
        for(int i = 0; i < encodables.length(); i++)
        {
            ProtocolFile::makeLineSeparator(output);
            output += encodables[i]->getDecodeString(isBigEndian, &bitcount, true);
        }

        if(bitcount != 0)
        {
            ProtocolFile::makeLineSeparator(output);
            output += Encodable::getCloseBitfieldString(&bitcount);
        }

        ProtocolFile::makeLineSeparator(output);
        output += "    return byteindex;\n";
        output += "}\n";

    }

    return output;

}// ProtocolStructure::getPrototypeDecodeString


/*!
 * Return the string that is used to encode this structure
 * \param isBigEndian should be true for big endian encoding.
 * \param encLength is appended for length information of this field.
 * \param bitcount points to the running count of bits in a bitfields and should persist between calls
 * \param isStructureMember is true if this encodable is accessed by structure pointer
 * \return the string to add to the source to encode this structure
 */
QString ProtocolStructure::getEncodeString(bool isBigEndian, EncodedLength& encLength, int* bitcount, bool isStructureMember) const
{
    QString output;
    QString access;
    QString spacing = "    ";

    // Close out any bitfields
    if(*bitcount != 0)
        output = getCloseBitfieldString(bitcount, &encLength);

    // A line between fields
    ProtocolFile::makeLineSeparator(output);

    if(!comment.isEmpty())
        output += spacing + "// " + comment + "\n";

    if(isStructureMember)
        access = "&user->" + name;
    else
        access = name;

    if(!dependsOn.isEmpty())
    {
        if(isStructureMember)
            output += spacing + "if(user->" + dependsOn + ")\n";
        else
            output += spacing + "if(" + dependsOn + ")\n";
        output += spacing + "{\n";
        spacing += "    ";
    }

    encLength.addToLength(encodedLength, array, !variableArray.isEmpty(), !dependsOn.isEmpty());

    if(isArray())
    {
        if(variableArray.isEmpty())
        {
            output += spacing + "for(i = 0; i < " + array + "; i++)\n";
        }
        else
        {
            // Variable length array
            if(isStructureMember)
                output += spacing + "for(i = 0; i < (int)user->" + variableArray + " && i < " + array + "; i++)\n";
            else
                output += spacing + "for(i = 0; i < (int)(" + variableArray + ") && i < " + array + "; i++)\n";
        }

        if(isStructureMember)
            access = "&user->" + name + "[i]";
        else
            access = "&" + name + "[i]";

        output += spacing + "    byteindex = encode" + typeName + "(data, byteindex, " + access + ");\n";
    }
    else
    {
        if(isStructureMember)
            access = "&user->" + name;
        else
            access = name;  // in this case, name is already pointer, so we don't need "&"

        output += spacing + "byteindex = encode" + typeName + "(data, byteindex, " + access + ");\n";
    }

    if(!dependsOn.isEmpty())
        output += "    }\n";

    return output;
}


/*!
 * Return the string that is used to decode this structure
 * \param isBigEndian should be true for big endian encoding.
 * \param bitcount points to the running count of bits in a bitfields and should persist between calls
 * \param isStructureMember is true if this encodable is accessed by structure pointer
 * \return the string to add to the source to decode this structure
 */
QString ProtocolStructure::getDecodeString(bool isBigEndian, int* bitcount, bool isStructureMember, bool defaultEnabled) const
{
    QString output;
    QString access;
    QString spacing = "    ";

    // Close out any bitfields
    if(*bitcount != 0)
        output = getCloseBitfieldString(bitcount);

    // A line between fields
    ProtocolFile::makeLineSeparator(output);

    if(!comment.isEmpty())
        output += spacing + "// " + comment + "\n";

    if(!dependsOn.isEmpty())
    {
        if(isStructureMember)
            output += spacing + "if(user->" + dependsOn + ")\n";
        else
            output += spacing + "if(" + dependsOn + ")\n";
        output += spacing + "{\n";
        spacing += "    ";
    }

    if(isArray())
    {
        if(variableArray.isEmpty())
            output += spacing + "for(i = 0; i < " + array + "; i++)\n";
        else
        {
            // Variable length array
            if(isStructureMember)
                output += spacing + "for(i = 0; i < (int)user->" + variableArray + " && i < " + array + "; i++)\n";
            else
                output += spacing + "for(i = 0; i < (int)(*" + variableArray + ") && i < " + array + "; i++)\n";
        }

        if(isStructureMember)
            access = "&user->" + name + "[i]";
        else
            access = "&" + name + "[i]";

        output += spacing + "    byteindex = decode" + typeName + "(data, byteindex, " + access + ");\n";
    }
    else
    {
        if(isStructureMember)
            access = "&user->" + name;
        else
            access = name;  // in this case, name is already pointer, so we don't need "&"

        output += spacing + "byteindex = decode" + typeName + "(data, byteindex, " + access + ");\n";
    }

    if(!dependsOn.isEmpty())
        output += "    }\n";

    return output;
}


//! Return markdown formatted information about this encodables fields
QString ProtocolStructure::getMarkdown(QString indent) const
{
    QString output = indent;

    // bulleted list with name in code
    output += "* `" + name + "`";

    if(!comment.isEmpty())
        output += " :   " + comment + ".";

    if(isArray())
    {
        if(variableArray.isEmpty())
            output += "  Repeat `" + array + "` times.";
        else
            output += "  Repeat `" + variableArray + "` times, up to `" + array + "` times.";
    }

    if(!dependsOn.isEmpty())
        output += "  Only included if `" + dependsOn + "` is non-zero.";

    output += "\n";

    // indent to the next level
    indent += "    ";

    for(int i = 0; i < encodables.length(); i++)
    {
        output += encodables[i]->getMarkdown(indent);
        output += "\n";
    }

    return output;
}


