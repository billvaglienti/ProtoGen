#include "encodable.h"
#include "protocolfield.h"
#include "protocolstructure.h"
#include "protocolcode.h"
#include "protocoldocumentation.h"
#include "protocolparser.h"

/*!
 * Constructor for encodable
 */
Encodable::Encodable(ProtocolParser* parse, const std::string& Parent, ProtocolSupport supported) :
    ProtocolDocumentation(parse, Parent, supported)
{
}


/*!
 * Check all names against C keyword and issue a warning if any of them match.
 * In addition the matching names will be updated to have a leading underscore
 */
void Encodable::checkAgainstKeywords(void)
{
    if(contains(keywords, name, true))
    {
        emitWarning("name matches C keyword, changed to _name");
        name = "_" + name;
    }

    if(contains(keywords, array, true))
    {
        emitWarning("array matches C keyword, changed to _array");
        array = "_" + array;
    }

    if(contains(keywords, variableArray, true))
    {
        emitWarning("variableArray matches C keyword, changed to _variableArray");
        variableArray = "_" + variableArray;
    }

    if(contains(keywords, array2d, true))
    {
        emitWarning("array2d matches C keyword, changed to _array2d");
        array2d = "_" + array2d;
    }

    if(contains(keywords, variable2dArray, true))
    {
        emitWarning("variable2dArray matches C keyword, changed to _variable2dArray");
        variable2dArray = "_" + variable2dArray;
    }

    if(contains(keywords, dependsOn, true))
    {
        emitWarning("dependsOn matches C keyword, changed to _dependsOn");
        dependsOn = "_" + dependsOn;
    }

    if(contains(keywords, dependsOnValue, true))
    {
        emitWarning("dependsOnValue matches C keyword, changed to _dependsOnValue");
        dependsOnValue = "_" + dependsOnValue;
    }

    if(contains(variablenames, name, true))
    {
        emitWarning("name matches ProtoGen variable, changed to _name");
        name = "_" + name;
    }

    if(contains(variablenames, array, true))
    {
        emitWarning("array matches ProtoGen variable, changed to _array");
        array = "_" + array;
    }

    if(contains(variablenames, variableArray, true))
    {
        emitWarning("variableArray matches ProtoGen variable, changed to _variableArray");
        variableArray = "_" + variableArray;
    }

    if(contains(variablenames, array2d, true))
    {
        emitWarning("array2d matches ProtoGen variable, changed to _array2d");
        array2d = "_" + array2d;
    }

    if(contains(variablenames, variable2dArray, true))
    {
        emitWarning("variable2dArray matches ProtoGen variable, changed to _variable2dArray");
        variable2dArray = "_" + variable2dArray;
    }

    if(contains(variablenames, dependsOn, true))
    {
        emitWarning("dependsOn matches ProtoGen variable, changed to _dependsOn");
        dependsOn = "_" + dependsOn;
    }

    if(contains(variablenames, dependsOnValue, true))
    {
        emitWarning("dependsOnValue matches ProtoGen variable, changed to _dependsOnValue");
        dependsOnValue = "_" + dependsOnValue;
    }

}// Encodable::checkAgainstKeywords


/*!
 * Reset all data to defaults
 */
void Encodable::clear(void)
{
    typeName.clear();
    name.clear();
    title.clear();
    comment.clear();
    array.clear();
    variableArray.clear();
    array2d.clear();
    variable2dArray.clear();
    encodedLength.clear();
    dependsOn.clear();
    dependsOnValue.clear();
    dependsOnCompare.clear();
}


/*!
 * Return the signature of this field in a encode function signature. The
 * string will start with ", " assuming this field is not the first part of
 * the function signature.
 * \return the string that provides this fields encode function signature
 */
std::string Encodable::getEncodeSignature(void) const
{
    if(isNotEncoded() || isNotInMemory() || isConstant())
        return "";
    else if(is2dArray())
        return ", const " + typeName + " " + name + "[" + array + "][" + array2d + "]";
    else if(isArray())
        return ", const " + typeName + " " + name + "[" + array + "]";
    else if(isPrimitive())
        return ", " + typeName + " " + name;
    else
        return ", const " + typeName + "* " + name;
}


/*!
 * Return the signature of this field in a decode function signature. The
 * string will start with ", " assuming this field is not the first part of
 * the function signature.
 * \return the string that provides this fields decode function signature
 */
std::string Encodable::getDecodeSignature(void) const
{
    if(isNotEncoded() || isNotInMemory())
        return "";
    else if(is2dArray())
        return ", " + typeName + " " + name + "[" + array + "][" + array2d + "]";
    else if(isArray())
        return ", " + typeName + " " + name + "[" + array + "]";
    else
        return ", " + typeName + "* " + name;
}


/*!
 * Return the string that documents this field as a encode function parameter.
 * The string starts with " * " and ends with a linefeed
 * \return the string that povides the parameter documentation
 */
std::string Encodable::getEncodeParameterComment(void) const
{
    if(isNotEncoded() || isNotInMemory() || isConstant())
        return "";
    else
        return " * \\param " + name + " is " + comment + "\n";
}


