#ifndef PROTOCOLSUPPORT_H
#define PROTOCOLSUPPORT_H

#include "tinyxml2.h"
#include <vector>
#include <string>

using namespace tinyxml2;

//! Make a copy of a string that is all lower case.
std::string toLower(const std::string& text);

//! Make a copy of a string that is all upper case.
std::string toUpper(const std::string& text);

//! Find the index of a sub string within a string, with optional case sensitivity
std::size_t find(const std::string& text, const std::string& test, bool casesensitive = false);

//! Determine if two strings are equal, with optional case sensitivity
bool isEqual(const std::string& text, const std::string& test, bool casesensitive = false);

//! Determine if a string contains a sub string.
bool contains(const std::string& text, const std::string& test, bool casesensitive = false);

//! Determine if a string list contains a string.
bool contains(const std::vector<std::string>& list, const std::string& test, bool casesensitive = false);

//! Return the string from a list that startsWith another string
std::string liststartsWith(const std::vector<std::string>& list, const std::string& test, bool casesensitive = false);

//! Determine if a string starts with another string
bool startsWith(const std::string& text, const std::string& test, bool casesensitive = false);

//! Determine if a string ends with another string
bool endsWith(const std::string& text, const std::string& test, bool casesensitive = false);

//! Trim white space from a std::string.
std::string trimm(const std::string& text);

//! Replace all occurences of `find` with `replace`;
std::string replace(const std::string& text, const std::string& find, const std::string& replace = std::string());

//! Replace all occurences of `find` with `replace`;
std::string& replaceinplace(std::string& text, const std::string& find, const std::string& replace = std::string());

//! Split a string into multiple sub strings spearated by a separator
std::vector<std::string> split(const std::string& text, const std::string& sep, bool keepemptyparts = false);

//! Split a string into multiple sub strings spearated by one of characters in a separator
std::vector<std::string> splitanyof(const std::string& text, const std::string& sep, bool keepemptyparts = false);

//! Join substrings together
std::string join(const std::vector<std::string> list, const std::string& joiner = std::string());

//! Remove duplicate strings
std::vector<std::string>& removeDuplicates(std::vector<std::string>& list, bool casesensitive = false);


class ProtocolSupport
{
public:
    ProtocolSupport();

    //! Parse attributes from the ProtocolTag
    void parse(const XMLAttribute* map);

    //! Parse the global file names
    void parseFileNames(const XMLAttribute* map);

    //! Return the list of attributes understood by ProtocolSupport
    std::vector<std::string> getAttriblist(void) const;

    //! The type of language being output
    typedef enum
    {
        c_language,                 //!< Standard (C99) language rules, also the default
        cpp_language,               //!< C++ language rules
        python_language             //!< Python language rules
    }LanguageType;

    //! Set the language override option, call this before the parse function
    void setLanguageOverride(LanguageType lang) {enablelanguageoverride = true; language = lang;}

    LanguageType language;             //!< Enumerator specifying the language type
    int maxdatasize;                   //!< Maximum number of data bytes in a packet, 0 if no limit
    bool int64;                        //!< true if support for integers greater than 32 bits is included
    bool float64;                      //!< true if support for double precision is included
    bool specialFloat;                 //!< true if support for float16 and float24 is included
    bool bitfield;                     //!< true if support for bitfields is included
    bool longbitfield;                 //!< true to support long bitfields
    bool bitfieldtest;                 //!< true to output the bitfield test function
    bool disableunrecognized;          //!< true to disable warnings about unrecognized attributes
    bool bigendian;                    //!< Protocol bigendian flag
    bool supportbool;                  //!< true if support for 'bool' is included
    bool limitonencode;                //!< true to enforce verification limits on encode
    bool compare;                      //!< True if the compare function is output for all structures
    bool print;                        //!< True if the textPrint and textRead function is output for all structures
    bool mapEncode;                    //!< True if the mapEncode and mapDecode function is output for all structures
    bool showAllItems;                 //!< Generate documentation even for elements marked hidden
    bool omitIfHidden;                 //!< Omit code generation for items marked hidden
    std::string globalFileName;        //!< File name to be used if a name is not given
    std::string globalVerifyName;      //!< Verify file name to be used if a name is not given
    std::string globalCompareName;     //!< Comparison file name to be used if a name is not given
    std::string globalPrintName;       //!< Print file name to be used if a name is not given
    std::string globalMapName;         //!< Map file name to be used if a name is not given
    std::string outputpath;            //!< path to output files to
    std::string packetStructureSuffix; //!< Name to use at end of encode/decode Packet structure functions
    std::string packetParameterSuffix; //!< Name to use at end of encode/decode Packet parameter functions
    std::string protoName;             //!< Name of the protocol
    std::string prefix;                //!< Prefix name
    std::string typeSuffix;            //!< Suffix on typedef structures
    std::string pointerType;           //!< Packet pointer type - default is "void*"
    std::string licenseText;           //!< License text to be added to each generated file
    std::string sourcefile;            //!< Source file name, used for warning outputs

protected:

    //! Set to true to enable the language override feature
    bool enablelanguageoverride;
};

#endif // PROTOCOLSUPPORT_H
