#ifndef PROTOCOLFIELD_H
#define PROTOCOLFIELD_H

#include <QString>
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

    //! True if this encodable has a default value
    virtual bool isDefault(void) const {return !defaultValue.isEmpty();}

    //! Get the declaration for this field
    virtual QString getDeclaration(void) const;

    //! Return the inclue directive needed for this encodable
    virtual QString getIncludeDirective(void);

    //! Return the signature of this field in an encode function signature
    virtual QString getEncodeSignature(void) const;

    //! Return markdown formatted information about this encodables fields
    virtual QString getMarkdown(QString indent) const;

    //! Return the string that is used to encode this encodable
    virtual QString getEncodeString(bool isBigEndian, EncodedLength& encLength, int* bitcount, bool isStructureMember) const;

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
    QString defaultValue;
    QString constantValue;
    TypeData inMemoryType;
    TypeData encodedType;

protected:

    //! Get the next lines(s) of source coded needed to encode a bitfield field
    QString getEncodeStringForBitfield(int* bitcount, bool isStructureMember) const;

    //! Get the next lines of source needed to encode a string field
    QString getEncodeStringForString(EncodedLength& encLength, bool isStructureMember) const;

    //! Get the next lines of source needed to encode a string field
    QString getEncodeStringForStructure(EncodedLength& encLength, bool isStructureMember) const;

    //! Get the next lines(s, bool isStructureMember) of source coded needed to encode a field, which is not a bitfield or a string
    QString getEncodeStringForField(bool isBigEndian, EncodedLength& encLength, bool isStructureMember) const;

    //! Get the next lines(s, bool isStructureMember) of source coded needed to decode a bitfield field
    QString getDecodeStringForBitfield(int* bitcount, bool isStructureMember) const;

    //! Get the next lines of source needed to decode a string field
    QString getDecodeStringForString(bool isStructureMember) const;

    //! Get the next lines of source needed to decode a string field
    QString getDecodeStringForStructure(bool isStructureMember) const;

    //! Get the next lines(s, bool isStructureMember) of source coded needed to decode a field, which is not a bitfield or a string
    QString getDecodeStringForField(bool isBigEndian, bool isStructureMember, bool defaultEnabled = false) const;

};

#endif // PROTOCOLFIELD_H
