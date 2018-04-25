#include "protocolstructure.h"
#include "protocolparser.h"
#include "protocolfield.h"
#include <QDomNodeList>
#include <QStringList>
#include <iostream>

/*!
 * Construct a protocol structure
 * \param parse points to the global protocol parser that owns everything
 * \param Parent is the hierarchical name of the object that owns this object.
 * \param support are the protocol support details
 */
ProtocolStructure::ProtocolStructure(ProtocolParser* parse, QString Parent, ProtocolSupport supported) :
    Encodable(parse, Parent, supported),
    numbitfieldgroupbytes(0),
    bitfields(false),
    usestempencodebitfields(false),
    usestempencodelongbitfields(false),
    usestempdecodebitfields(false),
    usestempdecodelongbitfields(false),
    needsEncodeIterator(false),
    needsDecodeIterator(false),
    needsInitIterator(false),
    needsVerifyIterator(false),
    needs2ndEncodeIterator(false),
    needs2ndDecodeIterator(false),
    needs2ndInitIterator(false),
    needs2ndVerifyIterator(false),
    defaults(false),
    hidden(false),
    hasinit(false),
    hasverify(false),
    attriblist()
{
    // List of attributes understood by ProtocolStructure
    attriblist << "name" << "title" << "array" << "variableArray" << "array2d" << "variable2dArray" << "dependsOn" << "comment" << "hidden";

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
    numbitfieldgroupbytes = 0;
    bitfields = false;
    usestempencodebitfields = false;
    usestempencodelongbitfields = false;
    usestempdecodebitfields = false;
    usestempdecodelongbitfields = false;
    needsEncodeIterator = false;
    needsDecodeIterator = false;
    needsInitIterator = false;
    needsVerifyIterator = false;
    needs2ndEncodeIterator = false;
    needs2ndDecodeIterator = false;
    needs2ndInitIterator = false;
    needs2ndVerifyIterator = false;
    defaults = false;
    hidden = false;
    hasinit = false;
    hasverify = false;
    structName.clear();

}// ProtocolStructure::clear


/*!
 * Parse the DOM data for this structure
 */
void ProtocolStructure::parse(void)
{
    QDomNamedNodeMap map = e.attributes();

    // All the attribute we care about
    name = ProtocolParser::getAttribute("name", map);
    title = ProtocolParser::getAttribute("title", map);
    array = ProtocolParser::getAttribute("array", map);
    variableArray = ProtocolParser::getAttribute("variableArray", map);
    dependsOn = ProtocolParser::getAttribute("dependsOn", map);
    comment = ProtocolParser::reflowComment(ProtocolParser::getAttribute("comment", map));
    hidden = ProtocolParser::isFieldSet("hidden", map);

    if(name.isEmpty())
        name = "_unknown";

    if(title.isEmpty())
        title = name;

    testAndWarnAttributes(map, attriblist);

    // for now the typename is derived from the name
    structName = typeName = support.prefix + name + "_t";

    // We can't have a variable array length without an array
    if(array.isEmpty() && !variableArray.isEmpty())
    {
        emitWarning("must specify array length to specify variable array length");
        variableArray.clear();
    }


    if(!dependsOn.isEmpty() && !variableArray.isEmpty())
    {
        emitWarning("variable length arrays cannot also use dependsOn");
        dependsOn.clear();
    }

    // Check to make sure we did not step on any keywords
    checkAgainstKeywords();

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
        enumList.append(parser->parseEnumeration(getHierarchicalName(), list.at(i).toElement()));

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
        Encodable* encodable = generateEncodable(parser, getHierarchicalName(), support, children.at(i).toElement());
        if(encodable != NULL)
        {            
            // If the encodable is null, then none of the metadata
            // matters, its not going to end up in the output
            if(!encodable->isNotEncoded())
            {
                ProtocolField* field = dynamic_cast<ProtocolField*>(encodable);

                if(field != NULL)
                {
                    // Let the new encodable know about the preceding one
                    field->setPreviousEncodable(prevEncodable);

                    if(field->overridesPreviousEncodable())
                    {
                        int prev;
                        for(prev = 0; prev < encodables.size(); prev++)
                        {
                            if(field->getOverriddenTypeData(dynamic_cast<ProtocolField*>(encodables.at(prev))))
                                break;
                        }

                        if(prev >= encodables.size())
                        {
                            field->emitWarning("override failed, could not find previous field");
                            delete encodable;
                            continue;
                        }

                    }// if encodable does override operation

                    // Track our metadata
                    if(field->usesBitfields())
                    {
                        field->getBitfieldGroupNumBytes(&numbitfieldgroupbytes);
                        bitfields = true;

                        if(field->usesEncodeTempBitfield())
                            usestempencodebitfields = true;

                        if(field->usesEncodeTempLongBitfield())
                            usestempencodelongbitfields = true;

                        if(field->usesDecodeTempBitfield())
                            usestempdecodebitfields = true;

                        if(field->usesDecodeTempLongBitfield())
                            usestempdecodelongbitfields = true;
                    }

                    if(field->usesEncodeIterator())
                        needsEncodeIterator = true;

                    if(field->usesDecodeIterator())
                        needsDecodeIterator = true;

                    if(field->usesInitIterator())
                        needsInitIterator = true;

                    if(field->usesVerifyIterator())
                        needsVerifyIterator = true;

                    if(field->uses2ndEncodeIterator())
                        needs2ndEncodeIterator = true;

                    if(field->uses2ndDecodeIterator())
                        needs2ndDecodeIterator = true;

                    if(field->uses2ndInitIterator())
                        needs2ndInitIterator = true;

                    if(field->uses2ndVerifyIterator())
                        needs2ndVerifyIterator = true;

                    if(field->usesDefaults())
                        defaults = true;
                    else if(defaults && field->invalidatesPreviousDefault())
                    {
                        // Check defaults. If a previous field was defaulted but this
                        // field is not, then we have to terminate the previous default,
                        // only the last fields can have defaults
                        for(int j = 0; j < encodables.size(); j++)
                        {
                            if(encodables[j]->usesDefaults())
                            {
                                encodables[j]->clearDefaults();
                                encodables[j]->emitWarning("default value ignored, field is followed by non-default");
                            }
                        }

                        defaults = false;
                    }

                }// if this encodable is a field
                else
                {
                    // Structures can be arrays as well.
                    if(encodable->isArray())
                    {
                        needsDecodeIterator = needsEncodeIterator = true;
                        needsInitIterator = encodable->hasInit();
                        needsVerifyIterator = encodable->hasVerify();
                    }

                    if(encodable->is2dArray())
                    {
                        needs2ndDecodeIterator = needs2ndEncodeIterator = true;
                        needs2ndInitIterator = encodable->hasInit();
                        needs2ndVerifyIterator = encodable->hasVerify();
                    }

                }// else if this encodable is not a field


                // Handle the variable array case. We have to make sure that the referenced variable exists
                if(!encodable->variableArray.isEmpty())
                {
                    int prev;
                    for(prev = 0; prev < encodables.size(); prev++)
                    {
                       Encodable* previous = encodables.at(prev);
                       if(previous == NULL)
                           continue;

                       // It has to be a named variable that is both in memory and encoded
                       if(previous->isNotEncoded() || previous->isNotInMemory())
                           continue;

                       // It has to be a primitive, and it cannot be an array itself
                       if(!previous->isPrimitive() && !previous->isArray())
                           continue;

                       // Now check to see if this previously defined encodable is our variable
                       if(previous->name == encodable->variableArray)
                           break;
                    }

                    if(prev >= encodables.size())
                    {
                       encodable->emitWarning("variable length array ignored, failed to find length variable");
                       encodable->variableArray.clear();
                    }

                }// if this is a variable length array

                // Handle the variable 2d array case. We have to make sure that the referenced variable exists
                if(!encodable->variable2dArray.isEmpty())
                {
                    int prev;
                    for(prev = 0; prev < encodables.size(); prev++)
                    {
                       Encodable* previous = encodables.at(prev);
                       if(previous == NULL)
                           continue;

                       // It has to be a named variable that is both in memory and encoded
                       if(previous->isNotEncoded() || previous->isNotInMemory())
                           continue;

                       // It has to be a primitive, and it cannot be an array itself
                       if(!previous->isPrimitive() && !previous->isArray())
                           continue;

                       // Now check to see if this previously defined encodable is our variable
                       if(previous->name == encodable->variable2dArray)
                           break;
                    }

                    if(prev >= encodables.size())
                    {
                       encodable->emitWarning("variable 2d length array ignored, failed to find 2d length variable");
                       encodable->variable2dArray.clear();
                    }

                }// if this is a 2d variable length array

                // Handle the dependsOn case. We have to make sure that the referenced variable exists
                if(!encodable->dependsOn.isEmpty())
                {
                    if(encodable->isBitfield())
                    {
                        encodable->emitWarning("bitfields cannot use dependsOn");
                        encodable->dependsOn.clear();
                    }
                    else
                    {
                        int prev;
                        for(prev = 0; prev < encodables.size(); prev++)
                        {
                           Encodable* previous = encodables.at(prev);
                           if(previous == NULL)
                               continue;

                           // It has to be a named variable that is both in memory and encoded
                           if(previous->isNotEncoded() || previous->isNotInMemory())
                               continue;

                           // It has to be a primitive, and it cannot be an array itself
                           if(!previous->isPrimitive() && !previous->isArray())
                               continue;

                           // Now check to see if this previously defined encodable is our variable
                           if(previous->name == encodable->dependsOn)
                               break;
                        }

                        if(prev >= encodables.size())
                        {
                           encodable->emitWarning("dependsOn ignored, failed to find dependsOn variable");
                           encodable->dependsOn.clear();
                        }
                    }

                }// if this field depends on another

                // If our child has init or verify capabilities
                // we have to inherit those as well
                if(encodable->hasInit())
                    hasinit = true;

                if(encodable->hasVerify())
                    hasverify = true;

                // We can only determine bitfield group numBytes after
                // we have given the encodable a look at its preceding members
                if(encodable->isPrimitive() && encodable->usesBitfields())
                    encodable->getBitfieldGroupNumBytes(&numbitfieldgroupbytes);

                // Remember who our previous encodable was
                prevEncodable = encodable;

            }// if is encoded

            // Remember this encodable
            encodables.push_back(encodable);

        }// if the child was an encodable

    }// for all children

}// ProtocolStructure::parseChildren


