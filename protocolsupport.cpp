#include "protocolsupport.h"
#include "protocolparser.h"

#include <QStringList>


/*!
 * Make a copy of a string that is all lower case.
 * \param text is the string to copy and make lower case.
 * \return a lower case version of text.
 */
std::string toLower(const std::string& text)
{
    std::string lower;

    for(const char& c: text)
    {
        if((c >= 'A') && (c <= 'Z'))
            lower.push_back(c + ('a' - 'A'));
        else
            lower.push_back(c);
    }

    return lower;
}


/*!
 * Make a copy of a string that is all upper case.
 * \param text is the string to copy and make upper case.
 * \return a upper case version of text.
 */
std::string toUpper(const std::string& text)
{
    std::string upper;

    for(const char& c: text)
    {
        if((c >= 'a') && (c <= 'z'))
            upper.push_back(c - ('a' - 'A'));
        else
            upper.push_back(c);
    }

    return upper;
}


/*!
 * Determine if a string contains a sub string.
 * \param text is the string to check.
 * \param test is the substring that is searched for within text.
 * \param casesensitive should be true to require the case to match.
 * \return true if text contains test.
 */
bool contains(const std::string& text, const std::string& test, bool casesensitive)
{
    if(casesensitive)
    {
        if(text.find(test) < text.size())
            return true;
        else
            return false;
    }
    else
    {
        if(toLower(text).find(toLower(test)) < text.size())
            return true;
        else
            return false;
    }
}


/*!
 * Trim white space from a std::string.
 * \param text is the string to trim.
 * \return A copy of text without any leading or trailing white space.
 */
std::string trimm(const std::string& text)
{
    std::size_t last = text.find_last_not_of(" \n\r\t");
    std::size_t first = text.find_first_not_of(" \n\r\t");

    if(first > text.size())
        first = 0;

    // Test case for leading white space:
    //  012345     01234
    // " test "   " test"
    // last  = 4; last = max;
    // first = 1; first = 1;
    return text.substr(first, last - first + 1);

}// trimm


/*!
 * Replace all occurences of `find` with `replace`;
 * \param text is the string that has its text replaced
 * \param find is the text to find and replace
 * \param replace is the text to put in place of `find`.
 * \return text with all occurrences of `find` replaced with `replace`.
 */
std::string replace(const std::string& text, const std::string& find, const std::string& replace)
{
    std::string output = text;
    std::size_t index = output.find(find);

    while(index < output.size())
    {
        // Get rid of the original bytes
        output.erase(index, find.size());

        // And insert the new ones
        if(!replace.empty())
        {
            output.insert(index, replace);

            // Jump past what we just replaced, in case `replace` contains `find`
            index += replace.size();
        }

        index = output.find(find, index);

    }

    return output;
}


/*!
 * Replace first occurence of `find` with `replace`;
 * \param text is the string that has its text replaced
 * \param find is the text to find and replace
 * \param replace is the text to put in place of `find`.
 * \return text with all occurrences of `find` replaced with `replace`.
 */
std::string replacefirst(const std::string& text, const std::string& find, const std::string& replace)
{
    std::string output = text;

    std::size_t index = output.find(find);

    if(index < output.size())
    {
        // Get rid of the original bytes
        output.erase(index, find.size());

        // And insert the new ones
        if(!replace.empty())
            output.insert(index, replace);
    }

    return output;
}


/*!
 * Replace first occurence of `find` with `replace`;
 * \param text is the string that has its text replaced
 * \param find is the text to find and replace
 * \param replace is the text to put in place of `find`.
 * \return text with all occurrences of `find` replaced with `replace`.
 */
std::string replacelast(const std::string& text, const std::string& find, const std::string& replace)
{
    std::string output = text;

    std::size_t index = output.rfind(find);

    if(index < output.size())
    {
        // Get rid of the original bytes
        output.erase(index, find.size());

        // And insert the new ones
        if(!replace.empty())
            output.insert(index, replace);
    }

    return output;
}


//! Convert a string list to a vector of std::strings
std::vector<std::string> convertStringList(const QStringList& list)
{
    std::vector<std::string> newlist;

    for(int i = 0; i < list.size(); i++)
        newlist.push_back(list.at(i).toStdString());

    return newlist;
}


ProtocolSupport::ProtocolSupport() :
    language(c_language),
    maxdatasize(0),
    int64(true),
    float64(true),
    specialFloat(true),
    bitfield(true),
    longbitfield(false),
    bitfieldtest(false),
    disableunrecognized(false),
    bigendian(true),
    supportbool(false),
    limitonencode(false),
    compare(false),
    print(false),
    mapEncode(false),
    packetStructureSuffix("PacketStructure"),
    packetParameterSuffix("Packet"),
    enablelanguageoverride(false)
{
}


