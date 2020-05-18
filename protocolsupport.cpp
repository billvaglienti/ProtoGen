#include "protocolsupport.h"
#include "protocolparser.h"

//! Split a string into multiple sub strings spearated by a separator.
static std::vector<std::string> _split(const std::string& text, const std::string& sep, bool keepemptyparts, bool anyof);

/*!
 * Make a copy of a string that is all lower case.
 * \param text is the string to copy and make lower case.
 * \return a lower case version of text.
 */
std::string toLower(const std::string& text)
{
    std::string lower;

    for(const char& c: text)
        lower.push_back(std::tolower(c));

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
        upper.push_back(std::toupper(c));

    return upper;
}


/*!
 * Find the index of a sub string within a string, with optional case sensitivity
 * \param text is the string to check.
 * \param test is the substring that is searched for within text.
 * \param casesensitive should be true to require the case to match.
 * \return the index position of the start of the sub string, or npos if no match is found.
 */
std::size_t find(const std::string& text, const std::string& test, bool casesensitive)
{
    if(text.empty() || test.empty())
        return std::string::npos;

    if(casesensitive)
        return text.find(test);
    else
        return toLower(text).find(toLower(test));
}


/*!
 * Determine if two strings are equal, with optional case sensitivity
 * \param text is the string to check.
 * \param test is the substring that is searched for within text.
 * \param casesensitive should be true to require the case to match.
 * \return true if the strings are equal.
 */
bool isEqual(const std::string& text, const std::string& test, bool casesensitive)
{
    if((text.size() == test.size() && find(text, test, casesensitive) != std::string::npos))
        return true;
    else
        return false;
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
    if(find(text, test, casesensitive) < text.size())
        return true;
    else
        return false;
}


//! Determine if a string starts with another string
bool startsWith(const std::string& text, const std::string& test, bool casesensitive)
{
    if(find(text, test, casesensitive) == 0)
        return true;
    else
        return false;
}


//! Determine if a string ends with another string
bool endsWith(const std::string& text, const std::string& test, bool casesensitive)
{
    if(test.length() > text.length())
        return false;

    if(casesensitive)
        return (text.rfind(test) == (text.length() - test.length()));
    else
        return (toLower(text).rfind(toLower(test)) == (text.length() - test.length()));
}


/*!
 * Determine if a string list contains a string.
 * \param list is the list to search.
 * \param test is the test string to search for.
 * \param casesensitive should be true to do the search case sensitive.
 * \return true if a member of list is equal to test.
 */
bool contains(const std::vector<std::string>& list, const std::string& test, bool casesensitive)
{
    if(list.empty() || test.empty())
        return false;

    if(casesensitive)
        return (std::find(list.begin(), list.end(), test) != list.end());
    else
    {
        std::string lowertest = toLower(test);
        for(std::size_t i = 0; i < list.size(); i++)
        {
            if(toLower(list.at(i)) == lowertest)
                return true;
        }

        return false;
    }
}



/*!
 * Return the string from a list that startsWith another string.
 * \param list is the list of strings to search.
 * \param test is the start string to test against.
 * \param casesensitive should be true to do the search case sensitive.
 * \return The string from list that starts with test, or an empty string if none is found.
 */
std::string liststartsWith(const std::vector<std::string>& list, const std::string& test, bool casesensitive)
{
    for(std::size_t i = 0; i < list.size(); i++)
    {
        if(startsWith(list.at(i), test, casesensitive))
            return list.at(i);
    }

    return std::string();
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
    replaceinplace(output, find, replace);
    return output;
}


/*!
 * Replace all occurences of `find` with `replace`;
 * \param text is the string that has its text replaced
 * \param find is the text to find and replace
 * \param replace is the text to put in place of `find`.
 * \return text with all occurrences of `find` replaced with `replace`.
 */
std::string& replaceinplace(std::string& text, const std::string& find, const std::string& replace)
{
    std::size_t index = text.find(find);

    while(index < text.size())
    {
        // Get rid of the original bytes
        text.erase(index, find.size());

        // And insert the new ones
        if(!replace.empty())
        {
            text.insert(index, replace);

            // Jump past what we just replaced, in case `replace` contains `find`
            index += replace.size();
        }

        index = text.find(find, index);

    }

    return text;
}


/*!
 * Split a string into multiple sub strings spearated by a separator. The
 * separator will not appear in the output list.
 * \param text is the source string to split.
 * \param sep is the separator string.
 * \param keepemptyparts should be true to insert empty strings in the list
 *        if there are consecutive separators.
 * \return The list of strings split by the separator.
 */
std::vector<std::string> split(const std::string& text, const std::string& sep, bool keepemptyparts)
{
    return _split(text, sep, keepemptyparts, false);
}

/*!
 * Split a string into multiple sub strings spearated by a character(s). The
 * separator characters will not appear in the output list.
 * \param text is the source string to split.
 * \param sep is a string of characters, any one of which is a valid separator.
 * \param keepemptyparts should be true to insert empty strings in the list
 *        if there are consecutive separators.
 * \return The list of strings split by the separator.
 */
std::vector<std::string> splitanyof(const std::string& text, const std::string& sep, bool keepemptyparts)
{
    return _split(text, sep, keepemptyparts, true);
}