//! Get the maximum number of temporary bytes needed for a bitfield group of our children
void ProtocolStructure::getBitfieldGroupNumBytes(int* num) const
{
    if(numbitfieldgroupbytes > (*num))
        (*num) = numbitfieldgroupbytes;
}


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
 * Append the include directives needed for this encodable. Mostly this is empty,
 * but for external structures or enumerations we need to bring in the include file
 * \param list is appended with any directives this encodable requires.
 */
void ProtocolStructure::getIncludeDirectives(QStringList& list) const
{
    // Includes that our encodable members may need
    for(int i = 0; i < encodables.length(); i++)
        encodables.at(i)->getIncludeDirectives(list);

    // Array sizes could be enumerations that need an include directive
    if(!array.isEmpty())
    {
        QString include = parser->lookUpIncludeName(array);
        if(!include.isEmpty())
            list.append(include);
    }

    // Array sizes could be enumerations that need an include directive
    if(!array2d.isEmpty())
    {
        QString include = parser->lookUpIncludeName(array2d);
        if(!include.isEmpty())
            list.append(include);
    }

    list.removeDuplicates();
}


/*!
 * Return the include directives needed for this encodable's init and verify functions
 * \param list is appended with any directives this encodable requires.
 */
void ProtocolStructure::getInitAndVerifyIncludeDirectives(QStringList& list) const
{
    // Includes that our encodable members may need
    for(int i = 0; i < encodables.length(); i++)
        encodables.at(i)->getInitAndVerifyIncludeDirectives(list);

    list.removeDuplicates();
}


/*!
 * Return the string used to declare this encodable as part of a structure.
 * This includes the spacing, typename, name, semicolon, comment, and linefeed
 * \return the declaration string for this encodable
 */
QString ProtocolStructure::getDeclaration(void) const
{
    QString output = TAB_IN + "" + typeName + " " + name;

    if(array.isEmpty())
        output += ";";
    else if(array2d.isEmpty())
        output += "[" + array + "];";
    else
        output += "[" + array + "][" + array2d + "]";

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
 * Get the prototype of the function that encodes this structure. The prototype
 * is different depending on the number of encodable parameters. The prototype
 * does not include the semicolon or line terminator
 * \return the prototype of the encode function.
 */
QString ProtocolStructure::getFunctionEncodePrototype() const
{
    QString output;

    if(getNumberOfEncodeParameters() > 0)
    {
        output = VOID_ENCODE + typeName + "(uint8_t* _pg_data, int* _pg_bytecount, const " + structName + "* _pg_user)";
    }
    else
    {
        output = VOID_ENCODE + typeName + "(uint8_t* _pg_data, int* _pg_bytecount)";
    }

    return output;
}


/*!
 * Return the string that gives the prototype of the functions used to encode
 * the structure, and all child structures. The encoding is to a simple byte array.
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

    output += getFunctionEncodePrototype() + ";\n";

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
    output += " * \\param _pg_data points to the byte array to add encoded data to\n";
    output += " * \\param _pg_bytecount points to the starting location in the byte array, and will be incremented by the number of encoded bytes.\n";
    if(numEncodes > 0)
        output += " * \\param _pg_user is the data to encode in the byte array\n";
    output += " */\n";

    output += getFunctionEncodePrototype() + "\n";

    output += "{\n";

    output += TAB_IN + "int _pg_byteindex = *_pg_bytecount;\n";

    if(usestempencodebitfields)
        output += TAB_IN + "unsigned int _pg_tempbitfield = 0;\n";

    if(usestempencodelongbitfields)
        output += TAB_IN + "uint64_t _pg_templongbitfield = 0;\n";

    if(numbitfieldgroupbytes > 0)
    {
        output += TAB_IN + "int _pg_bitfieldindex = 0;\n";
        output += TAB_IN + "uint8_t _pg_bitfieldbytes[" + QString::number(numbitfieldgroupbytes) + "];\n";
    }

    if(needsEncodeIterator)
        output += TAB_IN + "unsigned _pg_i = 0;\n";

    if(needs2ndEncodeIterator)
        output += TAB_IN + "unsigned _pg_j = 0;\n";

    int bitcount = 0;
    for(int i = 0; i < encodables.length(); i++)
    {
        ProtocolFile::makeLineSeparator(output);
        output += encodables[i]->getEncodeString(isBigEndian, &bitcount, true);
    }

    ProtocolFile::makeLineSeparator(output);
    output += TAB_IN + "*_pg_bytecount = _pg_byteindex;\n";
    output += "\n";
    output += "}// encode" + typeName + "\n";

    return output;

}// ProtocolStructure::getFunctionEncodeString


