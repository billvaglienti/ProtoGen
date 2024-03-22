#ifndef ENCODABLE_H
#define ENCODABLE_H

#include "protocolsupport.h"
#include "encodedlength.h"
#include "protocoldocumentation.h"

class Encodable : public ProtocolDocumentation
{
public:

    //! Constructor for basic encodable that sets protocol options
    Encodable(ProtocolParser* parse, const std::string& parent, ProtocolSupport supported);

    virtual ~Encodable() {;}

    //! Construct a protocol field by parsing a DOM element
    static Encodable* generateEncodable(ProtocolParser* parse, const std::string& parent, ProtocolSupport supported, const XMLElement* field);

    //! Provide the pointer to a previous encodable in the list
    virtual void setPreviousEncodable(Encodable* prev) {(void)prev;}

    //! Check names against the list of C keywords
    virtual void checkAgainstKeywords(void);

    //! Reset all data to defaults
    virtual void clear(void);

    //! The hierarchical name of this object
    virtual std::string getHierarchicalName(void) const = 0;

    //! Return the string that is used to declare this encodable
    virtual std::string getDeclaration(void) const = 0;

    //! Return the string that is used to encode this encodable
    virtual std::string getEncodeString(bool isBigEndian, int* bitcount, bool isStructureMember) const = 0;

    //! Return the string that is used to decode this encoable
    virtual std::string getDecodeString(bool isBigEndian, int* bitcount, bool isStructureMember, bool defaultEnabled = false) const = 0;

    //! Get the string used for verifying this field.
    virtual std::string getVerifyString(void) const {return std::string();}

    //! Return the string that sets this encodable to its initial value in code
    virtual std::string getSetInitialValueString(bool isStructureMember) const {(void)isStructureMember; return std::string();}

    //! Return the strings that #define initial and variable values
    virtual std::string getInitialAndVerifyDefines(bool includeComment = true) const {(void)includeComment; return std::string();}

    //! Get the string which identifies this encodable in a CAN DBC file
    virtual std::string getDBCSignalString(std::string prename, bool isBigEndian, int* bitcount) const {(void)prename; (void)isBigEndian; (void)bitcount; return std::string();}

    //! Get the string which comments this encodable in a CAN DBC file
    virtual std::string getDBCSignalComment(std::string prename, uint32_t ID) const {(void)prename; (void)ID; return std::string();}

    //! Get the string which comments this encodables enumerations in a CAN DBC file
    virtual std::string getDBCSignalEnum(std::string prename, uint32_t ID) const {(void)prename; (void)ID; return std::string();}

    //! Get the string used for comparing this field.
    virtual std::string getComparisonString(void) const {return std::string();}

    //! Get the string used for text printing this field.
    virtual std::string getTextPrintString(void) const {return std::string();}

    //! Get the string used for text reading this field.
    virtual std::string getTextReadString(void) const {return std::string();}

    //! Get the string used to encode this field to a map
    virtual std::string getMapEncodeString(void) const {return std::string();}

    //! Get the string used to decode this field from a map
    virtual std::string getMapDecodeString(void) const {return std::string();}

    //! Return the string that sets this encodable to its default value in code
    virtual std::string getSetToDefaultsString(bool isStructureMember) const {(void)isStructureMember; return std::string();}

    //! Return the include directives needed for this encodable
    virtual void getIncludeDirectives(std::vector<std::string>& list) const {(void)list;}

    //! Return the include directives that go into source code needed for this encodable
    virtual void getSourceIncludeDirectives(std::vector<std::string>& list) const {(void)list;}

    //! Return the include directives needed for this encodable's init and verify functions
    virtual void getInitAndVerifyIncludeDirectives(std::vector<std::string>& list) const {(void)list;}

    //! Return the include directives needed for this encodable's map functions
    virtual void getMapIncludeDirectives(std::vector<std::string>& list) const {(void)list;}

    //! Return the include directives needed for this encodable's compare functions
    virtual void getCompareIncludeDirectives(std::vector<std::string>& list) const {(void)list;}

    //! Return the include directives needed for this encodable's print functions
    virtual void getPrintIncludeDirectives(std::vector<std::string>& list) const {(void)list;}

    //! Return the signature of this field in an encode function signature
    virtual std::string getEncodeSignature(void) const;

    //! Return the signature of this field in a decode function signature
    virtual std::string getDecodeSignature(void) const;

    //! Return the string that documents this field as a encode function parameter
    virtual std::string getEncodeParameterComment(void) const;

    //! Return the string that documents this field as a decode function parameter
    virtual std::string getDecodeParameterComment(void) const;

    //! Get the string which accessses this field in code for purposes of encoding the field
    virtual std::string getEncodeFieldAccess(bool isStructureMember) const;

    //! Get the string which accessses this field in code for purposes of encoding the field
    virtual std::string getEncodeFieldAccess(bool isStructureMember, const std::string& variable) const;

    //! Get the string which accessses this field in code for purposes of decoding the field
    virtual std::string getDecodeFieldAccess(bool isStructureMember) const;

    //! Get the string which accessses this field in code for purposes of decoding the field
    virtual std::string getDecodeFieldAccess(bool isStructureMember, const std::string& variable) const;

    //! Get a positive or negative return code string, which is language specific
    virtual std::string getReturnCode(bool positive) const;

