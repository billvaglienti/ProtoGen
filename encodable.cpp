#include "encodable.h"
#include "protocolfield.h"
#include "protocolstructure.h"
#include "protocolcode.h"
#include "protocoldocumentation.h"

/*!
 * Constructor for encodable
 */
Encodable::Encodable(const QString& protocolName, const QString& protocolPrefix, ProtocolSupport supported) :
    support(supported),
    protoName(protocolName),
    prefix(protocolPrefix)
{
}

/*!
 * Reset all data to defaults
 */
void Encodable::clear(void)
{
    typeName.clear();
    name.clear();
    comment.clear();
    array.clear();
    encodedLength.clear();
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
 * Construct a protocol field by parsing a DOM element. The type of Encodable
 * created will be either a ProtocolStructure or a ProtocolField
 * \param protocolName is the name of the protocol
 * \param protocolPrefix is a prefix to use for typenames
 * \param supported describes what the protocol can support
 * \param field is the DOM element to parse (including its children)
 * \return a pointer to a newly allocated encodable. The caller is
 *         responsible for deleting this object.
 */
Encodable* Encodable::generateEncodable(const QString& protocolName, const QString& protocolPrefix, ProtocolSupport supported, const QDomElement& field)
{
    Encodable* enc = NULL;

    if(field.tagName().contains("Structure", Qt::CaseInsensitive))
        enc = new ProtocolStructure(protocolName, protocolPrefix, supported);
    else if(field.tagName().contains("Data", Qt::CaseInsensitive))
        enc = new ProtocolField(protocolName, protocolPrefix, supported);
    else if(field.tagName().contains("Code", Qt::CaseInsensitive))
        enc = new ProtocolCode(protocolName, protocolPrefix, supported);

    if(enc != NULL)
    {
        enc->setElement(field);
        enc->parse();
    }

    return enc;
}