QString ProtocolStructure::getFunctionDecodePrototype() const
{
    QString output;

    if(getNumberOfDecodeParameters() > 0)
        output = "int decode" + typeName + "(const uint8_t* _pg_data, int* _pg_bytecount, " + structName + "* _pg_user)";
    else
        output = "int decode" + typeName + "(const uint8_t* _pg_data, int* _pg_bytecount)";

    return output;
}


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

    output += getFunctionDecodePrototype() + ";\n";

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
    output += " * \\param _pg_data points to the byte array to decoded data from\n";
    output += " * \\param _pg_bytecount points to the starting location in the byte array, and will be incremented by the number of bytes decoded\n";
    if(numDecodes > 0)
        output += " * \\param _pg_user is the data to decode from the byte array\n";
    output += " * \\return 1 if the data are decoded, else 0. If 0 is returned _pg_bytecount will not be updated.\n";
    output += " */\n";

    output += getFunctionDecodePrototype() + "\n";

    output += "{\n";

    output += TAB_IN + "int _pg_byteindex = *_pg_bytecount;\n";

    if(usestempdecodebitfields)
        output += TAB_IN + "unsigned int _pg_tempbitfield = 0;\n";

    if(usestempdecodelongbitfields)
        output += TAB_IN + "uint64_t _pg_templongbitfield = 0;\n";

    if(numbitfieldgroupbytes > 0)
    {
        output += TAB_IN + "int _pg_bitfieldindex = 0;\n";
        output += TAB_IN + "uint8_t _pg_bitfieldbytes[" + QString::number(numbitfieldgroupbytes) + "];\n";
    }

    if(needsDecodeIterator)
        output += TAB_IN + "unsigned _pg_i = 0;\n";

    if(needs2ndDecodeIterator)
        output += TAB_IN + "unsigned _pg_j = 0;\n";

    int bitcount = 0;
    for(int i = 0; i < encodables.length(); i++)
    {
        ProtocolFile::makeLineSeparator(output);
        output += encodables[i]->getDecodeString(isBigEndian, &bitcount, true);
    }

    ProtocolFile::makeLineSeparator(output);
    output += TAB_IN + "*_pg_bytecount = _pg_byteindex;\n\n";
    output += TAB_IN + "return 1;\n";
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
    Q_UNUSED(isBigEndian);
    Q_UNUSED(bitcount);

    QString output;
    QString access;
    QString spacing = TAB_IN + "";

    if(!comment.isEmpty())
        output += spacing + "// " + comment + "\n";

    if(isStructureMember)
        access = "&_pg_user->" + name;
    else
        access = name;

    if(!dependsOn.isEmpty())
    {
        if(isStructureMember)
            output += spacing + "if(_pg_user->" + dependsOn + ")\n";
        else
            output += spacing + "if(" + dependsOn + ")\n";
        output += spacing + "{\n";
        spacing += TAB_IN + "";
    }

    if(isArray())
    {
        if(variableArray.isEmpty())
        {
            output += spacing + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";
        }
        else
        {
            // Variable length array
            if(isStructureMember)
                output += spacing + "for(_pg_i = 0; _pg_i < (unsigned)_pg_user->" + variableArray + " && _pg_i < " + array + "; _pg_i++)\n";
            else
                output += spacing + "for(_pg_i = 0; _pg_i < (unsigned)(" + variableArray + ") && _pg_i < " + array + "; _pg_i++)\n";
        }


        if(is2dArray())
        {
            spacing += TAB_IN + "";

            if(variable2dArray.isEmpty())
                output += spacing + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";
            else
            {
                // Variable length array
                if(isStructureMember)
                    output += spacing + "for(_pg_j = 0; _pg_j < (unsigned)_pg_user->" + variable2dArray + " && _pg_j < " + array2d + "; _pg_j++)\n";
                else
                    output += spacing + "for(_pg_j = 0; _pg_j < (unsigned)(" + variable2dArray + ") && _pg_j < " + array2d + "; _pg_j++)\n";
            }

            if(isStructureMember)
                access = "&_pg_user->" + name + "[_pg_i][_pg_j]";
            else
                access = "&" + name + "[_pg_i][_pg_j]";

        }// if 2d array
        else
        {
            if(isStructureMember)
                access = "&_pg_user->" + name + "[_pg_i]";
            else
                access = "&" + name + "[_pg_i]";
        }

        output += spacing + TAB_IN + "encode" + typeName + "(_pg_data, &_pg_byteindex, " + access + ");\n";
    }
    else
    {
        if(isStructureMember)
            access = "&_pg_user->" + name;
        else
            access = name;  // in this case, name is already pointer, so we don't need "&"

        output += spacing + "encode" + typeName + "(_pg_data, &_pg_byteindex, " + access + ");\n";
    }

    if(!dependsOn.isEmpty())
        output += TAB_IN + "}\n";

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
    Q_UNUSED(isBigEndian);
    Q_UNUSED(bitcount);
    Q_UNUSED(defaultEnabled);

    QString output;
    QString access;
    QString spacing = TAB_IN;

    // A line between fields
    ProtocolFile::makeLineSeparator(output);

    if(!comment.isEmpty())
        output += spacing + "// " + comment + "\n";

    if(!dependsOn.isEmpty())
    {
        if(isStructureMember)
            output += spacing + "if(_pg_user->" + dependsOn + ")\n";
        else
            output += spacing + "if(" + dependsOn + ")\n";
        output += spacing + "{\n";
        spacing += TAB_IN + "";
    }

    if(isArray())
    {
        if(variableArray.isEmpty())
            output += spacing + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";
        else
        {
            // Variable length array
            if(isStructureMember)
                output += spacing + "for(_pg_i = 0; _pg_i < (unsigned)_pg_user->" + variableArray + " && _pg_i < " + array + "; _pg_i++)\n";
            else
                output += spacing + "for(_pg_i = 0; _pg_i < (unsigned)(*" + variableArray + ") && _pg_i < " + array + "; _pg_i++)\n";
        }

        output += spacing + "{\n";

        if(is2dArray())
        {
            if(variable2dArray.isEmpty())
                output += spacing + TAB_IN + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";
            else
            {
                // Variable length array
                if(isStructureMember)
                    output += spacing + TAB_IN + "for(_pg_j = 0; _pg_j < (unsigned)_pg_user->" + variable2dArray + " && _pg_j < " + array2d + "; _pg_j++)\n";
                else
                    output += spacing + TAB_IN + "for(_pg_j = 0; _pg_j < (unsigned)(*" + variable2dArray + ") && _pg_j < " + array2d + "; _pg_j++)\n";
            }

            output += spacing + TAB_IN + "{\n";

            if(isStructureMember)
                access = "&_pg_user->" + name + "[_pg_i][_pg_j]";
            else
                access = "&" + name + "[_pg_i][_pg_j]";

            output += spacing + TAB_IN + "    if(decode" + typeName + "(_pg_data, &_pg_byteindex, " + access + ") == 0)\n";
            output += spacing + TAB_IN + "        return 0;\n";
            output += spacing + TAB_IN + "}\n";
            output += spacing + "}\n";
        }
        else
        {
            if(isStructureMember)
                access = "&_pg_user->" + name + "[_pg_i]";
            else
                access = "&" + name + "[_pg_i]";

            output += spacing + TAB_IN + "if(decode" + typeName + "(_pg_data, &_pg_byteindex, " + access + ") == 0)\n";
            output += spacing + TAB_IN + "    return 0;\n";
            output += spacing + "}\n";
        }
    }
    else
    {
        if(isStructureMember)
            access = "&_pg_user->" + name;
        else
            access = name;  // in this case, name is already pointer, so we don't need "&"

        output += spacing + "if(decode" + typeName + "(_pg_data, &_pg_byteindex, " + access + ") == 0)\n";
        output += spacing + TAB_IN + "return 0;\n";
    }

    if(!dependsOn.isEmpty())
        output += TAB_IN + "}\n";

    return output;
}


