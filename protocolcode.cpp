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
    encodecpp.clear();
    decodecpp.clear();
    encodepython.clear();
    decodepython.clear();
    include.clear();
}


/*!
 * Parse the DOM to determine the details of this ProtocolCode
 */
void ProtocolCode::parse(void)
{
    clear();

    if(e == nullptr)
        return;

    const XMLAttribute* map = e->FirstAttribute();

    // We use name as part of our debug outputs, so its good to have it first.
    name = ProtocolParser::getAttribute("name", map, "_unknown");

    for(; map != nullptr; map = map->Next())
    {
        QString attrname = QString(map->Name()).trimmed();

        if(attrname.compare("name", Qt::CaseInsensitive) == 0)
            name = QString(map->Value()).trimmed();
        else if((attrname.compare("encode", Qt::CaseInsensitive) == 0) || (attrname.compare("encode_c", Qt::CaseInsensitive) == 0))
            encode = QString(map->Value()).trimmed();
        else if((attrname.compare("decode", Qt::CaseInsensitive) == 0) || (attrname.compare("decode_c", Qt::CaseInsensitive) == 0))
            decode = QString(map->Value()).trimmed();
        else if(attrname.compare("encode_cpp", Qt::CaseInsensitive) == 0)
            encodecpp = QString(map->Value()).trimmed();
        else if(attrname.compare("decode_cpp", Qt::CaseInsensitive) == 0)
            decodecpp = QString(map->Value()).trimmed();
        else if(attrname.compare("encode_python", Qt::CaseInsensitive) == 0)
            encodepython = QString(map->Value()).trimmed();
        else if(attrname.compare("decode_python", Qt::CaseInsensitive) == 0)
            decodepython = QString(map->Value()).trimmed();
        else if(attrname.compare("comment", Qt::CaseInsensitive) == 0)
            comment = QString(map->Value()).trimmed();
        else if(attrname.compare("include", Qt::CaseInsensitive) == 0)
            include = QString(map->Value()).trimmed();
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

    if((support.language == ProtocolSupport::c_language) && !encode.isEmpty())
    {
        if(!comment.isEmpty())
            output += TAB_IN + "// " + comment + "\n";

        output += TAB_IN + encode + "\n";
    }
    else if((support.language == ProtocolSupport::cpp_language) && !encodecpp.isEmpty())
    {
        if(!comment.isEmpty())
            output += TAB_IN + "// " + comment + "\n";

        output += TAB_IN + encodecpp + "\n";
    }

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

    if((support.language == ProtocolSupport::c_language) && !decode.isEmpty())
    {
        if(!comment.isEmpty())
            output += TAB_IN + "// " + comment + "\n";

        output += TAB_IN + decode + "\n";
    }
    else if((support.language == ProtocolSupport::cpp_language) && !decodecpp.isEmpty())
    {
        if(!comment.isEmpty())
            output += TAB_IN + "// " + comment + "\n";

        output += TAB_IN + decodecpp + "\n";
    }

    return output;
}


//! Return the include directives that go into source code needed for this encodable
void ProtocolCode::getSourceIncludeDirectives(QStringList& list) const
{
    if(!include.isEmpty())
        list.append(include);
}


