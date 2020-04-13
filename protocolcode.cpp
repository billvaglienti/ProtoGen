#include "protocolcode.h"
#include "protocolparser.h"
#include <iostream>

/*!
 * Construct a blank protocol field
 * \param parse points to the global protocol parser that owns everything
 * \param Parent is the hierarchical name of the owning object
 * \param supported indicates what the protocol can support
 */
ProtocolCode::ProtocolCode(ProtocolParser* parse, QString Parent, ProtocolSupport supported):
    Encodable(parse, Parent, supported)
{
}


//! Reset all data to defaults
void ProtocolCode::clear(void)
{
    Encodable::clear();

    encode.clear();
    decode.clear();
    include.clear();
}


/*!
 * Parse the DOM to determine the details of this ProtocolCode
 */
void ProtocolCode::parse(void)
{
    clear();

    QDomNamedNodeMap map = e.attributes();

    // We use name as part of our debug outputs, so its good to have it first.
    name = ProtocolParser::getAttribute("name", map);

    if(name.isEmpty())
        name = "_unknown";

    for(int i = 0; i < map.count(); i++)
    {
        QDomAttr attr = map.item(i).toAttr();
        if(attr.isNull())
            continue;

        QString attrname = attr.name();

        if(attrname.compare("name", Qt::CaseInsensitive) == 0)
            name = attr.value().trimmed();
        else if(attrname.compare("encode", Qt::CaseInsensitive) == 0)
            encode = attr.value().trimmed();
        else if(attrname.compare("decode", Qt::CaseInsensitive) == 0)
            decode = attr.value().trimmed();
        else if(attrname.compare("comment", Qt::CaseInsensitive) == 0)
            comment = attr.value().trimmed();
        else if(attrname.compare("include", Qt::CaseInsensitive) == 0)
            include = attr.value().trimmed();
        else if(support.disableunrecognized == false)
            emitWarning("Unrecognized attribute \"" + attrname + "\"");

    }// for all attributes

}// ProtocolCode::parse


/*!
 * Get the next lines(s) of source coded needed to add this code to the encode function
 * \param isBigEndian should be true for big endian encoding, ignored.
 * \param bitcount points to the running count of bits in a bitfields and
 *        should persist between calls, ignored.
 * \param isStructureMember should be true if the left hand side is a
 *        member of a user structure, else the left hand side is a pointer
 *        to the inMemoryType, ignored.
 * \return The string to add to the source file for this code.
 */
QString ProtocolCode::getEncodeString(bool isBigEndian, int* bitcount, bool isStructureMember) const
{
    Q_UNUSED(isBigEndian);
    Q_UNUSED(bitcount);
    Q_UNUSED(isStructureMember);

    QString output;

    if(encode.isEmpty())
        return output;

    if(!comment.isEmpty())
        output += TAB_IN + "// " + comment + "\n";

    output += TAB_IN + encode + "\n";

    return output;
}


/*!
 * Get the next lines(s) of source coded needed to add this code to the decode function.
 * \param isBigEndian should be true for big endian encoding, ignored.
 * \param bitcount points to the running count of bits in a bitfields and
 *        should persist between calls, ignored.
 * \param isStructureMember should be true if the left hand side is a
 *        member of a user structure, else the left hand side is a pointer
 *        to the inMemoryType, ignored.
 * \param defaultEnabled should be true to handle defaults, ignored.
 * \return The string to add to the source file for this code.
 */
QString ProtocolCode::getDecodeString(bool isBigEndian, int* bitcount, bool isStructureMember, bool defaultEnabled) const
{
    Q_UNUSED(isBigEndian);
    Q_UNUSED(bitcount);
    Q_UNUSED(isStructureMember);
    Q_UNUSED(defaultEnabled);

    QString output;

    if(decode.isEmpty())
        return output;

    if(!comment.isEmpty())
        output += TAB_IN + "// " + comment + "\n";

    output += TAB_IN + decode + "\n";

    return output;
}


//! Return the include directives that go into source code needed for this encodable
void ProtocolCode::getSourceIncludeDirectives(QStringList& list) const
{
    if(!include.isEmpty())
        list.append(include);
}