/*!
 * Return the string that gives the prototypes of the functions used to set
 * this structure to initial values.
 * \param includeChildren should be true to output the children's functions.
 * \return The string including the comments and code with linefeeds and semicolons
 */
QString ProtocolStructure::getSetToInitialValueFunctionPrototype(bool includeChildren) const
{
    QString output;

    if(!hasinit)
        return output;

    // Go get any children structures set to initial functions
    if(includeChildren)
    {
        for(int i = 0; i < encodables.length(); i++)
        {
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(!structure)
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += structure->getVerifyFunctionPrototype(includeChildren);
        }
        ProtocolFile::makeLineSeparator(output);
    }

    // My set to initial values function
    output += "//! Set a " + typeName + " structure to initial values\n";
    output += "void init" + typeName + "(" + structName + "* _pg_user);\n";

    return output;

}// ProtocolStructure::getSetToInitialValueFunctionPrototype


/*!
 * Return the string that gives the function used to this structure to initial
 * values. This is NOT the call that sets this structure to initial values, this
 * is the actual code that is in that call.
 * \param includeChildren should be true to output the children's functions.
 * \return The string including the comments and code with linefeeds and semicolons
 */
QString ProtocolStructure::getSetToInitialValueFunctionString(bool includeChildren) const
{
    QString output;

    if(!hasinit)
        return output;

    // Go get any children structures set to initial functions
    if(includeChildren)
    {
        for(int i = 0; i < encodables.length(); i++)
        {
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(!structure)
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += structure->getSetToInitialValueFunctionString(includeChildren);
        }
        ProtocolFile::makeLineSeparator(output);
    }

    // My set to initial values function
    output += "/*!\n";
    output += " * \\brief Set a " + typeName + " structure to initial values.\n";
    output += " *\n";
    output += " * Set a " + typeName + " structure to initial values. Not all fields are set,\n";
    output += " * only those which the protocol specifies.\n";
    output += " * \\param _pg_user is the structure whose data are set to initial values\n";
    output += " */\n";
    output += "void init" + typeName + "(" + structName + "* _pg_user)\n";
    output += "{\n";

    if(needsInitIterator)
        output += TAB_IN + "int _pg_i = 0;\n";

    if(needs2ndInitIterator)
        output += TAB_IN + "int _pg_j = 0;\n";

    for(int i = 0; i < encodables.length(); i++)
    {
        ProtocolFile::makeLineSeparator(output);
        output += encodables[i]->getSetInitialValueString(true);
    }

    ProtocolFile::makeLineSeparator(output);
    output += "}// init" + typeName + "\n";

    return output;

}// ProtocolStructure::getFunctionSetToInitialValueString


/*!
 * Get the code which sets this structure member to initial values
 * \return the code to put in the source file
 */
QString ProtocolStructure::getSetInitialValueString(bool isStructureMember) const
{
    QString output;
    QString access;

    if(!hasinit)
        return output;

    if(!comment.isEmpty())
        output += TAB_IN + "// " + comment + "\n";

    if(isArray())
    {
        QString spacing;
        output += TAB_IN + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";

        if(isStructureMember)
            access = "&_pg_user->" + name + "[_pg_i]";
        else
            access = "&" + name + "[_pg_i]";

        if(is2dArray())
        {
            access += "[_pg_j]";
            spacing += TAB_IN;
            output += TAB_IN + TAB_IN + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";

        }// if 2D array of structures

        output += TAB_IN + TAB_IN + spacing + "init" + typeName + "(" + access + ");\n";

    }// if array of structures
    else
    {
        if(isStructureMember)
            access = "&_pg_user->" + name;
        else
            access = name;  // in this case, name is already pointer, so we don't need "&"

        output += TAB_IN + "init" + typeName + "(" + access + ");\n";

    }// else if simple structure, no array

    return output;

}// ProtocolStructure::getSetInitialValueString


/*!
 * Return the string that gives the prototypes of the functions used to verify
 * the data in this.
 * \param includeChildren should be true to output the children's functions.
 * \return The string including the comments and code with linefeeds and semicolons
 */
QString ProtocolStructure::getVerifyFunctionPrototype(bool includeChildren) const
{
    QString output;

    if(!hasverify)
        return output;

    // Go get any children structures set to initial functions
    if(includeChildren)
    {
        for(int i = 0; i < encodables.length(); i++)
        {
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(!structure)
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += structure->getVerifyFunctionPrototype(includeChildren);
        }
        ProtocolFile::makeLineSeparator(output);
    }

    // My set to initial values function
    output += "//! Verify a " + typeName + " structure has acceptable values\n";
    output += "int verify" + typeName + "(" + structName + "* _pg_user);\n";

    return output;

}// ProtocolStructure::getVerifyFunctionPrototype


/*!
 * Return the string that gives the function used to verify the data in this
 * structure. This is NOT the call that verifies this structure, this
 * is the actual code that is in that call.
 * \param includeChildren should be true to output the children's functions.
 * \return The string including the comments and code with linefeeds and semicolons
 */