    //! Get the array handling code for encoding context
    virtual std::string getEncodeArrayIterationCode(const std::string& spacing, bool isStructureMember) const;

    //! Get the array handling code for decoding context
    virtual std::string getDecodeArrayIterationCode(const std::string& spacing, bool isStructureMember) const;

    //! Return true if this encodable has documentation for markdown output
    virtual bool hasDocumentation(void) {return true;}

    //! Get details needed to produce documentation for this encodable.
    virtual void getDocumentationDetails(std::vector<int>& outline, std::string& startByte, std::vector<std::string>& bytes, std::vector<std::string>& names, std::vector<std::string>& encodings, std::vector<std::string>& repeats, std::vector<std::string>& comments) const = 0;

    //! Get documentation repeat details for array or 2d arrays
    std::string getRepeatsDocumentationDetails(void) const;

    //! Make this encodable not a default
    virtual void clearDefaults(void) {}

    //! Determine if this encodable a primitive object or a structure
    virtual bool isPrimitive(void) const = 0;

    //! Determine if this encodable a string object
    virtual bool isString(void) const = 0;

    //! Determine if this encodable is an array
    bool isArray(void) const {return !array.empty();}

    //! Determine if this encodable is a 2d array
    bool is2dArray(void) const {return (isArray() && !array2d.empty());}

    //! True if this encodable has verification data
    virtual bool hasVerify(void) const {return false;}

    //! True if this encodable has initialization data
    virtual bool hasInit(void) const {return false;}

    //! True if this encodable is NOT encoded
    virtual bool isNotEncoded(void) const {return false;}

    //! True if this encodable is NOT in memory
    virtual bool isNotInMemory(void) const {return false;}

    //! True if this encodable is a constant
    virtual bool isConstant(void) const {return false;}

    //! True if this encodable is a primitive bitfield
    virtual bool isBitfield(void) const {return false;}

    //! True if this encodable has a default value
    virtual bool isDefault(void) const {return false;}

    //! Get the maximum number of temporary bytes needed for a bitfield group
    virtual void getBitfieldGroupNumBytes(int* num) const {(void)num;}

    //! True if this encodable uses bitfields or has a child that does
    virtual bool usesBitfields(void ) const = 0;

    //! True if this encodable needs a temporary buffer for its bitfield during encode
    virtual bool usesEncodeTempBitfield(void) const {return false;}

    //! True if this encodable needs a temporary buffer for its bitfield during encode
    virtual bool usesEncodeTempLongBitfield(void) const {return false;}

    //! True if this encodable needs a temporary buffer for its bitfield during decode
    virtual bool usesDecodeTempBitfield(void) const {return false;}

    //! True if this encodable needs a temporary buffer for its bitfield during decode
    virtual bool usesDecodeTempLongBitfield(void) const {return false;}

    //! True if this encodable has a direct child that needs an iterator for encoding
    virtual bool usesEncodeIterator(void) const = 0;

    //! True if this encodable has a direct child that needs an iterator for decoding
    virtual bool usesDecodeIterator(void) const = 0;

    //! True if this encodable has a direct child that needs an iterator for verifying
    virtual bool usesVerifyIterator(void) const = 0;

    //! True if this encodable has a direct child that needs an iterator for initializing
    virtual bool usesInitIterator(void) const = 0;

    //! True if this encodable has a direct child that needs a second iterator for encoding
    virtual bool uses2ndEncodeIterator(void) const = 0;

    //! True if this encodable has a direct child that needs a second iterator for decoding
    virtual bool uses2ndDecodeIterator(void) const = 0;

    //! True if this encodable has a direct child that needs an second iterator for verifying
    virtual bool uses2ndVerifyIterator(void) const = 0;

    //! True if this encodable has a direct child that needs an second iterator for initializing
    virtual bool uses2ndInitIterator(void) const = 0;

    //! True if this encodable has a direct child that uses defaults
    virtual bool usesDefaults(void) const = 0;

    //! True if this encodable overrides a previous encodable
    virtual bool overridesPreviousEncodable(void) const {return false;}

    //! Clear the override flag, its not allowed
    virtual void clearOverridesPrevious(void) {}

    //! True if this encodable invalidates an earlier default
    virtual bool invalidatesPreviousDefault(void) const {return !usesDefaults();}

    //! Add successive length strings
    static void addToLengthString(std::string & totalLength, const std::string & length);

    //! Add successive length strings
    static void addToLengthString(std::string* totalLength, const std::string & length);

public:

    std::string typeName;        //!< The type name of this encodable, like "uint8_t" or "myStructure_t"
    std::string array;           //!< The array length of this encodable, empty if no array
    std::string array2d;         //!< The second dimension array length of this encodable, empty if no 2nd dimension
    std::string variableArray;   //!< variable that gives the length of the array in a packet
    std::string variable2dArray; //!< variable that gives the length of the 2nd array dimension in a packet
    std::string dependsOn;       //!< variable that determines if this field is present
    std::string dependsOnValue;  //!< String providing the details of the depends on value
    std::string dependsOnCompare;//!< Comparison to use for dependsOnValue
    EncodedLength encodedLength; //!< The lengths of the encodables
};

#endif // ENCODABLE_H
