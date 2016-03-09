#ifndef PROTOCOLSTRUCTURE_H
#define PROTOCOLSTRUCTURE_H

#include "encodable.h"
#include "enumcreator.h"
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

    //! True if this encodable has a direct child that needs an iterator
    virtual bool usesIterator(void) const {return needsIterator;}

    //! True if this encodable has a direct child that uses defaults
    virtual bool usesDefaults(void) const {return defaults;}

    //! Get the number of encoded fields
    int getNumberOfEncodes(void) const;

    //! Get the number of encoded fields whose value is set by the user.
    int getNumberOfNonConstEncodes(void) const;

    //! Get the declaration for this structure as a member of another
    virtual QString getDeclaration(void) const;

    //! Get the declaration that goes in the header which declares this structure and all its children
    virtual QString getStructureDeclaration(bool alwaysCreate) const;

    //! Return the string that gives the prototype of the function used to encode this encodable, may be empty
    virtual QString getPrototypeEncodeString(bool isBigEndian) const;

    //! Return the string that gives the prototype of the function used to decode this encodable, may be empty
    virtual QString getPrototypeDecodeString(bool isBigEndian) const;

    //! Return the string that is used to encode this encodable
    virtual QString getEncodeString(bool isBigEndian, int* bitcount, bool isStructureMember) const;

    //! Return the string that is used to decode this encoable
    virtual QString getDecodeString(bool isBigEndian, int* bitcount, bool isStructureMember, bool defaultEnabled = false) const;

    //! Get details needed to produce documentation for this encodable.
    virtual void getDocumentationDetails(QList<int>& outline, QString& startByte, QStringList& bytes, QStringList& names, QStringList& encodings, QStringList& repeats, QStringList& comments) const;

    //! Get details needed to produce documentation for this encodables sub-encodables.
    void getSubDocumentationDetails(QList<int>& outline, QString& startByte, QStringList& bytes, QStringList& names, QStringList& encodings, QStringList& repeats, QStringList& comments) const;

    bool isHidden() {return hidden;}

protected:

    //! Make a structure output be prettily aligned
    QString alignStructureData(const QString& structure) const;

    //! Parse all enumerations which are direct children of a DomNode
    void parseEnumerations(const QDomNode& node);

    //! This list of all children encodables
    QList<Encodable*> encodables;

    //! The list of our enumerations
    QList<const EnumCreator*> enumList;

    bool bitfields;             //!< True if this structure uses bitfields
    bool needsIterator;         //!< True if this structure uses arrays
    bool defaults;              //!< True if this structure uses default values
    bool strings;               //!< True if this structure uses strings

    bool hidden;                //!< True if this structure is to be hidden from the documentation
    bool telemetry;             //!< True if this structure is 'telemetry' (is transmitted automatically by the device)
    bool polled;                //!< True if the packet can be polled from the device by sending a zero-length msg
    bool broadcast;             //!< True if the packet can be 'broadcast' (i.e. the device will accept packets with 0xFF broadcast ID)

    QString direction;          //!< Data direction of this packet
};

#endif // PROTOCOLSTRUCTURE_H