QString ProtocolStructure::getVerifyFunctionString(bool includeChildren) const
{
    QString output;

    if(!hasverify)
        return output;

    // Go get any children structures verify functions
    if(includeChildren)
    {
        for(int i = 0; i < encodables.length(); i++)
        {
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(!structure)
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += structure->getVerifyFunctionString(includeChildren);
        }
        ProtocolFile::makeLineSeparator(output);
    }

    // My set to initial values function
    output += "/*!\n";
    output += " * \\brief Verify a " + typeName + " structure has acceptable values.\n";
    output += " *\n";
    output += " * Verify a " + typeName + " structure has acceptable values. Not all fields are\n";
    output += " * verified, only those which the protocol specifies. Fields which are outside\n";
    output += " * the allowable range are changed to the maximum or minimum allowable value. \n";
    output += " * \\param _pg_user is the structure whose data are verified\n";
    output += " * \\return 1 if all verifiable data where valid, else 0 if data had to be corrected\n";
    output += " */\n";
    output += "int verify" + typeName + "(" + structName + "* _pg_user)\n";
    output += "{\n";
    output += TAB_IN + "int _pg_good = 1;\n";

    if(needsVerifyIterator)
        output += TAB_IN + "int _pg_i = 0;\n";

    if(needs2ndVerifyIterator)
        output += TAB_IN + "int _pg_j = 0;\n";

    for(int i = 0; i < encodables.length(); i++)
    {
        ProtocolFile::makeLineSeparator(output);
        output += encodables[i]->getVerifyString(true);
    }

    ProtocolFile::makeLineSeparator(output);
    output += TAB_IN + "return _pg_good;\n";
    output += "\n";
    output += "}// verify" + typeName + "\n";

    return output;

}// ProtocolStructure::getVerifyFunctionString


/*!
 * Get the code which sets this structure member to initial values
 * \return the code to put in the source file
 */
QString ProtocolStructure::getVerifyString(bool isStructureMember) const
{
    QString output;
    QString access;

    if(!hasverify)
        return output;

    if(!comment.isEmpty())
        output += TAB_IN + "// " + comment + "\n";

    if(isArray())
    {
        QString spacing;
        output += TAB_IN + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";

        if(isStructureMember)
            access = "&_pg_user->" + name + "[_pg_i]";
        else
            access = "&" + name + "[_pg_i]";

        if(is2dArray())
        {
            access += "[_pg_j]";
            spacing += TAB_IN;
            output += TAB_IN + TAB_IN + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";

        }// if 2D array of structures

        output += TAB_IN + TAB_IN + spacing + "if(!verify" + typeName + "(" + access + "))\n";
        output += TAB_IN + TAB_IN + spacing + TAB_IN + "_pg_good = 0;\n";

    }// if array of structures
    else
    {
        if(isStructureMember)
            access = "&_pg_user->" + name;
        else
            access = name;  // in this case, name is already pointer, so we don't need "&"

        output += TAB_IN + "if(!verify" + typeName + "(" + access + "))\n";
        output += TAB_IN + TAB_IN + "_pg_good = 0;\n";

    }// else if simple structure, no array

    return output;

}// ProtocolStructure::getVerifyString


/*!
 * Return the string that gives the prototype of the function used to compare this structure
 * \param includeChildren should be true to include the function prototypes of the children structures of this structure
 * \return the function prototype string, which may be empty
 */
QString ProtocolStructure::getComparisonFunctionPrototype(bool includeChildren) const
{
    QString output;

    // We must parameters that we decode to do a comparison
    if(getNumberOfDecodeParameters() == 0)
        return output;

    // Go get any children structures compare functions
    if(includeChildren)
    {
        for(int i = 0; i < encodables.length(); i++)
        {
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(!structure)
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += structure->getComparisonFunctionPrototype(includeChildren);
        }
        ProtocolFile::makeLineSeparator(output);
    }

    // My comparison function
    output += "//! Compare two " + typeName + " structures and generate a report\n";
    output += "QString compare" + typeName + "(const QString& prename, const " + structName + "* _pg_user1, const " + structName + "* _pg_user2);\n";

    return output;
}


/*!
 * Return the string that gives the function used to compare this structure
 * \param includeChildren should be true to include the function prototypes of the children structures of this structure
 * \return the function string, which may be empty
 */
QString ProtocolStructure::getComparisonFunctionString(bool includeChildren) const
{
    QString output;

    // We must parameters that we decode to do a comparison
    if(getNumberOfDecodeParameters() == 0)
        return output;

    // Go get any childrens structure compare functions
    if(includeChildren)
    {
        for(int i = 0; i < encodables.length(); i++)
        {
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(!structure)
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += structure->getComparisonFunctionString(includeChildren);
        }
        ProtocolFile::makeLineSeparator(output);
    }

    // My compare function
    output += "/*!\n";
    output += " * Compare two " + typeName + " structures and generate a report of any differences.\n";
    output += " * \\param _pg_prename is prepended to the name of the data field in the comparison report\n";
    output += " * \\param _pg_user1 is the first data to compare\n";
    output += " * \\param _pg_user1 is the second data to compare\n";
    output += " * \\return a string describing any differences between _pg_user1 and _pg_user2. The string will be empty if there are no differences\n";
    output += " */\n";
    output += "QString compare" + typeName + "(const QString& _pg_prename, const " + structName + "* _pg_user1, const " + structName + "* _pg_user2)\n";
    output += "{\n";
    output += TAB_IN + "QString _pg_report;\n";

    if(needsDecodeIterator)
        output += TAB_IN + "unsigned _pg_i = 0;\n";

    if(needs2ndDecodeIterator)
        output += TAB_IN + "unsigned _pg_j = 0;\n";

    for(int i = 0; i < encodables.length(); i++)
    {
        ProtocolFile::makeLineSeparator(output);
        output += encodables[i]->getComparisonString(true);
    }

    ProtocolFile::makeLineSeparator(output);
    output += TAB_IN + "return _pg_report;\n";
    output += "\n";
    output += "}// compare" + typeName + "\n";

    return output;
}


/*!
 * Get the string used for comparing this field.
 * \param isStructureMember should be true to indicate this field is accessed as a member structure
 * \return the function string, which may be empty
 */
QString ProtocolStructure::getComparisonString(bool isStructureMember) const
{
    QString output;
    QString access1, access2;

    // We must have parameters that we decode to do a comparison
    if(getNumberOfDecodeParameters() == 0)
        return output;

    if(!comment.isEmpty())
        output += TAB_IN + "// " + comment + "\n";

    if(isArray())
    {
        QString spacing = TAB_IN;
        output += spacing + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";
        spacing += TAB_IN;

        if(isStructureMember)
        {
            // The dereference of the array gets us back to the object, but we need the pointer
            access1 = "&_pg_user1->" + name + "[_pg_i]";
            access2 = "&_pg_user2->" + name + "[_pg_i]";
        }
        else
        {
            access1 = "&" + name + "_1[i]";
            access2 = "&" + name + "_2[i]";
        }

        if(is2dArray())
        {
            access1 += "[_pg_j]";
            access2 += "[_pg_j]";
            output += spacing + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";
            spacing += TAB_IN;

        }// if 2D array of structures

        output += spacing + "_pg_report += compare" + typeName + "(_pg_prename + \":" + name + "\"";

        if(isArray())
            output += " + \"[\" + QString::number(_pg_i) + \"]\"";

        if(is2dArray())
            output += " + \"[\" + QString::number(_pg_j) + \"]\"";

        output += ", " + access1 + ", " + access2 + ");\n";

    }// if array of structures
    else
    {
        if(isStructureMember)
        {
            // The dereference of the user pointer gets us back to the object, but we need the pointer
            access1 = "&_pg_user1->" + name;
            access2 = "&_pg_user2->" + name;
        }
        else
        {
            access1 = name + "_1";
            access2 = name + "_2";
        }

        output += TAB_IN + "_pg_report += compare" + typeName + "(_pg_prename + \":" + name + "\", "+ access1 + ", " + access2 + ");\n";

    }// else if simple structure, no array

    return output;

}// ProtocolStructure::getComparisonString


