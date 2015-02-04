#ifndef PROTOCOLSTRUCTURE_H
#define PROTOCOLSTRUCTURE_H

#include "encodable.h"
#include <QString>
#include <QList>

class ProtocolStructure : public Encodable
{
public:

    //! Default constructor for protocol structure
    ProtocolStructure(const QString& protocolName, const QString& protocolPrefix, ProtocolSupport supported);

    //! Construct a protocol field by parsing a DOM element
    ProtocolStructure(const QString& protocolName, const QString& protocolPrefix, ProtocolSupport supported, const QDomElement& field);

    //! Destroy this protocol structure
    virtual ~ProtocolStructure();

    //! Reset all data to defaults
    virtual void clear(void);

    //! Parse the DOM data for this structure
    virtual void parse(const QDomElement& field);

    //! Parse the DOM data for this structures children
    void parseChildren(const QDomElement& field);

    //! Determine if this encodable is a primitive, rather than a structure
    virtual bool isPrimitive(void) const {return false;}

    //! True if this encodable has a direct child that uses bitfields
    virtual bool usesBitfields(void ) const {return bitfields;}

    //! True if this encodable has a direct child that uses arrays
    virtual bool usesArrays(void) const {return arrays;}

    //! True if this encodable has a direct child that uses a reserved array field
    virtual bool usesReservedArrays(void) const {return reservedArrays;}

    //! True if this encodable has a direct child that uses defaults
    virtual bool usesDefaults(void) const {return defaults;}

    //! True if this encodable has a direct child that uses default arrays
    virtual bool usesDefaultArrays(void) const {return defaultArrays;}

    //! True if this encodable has a direct child that uses strings
    virtual bool usesStrings(void) const {return strings;}

    //! Get the declaration for this structure as a member of another
    virtual QString getDeclaration(void) const;

    //! Get the declaration that goes in the header which declares this structure and all its children
    virtual QString getStructureDeclaration(bool alwaysCreate) const;

    //! Return the string that gives the prototype of the function used to encode this encodable, may be empty
    virtual QString getPrototypeEncodeString(bool isBigEndian, EncodedLength* encLength = NULL);

    //! Return the string that gives the prototype of the function used to decode this encodable, may be empty
    virtual QString getPrototypeDecodeString(bool isBigEndian) const;

    //! Return the string that is used to encode this encodable
    virtual QString getEncodeString(bool isBigEndian, EncodedLength& encLength, int* bitcount, bool isStructureMember) const;

    //! Return the string that is used to decode this encoable
    virtual QString getDecodeString(bool isBigEndian, int* bitcount, bool isStructureMember, bool defaultEnabled = false) const;

    //! Return markdown formatted information about this encodables fields
    virtual QString getMarkdown(QString indent) const;

protected:

    //! Make a structure output be prettily aligned
    QString alignStructureData(const QString& structure) const;

    //! This list of all children encodables
    QList<Encodable*> encodables;

    bool bitfields;             //!< True if this structure uses bitfields
    bool arrays;                //!< True if this structure uses arrays
    bool reservedArrays;        //!< True if this structure has a null (reserved) array field
    bool defaults;              //!< True if this structure uses default values
    bool defaultArrays;         //!< True if this structure uses default values in arrays
    bool strings;               //!< True if this structure uses strings

};

#endif // PROTOCOLSTRUCTURE_H
