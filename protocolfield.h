#ifndef PROTOCOLFIELD_H
#define PROTOCOLFIELD_H

#include <stdint.h>
#include <QString>
#include <QStringList>
#include <QDomElement>
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

    //! Pull a positive integer value from a string
    static int extractPositiveInt(const QString& string, bool* ok = 0);

    //! Pull a double precision value from a string
    static double extractDouble(const QString& string, bool* ok = 0);

    //! Create the type string
    QString toTypeString(QString enumName = QString(), QString structName = QString()) const;

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

    //! Construct a field, setting the protocol name and name prefix
    ProtocolField(ProtocolParser* parse, QString parent, ProtocolSupport supported);

    //! Provide the pointer to a previous encodable in the list
    virtual void setPreviousEncodable(Encodable* prev) Q_DECL_OVERRIDE;

    //! Get overriden type information
    bool getOverriddenTypeData(ProtocolField* prev);

    //! Get a properly formatted number string for a double precision number, with special care for pi
    static QString getDisplayNumberString(double number);

    //! Get a properly formatted number string for a double precision number
    QString getNumberString(double number, int bits = 64) const;

    //! Destroy the protocol field
    virtual ~ProtocolField(){}

    //! Reset all data to defaults
    virtual void clear(void) Q_DECL_OVERRIDE;

    //! Parse the DOM element
    virtual void parse(void) Q_DECL_OVERRIDE;

    //! Check names against the list of C keywords
    virtual void checkAgainstKeywords(void) Q_DECL_OVERRIDE;

    //! The hierarchical name of this object
    virtual QString getHierarchicalName(void) const Q_DECL_OVERRIDE {return parent + ":" + name;}

    //! Returns true since protocol field is a primitive type
    virtual bool isPrimitive(void) const Q_DECL_OVERRIDE {return true;}

    //! True if this encodable is NOT encoded
    virtual bool isNotEncoded(void) const Q_DECL_OVERRIDE {return (encodedType.isNull);}

    //! True if this encodable is NOT in memory. Note how overriding a previous field means we are not in memory (because the previous one is)
    virtual bool isNotInMemory(void) const Q_DECL_OVERRIDE {return (inMemoryType.isNull || overridesPrevious);}

    //! True if this encodable is a constant. Note that protocol fields which are null in memory are constant by definition
    virtual bool isConstant(void) const Q_DECL_OVERRIDE {return (!constantString.isEmpty() || inMemoryType.isNull);}

    //! True if this encoable is a primitive bitfield
    virtual bool isBitfield(void) const Q_DECL_OVERRIDE {return (encodedType.isBitfield && !isNotEncoded());}

    //! True if this encodable has a default value
    virtual bool isDefault(void) const Q_DECL_OVERRIDE {return !defaultString.isEmpty();}

    //! Get the maximum number of temporary bytes needed for a bitfield group
    virtual void getBitfieldGroupNumBytes(int* num) const Q_DECL_OVERRIDE;

    //! Get the declaration for this field
    virtual QString getDeclaration(void) const Q_DECL_OVERRIDE;

    //! Return the include directives needed for this encodable
    virtual void getIncludeDirectives(QStringList& list) const Q_DECL_OVERRIDE;

    //! Return the include directives needed for this encodable's init and verify functions
    virtual void getInitAndVerifyIncludeDirectives(QStringList& list) const Q_DECL_OVERRIDE;

    //! Return the signature of this field in an encode function signature
    virtual QString getEncodeSignature(void) const Q_DECL_OVERRIDE;

    //! Get details needed to produce documentation for this encodable.
    virtual void getDocumentationDetails(QList<int>& outline, QString& startByte, QStringList& bytes, QStringList& names, QStringList& encodings, QStringList& repeats, QStringList& comments) const Q_DECL_OVERRIDE;

    //! Return true if this field is hidden from documentation
    virtual bool isHidden (void) const Q_DECL_OVERRIDE {return hidden;}

    //! True if this encodable has verification data
    virtual bool hasVerify(void) const Q_DECL_OVERRIDE;

    //! True if this encodable has initialization data
    virtual bool hasInit(void) const Q_DECL_OVERRIDE;

    //! Return the string that is used to encode this encodable
    virtual QString getEncodeString(bool isBigEndian, int* bitcount, bool isStructureMember) const Q_DECL_OVERRIDE;

    //! Return the string that is used to decode this encoable
    virtual QString getDecodeString(bool isBigEndian, int* bitcount, bool isStructureMember, bool defaultEnabled = false) const Q_DECL_OVERRIDE;

    //! Return the string that sets this encodable to its default value in code
    virtual QString getSetToDefaultsString(bool isStructureMember) const Q_DECL_OVERRIDE;

    //! Get the string used for verifying this field.
    virtual QString getVerifyString(bool isStructureMember) const Q_DECL_OVERRIDE;

    //! Get the string used for comparing this field.
    virtual QString getComparisonString(bool isStructureMember) const Q_DECL_OVERRIDE;

    //! Get the string used for text printing this field.
    virtual QString getTextPrintString(bool isStructureMember) const Q_DECL_OVERRIDE;

    //! Get the string used for text reading this field.
    virtual QString getTextReadString(bool isStructureMember) const Q_DECL_OVERRIDE;

    //! Return the string that sets this encodable to its initial value in code
    virtual QString getSetInitialValueString(bool isStructureMember) const Q_DECL_OVERRIDE;

    //! Return the string that sets this encodable to specific value in code
    QString getSetToValueString(bool isStructureMember, QString value) const;

    //! Return the strings that #define initial and variable values
    virtual QString getInitialAndVerifyDefines(bool includeComment = true) const Q_DECL_OVERRIDE;

    //! Make this primitive not a default
    virtual void clearDefaults(void) Q_DECL_OVERRIDE {defaultString.clear();}

    //! True if this encodable overrides a previous encodable
    virtual bool overridesPreviousEncodable(void) const Q_DECL_OVERRIDE {return overridesPrevious;}

    //! Clear the override flag, its not allowed
    virtual void clearOverridesPrevious(void) Q_DECL_OVERRIDE {overridesPrevious = false;}

    //! True if this encodable invalidates an earlier default
    virtual bool invalidatesPreviousDefault(void) const Q_DECL_OVERRIDE {return !inMemoryType.isNull && !usesDefaults() && !overridesPrevious;}

    //! True if this encodable has a direct child that uses bitfields
    virtual bool usesBitfields(void) const Q_DECL_OVERRIDE;

    //! True if this field has a smaller encoded size than in memory size, which requires a size check
    bool requiresSizeCheck(void) const;

    //! True if this bitfield crosses a byte boundary
    bool bitfieldCrossesByteBoundary(void) const;

    //! True if this encodable needs a temporary buffer for its bitfield during encode
    virtual bool usesEncodeTempBitfield(void) const Q_DECL_OVERRIDE;

    //! True if this encodable needs a temporary buffer for its long bitfield during encode
    virtual bool usesEncodeTempLongBitfield(void) const Q_DECL_OVERRIDE;

    //! True if this encodable needs a temporary buffer for its bitfield during decode
    virtual bool usesDecodeTempBitfield(void) const Q_DECL_OVERRIDE;

    //! True if this encodable needs a temporary buffer for its long bitfield during decode
    virtual bool usesDecodeTempLongBitfield(void) const Q_DECL_OVERRIDE;

    //! True if this encodable has a direct child that needs an iterator on encode
    virtual bool usesEncodeIterator(void) const Q_DECL_OVERRIDE {return (isArray() && !isNotEncoded() && !inMemoryType.isString);}

    //! True if this encodable has a direct child that needs an iterator on decode
    virtual bool usesDecodeIterator(void) const Q_DECL_OVERRIDE {return (isArray() && !inMemoryType.isNull && !isNotEncoded() && !inMemoryType.isString);}

    //! True if this encodable has a direct child that needs an iterator for verifying
    virtual bool usesVerifyIterator(void) const Q_DECL_OVERRIDE {return hasVerify() && usesEncodeIterator();}

    //! True if this encodable has a direct child that needs an iterator for initializing
    virtual bool usesInitIterator(void) const Q_DECL_OVERRIDE {return hasInit() && usesEncodeIterator();}

    //! True if this encodable has a direct child that needs an iterator on encode
    virtual bool uses2ndEncodeIterator(void) const Q_DECL_OVERRIDE {return (is2dArray() && !isNotEncoded() && !inMemoryType.isString);}

    //! True if this encodable has a direct child that needs an iterator on decode
    virtual bool uses2ndDecodeIterator(void) const Q_DECL_OVERRIDE {return (is2dArray() && !inMemoryType.isNull && !isNotEncoded() && !inMemoryType.isString);}

    //! True if this encodable has a direct child that needs an second iterator for verifying
    virtual bool uses2ndVerifyIterator(void) const Q_DECL_OVERRIDE {return hasVerify() && uses2ndEncodeIterator();}

    //! True if this encodable has a direct child that needs an second iterator for initializing
    virtual bool uses2ndInitIterator(void) const Q_DECL_OVERRIDE {return hasInit() && uses2ndEncodeIterator();}

    //! True if this encodable has a direct child that uses defaults
    virtual bool usesDefaults(void) const Q_DECL_OVERRIDE {return (isDefault() && !isNotEncoded());}

