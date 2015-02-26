#include "encodable.h"
#include "protocolfield.h"
#include "protocolstructure.h"

/*!
 * Constructor for encodable
 */
Encodable::Encodable(const QString& protocolName, const QString& protocolPrefix, ProtocolSupport supported) :
    support(supported),
    protoName(protocolName),
    prefix(protocolPrefix),
    notEncoded(false),
    notInMemory(false),
    constant(false)
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
    notEncoded = false;
    notInMemory = false;
    constant = false;
}


/*!
 * Return the signature of this field in a encode function signature. The
 * string will start with ", " assuming this field is not the first part of
 * the function signature.
 * \return the string that provides this fields encode function signature
 */
QString Encodable::getEncodeSignature(void) const
{
    if(notEncoded || notInMemory || constant)
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
    if(notEncoded || notInMemory)
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
    if(notEncoded || notInMemory || constant)
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
    if(notEncoded || notInMemory)
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
    if(field.tagName().contains("Structure", Qt::CaseInsensitive))
        return new ProtocolStructure(protocolName, protocolPrefix, supported, field);
    else if(field.tagName().contains("Data", Qt::CaseInsensitive))
        return new ProtocolField(protocolName, protocolPrefix, supported, field);

    return NULL;
}