/*!
 * Return the string that gives the prototype of the function used to text print this structure
 * \param includeChildren should be true to include the function prototypes of the children structures of this structure
 * \return the function prototype string, which may be empty
 */
QString ProtocolStructure::getTextPrintFunctionPrototype(bool includeChildren) const
{
    QString output;

    // We must parameters that we decode to do a print out
    if(getNumberOfDecodeParameters() == 0)
        return output;

    // Go get any children structures textPrint functions
    if(includeChildren)
    {
        for(int i = 0; i < encodables.length(); i++)
        {
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(!structure)
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += structure->getTextPrintFunctionPrototype(includeChildren);
        }
        ProtocolFile::makeLineSeparator(output);
    }

    // My textPrint function
    output += "//! Generate a string that describes the contents of a " + typeName + " structure\n";
    output += "QString textPrint" + typeName + "(const QString& _pg_prename, const " + structName + "* _pg_user);\n";

    return output;
}


/*!
 * Return the string that gives the function used to compare this structure
 * \param includeChildren should be true to include the function prototypes of the children structures of this structure
 * \return the function string, which may be empty
 */
QString ProtocolStructure::getTextPrintFunctionString(bool includeChildren) const
{
    QString output;

    // We must parameters that we decode to do a print out
    if(getNumberOfDecodeParameters() == 0)
        return output;

    // Go get any childrens structure textPrint functions
    if(includeChildren)
    {
        for(int i = 0; i < encodables.length(); i++)
        {
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(!structure)
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += structure->getTextPrintFunctionString(includeChildren);
        }
        ProtocolFile::makeLineSeparator(output);
    }

    // My textPrint function
    output += "/*!\n";
    output += " * Generate a string that describes the contents of a " + typeName + " structure\n";
    output += " * \\param _pg_prename is prepended to the name of the data field in the report\n";
    output += " * \\param _pg_user is the structure to report\n";
    output += " * \\return a string containing a report of the contents of user\n";
    output += " */\n";
    output += "QString textPrint" + typeName + "(const QString& _pg_prename, const " + structName + "* _pg_user)\n";
    output += "{\n";
    output += TAB_IN + "QString _pg_report;\n";

    if(needsDecodeIterator)
        output += TAB_IN + "unsigned _pg_i = 0;\n";

    if(needs2ndDecodeIterator)
        output += TAB_IN + "unsigned _pg_j = 0;\n";

    for(int i = 0; i < encodables.length(); i++)
    {
        ProtocolFile::makeLineSeparator(output);
        output += encodables[i]->getTextPrintString(true);
    }

    ProtocolFile::makeLineSeparator(output);
    output += TAB_IN + "return _pg_report;\n";
    output += "\n";
    output += "}// textPrint" + typeName + "\n";

    return output;

}// ProtocolStructure::getTextPrintFunctionString


/*!
 * Get the string used for printing this field as text.
 * \param isStructureMember should be true to indicate this field is accessed as a member structure
 * \return the print string, which may be empty
 */
QString ProtocolStructure::getTextPrintString(bool isStructureMember) const
{
    QString output;
    QString access;

    // We must parameters that we decode to do a print out
    if(getNumberOfDecodeParameters() == 0)
        return output;

    if(!comment.isEmpty())
        output += TAB_IN + "// " + comment + "\n";

    if(isArray())
    {
        QString spacing = TAB_IN;
        output += spacing + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";
        spacing += TAB_IN;

        if(isStructureMember)
        {
            // The dereference of the array gets us back to the object, but we need the pointer
            access = "&_pg_user->" + name + "[_pg_i]";
        }
        else
        {
            access = "&" + name + "[_pg_i]";
        }

        if(is2dArray())
        {
            access += "[_pg_j]";
            output += spacing + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";
            spacing += TAB_IN;

        }// if 2D array of structures

        output += spacing + "_pg_report += textPrint" + typeName + "(_pg_prename + \":" + name + "\"";

        if(isArray())
            output += " + \"[\" + QString::number(_pg_i) + \"]\"";

        if(is2dArray())
            output += " + \"[\" + QString::number(_pg_j) + \"]\"";

        output += ", " + access + ");\n";

    }// if array of structures
    else
    {
        if(isStructureMember)
        {
            // The dereference of the user pointer gets us back to the object, but we need the pointer
            access = "&_pg_user->" + name;
        }
        else
        {
            access = name;
        }

        output += TAB_IN + "_pg_report += textPrint" + typeName + "(_pg_prename + \":" + name + "\", "+ access + ");\n";

    }// else if simple structure, no array

    return output;

}// ProtocolStructure::getTextPrintString


/*!
 * Return the string that gives the prototype of the function used to read this structure from text
 * \param includeChildren should be true to include the function prototypes of the children structures of this structure
 * \return the function prototype string, which may be empty
 */
QString ProtocolStructure::getTextReadFunctionPrototype(bool includeChildren) const
{
    QString output;

    // We must parameters that we decode to do a print out
    if(getNumberOfDecodeParameters() == 0)
        return output;

    // Go get any children structures textRead functions
    if(includeChildren)
    {
        for(int i = 0; i < encodables.length(); i++)
        {
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(!structure)
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += structure->getTextReadFunctionPrototype(includeChildren);
        }
        ProtocolFile::makeLineSeparator(output);
    }

    // My textRead function
    output += "//! Read the contents of a " + typeName + " structure from text\n";
    output += "int textRead" + typeName + "(const QString& _pg_prename, const QString& _pg_source, " + structName + "* _pg_user);\n";

    return output;
}


/*!
 * Get the string that gives the function used to read this structure from text
 * \param includeChildren should be true to include the function prototypes of
 *        the children structures of this structure
 * \return the function string, which may be empty
 */
QString ProtocolStructure::getTextReadFunctionString(bool includeChildren) const
{
    QString output;

    // We must parameters that we decode to do a print out
    if(getNumberOfDecodeParameters() == 0)
        return output;

    // Go get any childrens structure textRead functions
    if(includeChildren)
    {
        for(int i = 0; i < encodables.length(); i++)
        {
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(!structure)
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += structure->getTextReadFunctionString(includeChildren);
        }
        ProtocolFile::makeLineSeparator(output);
    }

    // My textRead function
    output += "/*!\n";
    output += " * Read the contents of a " + typeName + " structure from text\n";
    output += " * \\param _pg_prename is prepended to the name of the data field to form the text key\n";
    output += " * \\param _pg_source is text to search to find the data field keys\n";
    output += " * \\param _pg_user receives any data read from the text source\n";
    output += " * \\return The number of fields that were read from the text source\n";
    output += " */\n";
    output += "int textRead" + typeName + "(const QString& _pg_prename, const QString& _pg_source, " + structName + "* _pg_user)\n";
    output += "{\n";
    output += TAB_IN + "QString _pg_text;\n";
    output += TAB_IN + "int _pg_fieldcount = 0;\n";

    if(needsDecodeIterator)
        output += TAB_IN + "unsigned _pg_i = 0;\n";

    if(needs2ndDecodeIterator)
        output += TAB_IN + "unsigned _pg_j = 0;\n";

    for(int i = 0; i < encodables.length(); i++)
    {
        ProtocolFile::makeLineSeparator(output);
        output += encodables[i]->getTextReadString(true);
    }

    ProtocolFile::makeLineSeparator(output);
    output += TAB_IN + "return _pg_fieldcount;\n";
    output += "\n";
    output += "}// textRead" + typeName + "\n";

    return output;

}// ProtocolStructure::getTextReadFunctionString


