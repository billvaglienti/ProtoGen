#include "protocolstructure.h"
#include "protocolstructuremodule.h"
#include "protocolparser.h"
#include "protocolfield.h"
#include <string>
#include <iostream>

/*!
 * Construct a protocol structure
 * \param parse points to the global protocol parser that owns everything
 * \param parent is the hierarchical name of the object that owns this object.
 * \param support are the protocol support details
 */
ProtocolStructure::ProtocolStructure(ProtocolParser* parse, std::string parent, ProtocolSupport supported) :
    Encodable(parse, parent, supported),
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
    hasinit(supported.language == ProtocolSupport::cpp_language),
    hasverify(false),
    encode(true),
    decode(true),
    compare(false),
    print(false),
    mapEncode(false),
    redefines(nullptr)
{
    // List of attributes understood by ProtocolStructure
    attriblist = {"name",  "title",  "array",  "variableArray",  "array2d",  "variable2dArray",  "dependsOn",  "comment",  "hidden",  "limitOnEncode"};

}


/*!
 * Reset all data to defaults
 */
void ProtocolStructure::clear(void)
{
    Encodable::clear();

    // Delete any encodable objects in our list
    for(std::size_t i = 0; i < encodables.size(); i++)
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
    hasinit = (support.language == ProtocolSupport::cpp_language);
    hasverify = false;
    encode = decode = true;
    print = compare = mapEncode = false;
    structName.clear();
    redefines = nullptr;

}// ProtocolStructure::clear


/*!
 * Parse the DOM data for this structure
 */
void ProtocolStructure::parse(void)
{
    if(e == nullptr)
        return;

    const XMLAttribute* map = e->FirstAttribute();

    // All the attribute we care about
    name = ProtocolParser::getAttribute("name", map, "_unknown");
    title = ProtocolParser::getAttribute("title", map);
    array = ProtocolParser::getAttribute("array", map);
    variableArray = ProtocolParser::getAttribute("variableArray", map);
    dependsOn = ProtocolParser::getAttribute("dependsOn", map);
    dependsOnValue = ProtocolParser::getAttribute("dependsOnValue", map);
    dependsOnCompare = ProtocolParser::getAttribute("dependsOnCompare", map);
    comment = ProtocolParser::reflowComment(ProtocolParser::getAttribute("comment", map));
    hidden = ProtocolParser::isFieldSet("hidden", map);

    if(title.empty())
        title = name;

    // This will propagate to any of the children we create
    if(ProtocolParser::isFieldSet("limitOnEncode", map))
        support.limitonencode = true;
    else if(ProtocolParser::isFieldClear("limitOnEncode", map))
        support.limitonencode = false;

    testAndWarnAttributes(map);

    // for now the typename is derived from the name
    structName = typeName = support.prefix + name + support.typeSuffix;

    // We can't have a variable array length without an array
    if(array.empty() && !variableArray.empty())
    {
        emitWarning("must specify array length to specify variable array length");
        variableArray.clear();
    }

    if(!dependsOn.empty() && !variableArray.empty())
    {
        emitWarning("variable length arrays cannot also use dependsOn");
        dependsOn.clear();
    }

    if(!dependsOnValue.empty() && dependsOn.empty())
    {
        emitWarning("dependsOnValue does not make sense unless dependsOn is defined");
        dependsOnValue.clear();
    }

    if(!dependsOnCompare.empty() && dependsOnValue.empty())
    {
        emitWarning("dependsOnCompare does not make sense unless dependsOnValue is defined");
        dependsOnCompare.clear();
    }
    else if(dependsOnCompare.empty() && !dependsOnValue.empty())
    {
        // This is not a warning, it is expected
        dependsOnCompare = "==";
    }

    // Check to make sure we did not step on any keywords
    checkAgainstKeywords();

    // Get any enumerations
    parseEnumerations(e);

    // At this point a structure cannot be default, null, or reserved.
    parseChildren(e);

    // Sum the length of all the children
    EncodedLength length;
    for(std::size_t i = 0; i < encodables.size(); i++)
    {
        length.addToLength(encodables.at(i)->encodedLength);
    }

    // Account for array, variable array, and depends on
    encodedLength.clear();
    encodedLength.addToLength(length, array, !variableArray.empty(), !dependsOn.empty());

}// ProtocolStructure::parse


/*!
 * Return the string used to declare this encodable as part of a structure.
 * This includes the spacing, typename, name, semicolon, comment, and linefeed
 * \return the declaration string for this encodable
 */
std::string ProtocolStructure::getDeclaration(void) const
{
    std::string output = TAB_IN + "" + typeName + " " + name;

    if(array.empty())
        output += ";";
    else if(array2d.empty())
        output += "[" + array + "];";
    else
        output += "[" + array + "][" + array2d + "]";

    if(!comment.empty())
        output += " //!< " + comment;

    output += "\n";

    return output;

}// ProtocolStructure::getDeclaration


/*!
 * Return the string that is used to encode *this* structure
 * \param isBigEndian should be true for big endian encoding, ignored
 * \param bitcount points to the running count of bits in a bitfields and should persist between calls, ignored
 * \param isStructureMember is true if this encodable is accessed by structure pointer
 * \return the string to add to the source to encode this structure
 */
std::string ProtocolStructure::getEncodeString(bool isBigEndian, int* bitcount, bool isStructureMember) const
{
    (void)isBigEndian;
    (void)bitcount;

    std::string output;
    std::string access = getEncodeFieldAccess(isStructureMember);
    std::string spacing = TAB_IN;

    if(!comment.empty())
        output += spacing + "// " + comment + "\n";

    if(!dependsOn.empty())
    {
        output += spacing + "if(" + getEncodeFieldAccess(isStructureMember, dependsOn);

        if(!dependsOnValue.empty())
            output += " " + dependsOnCompare + " " + dependsOnValue;

        output += ")\n" + spacing + "{\n";
        spacing += TAB_IN;
    }

    // Array handling
    output += getEncodeArrayIterationCode(spacing, isStructureMember);

    // Spacing for arrays
    if(isArray())
    {
        spacing += TAB_IN;
        if(is2dArray())
            spacing += TAB_IN;
    }

    // The actual encode function
    if(support.language == ProtocolSupport::c_language)
        output += spacing + "encode" + typeName + "(_pg_data, &_pg_byteindex, " + access + ");\n";
    else
    {
        // Note that if we are an array the dereferencing is done by the array operator
        if(isStructureMember || isArray())
            output += spacing + access + ".encode(_pg_data, &_pg_byteindex);\n";
        else
            output += spacing + access + "->encode(_pg_data, &_pg_byteindex);\n";
    }

    // Close the depends on block
    if(!dependsOn.empty())
        output += TAB_IN + "}\n";

    return output;

}// ProtocolStructure::getEncodeString


/*!
 * Return the string that is used to decode this structure
 * \param isBigEndian should be true for big endian encoding.
 * \param bitcount points to the running count of bits in a bitfields and should persist between calls
 * \param isStructureMember is true if this encodable is accessed by structure pointer
 * \return the string to add to the source to decode this structure
 */
std::string ProtocolStructure::getDecodeString(bool isBigEndian, int* bitcount, bool isStructureMember, bool defaultEnabled) const
{
    (void)isBigEndian;
    (void)bitcount;
    (void)defaultEnabled;

    std::string output;
    std::string access = getEncodeFieldAccess(isStructureMember);
    std::string spacing = TAB_IN;

    if(!comment.empty())
        output += spacing + "// " + comment + "\n";

    if(!dependsOn.empty())
    {
        output += spacing + "if(" + getDecodeFieldAccess(isStructureMember, dependsOn);

        if(!dependsOnValue.empty())
            output += " " + dependsOnCompare + " " + dependsOnValue;

        output += ")\n" + spacing + "{\n";
        spacing += TAB_IN;
    }

    // Array handling
    output += getDecodeArrayIterationCode(spacing, isStructureMember);

    // Spacing for arrays
    if(isArray())
    {
        spacing += TAB_IN;
        if(is2dArray())
            spacing += TAB_IN;
    }

    if(support.language == ProtocolSupport::c_language)
    {
        output += spacing + "if(decode" + typeName + "(_pg_data, &_pg_byteindex, " + access + ") == 0)\n";
        output += spacing + TAB_IN + "return 0;\n";
    }
    else
    {       
        // Note that if we are an array the dereferencing is done by the array operator
        if(isStructureMember || isArray())
            output += spacing + "if(" + access + ".decode(_pg_data, &_pg_byteindex) == false)\n";
        else
            output += spacing + "if(" + access + "->decode(_pg_data, &_pg_byteindex) == false)\n";

        output += spacing + TAB_IN + "return false;\n";
    }

    if(!dependsOn.empty())
        output += TAB_IN + "}\n";

    return output;

}// ProtocolStructure::getDecodeString


/*!
 * Get the code which verifies this structure
 * \return the code to put in the source file
 */
