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
    needsEncodeIterator(false),
    needsDecodeIterator(false),
    defaults(false),
    hidden(false),
    attriblist()
{
    // List of attributes understood by ProtocolStructure
    attriblist << "name" << "array" << "variableArray" << "dependsOn" << "comment" << "hidden";

}


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
    needsEncodeIterator = false;
    needsDecodeIterator = false;
    defaults = false;
    hidden = false;

}// ProtocolStructure::clear


/*!
 * Parse the DOM data for this structure
 */
void ProtocolStructure::parse(void)
{
    QDomNamedNodeMap map = e.attributes();

    // All the attribute we care about
    name = ProtocolParser::getAttribute("name", map);
    array = ProtocolParser::getAttribute("array", map);
    variableArray = ProtocolParser::getAttribute("variableArray", map);
    dependsOn = ProtocolParser::getAttribute("dependsOn", map);
    comment = ProtocolParser::reflowComment(ProtocolParser::getAttribute("comment", map));
    hidden = ProtocolParser::isFieldSet(ProtocolParser::getAttribute("hidden", map));

    if(name.isEmpty())
        name = "_unknown";

    // Look for any other attributes that we don't recognize
    // Look for any other attributes that we don't recognize
    for(int i = 0; i < map.count(); i++)
    {
        QDomAttr attr = map.item(i).toAttr();
        if(attr.isNull())
            continue;

        if((attriblist.contains(attr.name(), Qt::CaseInsensitive) == false) && (support.disableunrecognized == false))
            std::cout << "Unrecognized attribute of structure: " << name.toStdString() << " : " << attr.name().toStdString() << std::endl;
    }

    // for now the typename is derived from the name
    typeName = prefix + name + "_t";

    // We can't have a variable array length without an array
    if(array.isEmpty() && !variableArray.isEmpty())
    {
        std::cout << name.toStdString() << ": must specify array length to specify variable array length" << std::endl;
        variableArray.clear();
    }


    if(!dependsOn.isEmpty() && !variableArray.isEmpty())
    {
        std::cout << name.toStdString() << ": variable length arrays cannot also use dependsOn" << std::endl;
        dependsOn.clear();
    }

    // Get any enumerations
    parseEnumerations(e);

    // At this point a structure cannot be default, null, or reserved.
    parseChildren(e);

    // Sum the length of all the children
    EncodedLength length;
    for(int i = 0; i < encodables.length(); i++)
    {
        length.addToLength(encodables.at(i)->encodedLength);
    }

    // Account for array, variable array, and depends on
    encodedLength.clear();
    encodedLength.addToLength(length, array, !variableArray.isEmpty(), !dependsOn.isEmpty());

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
    Encodable* prevEncodable = NULL;

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

                    if(encodable->usesEncodeIterator())
                        needsEncodeIterator = true;

                    if(encodable->usesDecodeIterator())
                        needsDecodeIterator = true;

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
                        needsDecodeIterator = needsEncodeIterator = true;
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

                // If this is a bitfield, assume it terminates the bitfield group until we learn otherwise
                if(encodable->isBitfield())
                    encodable->setTerminatesBitfield(true);

                // We need to know when the bitfields end
                if(prevEncodable != NULL)
                {
                    if(prevEncodable->isBitfield())
                    {
                        if(encodable->isBitfield())
                        {
                            // previous is not the terminator
                            prevEncodable->setTerminatesBitfield(false);

                            encodable->setStartingBitCount(prevEncodable->getEndingBitCount());
                        }

                    }// if the previous was a bitfield

                }// if we have a previous encodable

                // Remember who our previous encodable was
                prevEncodable = encodable;

            }// if is encoded

            // Remember this encodable
            encodables.push_back(encodable);

        }// if the child was an encodable

    }// for all children

}// ProtocolStructure::parseChildren


/*!
 * Get the number of encoded fields. This is not the same as the length of the
 * encodables list, because some or all of them could be isNotEncoded()
 * \return the number of encodables in the packet.
 */
int ProtocolStructure::getNumberOfEncodes(void) const
{
    int numEncodes = 0;
    for(int i = 0; i < encodables.length(); i++)
    {
        // If its not encoded at all, then it does not count
        if(encodables.at(i)->isNotEncoded())
            continue;
        else
            numEncodes++;
    }

    return numEncodes;
}


/*!
 * Get the number of encoded fields. This is not the same as the length of the
 * encodables list, because some or all of them could be isNotEncoded() or isConstant()
 * \return the number of encodables in the packet set by the user.
 */
