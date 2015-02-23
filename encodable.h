#ifndef ENCODABLE_H
#define ENCODABLE_H

#include "protocolsupport.h"
#include "encodedlength.h"
#include <QDomElement>
#include <QString>

class Encodable
{
public:

    //! Constructor for basic encodable that sets protocl name and prefix
    Encodable(const QString& protocolName, const QString& protocolPrefix, ProtocolSupport supported);

    virtual ~Encodable() {;}

    //! Construct a protocol field by parsing a DOM element
    static Encodable* generateEncodable(const QString& protocolName, const QString& protocolPrefix, ProtocolSupport supported, const QDomElement& field);

    //! Parse the DOM element
    virtual void parse(const QDomElement& field) = 0;

    //! Reset all data to defaults
    virtual void clear(void);

    //! Return the string that gives the prototype of the function used to encode this encodable, may be empty
    virtual QString getPrototypeEncodeString(bool isBigEndian, EncodedLength* encLength = NULL) {return QString();}

    //! Return the string that gives the prototype of the function used to decode this encodable, may be empty
    virtual QString getPrototypeDecodeString(bool isBigEndian) const {return QString();}

    //! Return the string that is used to encode this encodable
    virtual QString getEncodeString(bool isBigEndian, EncodedLength& encLength, int* bitcount, bool isStructureMember) const = 0;

    //! Return the string that is used to decode this encoable
    virtual QString getDecodeString(bool isBigEndian, int* bitcount, bool isStructureMember, bool defaultEnabled = false) const = 0;

    //! Return the string that is used to declare this encodable
    virtual QString getDeclaration(void) const = 0;

    //! Return the inclue directive needed for this encodable
    virtual QString getIncludeDirective(void) {return QString();}

    //! Return the string that declares the whole structure
    virtual QString getStructureDeclaration(bool alwaysCreate) const {return QString();}

    //! Return the signature of this field in an encode function signature
    virtual QString getEncodeSignature(void) const;

    //! Return the signature of this field in a decode function signature
    virtual QString getDecodeSignature(void) const;

    //! Return the string that documents this field as a encode function parameter
    virtual QString getEncodeParameterComment(void) const;

    //! Return the string that documents this field as a decode function parameter
    virtual QString getDecodeParameterComment(void) const;

    //! Return the string that sets this encodable to its default value in code
    virtual QString getSetToDefaultsString(bool isStructureMember) const {return QString();}

    //! Get details needed to produce documentation for this encodable.
    virtual void getDocumentationDetails(QString parentName, QStringList& names, QStringList& encodings, QStringList& repeats, QStringList& comments) const = 0;

    //! Make this encodable not a default
    virtual void clearDefaults(void) {}

    //! Determine if this encodable a primitive object or a structure?
    virtual bool isPrimitive(void) const = 0;

    //! Determine if this encodable an array
    bool isArray(void) const {return !array.isEmpty();}

    //! True if this encodable is NOT encoded
    bool isNotEncoded(void) const {return notEncoded;}

    //! True if this encodable is NOT in memory
    bool isNotInMemory(void) const {return notInMemory;}

    //! True if this encodable is a constant
    bool isConstant(void) const {return constant;}

    //! True if this encodable is a primitive bitfield
    virtual bool isBitfield(void) const {return false;}

    //! True if this encodable has a default value
    virtual bool isDefault(void) const {return false;}

    //! True if this encodable has a direct child that uses bitfields
    virtual bool usesBitfields(void ) const = 0;

    //! True if this encodable has a direct child that needs an iterator
    virtual bool usesIterator(void) const = 0;

    //! True if this encodable has a direct child that uses defaults
    virtual bool usesDefaults(void) const = 0;

    //! Add successive length strings
    static void addToLengthString(QString & totalLength, const QString & length);

    //! Add successive length strings
    static void addToLengthString(QString* totalLength, const QString & length);

protected:

    //! Get the source needed to close out a string of bitfields in the encode function.
    static QString getCloseBitfieldString(int* bitcount, EncodedLength* encLength = NULL);

public:
    ProtocolSupport support;//!< Information about what is supported
    QString protoName;      //!< Name of the protocol
    QString prefix;         //!< Prefix of names
    QString typeName;       //!< The type name of this encodable, like "uint8_t" or "myStructure_t"
    QString name;           //!< The name of this encodable
    QString comment;        //!< The comment that goes with this encodable
    QString array;          //!< The array length of this encodable, empty if no array
    QString variableArray;  //!< variable that gives the length of the array in a packet
    QString dependsOn;      //!< variable that determines if this field is present
    EncodedLength encodedLength;    //!< The lengths of the encodables
    bool notEncoded;        //!< True if this encodable is NOT encoded
    bool notInMemory;       //!< True if this encodable is NOT in memory
    bool constant;          //!< True if this encodable is a constant
};

#endif // ENCODABLE_H
