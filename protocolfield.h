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
    TypeData();

    //! Construct type data by copying another type
    TypeData(const TypeData& that);

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

    //! Pull a positive integer value from a string
    int extractPositiveInt(const QString& string, bool* ok = 0);

    //! Pull a double precision value from a string
    double extractDouble(const QString& string, bool* ok = 0);

    //! Extract the type information from the type string
    void extractType(const QString& typeString, const ProtocolSupport& support, const QString& name, bool inMemory);

    //! Create the type string
    QString toTypeString(QString enumName = QString(), QString structName = QString()) const;

};


class ProtocolField : public Encodable
{
public:

    //! Construct a field, setting the protocol name and name prefix
    ProtocolField(const QString& protocolName, const QString& protocolPrefix, ProtocolSupport supported);

    //! Construct a protocol field by parsing a DOM element
    ProtocolField(const QString& protocolName, const QString& protocolPrefix, ProtocolSupport supported, const QDomElement& field);

    //! Get a properly formatted number string for a double precision number
    static QString getNumberString(double number, int bits = 64);

    //! Get a properly formatted number string for a double precision number, with special care for pi
    static QString getDisplayNumberString(double number);

    //! Destroy the protocol field
    virtual ~ProtocolField(){}

    //! Reset all data to defaults
    virtual void clear(void);

    //! Parse the DOM element
    virtual void parse(const QDomElement& field);

    //! Returns true since protocol field is a primitive type
    virtual bool isPrimitive(void) const {return true;}

    //! True if this encoable is a primitive bitfield
    virtual bool isBitfield(void) const {return (encodedType.isBitfield && !isNotEncoded());}

    //! Indicate if this bitfield is the last bitfield in this group
    virtual void setTerminatesBitfield(bool terminate) {lastBitfield = terminate; computeEncodedLength();}

    //! Set the starting bitcount for this fields bitfield
    virtual void setStartingBitCount(int bitcount) {startingBitCount = bitcount; computeEncodedLength();}

    //! Get the ending bitcount for this fields bitfield
    virtual int getEndingBitCount(void){return startingBitCount + encodedType.bits;}

    //! True if this encodable has a default value
    virtual bool isDefault(void) const {return !defaultValue.isEmpty();}

    //! Get the declaration for this field
    virtual QString getDeclaration(void) const;

    //! Return the inclue directive needed for this encodable
    virtual QString getIncludeDirective(void);

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
    virtual void clearDefaults(void) {defaultValue.clear();}

    //! True if this encodable has a direct child that uses bitfields
    virtual bool usesBitfields(void) const {return (encodedType.isBitfield && !isNotEncoded());}

    //! True if this encodable has a direct child that needs an iterator
    virtual bool usesIterator(void) const {return (isArray() && !isNotEncoded() && !inMemoryType.isString);}

    //! True if this encodable has a direct child that uses defaults
    virtual bool usesDefaults(void) const {return (isDefault() && !isNotEncoded());}


public:
    QString enumName;
    double encodedMin;
    double encodedMax;
    double scaler;
    QString maxString;
    QString minString;
    QString scalerString;
    QString defaultValue;
    QString constantValue;
    QString encodeConstantValue;
    QString decodeConstantValue;
    TypeData inMemoryType;
    TypeData encodedType;

    QStringList extraInfoNames; //!< List of extra fields that can be appended to a <Data> tag within a packet description
    QStringList extraInfoValues;

protected:

    bool lastBitfield;      //!< True if this is the last bitfield in the local group
    int startingBitCount;   //!< The starting bit count for this field

    //! Return the constant value string, sourced from either constantValue, encodeConstantValue, decodeConstantValue
    QString getConstantString() const;

    //! Compute the encoded length string
    void computeEncodedLength(void);

    //! Get the next lines(s) of source coded needed to encode a bitfield field
    QString getEncodeStringForBitfield(int* bitcount, bool isStructureMember) const;

    //! Get the next lines of source needed to encode a string field
    QString getEncodeStringForString(bool isStructureMember) const;

    //! Get the next lines of source needed to encode a string field
    QString getEncodeStringForStructure(bool isStructureMember) const;

    //! Get the next lines(s, bool isStructureMember) of source coded needed to encode a field, which is not a bitfield or a string
    QString getEncodeStringForField(bool isBigEndian, bool isStructureMember) const;

    //! Get the next lines(s, bool isStructureMember) of source coded needed to decode a bitfield field
    QString getDecodeStringForBitfield(int* bitcount, bool isStructureMember) const;

    //! Get the next lines of source needed to decode a string field
    QString getDecodeStringForString(bool isStructureMember) const;

    //! Get the next lines of source needed to decode a string field
    QString getDecodeStringForStructure(bool isStructureMember) const;

    //! Get the next lines(s, bool isStructureMember) of source coded needed to decode a field, which is not a bitfield or a string
    QString getDecodeStringForField(bool isBigEndian, bool isStructureMember, bool defaultEnabled = false) const;

    //! Get the source needed to close out a string of bitfields in the encode function.
    QString getCloseBitfieldString(int* bitcount) const;

    //! Compute the power of 2 raised to some bits
    uint64_t pow2(uint8_t bits) const;

};

#endif // PROTOCOLFIELD_H