int ProtocolStructure::getNumberOfEncodeParameters(void) const
{
    int numEncodes = 0;
    for(int i = 0; i < encodables.length(); i++)
    {
        // If its not encoded at all, or its encoded as a constant, then it does not count
        if(encodables.at(i)->isNotEncoded() || encodables.at(i)->isConstant())
            continue;
        else
            numEncodes++;
    }

    return numEncodes;
}


/*!
 * Get the number of decoded fields whose value is written into memory. This is
 * not the same as the length of the encodables list, because some or all of
 * them could be isNotEncoded(), isNotInMemory()
 * \return the number of values decoded from the packet.
 */
int ProtocolStructure::getNumberOfDecodeParameters(void) const
{
    // Its is possible that all of our decodes are in fact not set by the user
    int numDecodes = 0;
    for(int i = 0; i < encodables.length(); i++)
    {
        // If its not encoded at all, or its not in memory (hence not decoded) it does not count
        if(encodables.at(i)->isNotEncoded() || encodables.at(i)->isNotInMemory())
            continue;
        else
            numDecodes++;
    }

    return numDecodes;
}


/*!
 * Get the number of fields in memory. This can be different than the number of decodes or encodes.
 * \return the number of user set encodables that appear in the packet.
 */
int ProtocolStructure::getNumberInMemory(void) const
{
    int num = 0;
    for(int i = 0; i < encodables.length(); i++)
    {
        if(encodables.at(i)->isNotInMemory())
            continue;
        else
            num++;
    }

    return num;
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

    // Output enumerations specific to this structure
    for(int i = 0; i < enumList.size(); i++)
    {
        output += enumList.at(i)->getOutput();
        ProtocolFile::makeLineSeparator(output);
    }

    if(getNumberInMemory() > 0)
    {        
        // We don't generate the structure if there is only one element, whats
        // the point? Unless the the caller tells us to always create it
        if((getNumberInMemory() > 1) || alwaysCreate)
        {
            // Declare our childrens structures first
            for(int i = 0; i < encodables.length(); i++)
            {
                if(!encodables[i]->isPrimitive())
                {
                    output += encodables[i]->getStructureDeclaration(true);
                    ProtocolFile::makeLineSeparator(output);
                }

            }// for all children

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
 * \param includeChildren should be true to output the children's prototypes.
 * \return The string including the comments and prototypes with linefeeds and semicolons
 */
QString ProtocolStructure::getPrototypeEncodeString(bool isBigEndian, bool includeChildren) const
{
    QString output;

    if(includeChildren)
    {
        for(int i = 0; i < encodables.length(); i++)
        {
            if(encodables.at(i)->isPrimitive())
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += encodables.at(i)->getPrototypeEncodeString(isBigEndian, includeChildren);
        }
        ProtocolFile::makeLineSeparator(output);
    }

    // My encoding and decoding prototypes in the header file
    output += "//! Encode a " + typeName + " structure into a byte array\n";
    if(getNumberOfEncodeParameters() > 0)
        output += "void encode" + typeName + "(uint8_t* data, int* bytecount, const " + typeName + "* user);\n";
    else
        output += "void encode" + typeName + "(uint8_t* data, int* bytecount);\n";

    return output;

}// ProtocolStructure::getPrototypeEncodeString


/*!
 * Return the string that gives the function used to encode this structure.
 * The encoding is to a simple byte array.
 * \param isBigEndian should be true for big endian encoding.
 * \param includeChildren should be true to output the children's functions.
 * \return The string including the comments and code with linefeeds and semicolons
 */
QString ProtocolStructure::getFunctionEncodeString(bool isBigEndian, bool includeChildren) const
{
    QString output;

    if(includeChildren)
    {
        for(int i = 0; i < encodables.length(); i++)
        {
            if(encodables.at(i)->isPrimitive())
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += encodables.at(i)->getFunctionEncodeString(isBigEndian, includeChildren);
        }
        ProtocolFile::makeLineSeparator(output);
    }

    int numEncodes = getNumberOfEncodeParameters();

    // My encoding function
    output += "/*!\n";
    output += " * \\brief Encode a " + typeName + " structure into a byte array\n";
    output += " *\n";
    output += ProtocolParser::outputLongComment(" *", comment) + "\n";
    output += " * \\param data points to the byte array to add encoded data to\n";
    output += " * \\param bytecount points to the starting location in the byte array, and will be incremented by the number of encoded bytes.\n";
    if(numEncodes > 0)
        output += " * \\param user is the data to encode in the byte array\n";
    output += " */\n";
    if(numEncodes > 0)
        output += "void encode" + typeName + "(uint8_t* data, int* bytecount, const " + typeName + "* user)\n";
    else
        output += "void encode" + typeName + "(uint8_t* data, int* bytecount)\n";
    output += "{\n";

    output += "    int byteindex = *bytecount;\n";

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
    output += "    *bytecount = byteindex;\n";
    output += "\n";
    output += "}// encode" + typeName + "\n";

    return output;

}// ProtocolStructure::getFunctionEncodeString


/*!
 * Return the string that gives the prototype of the functions used to decode
 * the structure. The encoding is to a simple byte array.
 * \param isBigEndian should be true for big endian encoding.
 * \param includeChildren should be true to output the children's prototypes.
 * \return The string including the comments and prototypes with linefeeds and semicolons
 */
QString ProtocolStructure::getPrototypeDecodeString(bool isBigEndian, bool includeChildren) const
{
    QString output;

    if(includeChildren)
    {
        for(int i = 0; i < encodables.length(); i++)
        {
            if(encodables.at(i)->isPrimitive())
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += encodables.at(i)->getPrototypeDecodeString(isBigEndian);
        }
        ProtocolFile::makeLineSeparator(output);
    }

    output += "//! Decode a " + typeName + " structure from a byte array\n";

    if(getNumberOfDecodeParameters() > 0)
        output += "int decode" + typeName + "(const uint8_t* data, int* bytecount, " + typeName + "* user);\n";
    else
        output += "int decode" + typeName + "(const uint8_t* data, int* bytecount);\n";

    return output;

}// ProtocolStructure::getPrototypeDecodeString


/*!
 * Return the string that gives the function used to decode this structure.
 * The decoding is from a simple byte array.
 * \param isBigEndian should be true for big endian decoding.
 * \param includeChildren should be true to output the children's functions.
 * \return The string including the comments and code with linefeeds and semicolons
 */
QString ProtocolStructure::getFunctionDecodeString(bool isBigEndian, bool includeChildren) const
{
    QString output;

    if(includeChildren)
    {
        for(int i = 0; i < encodables.length(); i++)
        {
            if(encodables.at(i)->isPrimitive())
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += encodables.at(i)->getFunctionDecodeString(isBigEndian);
        }
        ProtocolFile::makeLineSeparator(output);
    }

    int numDecodes = getNumberOfDecodeParameters();

    output += "/*!\n";
    output += " * \\brief Decode a " + typeName + " structure from a byte array\n";
    output += " *\n";
    output += ProtocolParser::outputLongComment(" *", comment) + "\n";
    output += " * \\param data points to the byte array to decoded data from\n";
    output += " * \\param bytecount points to the starting location in the byte array, and will be incremented by the number of bytes decoded\n";
    if(numDecodes > 0)
        output += " * \\param user is the data to decode from the byte array\n";
    output += " * \\return 1 if the data are decoded, else 0. If 0 is returned bytecount will not be updated.\n";
    output += " */\n";
    if(numDecodes > 0)
        output += "int decode" + typeName + "(const uint8_t* data, int* bytecount, " + typeName + "* user)\n";
    else
        output += "int decode" + typeName + "(const uint8_t* data, int* bytecount)\n";

    output += "{\n";

    output += "    int byteindex = *bytecount;\n";

    if(bitfields)
        output += "    int bitcount = 0;\n";

    if(needsDecodeIterator)
        output += "    int i = 0;\n";

    int bitcount = 0;
    for(int i = 0; i < encodables.length(); i++)
    {
        ProtocolFile::makeLineSeparator(output);
        output += encodables[i]->getDecodeString(isBigEndian, &bitcount, true);
    }

    ProtocolFile::makeLineSeparator(output);
    output += "    *bytecount = byteindex;\n\n";
    output += "    return 1;\n";
    output += "\n";
    output += "}// decode" + typeName + "\n";

    return output;

}// ProtocolStructure::getFunctionDecodeString


/*!
 * Return the string that is used to encode this structure
 * \param isBigEndian should be true for big endian encoding.
 * \param encLength is appended for length information of this field.
 * \param bitcount points to the running count of bits in a bitfields and should persist between calls
 * \param isStructureMember is true if this encodable is accessed by structure pointer
 * \return the string to add to the source to encode this structure
 */
QString ProtocolStructure::getEncodeString(bool isBigEndian, int* bitcount, bool isStructureMember) const
{
    QString output;
    QString access;
    QString spacing = "    ";

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

        output += spacing + "    encode" + typeName + "(data, &byteindex, " + access + ");\n";
    }
    else
    {
        if(isStructureMember)
            access = "&user->" + name;
        else
            access = name;  // in this case, name is already pointer, so we don't need "&"

        output += spacing + "encode" + typeName + "(data, &byteindex, " + access + ");\n";
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

        output += spacing + "{\n";

        if(isStructureMember)
            access = "&user->" + name + "[i]";
        else
            access = "&" + name + "[i]";

        output += spacing + "    if(decode" + typeName + "(data, &byteindex, " + access + ") == 0)\n";
        output += spacing + "        return 0;\n";
        output += spacing + "}\n";
    }
    else
    {
        if(isStructureMember)
            access = "&user->" + name;
        else
            access = name;  // in this case, name is already pointer, so we don't need "&"

        output += spacing + "if(decode" + typeName + "(data, &byteindex, " + access + ") == 0)\n";
        output += spacing + "    return 0;\n";
    }

    if(!dependsOn.isEmpty())
        output += "    }\n";

    return output;
}


/*!
 * Get details needed to produce documentation for this encodable.
 * \param parentName is the name of the parent which will be pre-pended to the name of this encodable
 * \param startByte is the starting byte location of this encodable, which will be updated for the following encodable.
 * \param bytes is appended for the byte range of this encodable.
 * \param names is appended for the name of this encodable.
 * \param encodings is appended for the encoding of this encodable.
 * \param repeats is appended for the array information of this encodable.
 * \param comments is appended for the description of this encodable.
 */
void ProtocolStructure::getDocumentationDetails(QList<int>& outline, QString& startByte, QStringList& bytes, QStringList& names, QStringList& encodings, QStringList& repeats, QStringList& comments) const
{
    QString description;

    QString maxEncodedLength = encodedLength.maxEncodedLength;

    // See if we can replace any enumeration names with values
    ProtocolParser::replaceEnumerationNameWithValue(maxEncodedLength);

    // The byte after this one
    QString nextStartByte = EncodedLength::collapseLengthString(startByte + "+" + maxEncodedLength);

    // The length data
    if(maxEncodedLength.isEmpty() || (maxEncodedLength.compare("1") == 0))
        bytes.append(startByte);
    else
    {
        QString endByte = EncodedLength::subtractOneFromLengthString(nextStartByte);

        // The range of the data
        bytes.append(startByte + "..." + endByte);
    }

    // The name information
    outline.last() += 1;
    QString outlineString;
    outlineString.setNum(outline.at(0));
    for(int i = 1; i < outline.size(); i++)
        outlineString += "." + QString().setNum(outline.at(i));
    outlineString += ")" + name;
    names.append(outlineString);

    // Encoding is blank for structures
    encodings.append(QString());

    // The repeat/array column
    if(array.isEmpty())
    {
        repeats.append(QString());
    }
    else
    {
        if(variableArray.isEmpty())
            repeats.append(array);
        else
            repeats.append(variableArray + ", up to " + array);
    }

    // The commenting
    description += comment;

    if(!dependsOn.isEmpty())
    {
        if(!description.endsWith('.'))
            description += ".";

        description += " Only included if " + dependsOn + " is non-zero.";
    }

    if(description.isEmpty())
        comments.append(QString());
    else
        comments.append(description);

    // Now go get the sub-encodables
    getSubDocumentationDetails(outline, startByte, bytes, names, encodings, repeats, comments);

    // These two may be the same, but they won't be if this structure is repeated.
    startByte = nextStartByte;

}// ProtocolStructure::getDocumentationDetails


/*!
 * Get details needed to produce documentation for this encodable.
 * \param parentName is the name of the parent which will be pre-pended to the name of this encodable
 * \param startByte is the starting byte location of this encodable, which will be updated for the following encodable.
 * \param bytes is appended for the byte range of this encodable.
 * \param names is appended for the name of this encodable.
 * \param encodings is appended for the encoding of this encodable.
 * \param repeats is appended for the array information of this encodable.
 * \param comments is appended for the description of this encodable.
 */
void ProtocolStructure::getSubDocumentationDetails(QList<int>& outline, QString& startByte, QStringList& bytes, QStringList& names, QStringList& encodings, QStringList& repeats, QStringList& comments) const
{
    outline.append(0);

    // Go get the sub-encodables
    for(int i = 0; i < encodables.length(); i++)
        encodables.at(i)->getDocumentationDetails(outline, startByte, bytes, names, encodings, repeats, comments);

    outline.removeLast();

}// ProtocolStructure::getSubDocumentationDetails