/*!
 * Return the string that documents this field as a decode function parameter.
 * The string starts with " * " and ends with a linefeed
 * \return the string that povides the parameter documentation
 */
std::string Encodable::getDecodeParameterComment(void) const
{
    if(isNotEncoded() || isNotInMemory())
        return "";
    else
        return " * \\param " + name + " receives " + comment + "\n";
}


/*!
 * Get a positive or negative return code string, which is language specific
 * \param positive should be true for "1" or "true", else "0", or "false"
 * \return The string in code that is the return.
 */
std::string Encodable::getReturnCode(bool positive) const
{
    if(positive)
    {
        if(support.language == ProtocolSupport::c_language)
            return "1";
        else
            return "true";
    }
    else
    {
        if(support.language == ProtocolSupport::c_language)
            return "0";
        else
            return "false";
    }

}// Encodable::getReturnCode


/*!
 * Get the string which accessses this field in code in a encoding context.
 * \param isStructureMember true if this field is in the scope of a containing structure.
 * \return The string that accesses this field in code for reading.
 */
std::string Encodable::getEncodeFieldAccess(bool isStructureMember) const
{
    return getEncodeFieldAccess(isStructureMember, name);

}// ProtocolField::getEncodeFieldAccess


/*!
 * Get the string which accessses this field in code in a encoding context.
 * \param isStructureMember true if this field is in the scope of a containing structure.
 * \param variable is the name of the variable to be accessed.
 * \return The string that accesses the variable in code for reading.
 */
std::string Encodable::getEncodeFieldAccess(bool isStructureMember, const std::string& variable) const
{
    std::string access;

    // How we are going to access the field
    if(isStructureMember)
    {
        if(support.language == ProtocolSupport::c_language)
            access = "_pg_user->" + variable; // Access via structure pointer
        else
            access = variable;                // Access via implicit class reference
    }
    else
        access = variable;                    // Access via parameter

    // If the variable we are tyring to access is ourselves (i.e. not dependsOn
    // or variableArray, etc.) then we need to apply array access rules also.
    if(variable == name)
    {
        if(isArray() && !isString())
        {
            access += "[_pg_i]";
            if(is2dArray())
                access += "[_pg_j]";
        }

        // If we are a structure, and the language is C, we need the address of
        // the structure, even for encoding. Note however that if we are a
        // parameter we are already a pointer (because we never pass structures
        // by value).
        if(!isPrimitive() && (support.language == ProtocolSupport::c_language) && (isStructureMember || isArray()))
            access = "&" + access;
    }

    return access;

}// Encodable::getEncodeFieldAccess


/*!
 * Get the string which accessses this field in code in a decoding context.
 * \param isStructureMember true if this field is in the scope of a containing structure.
 * \return The string that accesses this field in code for writing.
 */
std::string Encodable::getDecodeFieldAccess(bool isStructureMember) const
{
    return getDecodeFieldAccess(isStructureMember, name);
}


/*!
 * Get the string which accessses this field in code in a decoding context.
 * \param isStructureMember true if this field is in the scope of a containing structure.
 * \param variable is the name of the variable to be accessed.
 * \return The string that accesses this field in code for writing.
 */
std::string Encodable::getDecodeFieldAccess(bool isStructureMember, const std::string& variable) const
{
    std::string access;

    if(isStructureMember)
    {
        if(support.language == ProtocolSupport::c_language)
            access = "_pg_user->" + variable; // Access via structure pointer
        else
            access = variable;                // Access via implicit class reference

        if(variable == name)
        {
            // Apply array access rules also, strings are left alone, they are already pointers
            if(isArray() && !isString())
            {
                access += "[_pg_i]";          // Array de-reference
                if(is2dArray())
                    access += "[_pg_j]";

            }

            // If we are a structure, and the language is C, we need the address of the structure.
            if(!isPrimitive() && (support.language == ProtocolSupport::c_language))
                access = "&" + access;
        }
    }
    else
    {
        if(variable == name)
        {
            if(isString())
                access = variable;                // Access via string pointer
            else if(isArray())
            {
                access = variable + "[_pg_i]";    // Array de-reference
                if(is2dArray())
                    access += "[_pg_j]";

                // If we are a structure, and the language is C, we need the address of the structure.
                if(!isPrimitive() && (support.language == ProtocolSupport::c_language))
                    access = "&" + access;
            }
            else if(!isPrimitive())
                access = variable;
            else
                access = "(*" + variable + ")";   // Access via parameter pointer
        }
        else
            access = "(*" + variable + ")";       // Access via parameter pointer
    }

    return access;

}// Encodable::getDecodeFieldAccess


/*!
 * Get the code that performs array iteration, in a encode context
 * \param spacing is the spacing that begins the first array iteration line
 * \param isStructureMember should be true if variable array limits are members of a structure
 * \return the code for array iteration, which may be empty
 */
