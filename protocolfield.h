#ifndef PROTOCOLFIELD_H
#define PROTOCOLFIELD_H

#include <stdint.h>
#include <string>
#include "encodable.h"

class TypeData
{
public:

    //! Construct empty type data
    TypeData(ProtocolSupport sup);

    //! Construct type data by copying another type
    TypeData(const TypeData& that);

    //! Reset all members to default except the protocol support
    void clear(void);

public:
    bool isBool;        //!< true if type is a 'bool'
    bool isStruct;      //!< true if this is an externally defined struct
    bool isSigned;      //!< true if type is signed
    bool isBitfield;    //!< true if type is a bitfield
    bool isFloat;       //!< true if type is a floating point number
    bool isEnum;        //!< true if type is an enumeration
    bool isString;      //!< true if type is a variable length string
    bool isFixedString; //!< true if type is a fixed length string
    bool isNull;        //!< true if type is null, i.e not in memory OR not encoded
    int bits;           //!< number of bits used by type
    int sigbits;        //!< number of bits for the significand of a float16 or float24
    int enummax;        //!< maximum value of the enumeration if isEnum is true
    std::string enumName;   //! Name of the enumerated type, empty if not enumerated type

    //! Create the type string
    std::string toTypeString(const std::string& structName = std::string()) const;

    //! Determine the signature of this field (for example uint8).
    std::string toSigString(void) const;

    //! Determine the maximum floating point value this TypeData can hold
    double getMaximumFloatValue(void) const;

    //! Determine the minimum floating point value this TypeData can hold
    double getMinimumFloatValue(void) const;

    //! Determine the maximum integer value this TypeData can hold
    uint64_t getMaximumIntegerValue(void) const;

    //! Determine the minimum integer value this TypeData can hold
    int64_t getMinimumIntegerValue(void) const;

    //! Given a constant string (like default value) apply the type correct suffix
    std::string applyTypeToConstant(const std::string& number) const;

    //! Pull a positive integer value from a string
    static int extractPositiveInt(const std::string& string, bool* ok = nullptr);

    //! Pull a double precision value from a string
    static double extractDouble(const std::string& string, bool* ok = nullptr);

private:

    ProtocolSupport support;
};


class BitfieldData
{
public:

    //! Construct empty type data
    BitfieldData(void) :
        startingBitCount(0),
        groupBits(0),
        groupStart(false),
        groupMember(false),
        lastBitfield(true)
    {}


    //! Reset all members to default except the protocol support
    void clear(void)
    {
        startingBitCount = 0;
        groupBits = 0;
        groupStart = false;
        groupMember = false;
        lastBitfield = true;
    }

    int startingBitCount;   //!< The starting bit count for this field if a bitfield
    int groupBits;          //!< number of bits in the bitfield group, same for all members
    bool groupStart;        //!< true if this bitfield starts a group
    bool groupMember;       //!< true if this bitfield is a member of a group
    bool lastBitfield;      //!< true if this bitfield is the last in a list of bitfields
};


class ProtocolField : public Encodable
{
public:

    // Option enumeration for determining if a certain field should be encoded / decoded
    enum MapOptions
    {
        MAP_NONE,   // Do not encode or decode this field
        MAP_ENCODE, // Only encode this field
        MAP_DECODE, // Only decode this field
        MAP_BOTH    // Encode and decode this field
    };

    //! Construct a field, setting the protocol name and name prefix
    ProtocolField(ProtocolParser* parse, std::string parent, ProtocolSupport supported);

    //! Provide the pointer to a previous encodable in the list
    void setPreviousEncodable(Encodable* prev) override;

    //! Get overriden type information
    bool getOverriddenTypeData(ProtocolField* prev);

    //! Get a properly formatted number string for a double precision number, with special care for pi
    static std::string getDisplayNumberString(double number);

    //! Get a properly formatted number string for a double precision number
    std::string getNumberString(double number, int bits = 64) const;

    //! Reset all data to defaults
    void clear(void) override;

    //! Parse the DOM element
    void parse(void) override;

    //! Check names against the list of C keywords
    void checkAgainstKeywords(void) override;

    //! The hierarchical name of this object
    std::string getHierarchicalName(void) const override {return parent + ":" + name;}