std::string ProtocolStructure::getVerifyString(void) const
{
    std::string output;
    std::string access = name;
    std::string spacing = TAB_IN;

    if(!hasverify)
        return output;

    if(!comment.empty())
        output += spacing + "// " + comment + "\n";

    // Do not call getDecodeArrayIterationCode() because we explicity don't handle variable length arrays here
    if(isArray())
    {
        output += spacing + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";
        spacing += TAB_IN;

        if(is2dArray())
        {
            output += spacing + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";
            spacing += TAB_IN;
        }
    }

    if(support.language == ProtocolSupport::c_language)
    {
        output += spacing + "if(verify" + typeName + "(" + getDecodeFieldAccess(true) + ") == 0)\n";
        output += spacing + TAB_IN + "_pg_good = 0;\n";
    }
    else
    {
        output += spacing + "if(" + getDecodeFieldAccess(true) + ".verify() == false)\n";
        output += spacing + TAB_IN + "_pg_good = false;\n";
    }

    return output;

}// ProtocolStructure::getVerifyString


/*!
 * Get the code which sets this structure member to initial values
 * \return the code to put in the source file
 */
std::string ProtocolStructure::getSetInitialValueString(bool isStructureMember) const
{
    std::string output;
    std::string spacing = TAB_IN;

    // We only need this function if we are C language, C++ classes initialize themselves
    if((!hasinit) || (support.language != ProtocolSupport::c_language))
        return output;

    if(!comment.empty())
        output += spacing + "// " + comment + "\n";

    // Do not call getDecodeArrayIterationCode() because we explicity don't handle variable length arrays here
    if(isArray())
    {
        output += spacing + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";
        spacing += TAB_IN;

        if(is2dArray())
        {
            output += spacing + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";
            spacing += TAB_IN;
        }
    }

    output += spacing + "init" + typeName + "(" + getDecodeFieldAccess(isStructureMember) + ");\n";

    return output;

}// ProtocolStructure::getSetInitialValueString


//! Return the strings that #define initial and variable values
std::string ProtocolStructure::getInitialAndVerifyDefines(bool includeComment) const
{
    std::string output;

    for(std::size_t i = 0; i < encodables.size(); i++)
    {
        // Children's outputs do not have comments, just the top level stuff
        output += encodables.at(i)->getInitialAndVerifyDefines(false);
    }

    // I don't want to output this comment if there are no values being commented,
    // that's why I insert the comment after the #defines are output
    if(!output.empty() && includeComment)
    {
        output.insert(0, "// Initial and verify values for " + name + "\n");
    }

    return output;

}// ProtocolStructure::getInitialAndVerifyDefines


/*!
 * Get the string used for comparing this field.
 * \return the function string, which may be empty
 */
std::string ProtocolStructure::getComparisonString(void) const
{
    std::string output;
    std::string access1, access2;

    // We must have parameters that we decode to do a comparison
    if(!compare || (getNumberOfDecodeParameters() == 0))
        return output;

    std::string spacing = TAB_IN;

    if(!comment.empty())
        output += spacing + "// " + comment + "\n";

    /// TODO: obey variable array length limits?

    if(support.language == ProtocolSupport::c_language)
    {
        // The dereference of the array gets us back to the object, but we need the pointer
        access1 = "&_pg_user1->" + name;
        access2 = "&_pg_user2->" + name;
    }
    else
    {
        access1 = name;
        access2 = "&_pg_user->" + name;
    }

    if(isArray())
    {
        output += spacing + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";
        spacing += TAB_IN;

        access1 += "[_pg_i]";
        access2 += "[_pg_i]";

        if(is2dArray())
        {
            access1 += "[_pg_j]";
            access2 += "[_pg_j]";
            output += spacing + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";
            spacing += TAB_IN;

        }// if 2D array of structures

    }// if array of structures

    if(support.language == ProtocolSupport::c_language)
        output += spacing + "_pg_report += compare" + typeName + "(_pg_prename + \":" + name + "\"";
    else
        output += spacing + "_pg_report += " + access1 + ".compare(_pg_prename + \":" + name + "\"";

    if(isArray())
        output += " + \"[\" + std::to_string(_pg_i) + \"]\"";

    if(is2dArray())
        output += " + \"[\" + std::to_string(_pg_j) + \"]\"";

    if(support.language == ProtocolSupport::c_language)
        output += ", " + access1 + ", " + access2 + ");\n";
    else
        output += ", " + access2 + ");\n";

    return output;

}// ProtocolStructure::getComparisonString


/*!
 * Get the string used for printing this field as text.
 * \return the print string, which may be empty
 */
std::string ProtocolStructure::getTextPrintString(void) const
{
    std::string output;
    std::string access;
    std::string spacing = TAB_IN;

    // We must parameters that we decode to do a print out
    if(!print || (getNumberOfDecodeParameters() == 0))
        return output;

    if(!comment.empty())
        output += spacing + "// " + comment + "\n";

    output += getEncodeArrayIterationCode(spacing, true);
    if(isArray())
    {
        spacing += TAB_IN;
        if(is2dArray())
            spacing += TAB_IN;
    }

    if(support.language == ProtocolSupport::c_language)
        output += spacing + "_pg_report += textPrint" + typeName + "(_pg_prename + \":" + name + "\"";
    else
        output += spacing + "_pg_report += " + getEncodeFieldAccess(true) + ".textPrint(_pg_prename + \":" + name + "\"";

    if(isArray())
        output += " + \"[\" + std::to_string(_pg_i) + \"]\"";

    if(is2dArray())
        output += " + \"[\" + std::to_string(_pg_j) + \"]\"";

    if(support.language == ProtocolSupport::c_language)
        output += ", " + getEncodeFieldAccess(true);

    output += ");\n";

    return output;

}// ProtocolStructure::getTextPrintString


/*!
 * Get the string used for reading this field from text.
 * \return the read string, which may be empty
 */
std::string ProtocolStructure::getTextReadString(void) const
{
    std::string output;
    std::string access;
    std::string spacing = TAB_IN;

    // We must parameters that we decode to do a print out
    if(!print || (getNumberOfDecodeParameters() == 0))
        return output;

    if(!comment.empty())
        output += spacing + "// " + comment + "\n";

    output += getEncodeArrayIterationCode(spacing, true);
    if(isArray())
    {
        spacing += TAB_IN;
        if(is2dArray())
            spacing += TAB_IN;
    }

    if(support.language == ProtocolSupport::c_language)
        output += spacing + "_pg_fieldcount += textRead" + typeName + "(_pg_prename + \":" + name + "\"";
    else
        output += spacing + "_pg_fieldcount += " + getEncodeFieldAccess(true) + ".textRead(_pg_prename + \":" + name + "\"";

    if(isArray())
        output += " + \"[\" + std::to_string(_pg_i) + \"]\"";

    if(is2dArray())
        output += " + \"[\" + std::to_string(_pg_j) + \"]\"";

    output += ", _pg_source";

    if(support.language == ProtocolSupport::c_language)
        output += ", " + getEncodeFieldAccess(true);

    output += ");\n";

    return output;

}// ProtocolStructure::getTextReadString


/*!
 * Return the string used for encoding this field to a map
 * \return the encode string, which may be empty
 */
std::string ProtocolStructure::getMapEncodeString(void) const
{
    std::string output;
    std::string spacing = TAB_IN;

    // We must parameters that we decode to do a print out
    if(!mapEncode || (getNumberOfDecodeParameters() == 0))
        return output;

    if(!comment.empty())
        output += spacing + "// " + comment + "\n";

    std::string key = "\":" + name + "\"";

    output += getEncodeArrayIterationCode(spacing, true);
    if(isArray())
    {
        spacing += TAB_IN;
        key += " + \"[\" + QString::number(_pg_i) + \"]\"";

        if(is2dArray())
        {
            spacing += TAB_IN;
            key += " + \"[\" + QString::number(_pg_j) + \"]\"";
        }
    }

    if(support.language == ProtocolSupport::c_language)
        output += spacing + "mapEncode" + typeName + "(_pg_prename + " + key + ", _pg_map, " + getEncodeFieldAccess(true);
    else
        output += spacing + getEncodeFieldAccess(true) + ".mapEncode(_pg_prename + " + key + ", _pg_map";

    output += ");\n";

    return output;

}// ProtocolStructure::getMapEncodeString


/*!
 * Get the string used for decoding this field from a map.
 * \return the map decode string, which may be empty
 */
std::string ProtocolStructure::getMapDecodeString(void) const
{
    std::string output;
    std::string spacing = TAB_IN;

    // We must parameters that we decode to do a print out
    if(!mapEncode || (getNumberOfDecodeParameters() == 0))
        return output;

    if(!comment.empty())
        output += spacing + "// " + comment + "\n";

    std::string key = "\":" + name + "\"";

    output += getDecodeArrayIterationCode(spacing, true);
    if(isArray())
    {
        spacing += TAB_IN;
        key += " + \"[\" + QString::number(_pg_i) + \"]\"";

        if(is2dArray())
        {
            spacing += TAB_IN;
            key += " + \"[\" + QString::number(_pg_j) + \"]\"";
        }
    }

    if(support.language == ProtocolSupport::c_language)
        output += spacing + "mapDecode" + typeName + "(_pg_prename + " + key + ", _pg_map, " + getDecodeFieldAccess(true);
    else
        output += spacing + getDecodeFieldAccess(true) + ".mapDecode(_pg_prename + " + key + ", _pg_map";

    output += ");\n";

    return output;

}// ProtocolStructure::getMapDecodeString


/*!
 * Parse all enumerations which are direct children of a DomNode
 * \param node is parent node
 */