std::string Encodable::getEncodeArrayIterationCode(const std::string& spacing, bool isStructureMember) const
{
    std::string output;

    if(isArray())
    {
        if(variableArray.empty())
        {
            output += spacing + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";
        }
        else
        {
            output += spacing + "for(_pg_i = 0; _pg_i < (unsigned)" + getEncodeFieldAccess(isStructureMember, variableArray) + " && _pg_i < " + array + "; _pg_i++)\n";
        }

        if(is2dArray())
        {
            if(variable2dArray.empty())
            {
                output += spacing + TAB_IN + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";
            }
            else
            {
                output += spacing + TAB_IN + "for(_pg_j = 0; _pg_j < (unsigned)" + getEncodeFieldAccess(isStructureMember, variable2dArray) + " && _pg_j < " + array2d + "; _pg_j++)\n";
            }
        }

    }

    return output;

}// Encodable::getEncodeArrayIterationCode


/*!
 * Get the code that performs array iteration, in a decode context
 * \param spacing is the spacing that begins the first array iteration line
 * \param isStructureMember should be true if variable array limits are members of a structure
 * \return the code for array iteration, which may be empty
 */
std::string Encodable::getDecodeArrayIterationCode(const std::string& spacing, bool isStructureMember) const
{
    std::string output;

    if(isArray())
    {
        if(variableArray.empty())
        {
            output += spacing + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";
        }
        else
        {
            output += spacing + "for(_pg_i = 0; _pg_i < (unsigned)" + getDecodeFieldAccess(isStructureMember, variableArray) + " && _pg_i < " + array + "; _pg_i++)\n";
        }

        if(is2dArray())
        {
            if(variable2dArray.empty())
            {
                output += spacing + TAB_IN + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";
            }
            else
            {
                output += spacing + TAB_IN + "for(_pg_j = 0; _pg_j < (unsigned)" + getDecodeFieldAccess(isStructureMember, variable2dArray) + " && _pg_j < " + array2d + "; _pg_j++)\n";
            }
        }

    }

    return output;

}// Encodable::getDecodeArrayIterationCode


/*!
 * Get documentation repeat details for array or 2d arrays
 * \return The repeat details
 */
std::string Encodable::getRepeatsDocumentationDetails(void) const
{
    std::string repeats = "1";
    std::string arrayLink;
    std::string array2dLink;
    std::string variableArrayLink;
    std::string variable2dArrayLink;

    if(isArray())
    {
        arrayLink = parser->getEnumerationNameForEnumValue(array);

        if(arrayLink.empty())
            arrayLink = array;
        else
            arrayLink = "["+array+"](#"+arrayLink+")";

        if(variableArray.empty())
            variableArrayLink = parser->getEnumerationNameForEnumValue(variableArray);

        if(variableArrayLink.empty())
            variableArrayLink = variableArray;
        else
            variableArrayLink = "["+variableArray+"](#"+variableArrayLink+")";
    }

    if(is2dArray())
    {
        array2dLink = parser->getEnumerationNameForEnumValue(array2d);

        if(array2dLink.empty())
            array2dLink = array2d;
        else
            array2dLink = "["+array2d+"](#"+array2dLink+")";

        if(variable2dArray.empty())
            variable2dArrayLink = parser->getEnumerationNameForEnumValue(variable2dArray);

        if(variable2dArrayLink.empty())
            variable2dArrayLink = variable2dArray;
        else
            variable2dArrayLink = "["+variable2dArray+"](#"+variable2dArrayLink+")";
    }

    if(is2dArray())
    {
        if(variableArray.empty() && variable2dArray.empty())
            repeats = arrayLink+"*"+array2dLink;
        else
            repeats = variableArrayLink+"*"+variable2dArrayLink + ", up to " + arrayLink+"*"+array2dLink;
    }
    else if(isArray())
    {
        if(variableArray.empty())
            repeats = arrayLink;
        else
            repeats = variableArrayLink + ", up to " + arrayLink;
    }

    return repeats;

}// Encodable::getRepeatsDocumentationDetails


/*!
 * Construct a protocol field by parsing a DOM element. The type of Encodable
 * created will be either a ProtocolStructure or a ProtocolField
 * \param parse points to the global protocol parser that owns everything
 * \param Parent is the hierarchical name of the objec which owns the newly created object
 * \param supported describes what the protocol can support
 * \param field is the DOM element to parse (including its children)
 * \return a pointer to a newly allocated encodable. The caller is
 *         responsible for deleting this object.
 */
Encodable* Encodable::generateEncodable(ProtocolParser* parse, const std::string& parent, ProtocolSupport supported, const XMLElement* field)
{
    Encodable* enc = NULL;

    if(field == nullptr)
        return enc;

    std::string tagname(field->Name());

    if(contains(tagname, "structure"))
        enc = new ProtocolStructure(parse, parent, supported);
    else if(contains(tagname, "data"))
        enc = new ProtocolField(parse, parent, supported);
    else if(contains(tagname, "code"))
        enc = new ProtocolCode(parse, parent, supported);

    if(enc != NULL)
    {
        enc->setElement(field);
        enc->parse();
    }

    return enc;
}