    //! Returns true since protocol field is a primitive type
    bool isPrimitive(void) const override {return !inMemoryType.isStruct;}

    //! Determine if this encodable a string object
    bool isString(void) const override {return inMemoryType.isString;}

    //! True if this encodable is NOT encoded
    bool isNotEncoded(void) const override {return (encodedType.isNull);}

    //! True if this encodable is NOT in memory. Note how overriding a previous field means we are not in memory (because the previous one is)
    bool isNotInMemory(void) const override {return (inMemoryType.isNull || overridesPrevious);}

    //! True if this encodable is a constant. Note that protocol fields which are null in memory are constant by definition
    bool isConstant(void) const override {return (!constantString.empty() || inMemoryType.isNull);}

    //! True if this encoable is a primitive bitfield
    bool isBitfield(void) const override {return (encodedType.isBitfield && !isNotEncoded());}

    //! True if this encodable has a default value
    bool isDefault(void) const override {return !defaultString.empty();}

    //! Get the maximum number of temporary bytes needed for a bitfield group
    void getBitfieldGroupNumBytes(int* num) const override;

    //! Get the declaration for this field
    std::string getDeclaration(void) const override;

    //! Return the include directives needed for this encodable
    void getIncludeDirectives(std::vector<std::string>& list) const override;

    //! Return the include directives needed for this encodable's init and verify functions
    void getInitAndVerifyIncludeDirectives(std::vector<std::string>& list) const override;

    //! Return the include directives needed for this encodable's map functions
    void getMapIncludeDirectives(std::vector<std::string>& list) const override;

    //! Return the include directives needed for this encodable's compare functions
    void getCompareIncludeDirectives(std::vector<std::string>& list) const override;

    //! Return the include directives needed for this encodable's print functions
    void getPrintIncludeDirectives(std::vector<std::string>& list) const override;

    //! Return the signature of this field in an encode function signature
    std::string getEncodeSignature(void) const override;

    //! Get details needed to produce documentation for this encodable.
    void getDocumentationDetails(std::vector<int>& outline, std::string& startByte, std::vector<std::string>& bytes, std::vector<std::string>& names, std::vector<std::string>& encodings, std::vector<std::string>& repeats, std::vector<std::string>& comments) const override;

    //! Return true if this field is hidden from documentation
    bool isHidden (void) const override {return hidden;}

    //! True if this encodable has verification data
    bool hasVerify(void) const override;

    //! True if this encodable has initialization data
    bool hasInit(void) const override;

    //! Return the string that is used to encode this encodable
    std::string getEncodeString(bool isBigEndian, int* bitcount, bool isStructureMember) const override;

    //! Return the string that is used to decode this encoable
    std::string getDecodeString(bool isBigEndian, int* bitcount, bool isStructureMember, bool defaultEnabled = false) const override;

    //! Return the string that sets this encodable to its default value in code
    std::string getSetToDefaultsString(bool isStructureMember) const override;

    //! Get the string used for verifying this field.
    std::string getVerifyString(void) const override;

    //! Get the string used for comparing this field.
    std::string getComparisonString(void) const override;

    //! Get the string used for text printing this field.
    std::string getTextPrintString(void) const override;

    //! Get the string used for text reading this field.
    std::string getTextReadString(void) const override;

    //! Get the string used for map encoding this field
    std::string getMapEncodeString(void) const override;

    //! Get the string used for map decoding this field
    std::string getMapDecodeString(void) const override;

    //! Return the string that sets this encodable to its initial value in code
    std::string getSetInitialValueString(bool isStructureMember) const override;

    //! Return the string that sets this encodable to specific value in code
    std::string getSetToValueString(bool isStructureMember, std::string value) const;

    //! Return the strings that #define initial and variable values
    std::string getInitialAndVerifyDefines(bool includeComment = true) const override;

    //! Make this primitive not a default
    void clearDefaults(void) override {defaultString.clear();}

    //! True if this encodable overrides a previous encodable
    bool overridesPreviousEncodable(void) const override {return overridesPrevious;}

    //! Clear the override flag, its not allowed
    void clearOverridesPrevious(void) override {overridesPrevious = false;}

    //! True if this encodable invalidates an earlier default
    bool invalidatesPreviousDefault(void) const override {return !inMemoryType.isNull && !usesDefaults() && !overridesPrevious;}