void ProtocolStructure::parseEnumerations(const XMLNode* node)
{
    for(const XMLElement* child = node->FirstChildElement(); child != nullptr; child = child->NextSiblingElement())
    {
        if(contains(child->Name(), "enum"))
            enumList.push_back(parser->parseEnumeration(getHierarchicalName(), child));

    }// for all my enum tags

}// ProtocolStructure::parseEnumerations


/*!
 * Parse the DOM data for the children of this structure
 * \param field is the DOM data for this structure
 */
void ProtocolStructure::parseChildren(const XMLElement* field)
{
    Encodable* prevEncodable = NULL;

    // Make encodables out of them, and add to our list
    for(const XMLElement* child = field->FirstChildElement(); child != nullptr; child = child->NextSiblingElement())
    {
        Encodable* encodable = generateEncodable(parser, getHierarchicalName(), support, child);
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
                        std::size_t prev;
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
                        for(std::size_t j = 0; j < encodables.size(); j++)
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
                if(!encodable->variableArray.empty())
                {
                    std::size_t prev;
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
                if(!encodable->variable2dArray.empty())
                {
                    std::size_t prev;
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
                if(!encodable->dependsOn.empty())
                {
                    if(encodable->isBitfield())
                    {
                        encodable->emitWarning("bitfields cannot use dependsOn");
                        encodable->dependsOn.clear();
                    }
                    else
                    {
                        std::size_t prev;
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
                           encodable->dependsOnValue.clear();
                           encodable->dependsOnCompare.clear();
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


//! Set the compare flag for this structure and all children structure
void ProtocolStructure::setCompare(bool enable)
{
    for(std::size_t i = 0; i < encodables.size(); i++)
    {
        // Is this encodable a structure?
        ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

        if(structure == nullptr)
            continue;

        structure->setCompare(enable);

    }// for all children

    compare = enable;
}


//! Set the print flag for this structure and all children structure
void ProtocolStructure::setPrint(bool enable)
{
    for(std::size_t i = 0; i < encodables.size(); i++)
    {
        // Is this encodable a structure?
        ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

        if(structure == nullptr)
            continue;

        structure->setPrint(enable);

    }// for all children

    print = enable;
}


//! Set the mapEncode flag for this structure and all children structure
void ProtocolStructure::setMapEncode(bool enable)
{
    for(std::size_t i = 0; i < encodables.size(); i++)
    {
        // Is this encodable a structure?
        ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

        if(structure == nullptr)
            continue;

        structure->setMapEncode(enable);

    }// for all children

    mapEncode = enable;
}


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
    for(std::size_t i = 0; i < encodables.size(); i++)
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
    for(std::size_t i = 0; i < encodables.size(); i++)
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
    for(std::size_t i = 0; i < encodables.size(); i++)
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
    for(std::size_t i = 0; i < encodables.size(); i++)
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
void ProtocolStructure::getIncludeDirectives(std::vector<std::string>& list) const
{
    // Includes that our encodable members may need
    for(std::size_t i = 0; i < encodables.size(); i++)
        encodables.at(i)->getIncludeDirectives(list);

    // Array sizes could be enumerations that need an include directive
    if(!array.empty())
    {
        std::string include = parser->lookUpIncludeName(array);
        if(!include.empty())
            list.push_back(include);
    }

    // Array sizes could be enumerations that need an include directive
    if(!array2d.empty())
    {
        std::string include = parser->lookUpIncludeName(array2d);
        if(!include.empty())
            list.push_back(include);
    }

    removeDuplicates(list);

}// ProtocolStructure::getIncludeDirectives


/*!
 * Append the include directives in source code for this encodable. Mostly this is empty,
 * but code encodables may have source code includes.
 * \param list is appended with any directives this encodable requires.
 */
void ProtocolStructure::getSourceIncludeDirectives(std::vector<std::string>& list) const
{
    // Includes that our encodable members may need
    for(std::size_t i = 0; i < encodables.size(); i++)
        encodables.at(i)->getSourceIncludeDirectives(list);

    removeDuplicates(list);
}


/*!
 * Return the include directives needed for this encodable's init and verify functions
 * \param list is appended with any directives this encodable requires.
 */
void ProtocolStructure::getInitAndVerifyIncludeDirectives(std::vector<std::string>& list) const
{
    // Includes that our encodable members may need
    for(std::size_t i = 0; i < encodables.size(); i++)
        encodables.at(i)->getInitAndVerifyIncludeDirectives(list);

    removeDuplicates(list);
}


/*!
 * Return the include directives needed for this encodable's map functions
 * \param list is appended with any directives this encodable requires.
 */
void ProtocolStructure::getMapIncludeDirectives(std::vector<std::string>& list) const
{
    // Includes that our encodable members may need
    for(std::size_t i = 0; i < encodables.size(); i++)
        encodables.at(i)->getMapIncludeDirectives(list);

    removeDuplicates(list);
}


/*!
 * Return the include directives needed for this encodable's compare functions
 * \param list is appended with any directives this encodable requires.
 */
void ProtocolStructure::getCompareIncludeDirectives(std::vector<std::string>& list) const
{
    // Includes that our encodable members may need
    for(std::size_t i = 0; i < encodables.size(); i++)
        encodables.at(i)->getCompareIncludeDirectives(list);

    removeDuplicates(list);
}


/*!
 * Return the include directives needed for this encodable's print functions
 * \param list is appended with any directives this encodable requires.
 */
void ProtocolStructure::getPrintIncludeDirectives(std::vector<std::string>& list) const
{
    // Includes that our encodable members may need
    for(std::size_t i = 0; i < encodables.size(); i++)
        encodables.at(i)->getPrintIncludeDirectives(list);

    removeDuplicates(list);
}


/*!
 * Get the declaration that goes in the header which declares this structure
 * and all its children.
 * \param alwaysCreate should be true to force creation of the structure, even if there is only one member
 * \return the string that represents the structure declaration
 */
std::string ProtocolStructure::getStructureDeclaration(bool alwaysCreate) const
{
    std::string output;
    std::string structure;

    /// TODO: Move this to be within the class for C++
    // Output enumerations specific to this structure, be sure to do it before
    // declaring the structure or any of its children
    for(std::size_t i = 0; i < enumList.size(); i++)
    {
        output += enumList.at(i)->getOutput();
        ProtocolFile::makeLineSeparator(output);
    }

    // Declare our childrens structures first, but only if we are not redefining
    // someone else, in which case they have already declared the children
    if(redefines == nullptr)
    {
        for(std::size_t i = 0; i < encodables.size(); i++)
        {
            // Is this encodable a structure?
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(structure == nullptr)
                continue;

            output += structure->getStructureDeclaration(true);
            ProtocolFile::makeLineSeparator(output);

        }// for all children

    }// if not redefining

    if(support.language == ProtocolSupport::c_language)
        output += getStructureDeclaration_C(alwaysCreate);
    else
        output += getClassDeclaration_CPP();

    return output;

}// ProtocolStructure::getStructureDeclaration


/*!
 * Get the structure declaration, for this structure only (not its children) for the C language
 * \return The string that gives the structure declaration
 */
std::string ProtocolStructure::getStructureDeclaration_C(bool alwaysCreate) const
{
    std::string output;

    // We don't generate the structure if there is only one element, whats
    // the point? Unless the the caller tells us to always create it. We
    // also do not write the structure if we are redefining, in that case
    // the structure already exists.
    if((redefines == nullptr) && (getNumberInMemory() > 0) && ((getNumberInMemory() > 1) || alwaysCreate))
    {
        std::string structure;

        // The top level comment for the structure definition
        if(!comment.empty())
        {
            output += "/*!\n";
            output += ProtocolParser::outputLongComment(" * ", comment) + "\n";
            output += " */\n";
        }

        // The opening to the structure
        output += "typedef struct\n";
        output += "{\n";
        for(std::size_t i = 0; i < encodables.size(); i++)
            structure += encodables[i]->getDeclaration();

        // Make structures pretty with alignment goodness
        output += alignStructureData(structure);

        // Close out the structure
        output += "}" + typeName + ";\n";

    }// if we have some data to declare

    return output;

}// ProtocolStructure::getStructureDeclaration_C


/*!
 * Get the class declaration, for this structure only (not its children) for the C++ language
 * \return The string that gives the class declaration
 */
std::string ProtocolStructure::getClassDeclaration_CPP(void) const
{
    std::string output;
    std::string structure;

    // The top level comment for the class definition
    if(!comment.empty())
    {
        output += "/*!\n";
        output += ProtocolParser::outputLongComment(" * ", comment) + "\n";
        output += " */\n";
    }

    // The opening to the class.
    if(redefines == nullptr)
        output += "class " + typeName + "\n";
    else
    {
        // In the context of C++ redefining means inheriting from a base class,
        // and adding a new encode or decode function. All the other members and
        // methods come from the base class
        // The opening to the class.
        output += "class " + typeName + " : public " + redefines->typeName + "\n";
    }

    output += "{\n";

    // All members of the structure are public.
    output += "public:\n";

    // Function prototypes, in C++ these are part of the class definition
    if((getNumberInMemory() > 0) && (redefines == nullptr))
    {
        ProtocolFile::makeLineSeparator(output);
        output += getSetToInitialValueFunctionPrototype(TAB_IN, false);
        ProtocolFile::makeLineSeparator(output);
    }

    if(encode)
    {
        ProtocolFile::makeLineSeparator(output);
        output += getEncodeFunctionPrototype(TAB_IN, false);
        ProtocolFile::makeLineSeparator(output);
    }

    if(decode)
    {
        ProtocolFile::makeLineSeparator(output);
        output += getDecodeFunctionPrototype(TAB_IN, false);
        ProtocolFile::makeLineSeparator(output);
    }

    if((encode != false) || (decode != false))
    {
        ProtocolFile::makeLineSeparator(output);
        output += createUtilityFunctions(TAB_IN);
        ProtocolFile::makeLineSeparator(output);
    }

    if(redefines == nullptr)
    {
        if(hasVerify())
        {
            ProtocolFile::makeLineSeparator(output);
            output += getVerifyFunctionPrototype(TAB_IN, false);
            ProtocolFile::makeLineSeparator(output);
        }

        if(print)
        {
            ProtocolFile::makeLineSeparator(output);
            output += getTextPrintFunctionPrototype(TAB_IN, false);
            ProtocolFile::makeLineSeparator(output);
            output += getTextReadFunctionPrototype(TAB_IN, false);
            ProtocolFile::makeLineSeparator(output);
        }

        if(mapEncode)
        {
            ProtocolFile::makeLineSeparator(output);
            output += getMapEncodeFunctionPrototype(TAB_IN, false);
            ProtocolFile::makeLineSeparator(output);
            output += getMapDecodeFunctionPrototype(TAB_IN, false);
            ProtocolFile::makeLineSeparator(output);
        }

        if(compare)
        {
            ProtocolFile::makeLineSeparator(output);
            output += getComparisonFunctionPrototype(TAB_IN, false);
            ProtocolFile::makeLineSeparator(output);
        }

        // Now declare the members of this class
        for(std::size_t i = 0; i < encodables.size(); i++)
            structure += encodables[i]->getDeclaration();

        // Make classes pretty with alignment goodness
        output += alignStructureData(structure);

        ProtocolFile::makeLineSeparator(output);

    }// if not redefining

    // Close out the class
    output += "}; // " + typeName + "\n";

    return output;

}// ProtocolStructure::getClassDeclaration_CPP


/*!
 * Make a structure output be prettily aligned
 * \param structure is the input structure string
 * \return a new string that is the aligned structure string
 */
std::string ProtocolStructure::alignStructureData(const std::string& structure) const
{
    std::size_t i, index;

    // The strings as a list separated by line feeds
    std::vector<std::string> list = split(structure, "\n");

    // The space separates the typeName from the name,
    // but skip the indent spaces
    std::size_t max = 0;
    for(i = 0; i < list.size(); i++)
    {
        index = list.at(i).find(" ", TAB_IN.size());
        if((index < list.at(i).size()) && (index > max))
            max = index;
    }

    for(i = 0; i < list.size(); i++)
    {
        // Insert spaces until we have reached the max
        index = list.at(i).find(" ", TAB_IN.size());
        if((index < list.at(i).size()))
        {
            for(; index < max; index++)
                list[i].insert(index, " ");
        }
    }

    // The first semicolon we find separates the name from the comment
    max = 0;
    for(i = 0; i < list.size(); i++)
    {
        // we want the character after the semicolon
        index = list[i].find(";");
        if(index < list.at(i).size())
        {
            if(index + 1 > max)
                max = index + 1;
        }
    }

    for(i = 0; i < list.size(); i++)
    {
        // Insert spaces until we have reached the max
        index = list[i].find(";");
        if(index < list.at(i).size())
        {
            for(index += 1; index < max; index++)
                list[i].insert(index, " ");
        }
    }

    // Re-assemble the output, put the line feeds back on
    return join(list, "\n") + "\n";

}// ProtocolStructure::alignStructureData


/*!
 * Get the signature of the function that encodes this structure.
 * \param insource should be true to indicate this signature is in source code
 * \return the signature of the encode function.
 */
std::string ProtocolStructure::getEncodeFunctionSignature(bool insource) const
{
    std::string output;

    if(support.language == ProtocolSupport::c_language)
    {
        std::string pg;

        if(insource)
            pg = "_pg_";

        if(getNumberOfEncodeParameters() > 0)
        {
            output = "void encode" + typeName + "(uint8_t* " + pg + "data, int* " + pg + "bytecount, const " + structName + "* " + pg+ "user)";
        }
        else
        {
            output = "void encode" + typeName + "(uint8_t* " + pg + "data, int* " + pg + "bytecount)";
        }
    }
    else
    {
        // For C++ these functions are within the class namespace and they
        // reference their own members. This function is const, unless it
        // doesn't have any encode parameters, in which case it is static.
        if(getNumberOfEncodeParameters() > 0)
        {
            if(insource)
                output = "void " + typeName + "::encode(uint8_t* _pg_data, int* _pg_bytecount) const";
            else
                output = "void encode(uint8_t* data, int* bytecount) const";
        }
        else
        {
            if(insource)
                output = "void " + typeName + "::encode(uint8_t* _pg_data, int* _pg_bytecount)";
            else
                output = "static void encode(uint8_t* data, int* bytecount)";
        }
    }

    return output;

}// ProtocolStructure::getEncodeFunctionSignature


/*!
 * Return the string that gives the prototype of the functions used to encode
 * the structure, and all child structures. The encoding is to a simple byte array.
 * \param spacing gives the spacing to offset each line.
 * \param includeChildren should be true to output the children's prototypes.
 * \return The string including the comments and prototypes with linefeeds and semicolons.
 */
std::string ProtocolStructure::getEncodeFunctionPrototype(const std::string& spacing, bool includeChildren) const
{
    std::string output;

    // Only the C language needs this. C++ declares the prototype within the class
    if(includeChildren && (support.language == ProtocolSupport::c_language))
    {
        for(std::size_t i = 0; i < encodables.size(); i++)
        {
            // Is this encodable a structure?
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(structure == nullptr)
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += structure->getEncodeFunctionPrototype(spacing, includeChildren);
        }
        ProtocolFile::makeLineSeparator(output);

    }

    // My encoding and decoding prototypes in the header file
    output += spacing + "//! Encode a " + typeName + " into a byte array\n";
    output += spacing + getEncodeFunctionSignature(false) + ";\n";

    return output;

}// ProtocolStructure::getEncodeFunctionPrototype


/*!
 * Return the string that gives the function used to encode this structure and
 * all its children to a simple byte array.
 * \param isBigEndian should be true for big endian encoding.
 * \param includeChildren should be true to output the children's functions.
 * \return The string including the comments and code with linefeeds and semicolons.
 */
std::string ProtocolStructure::getEncodeFunctionBody(bool isBigEndian, bool includeChildren) const
{
    std::string output;

    if(includeChildren)
    {
        for(std::size_t i = 0; i < encodables.size(); i++)
        {
            // Is this encodable a structure?
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(structure == nullptr)
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += structure->getEncodeFunctionBody(isBigEndian, includeChildren);
        }
        ProtocolFile::makeLineSeparator(output);
    }

    // My encoding function
    output += "/*!\n";
    output += " * \\brief Encode a " + typeName + " into a byte array\n";
    output += " *\n";
    output += ProtocolParser::outputLongComment(" * ", comment) + "\n";
    output += " * \\param _pg_data points to the byte array to add encoded data to\n";
    output += " * \\param _pg_bytecount points to the starting location in the byte array, and will be incremented by the number of encoded bytes.\n";
    if((support.language == ProtocolSupport::c_language) && (getNumberOfEncodeParameters() > 0))
        output += " * \\param _pg_user is the data to encode in the byte array\n";
    output += " */\n";

    output += getEncodeFunctionSignature(true) + "\n";
    output += "{\n";

    output += TAB_IN + "int _pg_byteindex = *_pg_bytecount;\n";

    if(usestempencodebitfields)
        output += TAB_IN + "unsigned int _pg_tempbitfield = 0;\n";

    if(usestempencodelongbitfields)
        output += TAB_IN + "uint64_t _pg_templongbitfield = 0;\n";

    if(numbitfieldgroupbytes > 0)
    {
        output += TAB_IN + "int _pg_bitfieldindex = 0;\n";
        output += TAB_IN + "uint8_t _pg_bitfieldbytes[" + std::to_string(numbitfieldgroupbytes) + "];\n";
    }

    if(needsEncodeIterator)
        output += TAB_IN + "unsigned _pg_i = 0;\n";

    if(needs2ndEncodeIterator)
        output += TAB_IN + "unsigned _pg_j = 0;\n";

    int bitcount = 0;
    for(std::size_t i = 0; i < encodables.size(); i++)
    {
        ProtocolFile::makeLineSeparator(output);
        output += encodables[i]->getEncodeString(isBigEndian, &bitcount, true);
    }

    ProtocolFile::makeLineSeparator(output);
    output += TAB_IN + "*_pg_bytecount = _pg_byteindex;\n";
    output += "\n";

    if(support.language == ProtocolSupport::c_language)
        output += "}// encode" + typeName + "\n";
    else
        output += "}// " + typeName + "::encode\n";

    return output;

}// ProtocolStructure::getEncodeFunctionBody


/*!
 * Get the signature of the function that decodes this structure.
 * \param insource should be true to indicate this signature is in source code
 * \return the signature of the decode function.
 */
std::string ProtocolStructure::getDecodeFunctionSignature(bool insource) const
{
    std::string output;

    if(support.language == ProtocolSupport::c_language)
    {
        std::string pg;

        if(insource)
            pg = "_pg_";

        if(getNumberOfDecodeParameters() > 0)
        {
            output = "int decode" + typeName + "(const uint8_t* " + pg + "data, int* " + pg + "bytecount, " + structName + "* " + pg + "user)";
        }
        else
        {
            output = "int decode" + typeName + "(const uint8_t* " + pg + "data, int* " + pg + "bytecount)";
        }
    }
    else
    {
        // For C++ these functions are within the class namespace and they
        // reference their own members.
        if(insource)
            output = "bool " + typeName + "::decode(const uint8_t* _pg_data, int* _pg_bytecount)";
        else
            output = "bool decode(const uint8_t* data, int* bytecount)";
    }

    return output;

}// ProtocolStructure::getDecodeFunctionSignature


/*!
 * Return the string that gives the prototype of the functions used to decode
 * the structure. The encoding is to a simple byte array.
 * \param spacing gives the spacing to offset each line.
 * \param includeChildren should be true to output the children's prototypes.
 * \return The string including the comments and prototypes with linefeeds and semicolons.
 */
std::string ProtocolStructure::getDecodeFunctionPrototype(const std::string& spacing, bool includeChildren) const
{
    std::string output;

    // Only the C language needs this. C++ declares the prototype within the class
    if(includeChildren && (support.language == ProtocolSupport::c_language))
    {
        for(std::size_t i = 0; i < encodables.size(); i++)
        {
            // Is this encodable a structure?
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(structure == nullptr)
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += structure->getDecodeFunctionPrototype(spacing, includeChildren);
        }
        ProtocolFile::makeLineSeparator(output);
    }

    output += spacing + "//! Decode a " + typeName + " from a byte array\n";
    output += spacing + getDecodeFunctionSignature(false) + ";\n";

    return output;

}// ProtocolStructure::getDecodeFunctionPrototype


/*!
 * Return the string that gives the function used to decode this structure.
 * The decoding is from a simple byte array.
 * \param isBigEndian should be true for big endian decoding.
 * \param includeChildren should be true to output the children's functions.
 * \return The string including the comments and code with linefeeds and semicolons.
 */
std::string ProtocolStructure::getDecodeFunctionBody(bool isBigEndian, bool includeChildren) const
{
    std::string output;

    if(includeChildren)
    {
        for(std::size_t i = 0; i < encodables.size(); i++)
        {
            // Is this encodable a structure?
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(structure == nullptr)
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += structure->getDecodeFunctionBody(isBigEndian);
        }
        ProtocolFile::makeLineSeparator(output);
    }

    output += "/*!\n";
    output += " * \\brief Decode a " + typeName + " from a byte array\n";
    output += " *\n";
    output += ProtocolParser::outputLongComment(" * ", comment) + "\n";
    output += " * \\param _pg_data points to the byte array to decoded data from\n";
    output += " * \\param _pg_bytecount points to the starting location in the byte array, and will be incremented by the number of bytes decoded\n";
    if((support.language == ProtocolSupport::c_language) && (getNumberOfDecodeParameters() > 0))
        output += " * \\param _pg_user is the data to decode from the byte array\n";

    output += " * \\return " + getReturnCode(true) + " if the data are decoded, else " + getReturnCode(false) + ".\n";
    output += " */\n";
    output += getDecodeFunctionSignature(true) + "\n";
    output += "{\n";

    output += TAB_IN + "int _pg_byteindex = *_pg_bytecount;\n";

    if(usestempdecodebitfields)
        output += TAB_IN + "unsigned int _pg_tempbitfield = 0;\n";

    if(usestempdecodelongbitfields)
        output += TAB_IN + "uint64_t _pg_templongbitfield = 0;\n";

    if(numbitfieldgroupbytes > 0)
    {
        output += TAB_IN + "int _pg_bitfieldindex = 0;\n";
        output += TAB_IN + "uint8_t _pg_bitfieldbytes[" + std::to_string(numbitfieldgroupbytes) + "];\n";
    }

    if(needsDecodeIterator)
        output += TAB_IN + "unsigned _pg_i = 0;\n";

    if(needs2ndDecodeIterator)
        output += TAB_IN + "unsigned _pg_j = 0;\n";

    int bitcount = 0;
    for(std::size_t i = 0; i < encodables.size(); i++)
    {
        ProtocolFile::makeLineSeparator(output);
        output += encodables[i]->getDecodeString(isBigEndian, &bitcount, true);
    }

    ProtocolFile::makeLineSeparator(output);
    output += TAB_IN + "*_pg_bytecount = _pg_byteindex;\n\n";
    output += TAB_IN + "return " + getReturnCode(true) + ";\n";
    output += "\n";

    if(support.language == ProtocolSupport::c_language)
        output += "}// decode" + typeName + "\n";
    else
        output += "}// " + typeName + "::decode\n";

    return output;

}// ProtocolStructure::getDecodeFunctionBody


/*!
 * Get the signature of the function that sets initial values of this structure.
 * This is just the constructor for C++
 * \param insource should be true to indicate this signature is in source code.
 * \return the signature of the set to initial value function.
 */
std::string ProtocolStructure::getSetToInitialValueFunctionSignature(bool insource) const
{
    std::string output;

    if(support.language == ProtocolSupport::c_language)
    {
        if(getNumberInMemory() > 0)
        {
            if(insource)
                output = "void init" + typeName + "(" + structName + "* _pg_user)";
            else
                output = "void init" + typeName + "(" + structName + "* user)";
        }
        else
        {
            output = "void init" + typeName + "(void)";
        }
    }
    else
    {
        // For C++ this function is the constructor
        if(insource)
            output = typeName + "::" + typeName + "(void)";
        else
            output = typeName + "(void)";
    }

    return output;

}// ProtocolStructure::getSetToInitialValueFunctionSignature


/*!
 * Return the string that gives the prototypes of the functions used to set
 * this structure to initial values.
 * \param spacing gives the spacing to offset each line.
 * \param includeChildren should be true to output the children's functions.
 * \return The string including the comments and code with linefeeds and semicolons
 */
std::string ProtocolStructure::getSetToInitialValueFunctionPrototype(const std::string& spacing, bool includeChildren) const
{
    std::string output;

    // C++ always has init (constructor) functions, but not C
    if(!hasInit() && (support.language == ProtocolSupport::c_language))
        return output;

    // Go get any children structures set to initial functions
    if(includeChildren && (support.language == ProtocolSupport::c_language))
    {
        for(std::size_t i = 0; i < encodables.size(); i++)
        {
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(!structure)
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += structure->getSetToInitialValueFunctionPrototype(spacing, includeChildren);
        }
        ProtocolFile::makeLineSeparator(output);
    }

    // My set to initial values function
    if(support.language == ProtocolSupport::c_language)
        output += spacing + "//! Set a " + typeName + " to initial values\n";
    else
        output += spacing + "//! Construct a " + typeName + "\n";

    output += spacing + getSetToInitialValueFunctionSignature(false) + ";\n";

    return output;

}// ProtocolStructure::getSetToInitialValueFunctionPrototype


/*!
 * Return the string that gives the function used to this structure to initial
 * values. This is NOT the call that sets this structure to initial values, this
 * is the actual code that is in that call.
 * \param includeChildren should be true to output the children's functions.
 * \return The string including the comments and code with linefeeds and semicolons
 */
std::string ProtocolStructure::getSetToInitialValueFunctionBody(bool includeChildren) const
{
    std::string output;

    // C++ always has init (constructor) functions, but not C
    if(!hasInit() && (support.language == ProtocolSupport::c_language))
        return output;

    // Go get any children structures set to initial functions
    if(includeChildren)
    {
        for(std::size_t i = 0; i < encodables.size(); i++)
        {
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(!structure)
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += structure->getSetToInitialValueFunctionBody(includeChildren);
        }
        ProtocolFile::makeLineSeparator(output);
    }

    if(support.language == ProtocolSupport::c_language)
    {
        // My set to initial values function
        output += "/*!\n";
        output += " * \\brief Set a " + typeName + " to initial values.\n";
        output += " *\n";
        output += " * Set a " + typeName + " to initial values. Not all fields are set,\n";
        output += " * only those which the protocol specifies.\n";
        output += " * \\param _pg_user is the structure whose data are set to initial values\n";
        output += " */\n";
        output += getSetToInitialValueFunctionSignature(true) + "\n";
        output += "{\n";

        if(needsInitIterator)
            output += TAB_IN + "int _pg_i = 0;\n";

        if(needs2ndInitIterator)
            output += TAB_IN + "int _pg_j = 0;\n";

        for(std::size_t i = 0; i < encodables.size(); i++)
        {
            /// TODO: change this to zeroize all fields
            ProtocolFile::makeLineSeparator(output);
            output += encodables[i]->getSetInitialValueString(true);
        }

        ProtocolFile::makeLineSeparator(output);
        output += "}// init" + typeName + "\n";

    }// If the C language output
    else
    {
        // Set to initial values is just the constructor. We initialize every
        // member that is not itself another class (they take care of themselves).
        std::string initializerlist;

        for(std::size_t i = 0; i < encodables.size(); i++)
        {
            // Structures (classes really) take care of themselves
            if(!encodables.at(i)->isPrimitive() || encodables.at(i)->isNotInMemory())
                continue;

            initializerlist += encodables.at(i)->getSetInitialValueString(true);
        }

        // Get rid of the comma on the last member of the initializer list
        if(endsWith(initializerlist, ",\n"))
            initializerlist = initializerlist.erase(initializerlist.rfind(",\n")) + "\n";

        if(initializerlist.empty())
            initializerlist = "\n";
        else
            initializerlist = " :\n" + initializerlist;

        output += "/*!\n";
        output += " * Construct a " + typeName + "\n";
        output += " */\n";
        output += getSetToInitialValueFunctionSignature(true) + initializerlist;
        output += "{\n";
        output += "}// " + typeName + "::" + typeName + "\n";

    }// else if C++ language

    return output;

}// ProtocolStructure::getSetToInitialValueFunctionBody


/*!
 * Get the signature of the verify function.
 * \param insource should be true to indicate this signature is in source code.
 * \return the signature of the verify function.
 */
std::string ProtocolStructure::getVerifyFunctionSignature(bool insource) const
{
    if(support.language == ProtocolSupport::c_language)
    {
        if(insource)
            return "int verify" + typeName + "(" + structName + "* _pg_user)";
        else
            return "int verify" + typeName + "(" + structName + "* user)";
    }
    else
    {
        if(insource)
            return "bool " + typeName + "::verify(void)";
        else
            return "bool verify(void)";
    }

}// ProtocolStructure::getVerifyFunctionSignature


/*!
 * Return the string that gives the prototypes of the functions used to verify
 * the data in this.
 * \param spacing gives the spacing to offset each line.
 * \param includeChildren should be true to output the children's functions.
 * \return The string including the comments and code with linefeeds and semicolons
 */
std::string ProtocolStructure::getVerifyFunctionPrototype(const std::string& spacing, bool includeChildren) const
{
    std::string output;

    // Only if we have verify functions
    if(!hasVerify())
        return output;

    // Go get any children structures verify functions
    if(includeChildren && (support.language == ProtocolSupport::c_language))
    {
        for(std::size_t i = 0; i < encodables.size(); i++)
        {
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(!structure)
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += structure->getVerifyFunctionPrototype(spacing, includeChildren);
        }
        ProtocolFile::makeLineSeparator(output);
    }

    // My verify values function
    output += spacing + "//! Verify a " + typeName + " has acceptable values\n";
    output += spacing + getVerifyFunctionSignature(false) + ";\n";

    return output;

}// ProtocolStructure::getVerifyFunctionPrototype


/*!
 * Return the string that gives the function used to verify the data in this
 * structure. This is NOT the call that verifies this structure, this
 * is the actual code that is in that call.
 * \param includeChildren should be true to output the children's functions.
 * \return The string including the comments and code with linefeeds and semicolons
 */
std::string ProtocolStructure::getVerifyFunctionBody(bool includeChildren) const
{
    std::string output;

    // Only if we have verify functions
    if(!hasVerify())
        return output;

    // Go get any children structures verify functions
    if(includeChildren)
    {
        for(std::size_t i = 0; i < encodables.size(); i++)
        {
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(!structure)
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += structure->getVerifyFunctionBody(includeChildren);
        }
        ProtocolFile::makeLineSeparator(output);
    }

    // My set to initial values function
    output += "/*!\n";
    output += " * \\brief Verify a " + typeName + " has acceptable values.\n";
    output += " *\n";
    output += " * Verify a " + typeName + " has acceptable values. Not all fields are\n";
    output += " * verified, only those which the protocol specifies. Fields which are outside\n";
    output += " * the allowable range are changed to the maximum or minimum allowable value. \n";

    if(support.language == ProtocolSupport::c_language)
    {
        output += " * \\param _pg_user is the structure whose data are verified\n";
        output += " * \\return 1 if all verifiable data where valid, else 0 if data had to be corrected\n";
        output += " */\n";
        output += getVerifyFunctionSignature(true) + "\n";
        output += "{\n";
        output += TAB_IN + "int _pg_good = 1;\n";
    }
    else
    {
        output += " * \\return true if all verifiable data where valid, else false if data had to be corrected\n";
        output += " */\n";
        output += getVerifyFunctionSignature(true) + "\n";
        output += "{\n";
        output += TAB_IN + "bool _pg_good = true;\n";
    }

    if(needsVerifyIterator)
        output += TAB_IN + "unsigned _pg_i = 0;\n";

    if(needs2ndVerifyIterator)
        output += TAB_IN + "unsigned _pg_j = 0;\n";

    for(std::size_t i = 0; i < encodables.size(); i++)
    {
        ProtocolFile::makeLineSeparator(output);
        output += encodables[i]->getVerifyString();
    }

    ProtocolFile::makeLineSeparator(output);
    output += TAB_IN + "return _pg_good;\n";
    output += "\n";
    if(support.language == ProtocolSupport::c_language)
        output += "}// verify" + typeName + "\n";
    else
        output += "}// " + typeName + "::verify\n";

    return output;

}// ProtocolStructure::getVerifyFunctionBody


/*!
 * Get the signature of the comparison function.
 * \param insource should be true to indicate this signature is in source code.
 * \return the signature of the comparison function.
 */
std::string ProtocolStructure::getComparisonFunctionSignature(bool insource) const
{
    if(support.language == ProtocolSupport::c_language)
    {
        if(insource)
            return "std::string compare" + typeName + "(const std::string& _pg_prename, const " + structName + "* _pg_user1, const " + structName + "* _pg_user2)";
        else
            return "std::string compare" + typeName + "(const std::string& prename, const " + structName + "* user1, const " + structName + "* user2)";
    }
    else
    {
        if(insource)
            return "std::string " + typeName + "::compare(const std::string& _pg_prename, const " + structName + "* _pg_user) const";
        else
            return "std::string compare(const std::string& prename, const " + structName + "* user) const";
    }

}// ProtocolStructure::getComparisonFunctionSignature


/*!
 * Return the string that gives the prototype of the function used to compare this structure
 * \param spacing gives the spacing to offset each line.
 * \param includeChildren should be true to include the function prototypes of the children structures of this structure
 * \return the function prototype string, which may be empty
 */
std::string ProtocolStructure::getComparisonFunctionPrototype(const std::string& spacing, bool includeChildren) const
{
    std::string output;

    // We must have parameters that we decode to do a compare
    if(!compare || (getNumberOfDecodeParameters() == 0))
        return output;

    // Go get any children structures compare functions
    if(includeChildren && (support.language == ProtocolSupport::c_language))
    {
        for(std::size_t i = 0; i < encodables.size(); i++)
        {
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(!structure)
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += structure->getComparisonFunctionPrototype(spacing, includeChildren);
        }
        ProtocolFile::makeLineSeparator(output);
    }


    // My comparison function
    output += spacing + "//! Compare two " + typeName + " and generate a report\n";
    output += spacing + getComparisonFunctionSignature(false) + ";\n";

    return output;

}// ProtocolStructure::getComparisonFunctionPrototype


/*!
 * Return the string that gives the function used to compare this structure
 * \param includeChildren should be true to include the function prototypes of
 *        the children structures of this structure
 * \return the function string, which may be empty
 */
std::string ProtocolStructure::getComparisonFunctionBody(bool includeChildren) const
{
    std::string output;

    // We must have parameters that we decode to do a compare
    if(!compare || (getNumberOfDecodeParameters() == 0))
        return output;

    // Go get any childrens structure compare functions
    if(includeChildren)
    {
        for(std::size_t i = 0; i < encodables.size(); i++)
        {
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(!structure)
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += structure->getComparisonFunctionBody(includeChildren);
        }
        ProtocolFile::makeLineSeparator(output);
    }

    // My compare function
    output += "/*!\n";

    if(support.language == ProtocolSupport::c_language)
    {
        output += " * Compare two " + typeName + " and generate a report of any differences.\n";
        output += " * \\param _pg_prename is prepended to the name of the data field in the comparison report\n";
        output += " * \\param _pg_user1 is the first data to compare\n";
        output += " * \\param _pg_user1 is the second data to compare\n";
        output += " * \\return a string describing any differences between _pg_user1 and _pg_user2. The string will be empty if there are no differences\n";
    }
    else
    {
        output += " * Compare this " + typeName + " with another " + typeName + " and generate a report of any differences.\n";
        output += " * \\param _pg_prename is prepended to the name of the data field in the comparison report\n";
        output += " * \\param _pg_user is the data to compare\n";
        output += " * \\return a string describing any differences between this " + typeName + " and `_pg_user`. The string will be empty if there are no differences\n";
    }
    output += " */\n";
    output += getComparisonFunctionSignature(true) + "\n";
    output += "{\n";
    output += TAB_IN + "std::string _pg_report;\n";

    if(needsDecodeIterator)
        output += TAB_IN + "unsigned _pg_i = 0;\n";

    if(needs2ndDecodeIterator)
        output += TAB_IN + "unsigned _pg_j = 0;\n";

    for(std::size_t i = 0; i < encodables.size(); i++)
    {
        ProtocolFile::makeLineSeparator(output);
        output += encodables[i]->getComparisonString();
    }

    ProtocolFile::makeLineSeparator(output);
    output += TAB_IN + "return _pg_report;\n";
    output += "\n";
    if(support.language == ProtocolSupport::c_language)
        output += "}// compare" + typeName + "\n";
    else
        output += "}// " + typeName + "::compare\n";

    return output;

}// ProtocolStructure::getComparisonFunctionBody


/*!
 * Get the signature of the textPrint function.
 * \param insource should be true to indicate this signature is in source code.
 * \return the signature of the comparison function.
 */
std::string ProtocolStructure::getTextPrintFunctionSignature(bool insource) const
{
    if(support.language == ProtocolSupport::c_language)
    {
        if(insource)
            return "std::string textPrint" + typeName + "(const std::string& _pg_prename, const " + structName + "* _pg_user)";
        else
            return "std::string textPrint" + typeName + "(const std::string& prename, const " + structName + "* user)";
    }
    else
    {
        if(insource)
            return "std::string " + typeName + "::textPrint(const std::string& _pg_prename) const";
        else
            return "std::string textPrint(const std::string& prename) const";
    }

}// ProtocolStructure::getTextPrintFunctionSignature


/*!
 * Return the string that gives the prototype of the function used to text print this structure
 * \param spacing gives the spacing to offset each line.
 * \param includeChildren should be true to include the function prototypes of the children of this structure.
 * \return the function prototype string, which may be empty
 */
std::string ProtocolStructure::getTextPrintFunctionPrototype(const std::string& spacing, bool includeChildren) const
{
    std::string output;

    // We must have parameters that we decode to do a print out
    if(!print || (getNumberOfDecodeParameters() == 0))
        return output;

    // Go get any children structures textPrint functions
    if(includeChildren && (support.language == ProtocolSupport::c_language))
    {
        for(std::size_t i = 0; i < encodables.size(); i++)
        {
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(!structure)
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += structure->getTextPrintFunctionPrototype(spacing, includeChildren);
        }
        ProtocolFile::makeLineSeparator(output);
    }

    // My textPrint function
    output += spacing + "//! Generate a string that describes the contents of a " + typeName + "\n";
    output += spacing + getTextPrintFunctionSignature(false) + ";\n";

    return output;

}// ProtocolStructure::getTextPrintFunctionPrototype


/*!
 * Return the string that gives the function used to compare this structure
 * \param includeChildren should be true to include the function prototypes of
 *        the children structures of this structure
 * \return the function string, which may be empty
 */
std::string ProtocolStructure::getTextPrintFunctionBody(bool includeChildren) const
{
    std::string output;

    // We must have parameters that we decode to do a print out
    if(!print || (getNumberOfDecodeParameters() == 0))
        return output;

    // Go get any childrens structure textPrint functions
    if(includeChildren)
    {
        for(std::size_t i = 0; i < encodables.size(); i++)
        {
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(!structure)
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += structure->getTextPrintFunctionBody(includeChildren);
        }
        ProtocolFile::makeLineSeparator(output);
    }

    // My textPrint function
    output += "/*!\n";
    output += " * Generate a string that describes the contents of a " + typeName + "\n";
    output += " * \\param _pg_prename is prepended to the name of the data field in the report\n";
    if(support.language == ProtocolSupport::c_language)
        output += " * \\param _pg_user is the structure to report\n";
    output += " * \\return a string containing a report of the contents of user\n";
    output += " */\n";
    output += getTextPrintFunctionSignature(true) + "\n";
    output += "{\n";
    output += TAB_IN + "std::string _pg_report;\n";

    if(needsDecodeIterator)
        output += TAB_IN + "unsigned _pg_i = 0;\n";

    if(needs2ndDecodeIterator)
        output += TAB_IN + "unsigned _pg_j = 0;\n";

    for(std::size_t i = 0; i < encodables.size(); i++)
    {
        ProtocolFile::makeLineSeparator(output);
        output += encodables[i]->getTextPrintString();
    }

    ProtocolFile::makeLineSeparator(output);
    output += TAB_IN + "return _pg_report;\n";
    output += "\n";
    if(support.language == ProtocolSupport::c_language)
        output += "}// textPrint" + typeName + "\n";
    else
        output += "}// " + typeName + "::textPrint\n";

    return output;

}// ProtocolStructure::getTextPrintFunctionString


/*!
 * Get the signature of the textRead function.
 * \param insource should be true to indicate this signature is in source code.
 * \return the signature of the comparison function.
 */
std::string ProtocolStructure::getTextReadFunctionSignature(bool insource) const
{
    if(support.language == ProtocolSupport::c_language)
    {
        if(insource)
            return "int textRead" + typeName + "(const std::string& _pg_prename, const std::string& _pg_source, " + structName + "* _pg_user)";
        else
            return "int textRead" + typeName + "(const std::string& prename, const std::string& source, " + structName + "* user)";
    }
    else
    {
        if(insource)
            return "int " + typeName + "::textRead(const std::string& _pg_prename, const std::string& _pg_source)";
        else
            return "int textRead(const std::string& prename, const std::string& source)";
    }

}// ProtocolStructure::getTextReadFunctionSignature


/*!
 * Return the string that gives the prototype of the function used to read this
 * structure from text
 * \param spacing gives the spacing to offset each line.
 * \param includeChildren should be true to include the function prototypes of
 *        the children structures of this structure
 * \return the function prototype string, which may be empty
 */
std::string ProtocolStructure::getTextReadFunctionPrototype(const std::string& spacing, bool includeChildren) const
{
    std::string output;

    // We must have parameters that we decode to do a read
    if(!print || (getNumberOfDecodeParameters() == 0))
        return output;

    // Go get any children structures textRead functions
    if(includeChildren && (support.language == ProtocolSupport::c_language))
    {
        for(std::size_t i = 0; i < encodables.size(); i++)
        {
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(!structure)
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += structure->getTextReadFunctionPrototype(spacing, includeChildren);
        }
        ProtocolFile::makeLineSeparator(output);
    }

    // My textRead function
    output += spacing + "//! Read the contents of a " + typeName + " from text\n";
    output += spacing + getTextReadFunctionSignature(false) + ";\n";

    return output;

}// ProtocolStructure::getTextReadFunctionPrototype


/*!
 * Get the string that gives the function used to read this structure from text
 * \param includeChildren should be true to include the function prototypes of
 *        the children structures of this structure
 * \return the function string, which may be empty
 */
std::string ProtocolStructure::getTextReadFunctionBody(bool includeChildren) const
{
    std::string output;

    // We must have parameters that we decode to do a read
    if(!print || (getNumberOfDecodeParameters() == 0))
        return output;

    // Go get any childrens structure textRead functions
    if(includeChildren)
    {
        for(std::size_t i = 0; i < encodables.size(); i++)
        {
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(!structure)
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += structure->getTextReadFunctionBody(includeChildren);
        }
        ProtocolFile::makeLineSeparator(output);
    }

    // My textRead function
    output += "/*!\n";
    output += " * Read the contents of a " + typeName + " structure from text\n";
    output += " * \\param _pg_prename is prepended to the name of the data field to form the text key\n";
    output += " * \\param _pg_source is text to search to find the data field keys\n";
    if(support.language == ProtocolSupport::c_language)
        output += " * \\param _pg_user receives any data read from the text source\n";
    output += " * \\return The number of fields that were read from the text source\n";
    output += " */\n";
    output += getTextReadFunctionSignature(true) + "\n";
    output += "{\n";
    output += TAB_IN + "std::string _pg_text;\n";
    output += TAB_IN + "int _pg_fieldcount = 0;\n";

    if(needsDecodeIterator)
        output += TAB_IN + "unsigned _pg_i = 0;\n";

    if(needs2ndDecodeIterator)
        output += TAB_IN + "unsigned _pg_j = 0;\n";

    for(std::size_t i = 0; i < encodables.size(); i++)
    {
        ProtocolFile::makeLineSeparator(output);
        output += encodables[i]->getTextReadString();
    }

    ProtocolFile::makeLineSeparator(output);
    output += TAB_IN + "return _pg_fieldcount;\n";
    output += "\n";
    if(support.language == ProtocolSupport::c_language)
        output += "}// textRead" + typeName + "\n";
    else
        output += "}// " + typeName + "::textRead\n";

    return output;

}// ProtocolStructure::getTextReadFunctionString


/*!
 * Get the signature of the mapEncode function.
 * \param insource should be true to indicate this signature is in source code.
 * \return the signature of the comparison function.
 */
std::string ProtocolStructure::getMapEncodeFunctionSignature(bool insource) const
{
    if(support.language == ProtocolSupport::c_language)
    {
        if(insource)
            return "void mapEncode" + typeName + "(const QString& _pg_prename, QVariantMap& _pg_map, const " + structName + "* _pg_user)";
        else
            return "void mapEncode" + typeName + "(const QString& prename, QVariantMap& map, const " + structName + "* user)";
    }
    else
    {
        if(insource)
            return "void " + typeName + "::mapEncode(const QString& _pg_prename, QVariantMap& _pg_map) const";
        else
            return "void mapEncode(const QString& prename, QVariantMap& map) const";
    }

}// ProtocolStructure::getMapEncodeFunctionSignature


/*!
 * Return the string that gives the prototype of the function used to encode
 * this structure to a map
 * \param spacing gives the spacing to offset each line.
 * \param includeChildren should be true to include the function prototypes
 *        of the child structures
 * \return the function prototype string, which may be empty
 */
std::string ProtocolStructure::getMapEncodeFunctionPrototype(const std::string& spacing, bool includeChildren) const
{
    std::string output;

    // Only the C language has to create the prototype functions, in C++ this is part of the class declaration
    if(!mapEncode || (getNumberOfDecodeParameters() == 0))
        return output;

    if(includeChildren && (support.language == ProtocolSupport::c_language))
    {
        for(std::size_t i = 0; i < encodables.size();  i++)
        {
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(!structure)
                continue;

            ProtocolFile::makeLineSeparator(output);

            output += structure->getMapEncodeFunctionPrototype(spacing, includeChildren);
        }

        ProtocolFile::makeLineSeparator(output);
    }

    // My mapEncode function
    output += spacing + "//! Encode the contents of a " + typeName + " to a string Key:Value map\n";
    output += spacing + getMapEncodeFunctionSignature(false) + ";\n";

    return output;

}// ProtocolStructure::getMapEncodeFunctionPrototype


/*!
 * Return the string that gives the function used to encode this structure to a map
 * \param includeChildren should be true to include the functions of the child structures
 * \return the function string, which may be empty
 */
std::string ProtocolStructure::getMapEncodeFunctionBody(bool includeChildren) const
{
    std::string output;

    if(!mapEncode || (getNumberOfDecodeParameters() == 0))
        return output;

    if(includeChildren)
    {
        for(std::size_t i = 0; i < encodables.size(); i++)
        {
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if (!structure)
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += structure->getMapEncodeFunctionBody(includeChildren);
        }

        ProtocolFile::makeLineSeparator(output);
    }

    // My mapEncode function
    output += "/*!\n";
    output += " * Encode the contents of a " + typeName + " to a Key:Value string map\n";
    output += " * \\param _pg_prename is prepended to the key fields in the map\n";
    output += " * \\param _pg_map is a reference to the map\n";
    if(support.language == ProtocolSupport::c_language)
        output += " * \\param _pg_user is the structure to encode\n";
    output += " */\n";
    output += getMapEncodeFunctionSignature(true) + "\n";
    output += "{\n";
    output += TAB_IN + "QString key;\n";

    if(needsDecodeIterator)
        output += TAB_IN + "unsigned _pg_i = 0;\n";

    if(needs2ndDecodeIterator)
        output += TAB_IN + "unsigned _pg_j = 0;\n";

    for(std::size_t i = 0; i < encodables.size(); i++)
    {
        ProtocolFile::makeLineSeparator(output);
        output += encodables[i]->getMapEncodeString();
    }

    ProtocolFile::makeLineSeparator(output);

    if(support.language == ProtocolSupport::c_language)
        output += "}// mapEncode" + typeName + "\n";
    else
        output += "}// " + typeName + "::mapEncode\n";

    return output;

}// ProtocolStructure::getMapEncodeFunctionString


/*!
 * Get the signature of the mapDecode function.
 * \param insource should be true to indicate this signature is in source code.
 * \return the signature of the comparison function.
 */
std::string ProtocolStructure::getMapDecodeFunctionSignature(bool insource) const
{
    if(support.language == ProtocolSupport::c_language)
    {
        if(insource)
            return "void mapDecode" + typeName + "(const QString& _pg_prename, const QVariantMap& _pg_map, " + structName + "* _pg_user)";
        else
            return "void mapDecode" + typeName + "(const QString& prename, const QVariantMap& map, " + structName + "* user)";
    }
    else
    {
        if(insource)
            return "void " + typeName + "::mapDecode(const QString& _pg_prename, const QVariantMap& _pg_map)";
        else
            return "void mapDecode(const QString& prename, const QVariantMap& map)";
    }

}// ProtocolStructure::getMapDecodeFunctionSignature


/*!
 * Get the string that gives the prototype of the function used to decode this
 * structure from a map
 * \param spacing gives the spacing to offset each line.
 * \param includeChildren should be true to include the function prototypes of
 *        the children structures of this structure
 * \return the function string, which may be empty
 */
std::string ProtocolStructure::getMapDecodeFunctionPrototype(const std::string& spacing, bool includeChildren) const
{
    std::string output;

    if(!mapEncode || (getNumberOfDecodeParameters() == 0))
        return output;

    if(includeChildren && (support.language == ProtocolSupport::c_language))
    {
        for(std::size_t i = 0; i < encodables.size();  i++)
        {
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if(!structure)
                continue;

            ProtocolFile::makeLineSeparator(output);

            output += structure->getMapDecodeFunctionPrototype(spacing, includeChildren);
        }

        ProtocolFile::makeLineSeparator(output);
    }

    // My mapEncode function
    output += spacing + "//! Decode the contents of a " + typeName + " from a string Key:Value map\n";
    output += spacing + getMapDecodeFunctionSignature(false) + ";\n";

    return output;

}// ProtocolStructure::getMapDecodeeFunctionPrototype


/*!
 * Get the string that gives the function used to decode this structure from a map
 * \param includeChildren should be true to include the function prototypes of
 *        the children structures of this structure
 * \return the function string, which may be empty
 */
std::string ProtocolStructure::getMapDecodeFunctionBody(bool includeChildren) const
{
    std::string output;

    if(!mapEncode || (getNumberOfDecodeParameters() == 0))
        return output;

    if(includeChildren)
    {
        for(std::size_t i = 0; i < encodables.size(); i++)
        {
            ProtocolStructure* structure = dynamic_cast<ProtocolStructure*>(encodables.at(i));

            if (!structure)
                continue;

            ProtocolFile::makeLineSeparator(output);
            output += structure->getMapDecodeFunctionBody(includeChildren);
        }

        ProtocolFile::makeLineSeparator(output);
    }

    // My mapDecode function
    output += "/*!\n";
    output += " * Decode the contents of a " + typeName + " from a Key:Value string map\n";
    output += " * \\param _pg_prename is prepended to the key fields in the map\n";
    output += " * \\param _pg_map is a reference to the map\n";
    if(support.language == ProtocolSupport::c_language)
        output += " * \\param _pg_user is the structure to decode\n";
    output += " */\n";
    output += getMapDecodeFunctionSignature(true) + "\n";
    output += "{\n";
    output += TAB_IN + "QString key;\n";
    output += TAB_IN + "bool ok = false;\n";

    if(needsDecodeIterator)
        output += TAB_IN + "unsigned _pg_i = 0;\n";

    if(needs2ndDecodeIterator)
        output += TAB_IN + "unsigned _pg_j = 0;\n";

    for(std::size_t i = 0; i < encodables.size(); i++)
    {
        ProtocolFile::makeLineSeparator(output);
        output += encodables[i]->getMapDecodeString();
    }

    ProtocolFile::makeLineSeparator(output);

    if(support.language == ProtocolSupport::c_language)
        output += "}// mapDecode" + typeName + "\n";
    else
        output += "}// " + typeName + "::mapDecode\n";

    return output;

}// ProtocolStructure::getMapDecodeFunctionString


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
void ProtocolStructure::getDocumentationDetails(std::vector<int>& outline, std::string& startByte, std::vector<std::string>& bytes, std::vector<std::string>& names, std::vector<std::string>& encodings, std::vector<std::string>& repeats, std::vector<std::string>& comments) const
{
    std::string description;

    std::string maxEncodedLength = encodedLength.maxEncodedLength;

    // See if we can replace any enumeration names with values
    maxEncodedLength = parser->replaceEnumerationNameWithValue(maxEncodedLength);

    // The byte after this one
    std::string nextStartByte = EncodedLength::collapseLengthString(startByte + "+" + maxEncodedLength);

    // The length data
    if(maxEncodedLength.empty() || (maxEncodedLength.compare("1") == 0))
        bytes.push_back(startByte);
    else
    {
        std::string endByte = EncodedLength::subtractOneFromLengthString(nextStartByte);

        // The range of the data
        bytes.push_back(startByte + "..." + endByte);
    }

    // The name information
    outline.back() += 1;
    std::string outlineString = std::to_string(outline.at(0));
    for(std::size_t i = 1; i < outline.size(); i++)
        outlineString += "." + std::to_string(outline.at(i));
    outlineString += ")" + title;
    names.push_back(outlineString);

    // Encoding is blank for structures
    encodings.push_back(std::string());

    // The repeat/array column
    if(array.empty())
        repeats.push_back(std::string());
    else
        repeats.push_back(getRepeatsDocumentationDetails());

    // The commenting
    description += comment;

    if(!dependsOn.empty())
    {
        if(description.back() != '.')
            description += ".";

        if(dependsOnValue.empty())
            description += " Only included if " + dependsOn + " is non-zero.";
        else
        {
            if(dependsOnCompare.empty())
                description += " Only included if " + dependsOn + " equal to " + dependsOnValue + ".";
            else
                description += " Only included if " + dependsOn + " " + dependsOnCompare + " " + dependsOnValue + ".";
        }
    }

    if(description.empty())
        comments.push_back(std::string());
    else
        comments.push_back(description);

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
void ProtocolStructure::getSubDocumentationDetails(std::vector<int>& outline, std::string& startByte, std::vector<std::string>& bytes, std::vector<std::string>& names, std::vector<std::string>& encodings, std::vector<std::string>& repeats, std::vector<std::string>& comments) const
{
    outline.push_back(0);

    // Go get the sub-encodables
    for(std::size_t i = 0; i < encodables.size(); i++)
        encodables.at(i)->getDocumentationDetails(outline, startByte, bytes, names, encodings, repeats, comments);

    outline.pop_back();

}// ProtocolStructure::getSubDocumentationDetails

