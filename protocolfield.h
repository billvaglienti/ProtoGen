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
    virtual void setPreviousEncodable(Encodable* prev);

    //! Get a properly formatted number string for a double precision number, with special care for pi
    static QString getDisplayNumberString(double number);

    //! Get a properly formatted number string for a double precision number
    QString getNumberString(double number, int bits = 64) const;

    //! Destroy the protocol field
    virtual ~ProtocolField(){}

    //! Reset all data to defaults
    virtual void clear(void);

    //! Parse the DOM element
    virtual void parse(void);

    //! Check names against the list of C keywords
    virtual void checkAgainstKeywords(void);

    //! The hierarchical name of this object
    virtual QString getHierarchicalName(void) const {return parent + ":" + name;}

    //! Returns true since protocol field is a primitive type
    virtual bool isPrimitive(void) const {return true;}

    //! True if this encodable is NOT encoded
    virtual bool isNotEncoded(void) const {return (encodedType.isNull);}

    //! True if this encodable is NOT in memory
    virtual bool isNotInMemory(void) const {return inMemoryType.isNull;}

    //! True if this encodable is a constant
    virtual bool isConstant(void) const {return !constantString.isEmpty();}

    //! True if this encoable is a primitive bitfield
    virtual bool isBitfield(void) const {return (encodedType.isBitfield && !isNotEncoded());}

    //! True if this encodable has a default value
    virtual bool isDefault(void) const {return !defaultString.isEmpty();}

    //! Get the maximum number of temporary bytes needed for a bitfield group
    virtual void getBitfieldGroupNumBytes(int* num) const;

    //! Get the declaration for this field
    virtual QString getDeclaration(void) const;

    //! Return the include directives needed for this encodable
    virtual void getIncludeDirectives(QStringList& list) const;

    //! Return the signature of this field in an encode function signature
    virtual QString getEncodeSignature(void) const;

    //! Get details needed to produce documentation for this encodable.
    virtual void getDocumentationDetails(QList<int>& outline, QString& startByte, QStringList& bytes, QStringList& names, QStringList& encodings, QStringList& repeats, QStringList& comments) const;

    //! Return the string that is used to encode this encodable
    virtual QString getEncodeString(bool isBigEndian, int* bitcount, bool isStructureMember) const;

    //! Return the string that is used to decode this encoable
    virtual QString getDecodeString(bool isBigEndian, int* bitcount, bool isStructureMember, bool defaultEnabled = false) const;

    //! Return the string that sets this encodable to its default value in code
    virtual QString getSetToDefaultsString(bool isStructureMember) const;

    //! Make this primitive not a default
    virtual void clearDefaults(void) {defaultString.clear();}

    //! True if this encodable has a direct child that uses bitfields
    virtual bool usesBitfields(void) const {return (encodedType.isBitfield && !isNotEncoded());}

    //! True if this encodable has a direct child that needs an iterator on encode
    virtual bool usesEncodeIterator(void) const {return (isArray() && !isNotEncoded() && !inMemoryType.isString);}

    //! True if this encodable has a direct child that needs an iterator on decode
    virtual bool usesDecodeIterator(void) const {return (isArray() && !inMemoryType.isNull && !isNotEncoded() && !inMemoryType.isString);}

    //! True if this encodable has a direct child that needs an iterator on encode
    virtual bool uses2ndEncodeIterator(void) const {return (is2dArray() && !isNotEncoded() && !inMemoryType.isString);}

    //! True if this encodable has a direct child that needs an iterator on decode
    virtual bool uses2ndDecodeIterator(void) const {return (is2dArray() && !inMemoryType.isNull && !isNotEncoded() && !inMemoryType.isString);}

    //! True if this encodable has a direct child that uses defaults
    virtual bool usesDefaults(void) const {return (isDefault() && !isNotEncoded());}

protected:
    QString enumName;
    double encodedMin;
    double encodedMax;
    double scaler;
    QString maxString;
    QString minString;
    QString scalerString;
    QString defaultString;
    QString defaultStringForDisplay;
    QString constantString;
    QString constantStringForDisplay;
    bool checkConstant;
    TypeData inMemoryType;
    TypeData encodedType;
    BitfieldData bitfieldData;

    QStringList extraInfoNames; //!< List of extra fields that can be appended to a <Data> tag within a packet description
    QStringList extraInfoValues;

    ProtocolField* prevField;   //!< Pointer to the previous protocol field, or NULL if none

    //! Extract the type information from the type string
    void extractType(TypeData& data, const QString& typeString, const QString& name, bool inMemory);

    //! Return the constant value string, sourced from either constantValue, encodeConstantValue, decodeConstantValue
    QString getConstantString() const;

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
    QString getDecodeStringForString(bool isStructureMember) const;

    //! Get the next lines of source needed to decode a string field
    QString getDecodeStringForStructure(bool isStructureMember) const;

    //! Get the next lines(s, bool isStructureMember) of source coded needed to decode a field, which is not a bitfield or a string
    QString getDecodeStringForField(bool isBigEndian, bool isStructureMember, bool defaultEnabled) const;

    //! Get the source needed to close out a string of bitfields in the encode function.
    QString getCloseBitfieldString(int* bitcount) const;

    //! Compute the power of 2 raised to some bits
    uint64_t pow2(uint8_t bits) const;

};

#endif // PROTOCOLFIELD_H