    //! True if this encodable has a direct child that uses bitfields
    bool usesBitfields(void) const override;

    //! True if this bitfield crosses a byte boundary
    bool bitfieldCrossesByteBoundary(void) const;

    //! True if this encodable needs a temporary buffer for its bitfield during encode
    bool usesEncodeTempBitfield(void) const override;

    //! True if this encodable needs a temporary buffer for its long bitfield during encode
    bool usesEncodeTempLongBitfield(void) const override;

    //! True if this encodable needs a temporary buffer for its bitfield during decode
    bool usesDecodeTempBitfield(void) const override;

    //! True if this encodable needs a temporary buffer for its long bitfield during decode
    bool usesDecodeTempLongBitfield(void) const override;

    //! True if this encodable has a direct child that needs an iterator on encode
    bool usesEncodeIterator(void) const override {return (isArray() && !isNotEncoded() && !inMemoryType.isString);}

    //! True if this encodable has a direct child that needs an iterator on decode
    bool usesDecodeIterator(void) const override {return (isArray() && !inMemoryType.isNull && !isNotEncoded() && !inMemoryType.isString);}

    //! True if this encodable has a direct child that needs an iterator for verifying
    bool usesVerifyIterator(void) const override {return hasVerify() && usesEncodeIterator();}

    //! True if this encodable has a direct child that needs an iterator for initializing
    bool usesInitIterator(void) const override {return hasInit() && usesEncodeIterator();}

    //! True if this encodable has a direct child that needs an iterator on encode
    bool uses2ndEncodeIterator(void) const override {return (is2dArray() && !isNotEncoded() && !inMemoryType.isString);}

    //! True if this encodable has a direct child that needs an iterator on decode
    bool uses2ndDecodeIterator(void) const override {return (is2dArray() && !inMemoryType.isNull && !isNotEncoded() && !inMemoryType.isString);}

    //! True if this encodable has a direct child that needs an second iterator for verifying
    bool uses2ndVerifyIterator(void) const override {return hasVerify() && uses2ndEncodeIterator();}

    //! True if this encodable has a direct child that needs an second iterator for initializing
    bool uses2ndInitIterator(void) const override {return hasInit() && uses2ndEncodeIterator();}

    //! True if this encodable has a direct child that uses defaults
    bool usesDefaults(void) const override {return (isDefault() && !isNotEncoded());}

protected:

    //! Minimum encoded value (as a number), used by scaling routines for unsigned encodings
    double encodedMin;

    //! Maximum encoded value (as a number), used by scaling routines
    double encodedMax;

    //! Multiplier (as a number) to convert in-Memory to scaled encoded value
    double scaler;

    //! String providing the maximum encoded value
    std::string maxString;

    //! String providing the minimum encoded value
    std::string minString;

    //! String providing the scaler from in-Memory to encoded
    std::string scalerString;

    //! The string used to multiply the in-memory type to compare and print to text
    std::string printScalerString;

    //! The string used to divide the in-memory type to read from text
    std::string readScalerString;

    //! String giving the default value to use if the packet is too short
    std::string defaultString;

    //! String giving the default value for documentation purposes only
    std::string defaultStringForDisplay;

    //! String giving the constant value to use on encoding
    std::string constantString;

    //! String giving the constant value for documentation purposes only
    std::string constantStringForDisplay;

    //! The string used in the set to defaults function
    std::string initialValueString;

    //! The string used in the set to defaults function, for documentation purposes only
    std::string initialValueStringForDisplay;

    //! The string used to verify the value on the low side
    std::string verifyMinString;

    //! The string used to verify the value on the low side, for documentation purposes only
    std::string verifyMinStringForDisplay;

    //! Flag if we know the verify min value
    bool hasVerifyMinValue;

    //! The minimum verify value
    double verifyMinValue;

    //! The minimum value of the encoding, or the verifyMin value, whichever is min
    double limitMinValue;

    //! The string for the limit min value
    std::string limitMinString;

    //! The string for the limit min value used in comments
    std::string limitMinStringForComment;

    //! The string used to verify the value on the high side
    std::string verifyMaxString;

    //! The string used to verify the value on the high side, for documentation purposes only
    std::string verifyMaxStringForDisplay;

