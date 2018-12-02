#include "encodable.h"
#include "protocolfield.h"
#include "protocolstructure.h"
#include "protocolcode.h"
#include "protocoldocumentation.h"
#include "protocolparser.h"

// Initialize convenience strings
const QString Encodable::VOID_ENCODE = "void encode";
const QString Encodable::INT_DECODE = "int decode";

/*!
 * Constructor for encodable
 */
Encodable::Encodable(ProtocolParser* parse, QString Parent, ProtocolSupport supported) :
    ProtocolDocumentation(parse, Parent, supported)
{
}


/*!
 * Check all names against C keyword and issue a warning if any of them match.
 * In addition the matching names will be updated to have a leading underscore
 */
void Encodable::checkAgainstKeywords(void)
{
    if(keywords.contains(name))
    {
        emitWarning("name matches C keyword, changed to _name");
        name = "_" + name;
    }

    if(keywords.contains(array))
    {
        emitWarning("array matches C keyword, changed to _array");
        array = "_" + array;
    }

    if(keywords.contains(variableArray))
    {
        emitWarning("variableArray matches C keyword, changed to _variableArray");
        variableArray = "_" + variableArray;
    }

    if(keywords.contains(array2d))
    {
        emitWarning("array2d matches C keyword, changed to _array2d");
        array2d = "_" + array2d;
    }

    if(keywords.contains(variable2dArray))
    {
        emitWarning("variable2dArray matches C keyword, changed to _variable2dArray");
        variable2dArray = "_" + variable2dArray;
    }

    if(keywords.contains(dependsOn))
    {
        emitWarning("dependsOn matches C keyword, changed to _dependsOn");
        dependsOn = "_" + dependsOn;
    }

    if(keywords.contains(dependsOnValue))
    {
        emitWarning("dependsOnValue matches C keyword, changed to _dependsOnValue");
        dependsOnValue = "_" + dependsOnValue;
    }

    if(variablenames.contains(name))
    {
        emitWarning("name matches ProtoGen variable, changed to _name");
        name = "_" + name;
    }

    if(variablenames.contains(array))
    {
        emitWarning("array matches ProtoGen variable, changed to _array");
        array = "_" + array;
    }

    if(variablenames.contains(variableArray))
    {
        emitWarning("variableArray matches ProtoGen variable, changed to _variableArray");
        variableArray = "_" + variableArray;
    }

    if(variablenames.contains(array2d))
    {
        emitWarning("array2d matches ProtoGen variable, changed to _array2d");
        array2d = "_" + array2d;
    }

    if(variablenames.contains(variable2dArray))
    {
        emitWarning("variable2dArray matches ProtoGen variable, changed to _variable2dArray");
        variable2dArray = "_" + variable2dArray;
    }

    if(variablenames.contains(dependsOn))
    {
        emitWarning("dependsOn matches ProtoGen variable, changed to _dependsOn");
        dependsOn = "_" + dependsOn;
    }

    if(variablenames.contains(dependsOnValue))
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
QString Encodable::getEncodeSignature(void) const
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
QString Encodable::getDecodeSignature(void) const
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
QString Encodable::getEncodeParameterComment(void) const
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
QString Encodable::getDecodeParameterComment(void) const
{
    if(isNotEncoded() || isNotInMemory())
        return "";
    else
        return " * \\param " + name + " receives " + comment + "\n";
}


/*!
 * Get documentation repeat details for array or 2d arrays
 * \return The repeat details
 */
QString Encodable::getRepeatsDocumentationDetails(void) const
{
    QString repeats = "1";
    QString arrayLink;
    QString array2dLink;
    QString variableArrayLink;
    QString variable2dArrayLink;

    if(isArray())
    {
        arrayLink = parser->getEnumerationNameForEnumValue(array);

        if(arrayLink.isEmpty())
            arrayLink = array;
        else
            arrayLink = "["+array+"](#"+arrayLink+")";

        if(variableArray.isEmpty())
            variableArrayLink = parser->getEnumerationNameForEnumValue(variableArray);

        if(variableArrayLink.isEmpty())
            variableArrayLink = variableArray;
        else
            variableArrayLink = "["+variableArray+"](#"+variableArrayLink+")";
    }

    if(is2dArray())
    {
        array2dLink = parser->getEnumerationNameForEnumValue(array2d);

        if(array2dLink.isEmpty())
            array2dLink = array2d;
        else
            array2dLink = "["+array2d+"](#"+array2dLink+")";

        if(variable2dArray.isEmpty())
            variable2dArrayLink = parser->getEnumerationNameForEnumValue(variable2dArray);

        if(variable2dArrayLink.isEmpty())
            variable2dArrayLink = variable2dArray;
        else
            variable2dArrayLink = "["+variable2dArray+"](#"+variable2dArrayLink+")";
    }

    if(is2dArray())
    {
        if(variableArray.isEmpty() && variable2dArray.isEmpty())
            repeats = arrayLink+"*"+array2dLink;
        else
            repeats = variableArrayLink+"*"+variable2dArrayLink + ", up to " + arrayLink+"*"+array2dLink;
    }
    else if(isArray())
    {
        if(variableArray.isEmpty())
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
Encodable* Encodable::generateEncodable(ProtocolParser* parse, QString Parent, ProtocolSupport supported, const QDomElement& field)
{
    Encodable* enc = NULL;

    if(field.tagName().contains("Structure", Qt::CaseInsensitive))
        enc = new ProtocolStructure(parse, Parent, supported);
    else if(field.tagName().contains("Data", Qt::CaseInsensitive))
        enc = new ProtocolField(parse, Parent, supported);
    else if(field.tagName().contains("Code", Qt::CaseInsensitive))
        enc = new ProtocolCode(parse, Parent, supported);

    if(enc != NULL)
    {
        enc->setElement(field);
        enc->parse();
    }

    return enc;
}