/*!
 * Get the string used for reading this field from text.
 * \param isStructureMember should be true to indicate this field is accessed as a member structure
 * \return the read string, which may be empty
 */
QString ProtocolStructure::getTextReadString(bool isStructureMember) const
{
    QString output;
    QString access;

    // We must parameters that we decode to do a print out
    if(getNumberOfDecodeParameters() == 0)
        return output;

    if(!comment.isEmpty())
        output += TAB_IN + "// " + comment + "\n";

    if(isArray())
    {
        QString spacing = TAB_IN;
        output += spacing + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";
        spacing += TAB_IN;

        if(isStructureMember)
        {
            // The dereference of the array gets us back to the object, but we need the pointer
            access = "&_pg_user->" + name + "[_pg_i]";
        }
        else
        {
            access = "&" + name + "[_pg_i]";
        }

        if(is2dArray())
        {
            access += "[_pg_j]";
            output += spacing + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";
            spacing += TAB_IN;

        }// if 2D array of structures

        output += spacing + "_pg_fieldcount += textRead" + typeName + "(_pg_prename + \":" + name + "\"";

        if(isArray())
            output += " + \"[\" + QString::number(_pg_i) + \"]\"";

        if(is2dArray())
            output += " + \"[\" + QString::number(_pg_j) + \"]\"";

        output += ", _pg_source, " + access + ");\n";

    }// if array of structures
    else
    {
        if(isStructureMember)
        {
            // The dereference of the user pointer gets us back to the object, but we need the pointer
            access = "&_pg_user->" + name;
        }
        else
        {
            access = name;
        }

        output += TAB_IN + "_pg_fieldcount += textRead" + typeName + "(_pg_prename + \":" + name + "\", _pg_source, " + access + ");\n";

    }// else if simple structure, no array

    return output;

}// ProtocolStructure::getTextReadString


/*!
 * Return the string that gives the prototype of the function used to encode
 * this structure to a map
 * \param includeChildren should be true to include the function prototypes
 *        of the child structures
 * \return the function prototype string, which may be empty
 */
QString ProtocolStructure::getMapEncodeFunctionPrototype(bool includeChildren) const
{
    QString output;

    if(getNumberOfEncodeParameters() == 0)
    {
        return output;
    }

    if(includeChildren)
    {
        for(int i = 0; i < encodables.length();  i++)
        {
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(!structure)
                continue;

            ProtocolFile::makeLineSeparator(output);

            output += structure->getMapEncodeFunctionPrototype(includeChildren);
        }

        ProtocolFile::makeLineSeparator(output);
    }

    // My mapEncode function

    output += "//! Encode the contents of a " + typeName + " structure to a string Key:Value map\n";
    output += "void mapEncode" + typeName + "(const QString& _pg_prename, QVariantMap& _pg_map, const " + structName + "* _pg_user);\n";

    return output;

}// ProtocolStructure::getMapEncodeFunctionPrototype


/*!
 * Return the string that gives the function used to encode this structure to a map
 * \param includeChildren should be true to include the functions of the child structures
 * \return the function string, which may be empty
 */
QString ProtocolStructure::getMapEncodeFunctionString(bool includeChildren) const
{
    QString output;

    if(getNumberOfDecodeParameters() == 0)
        return output;

    if(includeChildren)
    {
        for(int i = 0; i < encodables.length(); i++)
        {
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if (!structure)
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += structure->getMapEncodeFunctionString(includeChildren);
        }

        ProtocolFile::makeLineSeparator(output);
    }

    // My mapEncode function
    output += "/*!\n";
    output += " * Encode the contents of a " + typeName + " structure to a Key:Value string map\n";
    output += " * \\param _pg_prename is prepended to the key fields in the map\n";
    output += " * \\param _pg_map is a reference to the map\n";
    output += " * \\param _pg_user is the structure to encode\n";
    output += " */\n";

    output += "void mapEncode" + typeName + "(const QString& _pg_prename, QVariantMap& _pg_map, const " + structName + "* _pg_user)\n";
    output += "{\n";

    if(needsDecodeIterator)
        output += TAB_IN + "unsigned _pg_i = 0;\n";

    if(needs2ndDecodeIterator)
        output += TAB_IN + "unsigned _pg_j = 0;\n";

    for(int i=0; i<encodables.length(); i++)
    {
        ProtocolFile::makeLineSeparator(output);
        output += encodables[i]->getMapEncodeString(true);
    }

    ProtocolFile::makeLineSeparator(output);

    output += "\n";
    output += "}// mapEncode" + typeName + "\n";

    return output;

}// ProtocolStructure::getMapEncodeFunctionString


/*!
 * Return the string used for encoding this field to a map
 * \param isStructureMember should be true to indicate this field is accessed
 *        as a member structure
 * \return the encode string, which may be empty
 */
QString ProtocolStructure::getMapEncodeString(bool isStructureMember) const
{
    QString output;
    QString access;

    if(getNumberOfDecodeParameters() == 0)
        return output;

    if(!comment.isEmpty())
        output += TAB_IN + "// " + comment + "\n";

    QString key = "\":" + name + "\"";

    if(isArray())
    {
        key += " + \"[\" + QString::number(_pg_i) + \"]\"";

        QString spacing = TAB_IN;
        output += spacing + "for(_pg_i = 0; ";

        if(variableArray.isEmpty())
            output += "_pg_i < " + array + ";";
        else
        {
            if(isStructureMember)
                output += "(_pg_i < " + array + ") && (_pg_i < (unsigned)_pg_user->" + variableArray + ");";
            else
                output += "(_pg_i < " + array + ") && (_pg_i < " + variableArray + ");";
        }

        output += " _pg_i++)\n";
        spacing += TAB_IN;

        if(isStructureMember)
        {
            // The dereference of the array gets us back to the object, but we need the pointer
            access = "&_pg_user->" + name + "[_pg_i]";
        }
        else
        {
            access = "&" + name + "[_pg_i]";
        }

        if(is2dArray())
        {
            access += "[_pg_j]";
            output += spacing + "for(_pg_j = 0;";

            if(variable2dArray.isEmpty())
                output += "_pg_j < " + array2d + ";";
            else
            {
                if(isStructureMember)
                    output += "(_pg_j < " + array2d + ") && (_pg_j < (unsigned)_pg_user->" + variable2dArray + ");";
                else
                    output += "(_pg_j < " + array2d + ") && (_pg_j < " + variable2dArray + ");";
            }

            output += " _pg_j++)\n";

            spacing += TAB_IN;

            key += " + \"[\" + QString::number(_pg_j) + \"]\"";

        }// if 2D array of structures

        output += spacing + "mapEncode" + typeName + "(_pg_prename + " + key + ", _pg_map, " + access + ");\n";

    }// if array of structures
    else
    {
        if(isStructureMember)
        {
            access = "&_pg_user->" + name;
        }
        else
        {
            access = name;
        }

        output += TAB_IN + "mapEncode" + typeName + "(_pg_prename + " + key + ", _pg_map, " + access + ");\n";

    }// else if simple structure, no array

    return output;

}// ProtocolStructure::getMapEncodeString


