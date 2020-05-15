#ifndef ENCODEDLENGTH_H
#define ENCODEDLENGTH_H

#include <string>

class EncodedLength
{
public:
    EncodedLength(void);

    //! Add successive length strings
    void addToLength(const std::string & length, bool isString = false, bool isVariable = false, bool  isDependent = false, bool isDefault = false);

    //! Add a grouping of length strings
    void addToLength(const EncodedLength& rightLength, const std::string& array = std::string(), bool isVariable = false, bool isDependent = false, const std::string& array2d = std::string());

    //! Add a grouping of length strings
    static void add(EncodedLength* leftLength, const EncodedLength& rightLength, const std::string& array = std::string(), bool isVariable = false, bool isDependent = false, const std::string& array2d = std::string());

    //! Clear the encoded length
    void clear(void);

    //! Determine if there is any data here
    bool isEmpty(void);

    //! Determine if the length is zero
    bool isZeroLength(void) const;

    //! The minimum encoded length
    std::string minEncodedLength;

    //! The maximum encoded length
    std::string maxEncodedLength;

    //! The maximum encoded length of everything except default fields
    std::string nonDefaultEncodedLength;

    //! Collapse a length string as best we can by summing terms
    static std::string collapseLengthString(std::string totalLength, bool keepZero = false, bool minusOne = false);

    //! Subtract one from a length string
    static std::string subtractOneFromLengthString(std::string totalLength, bool keepZero = false);

private:

    //! Create a length string like "4 + 3 + N3D*2" by adding successive length strings
    static void addToLengthString(std::string& totalLength, std::string length, std::string array = std::string(), std::string array2d = std::string());

    //! Determine if text is a number for our cases
    static bool isNumber(const std::string& text);

};

#endif // ENCODEDLENGTH_H