/*!
 * Split a string into multiple sub strings spearated by a separator. The
 * separator will not appear in the output list.
 * \param text is the source string to split.
 * \param sep is the separator string.
 * \param keepemptyparts should be true to insert empty strings in the list
 *        if there are consecutive separators.
 * \param anyof should be true to treach any character of sep as a valid
 *        separator, otherwise the entire sep string is used as the separator.
 * \return The list of strings split by the separator.
 */
std::vector<std::string> _split(const std::string& text, const std::string& sep, bool keepemptyparts, bool anyof)
{
    std::vector<std::string> list;

    if(sep.empty() || text.empty())
    {
        // No separators, return the whole string, unless it is empty, unless we keep empty parts
        if(!text.empty() || keepemptyparts)
            list.push_back(text);

        return list;
    }

    std::size_t start = 0;

    // Find the first occurence of the separator
    std::size_t index;

    if(anyof)
        index = text.find_first_of(sep, start); // Any character is valid
    else
        index = text.find(sep, start);          // Only whole string is valid

    while(index < text.size())
    {
        // Extract the string, except for the separator
        if(index == start)
        {
            // This will happen when we have consecutive separators
            if(keepemptyparts)
                list.push_back(std::string());
        }
        else
            list.push_back(text.substr(start, index - start));

        // Find the next separator
        if(anyof)
        {
            // Index is the first character of the separator, our next search after the character
            start = index + 1;
            index = text.find_first_of(sep, start);
        }
        else
        {
            // Index is the first character of the separator, our next search should be after the separator
            start = index + sep.length();
            index = text.find(sep, start);
        }
    }

    // It is possible that this is the entire text if there were no separators
    if((index > start) && (start < text.length()))
        list.push_back(text.substr(start, index - start));

    return list;

}// _split


/*!
 * Join substrings together.
 * \param list is the list of strings to join.
 * \param joiner is text to insert between the substrings of the list.
 * \return the concatenated list with joiners.
 */
std::string join(const std::vector<std::string> list, const std::string& joiner)
{
    std::string text;

    if(list.size() > 0)
    {
        text = list.front();

        for(std::size_t i = 1; i < list.size(); i++)
            text += joiner + list.at(i);

    }

    return text;
}


/*!
 * Remove duplicate strings.
 * \param list is the string list whose duplicates are removed. list will be updated.
 * \param casesensitive should be true to consider case when testing strings.
 * \return a reference to list.
 */
std::vector<std::string>& removeDuplicates(std::vector<std::string>& list, bool casesensitive)
{
    // Don't use iterators here, I will be changing the list size. I know
    // there is a better way to do this, but I need more STL fu.
    for(std::size_t i = 0; i < list.size(); i++)
    {
        for(std::size_t j = i + 1; j < list.size(); j++)
        {
            if(casesensitive)
            {
                if(list.at(i) == list.at(j))
                    list.erase(list.begin() + j);
            }
            else
            {
                if(toLower(list.at(i)) == toLower(list.at(j)))
                    list.erase(list.begin() + j);
            }
        }
    }

    return list;
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
std::vector<std::string> ProtocolSupport::getAttriblist(void) const
{
    std::vector<std::string> attribs;

    attribs.push_back("maxSize");
    attribs.push_back("supportInt64");
    attribs.push_back("supportFloat64");
    attribs.push_back("supportSpecialFloat");
    attribs.push_back("supportBitfield");
    attribs.push_back("supportLongBitfield");
    attribs.push_back("bitfieldTest");
    attribs.push_back("file");
    attribs.push_back("verifyfile");
    attribs.push_back("comparefile");
    attribs.push_back("printfile");
    attribs.push_back("mapfile");
    attribs.push_back("prefix");
    attribs.push_back("packetStructureSuffix");
    attribs.push_back("packetParameterSuffix");
    attribs.push_back("endian");
    attribs.push_back("pointer");
    attribs.push_back("supportBool");
    attribs.push_back("limitOnEncode");
    attribs.push_back("C");
    attribs.push_back("CPP");
    attribs.push_back("compare");
    attribs.push_back("print");
    attribs.push_back("map");

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
    maxdatasize = std::stoi(ProtocolParser::getAttribute("maxSize", map, "0"));

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
    if(pointerType.back() != '*')
        pointerType += '*';

    // Packet name post fixes
    packetStructureSuffix = ProtocolParser::getAttribute("packetStructureSuffix", map, packetStructureSuffix);
    packetParameterSuffix = ProtocolParser::getAttribute("packetParameterSuffix", map, packetParameterSuffix);

    if(contains(ProtocolParser::getAttribute("endian", map), "little"))
        bigendian = false;

}// ProtocolSupport::parse


/*!
 * Parse the global file names used for this support object from the DOM map
 * \param map is the DOM map
 */
void ProtocolSupport::parseFileNames(const XMLAttribute* map)
{
    // Global file names can be specified, but cannot have a "." in it
    globalFileName = ProtocolParser::getAttribute("file", map);
    globalVerifyName = ProtocolParser::getAttribute("verifyfile", map);
    globalCompareName = ProtocolParser::getAttribute("comparefile", map);
    globalPrintName = ProtocolParser::getAttribute("printfile", map);
    globalMapName = ProtocolParser::getAttribute("mapfile", map);

    replaceinplace(globalFileName, ".");
    replaceinplace(globalVerifyName, ".");
    replaceinplace(globalCompareName, ".");
    replaceinplace(globalPrintName, ".");
    replaceinplace(globalMapName, ".");

}// ProtocolSupport::parseFileNames