/*!
 * Get the string that gives the prototype of the function used to decode this
 * structure from a map
 * \param includeChildren should be true to include the function prototypes of
 *        the children structures of this structure
 * \return the function string, which may be empty
 */
QString ProtocolStructure::getMapDecodeFunctionPrototype(bool includeChildren) const
{
    QString output;

    if(getNumberOfEncodeParameters() == 0)
    {
        return output;
    }

    if(includeChildren)
    {
        for(int i = 0; i < encodables.length();  i++)
        {
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(!structure)
                continue;

            ProtocolFile::makeLineSeparator(output);

            output += structure->getMapDecodeFunctionPrototype(includeChildren);
        }

        ProtocolFile::makeLineSeparator(output);
    }

    // My mapEncode function

    output += "//! Decode the contents of a " + typeName + " structure from a string Key:Value map\n";
    output += "void mapDecode" + typeName + "(const QString& _pg_prename, QVariantMap& _pg_map, " + structName + "* _pg_user);\n";

    return output;
}// ProtocolStructure::getMapDecodeeFunctionPrototype


/*!
 * Get the string that gives the function used to decode this structure from a map
 * \param includeChildren should be true to include the function prototypes of
 *        the children structures of this structure
 * \return the function string, which may be empty
 */
QString ProtocolStructure::getMapDecodeFunctionString(bool includeChildren) const
{
    QString output;

    if(getNumberOfDecodeParameters() == 0)
        return output;

    if(includeChildren)
    {
        for(int i = 0; i < encodables.length(); i++)
        {
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if (!structure)
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += structure->getMapDecodeFunctionString(includeChildren);
        }

        ProtocolFile::makeLineSeparator(output);
    }

    // My mapEncode function
    output += "/*!\n";
    output += " * Decode the contents of a " + typeName + " structure to a Key:Value string map\n";
    output += " * \\param _pg_prename is prepended to the key fields in the map\n";
    output += " * \\param _pg_map is a reference to the map\n";
    output += " * \\param _pg_user is the structure to encode\n";
    output += " */\n";

    output += "void mapDecode" + typeName + "(const QString& _pg_prename, QVariantMap& _pg_map, " + structName + "* _pg_user)\n";
    output += "{\n";

    output += TAB_IN + "QString key;  // Temporary map key variable\n";
    output += TAB_IN + "bool ok = false;  // Temporary data validation variable\n";

    if(needsDecodeIterator)
        output += TAB_IN + "unsigned _pg_i = 0;  // Array iterator\n";

    if(needs2ndDecodeIterator)
        output += TAB_IN + "unsigned _pg_j = 0; // Secondary array iterator\n";

    for(int i=0; i<encodables.length(); i++)
    {
        ProtocolFile::makeLineSeparator(output);
        output += encodables[i]->getMapDecodeString(true);
    }

    ProtocolFile::makeLineSeparator(output);

    output += "\n";
    output += "}// mapDecode" + typeName + "\n";

    return output;

}// ProtocolStructure::getMapDecodeFunctionString


/*!
 * Get the string used for decoding this field from a map.
 * \param isStructureMember should be true to indicate this field is accessed
 *        as a member structure
 * \return the map decode string, which may be empty
 */
QString ProtocolStructure::getMapDecodeString(bool isStructureMember) const
{
    QString output;
    QString access;

    if(getNumberOfDecodeParameters() == 0)
        return output;

    if(!comment.isEmpty())
        output += TAB_IN + "// " + comment + "\n";

    QString key = "\":" + name + "\"";

    if(isArray())
    {
        key = key + " + \"[\" + QString::number(_pg_i) + \"]\"";

        QString spacing = TAB_IN;
        output += spacing + "for(_pg_i = 0; ";

        if(variableArray.isEmpty())
            output += "_pg_i < " + array + ";";
        else
        {
            if(isStructureMember)
                output += "(_pg_i < " + array + ") && (_pg_i < (unsigned)_pg_user->" + variableArray + ");";
            else
                output += "(_pg_i < " + array + ") && (_pg_i < " + variableArray + ");";
        }

        output += " _pg_i++)\n";

        spacing += TAB_IN;

        if(isStructureMember)
        {
            // The dereference of the array gets us back to the object, but we need the pointer
            access = "&_pg_user->" + name + "[_pg_i]";
        }
        else
        {
            access = "&" + name + "[_pg_i]";
        }

        if(is2dArray())
        {
            access += "[_pg_j]";
            output += spacing + "for(_pg_j = 0; ";

            if(variable2dArray.isEmpty())
                output += "_pg_j < " + array2d + ";";
            else
            {
                if(isStructureMember)
                    output += "(_pg_j < " + array2d + ") && (_pg_j < (unsigned)_pg_user->" + variable2dArray + ");";
                else
                    output += "(_pg_j < " + array2d + ") && (_pg_j < " + variable2dArray + ");";
            }

            output += " _pg_j++)\n";
            spacing += TAB_IN;

        }// if 2D array of structures

        output += spacing + "mapDecode" + typeName + "(_pg_prename + " + key + ", _pg_map, " + access + ");\n";

        //TODO
    }// if array of structures
    else
    {
        if(isStructureMember)
        {
            access = "&_pg_user->" + name;
        }
        else
        {
            access = name;
        }

        output += TAB_IN + "mapDecode" + typeName + "(_pg_prename + " + key + ", _pg_map, " + access + ");\n";

    }// else if simple structure, no array

    return output;
}


//! Return the strings that #define initial and variable values
QString ProtocolStructure::getInitialAndVerifyDefines(bool includeComment) const
{
    QString output;

    for(int i = 0; i < encodables.count(); i++)
    {
        // Children's outputs do not have comments, just the top level stuff
        output += encodables.at(i)->getInitialAndVerifyDefines(false);
    }

    // I don't want to output this comment if there are no values being commented,
    // that's why I insert the comment after the #defines are output
    if(!output.isEmpty() && includeComment)
    {
        output.insert(0, "// Initial and verify values for " + name + "\n");
    }

    return output;

}// ProtocolStructure::getInitialAndVerifyDefines


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
    parser->replaceEnumerationNameWithValue(maxEncodedLength);

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
    outlineString += ")" + title;
    names.append(outlineString);

    // Encoding is blank for structures
    encodings.append(QString());

    // The repeat/array column
    if(array.isEmpty())
        repeats.append(QString());
    else
        repeats.append(getRepeatsDocumentationDetails());

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