//! Return the list of attributes understood by ProtocolSupport
QStringList ProtocolSupport::getAttriblist(void) const
{
    QStringList attribs;

    attribs << "maxSize"
            << "supportInt64"
            << "supportFloat64"
            << "supportSpecialFloat"
            << "supportBitfield"
            << "supportLongBitfield"
            << "bitfieldTest"
            << "file"
            << "verifyfile"
            << "comparefile"
            << "printfile"
            << "mapfile"
            << "prefix"
            << "packetStructureSuffix"
            << "packetParameterSuffix"
            << "endian"
            << "pointer"
            << "supportBool"
            << "limitOnEncode"
            << "C"
            << "CPP"
            << "compare"
            << "print"
            << "map";

    return attribs;
}


/*!
 * Parse the attributes for this support object from the DOM map
 * \param map is the DOM map
 */
void ProtocolSupport::parse(const XMLAttribute* map)
{
    if(!enablelanguageoverride)
    {
        // The type of language, C and C++ are the only supported options right now
        if(ProtocolParser::isFieldSet(ProtocolParser::getAttribute("CPP", map)))
            language = cpp_language;
        else
            language = c_language;

        // Don't let any copies of us override the language setting. We need this to be the same for all support objects.
        enablelanguageoverride = true;
    }

    // Maximum bytes of data in a packet.
    maxdatasize = ProtocolParser::getAttribute("maxSize", map, "0").toInt();

    // 64-bit support can be turned off
    if(ProtocolParser::isFieldClear(ProtocolParser::getAttribute("supportInt64", map)))
        int64 = false;

    // double support can be turned off
    if(ProtocolParser::isFieldClear(ProtocolParser::getAttribute("supportFloat64", map)))
        float64 = false;

    // special float support can be turned off
    if(ProtocolParser::isFieldClear(ProtocolParser::getAttribute("supportSpecialFloat", map)))
        specialFloat = false;

    // bitfield support can be turned off
    if(ProtocolParser::isFieldClear(ProtocolParser::getAttribute("supportBitfield", map)))
        bitfield = false;

    // long bitfield support can be turned on
    if(int64 && ProtocolParser::isFieldSet("supportLongBitfield", map))
        longbitfield = true;

    // bitfield test support can be turned on
    if(ProtocolParser::isFieldSet("bitfieldTest", map))
        bitfieldtest = true;

    // bool support default is based on language type
    if(language == c_language)
        supportbool = false;
    else
        supportbool = true;

    // bool support can be turned on or off
    if(ProtocolParser::isFieldSet("supportBool", map))
        supportbool = true;
    else if(ProtocolParser::isFieldClear("supportBool", map))
        supportbool = false;

    // Limit on encode can be turned on
    if(ProtocolParser::isFieldSet("limitOnEncode", map))
        limitonencode = true;

    // Global flags to force output for compare, print, and map functions
    compare = ProtocolParser::isFieldSet(ProtocolParser::getAttribute("compare", map));
    print = ProtocolParser::isFieldSet(ProtocolParser::getAttribute("print", map));
    mapEncode = ProtocolParser::isFieldSet(ProtocolParser::getAttribute("map", map));

    // The global file names
    parseFileNames(map);

    // Prefix is not required
    prefix = ProtocolParser::getAttribute("prefix", map);

    // Packet pointer type (default is 'void')
    pointerType = ProtocolParser::getAttribute("pointer", map, "void*");

    // Must be a pointer type
    if(!pointerType.endsWith("*"))
        pointerType += "*";

    // Packet name post fixes
    packetStructureSuffix = ProtocolParser::getAttribute("packetStructureSuffix", map, packetStructureSuffix);
    packetParameterSuffix = ProtocolParser::getAttribute("packetParameterSuffix", map, packetParameterSuffix);

    if(ProtocolParser::getAttribute("endian", map).contains("little", Qt::CaseInsensitive))
        bigendian = false;

}// ProtocolSupport::parse


/*!
 * Parse the global file names used for this support object from the DOM map
 * \param map is the DOM map
 */
void ProtocolSupport::parseFileNames(const XMLAttribute* map)
{
    // Global file names can be specified, but cannot have a "." in it
    globalFileName = replace(ProtocolParser::getAttribute("file", map).toStdString(), ".");
    globalVerifyName = replace(ProtocolParser::getAttribute("verifyfile", map).toStdString(), ".");
    globalCompareName = replace(ProtocolParser::getAttribute("comparefile", map).toStdString(), ".");
    globalPrintName = replace(ProtocolParser::getAttribute("printfile", map).toStdString(), ".");
    globalMapName = replace(ProtocolParser::getAttribute("mapfile", map).toStdString(), ".");

}// ProtocolSupport::parseFileNames
