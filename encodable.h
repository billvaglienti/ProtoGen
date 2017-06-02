#ifndef ENCODABLE_H
#define ENCODABLE_H

#include "protocolsupport.h"
#include "encodedlength.h"
#include "protocoldocumentation.h"
#include <QDomElement>
#include <QString>

class Encodable : public ProtocolDocumentation
{
public:

    //! Constructor for basic encodable that sets protocol options
    Encodable(ProtocolParser* parse, QString Parent, ProtocolSupport supported);

    virtual ~Encodable() {;}

    //! Construct a protocol field by parsing a DOM element
    static Encodable* generateEncodable(ProtocolParser* parse, QString Parent, ProtocolSupport supported, const QDomElement& field);

    //! Provide the pointer to a previous encodable in the list
    virtual void setPreviousEncodable(Encodable* prev) {}

    //! Check names against the list of C keywords
    virtual void checkAgainstKeywords(void);

    //! Reset all data to defaults
    virtual void clear(void);

    //! The hierarchical name of this object
    virtual QString getHierarchicalName(void) const = 0;

    //! Return the string that gives the prototype of the function used to encode this encodable, may be empty
    virtual QString getPrototypeEncodeString(bool isBigEndian, bool includeChildren = true) const {return QString();}

    //! Return the string that gives the prototype of the function used to decode this encodable, may be empty
    virtual QString getPrototypeDecodeString(bool isBigEndian, bool includeChildren = true) const {return QString();}

    //! Return the string that gives the function used to encode this encodable, may be empty
    virtual QString getFunctionEncodeString(bool isBigEndian, bool includeChildren = true) const {return QString();}

    //! Return the string that gives the function used to decode this encodable, may be empty
    virtual QString getFunctionDecodeString(bool isBigEndian, bool includeChildren = true) const {return QString();}

    //! Return the string that is used to encode this encodable
    virtual QString getEncodeString(bool isBigEndian, int* bitcount, bool isStructureMember) const = 0;

    //! Return the string that is used to decode this encoable
    virtual QString getDecodeString(bool isBigEndian, int* bitcount, bool isStructureMember, bool defaultEnabled = false) const = 0;

    //! Return the string that is used to declare this encodable
    virtual QString getDeclaration(void) const = 0;

    //! Return the include directives needed for this encodable
    virtual void getIncludeDirectives(QStringList& list) const {}

    //! Return the include directives needed for this encodable's init and verify functions
    virtual void getInitAndVerifyIncludeDirectives(QStringList& list) const {}

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

    //! Get the string used for verifying this field.
    virtual QString getVerifyString(bool isStructureMember) const {return QString();}

    //! Return the string that sets this encodable to its initial value in code
    virtual QString getSetInitialValueString(bool isStructureMember) const {return QString();}

    //! Return the strings that #define initial and variable values
    virtual QString getInitialAndVerifyDefines(bool includeComment = true) const {return QString();}

    //! Return true if this encodable has documentation for markdown output
    virtual bool hasDocumentation(void) {return true;}

    //! Get details needed to produce documentation for this encodable.
    virtual void getDocumentationDetails(QList<int>& outline, QString& startByte, QStringList& bytes, QStringList& names, QStringList& encodings, QStringList& repeats, QStringList& comments) const = 0;

    //! Get documentation repeat details for array or 2d arrays
    QString getRepeatsDocumentationDetails(void) const;

    //! Make this encodable not a default
    virtual void clearDefaults(void) {}

    //! Determine if this encodable a primitive object or a structure?
    virtual bool isPrimitive(void) const = 0;

    //! Determine if this encodable is an array
    bool isArray(void) const {return !array.isEmpty();}

    //! Determine if this encodable is a 2d array
    bool is2dArray(void) const {return (isArray() && !array2d.isEmpty());}

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
    virtual void getBitfieldGroupNumBytes(int* num) const {}

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

    //! True if this encodable has a direct child that needs a second iterator for encoding
    virtual bool uses2ndEncodeIterator(void) const = 0;

    //! True if this encodable has a direct child that needs a second iterator for decoding
    virtual bool uses2ndDecodeIterator(void) const = 0;

    //! True if this encodable has a direct child that uses defaults
    virtual bool usesDefaults(void) const = 0;

    //! True if this encodable overrides a previous encodable
    virtual bool overridesPreviousEncodable(void) const {return false;}

    //! Clear the override flag, its not allowed
    virtual void clearOverridesPrevious(void) {}

    //! True if this encodable invalidates an earlier default
    virtual bool invalidatesPreviousDefault(void) const {return !usesDefaults();}

    //! Add successive length strings
    static void addToLengthString(QString & totalLength, const QString & length);

    //! Add successive length strings
    static void addToLengthString(QString* totalLength, const QString & length);

public:

    //! String frequently reused for the beginning of encode functions
    static const QString VOID_ENCODE;

    //! String frequently reused for the beginning of decode functions
    static const QString INT_DECODE;

    QString structureSuffix;//!< Suffix added when defining a structure for this encodable
    QString typeName;       //!< The type name of this encodable, like "uint8_t" or "myStructure_t"
    QString array;          //!< The array length of this encodable, empty if no array
    QString array2d;        //!< The second dimension array length of this encodable, empty if no 2nd dimension
    QString variableArray;  //!< variable that gives the length of the array in a packet
    QString variable2dArray;//!< variable that gives the length of the 2nd array dimension in a packet
    QString dependsOn;      //!< variable that determines if this field is present
    EncodedLength encodedLength;    //!< The lengths of the encodables
};

#endif // ENCODABLE_H