    //! Flag if we know the verify max value
    bool hasVerifyMaxValue;

    //! The maximum verify value
    double verifyMaxValue;

    //! The maximum value of the encoding, or the verifyMax value, whichever is less
    double limitMaxValue;

    //! The string for the limit max value
    std::string limitMaxString;

    //! The string for the limit max value used in comments
    std::string limitMaxStringForComment;

    //! Flag to force this the decode function to verify the result against the constant value
    bool checkConstant;

    //! Flag indicating this field overrides a previous field
    bool overridesPrevious;

    //! Flag indicating this field is being overriden by a later one
    bool isOverriden;

    //! In-memory type information
    TypeData inMemoryType;

    //! Encoded type information
    TypeData encodedType;

    //! Details about the bitfield (if any) and its relationship to adjacent bitfields
    BitfieldData bitfieldData;

    //! List of extra fields that can be appended to a <Data> tag within a packet description
    std::vector<std::string> extraInfoNames;
    std::vector<std::string> extraInfoValues;

    //! Pointer to the previous protocol field, or NULL if none
    ProtocolField* prevField;

    //! Indicates if this field is hidden from documentation
    bool hidden;

    //! Indicates this field should never be omitted, even if hidden and -omit-hidden is set
    bool neverOmit;

    //! Indicates the map encode / decode settings for this field
    int mapOptions;

    //! Extract the type information from the type string
    void extractType(TypeData& data, const std::string& typeString, bool inMemory, const std::string& enumName = std::string());

    //! Return the constant value string, sourced from either constantValue, encodeConstantValue, decodeConstantValue
    std::string getConstantString() const;

    //! Adjust input string for presence of numeric constants "pi" and "e"
    std::string handleNumericConstants(std::string& input) const;

    //! Compute the encoded length string
    void computeEncodedLength(void);

    //! Indicate if this bitfield is the last bitfield in this group
    void setTerminatesBitfield(bool terminate) {bitfieldData.lastBitfield = terminate; computeEncodedLength();}

    //! Set the starting bitcount for this fields bitfield
    void setStartingBitCount(int bitcount) {bitfieldData.startingBitCount = bitcount; computeEncodedLength();}

    //! Get the ending bitcount for this fields bitfield
    int getEndingBitCount(void){return bitfieldData.startingBitCount + encodedType.bits;}

    //! Get the comment that describes the encoded range
    std::string getRangeComment(bool limitonencode = false) const;

    //! Determine if an argument should be passed to the limiting macro
    std::string getLimitedArgument(std::string argument) const;

    //! Get the next lines(s) of source coded needed to encode a bitfield field
    std::string getEncodeStringForBitfield(int* bitcount, bool isStructureMember) const;

    //! Get the next lines of source needed to encode a string field
    std::string getEncodeStringForString(bool isStructureMember) const;

    //! Get the next lines of source needed to encode a string field
    std::string getEncodeStringForStructure(bool isStructureMember) const;

    //! Get the next lines(s, bool isStructureMember) of source coded needed to encode a field, which is not a bitfield or a string
    std::string getEncodeStringForField(bool isBigEndian, bool isStructureMember) const;

    //! Get the next lines(s, bool isStructureMember) of source coded needed to decode a bitfield field
    std::string getDecodeStringForBitfield(int* bitcount, bool isStructureMember, bool defaultEnabled) const;

    //! Get the next lines of source needed to decode a string field
    std::string getDecodeStringForString(bool isStructureMember, bool defaultEnabled) const;

    //! Get the next lines of source needed to decode a string field
    std::string getDecodeStringForStructure(bool isStructureMember) const;

    //! Get the next lines(s, bool isStructureMember) of source coded needed to decode a field, which is not a bitfield or a string
    std::string getDecodeStringForField(bool isBigEndian, bool isStructureMember, bool defaultEnabled) const;

    //! Get the source needed to close out a string of bitfields in the encode function.
    std::string getCloseBitfieldString(int* bitcount) const;

    //! Check to see if we should be doing floating point scaling on this field
    bool isFloatScaling(void) const;

    //! Check to see if we should be doing integer scaling on this field
    bool isIntegerScaling(void) const;

};

#endif // PROTOCOLFIELD_H
