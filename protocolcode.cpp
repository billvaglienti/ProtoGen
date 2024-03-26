#include "protocolcode.h"
#include "protocolparser.h"

/*!
 * Construct a blank protocol field
 * \param parse points to the global protocol parser that owns everything
 * \param parent is the hierarchical name of the owning object
 * \param supported indicates what the protocol can support
 */
ProtocolCode::ProtocolCode(ProtocolParser* parse, std::string parent, ProtocolSupport supported):
    Encodable(parse, parent, supported)
{
    attriblist = {"name", "encode", "decode", "encode_c", "decode_c", "encode_cpp", "decode_cpp", "encode_python", "decode_python", "comment", "include"};
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
void ProtocolCode::parse(bool nocode)
{
    (void)nocode;

    clear();

    if(e == nullptr)
        return;

    const XMLAttribute* map = e->FirstAttribute();

    // We use name as part of our debug outputs, so its good to have it first.
    name = ProtocolParser::getAttribute("name", map, "_unknown");
    encode = ProtocolParser::getAttribute("encode_c", map);
    decode = ProtocolParser::getAttribute("decode_c", map);
    encodecpp = ProtocolParser::getAttribute("encode_cpp", map);
    decodecpp = ProtocolParser::getAttribute("decode_cpp", map);
    encodepython = ProtocolParser::getAttribute("encode_python", map);
    decodepython = ProtocolParser::getAttribute("decode_python", map);
    comment = ProtocolParser::getAttribute("comment", map);
    include = ProtocolParser::getAttribute("include", map);

    if(encode.empty())
        encode = ProtocolParser::getAttribute("encode", map);

    if(decode.empty())
        decode = ProtocolParser::getAttribute("decode", map);

    testAndWarnAttributes(map);

}// ProtocolCode::parse


/*!
 * Get the next lines(s) of source coded needed to add this code to the encode function
 * \param bitcount points to the running count of bits in a bitfields and
 *        should persist between calls, ignored.
 * \param isStructureMember should be true if the left hand side is a
 *        member of a user structure, else the left hand side is a pointer
 *        to the inMemoryType, ignored.
 * \return The string to add to the source file for this code.
 */
std::string ProtocolCode::getEncodeString(int* bitcount, bool isStructureMember) const
{
    (void)bitcount;
    (void)isStructureMember;

    std::string output;

    if((support.language == ProtocolSupport::c_language) && !encode.empty())
    {
        if(!comment.empty())
            output += TAB_IN + "// " + comment + "\n";

        output += TAB_IN + encode + "\n";
    }
    else if((support.language == ProtocolSupport::cpp_language) && !encodecpp.empty())
    {
        if(!comment.empty())
            output += TAB_IN + "// " + comment + "\n";

        output += TAB_IN + encodecpp + "\n";
    }

    return output;

}


/*!
 * Get the next lines(s) of source coded needed to add this code to the decode function.
 * \param bitcount points to the running count of bits in a bitfields and
 *        should persist between calls, ignored.
 * \param isStructureMember should be true if the left hand side is a
 *        member of a user structure, else the left hand side is a pointer
 *        to the inMemoryType, ignored.
 * \param defaultEnabled should be true to handle defaults, ignored.
 * \return The string to add to the source file for this code.
 */
std::string ProtocolCode::getDecodeString(int* bitcount, bool isStructureMember, bool defaultEnabled) const
{
    (void)bitcount;
    (void)isStructureMember;
    (void)defaultEnabled;

    std::string output;

    if((support.language == ProtocolSupport::c_language) && !decode.empty())
    {
        if(!comment.empty())
            output += TAB_IN + "// " + comment + "\n";

        output += TAB_IN + decode + "\n";
    }
    else if((support.language == ProtocolSupport::cpp_language) && !decodecpp.empty())
    {
        if(!comment.empty())
            output += TAB_IN + "// " + comment + "\n";

        output += TAB_IN + decodecpp + "\n";
    }

    return output;
}


//! Return the include directives that go into source code needed for this encodable
void ProtocolCode::getSourceIncludeDirectives(std::vector<std::string>& list) const
{
    if(!include.empty())
        list.push_back(include);
}