protected:

    //! Name of the enumerated type for this field, empty if not enumerated type
    QString enumName;

    //! Minimum encoded value (as a number), used by scaling routines for unsigned encodings
    double encodedMin;

    //! Maximum encoded value (as a number), used by scaling routines
    double encodedMax;

    //! Multiplier (as a number) to convert in-Memory to scaled encoded value
    double scaler;

    //! String providing the maximum encoded value
    QString maxString;

    //! String providing the minimum encoded value
    QString minString;

    //! String providing the scaler from in-Memory to encoded
    QString scalerString;

    //! The string used to multiply the in-memory type to compare and print to text
    QString printScalerString;

    //! The string used to divide the in-memory type to read from text
    QString readScalerString;

    //! String giving the default value to use if the packet is too short
    QString defaultString;

    //! String giving the default value for documentation purposes only
    QString defaultStringForDisplay;

    //! String giving the constant value to use on encoding
    QString constantString;

    //! String giving the constant value for documentation purposes only
    QString constantStringForDisplay;

    //! The string used in the set to defaults function
    QString initialValueString;

    //! The string used in the set to defaults function, for documentation purposes only
    QString initialValueStringForDisplay;

    //! The string used to verify the value on the low side
    QString verifyMinString;

    //! The string used to verify the value on the low side, for documentation purposes only
    QString verifyMinStringForDisplay;

    //! The string used to verify the value on the high side
    QString verifyMaxString;

    //! The string used to verify the value on the high side, for documentation purposes only
    QString verifyMaxStringForDisplay;

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
    QStringList extraInfoNames;
    QStringList extraInfoValues;

    //! Pointer to the previous protocol field, or NULL if none
    ProtocolField* prevField;

    //! Indicates if this field is hidden from documentation
    bool hidden;

    //! Extract the type information from the type string
    void extractType(TypeData& data, const QString& typeString, const QString& name, bool inMemory);

    //! Return the constant value string, sourced from either constantValue, encodeConstantValue, decodeConstantValue
    QString getConstantString() const;

    //! Adjust input string for presence of numeric constants "pi" and "e"
    QString handleNumericConstants(QString& input) const;

    //! Compute the encoded length string
    void computeEncodedLength(void);

    //! Indicate if this bitfield is the last bitfield in this group
    void setTerminatesBitfield(bool terminate) {bitfieldData.lastBitfield = terminate; computeEncodedLength();}

    //! Set the starting bitcount for this fields bitfield
    void setStartingBitCount(int bitcount) {bitfieldData.startingBitCount = bitcount; computeEncodedLength();}

    //! Get the ending bitcount for this fields bitfield
    int getEndingBitCount(void){return bitfieldData.startingBitCount + encodedType.bits;}

    //! Get the next lines(s) of source coded needed to encode a bitfield field
    QString getEncodeStringForBitfield(int* bitcount, bool isStructureMember) const;

    //! Get the next lines of source needed to encode a string field
    QString getEncodeStringForString(bool isStructureMember) const;

    //! Get the next lines of source needed to encode a string field
    QString getEncodeStringForStructure(bool isStructureMember) const;

    //! Get the next lines(s, bool isStructureMember) of source coded needed to encode a field, which is not a bitfield or a string
    QString getEncodeStringForField(bool isBigEndian, bool isStructureMember) const;

    //! Get the next lines(s, bool isStructureMember) of source coded needed to decode a bitfield field
    QString getDecodeStringForBitfield(int* bitcount, bool isStructureMember, bool defaultEnabled) const;

    //! Get the next lines of source needed to decode a string field
    QString getDecodeStringForString(bool isStructureMember, bool defaultEnabled) const;

    //! Get the next lines of source needed to decode a string field
    QString getDecodeStringForStructure(bool isStructureMember) const;

    //! Get the next lines(s, bool isStructureMember) of source coded needed to decode a field, which is not a bitfield or a string
    QString getDecodeStringForField(bool isBigEndian, bool isStructureMember, bool defaultEnabled) const;

    //! Get the source needed to close out a string of bitfields in the encode function.
    QString getCloseBitfieldString(int* bitcount) const;

    //! Check to see if we should be doing floating point scaling on this field
    bool isFloatScaling(void) const;

    //! Check to see if we should be doing integer scaling on this field
    bool isIntegerScaling(void) const;

    //! Compute the power of 2 raised to some bits
    uint64_t pow2(uint8_t bits) const;

};

#endif // PROTOCOLFIELD_H
