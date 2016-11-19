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
    ProtocolStructure(ProtocolParser* parse, QString Parent, const QString& protocolName, ProtocolSupport supported);

    //! Destroy this protocol structure
    virtual ~ProtocolStructure();

    //! Reset all data to defaults
    virtual void clear(void);

    //! Parse the DOM data for this structure
    virtual void parse(void);

    //! The hierarchical name of this object
    virtual QString getHierarchicalName(void) const {return parent + ":" + name;}

    //! Parse the DOM data for this structures children
    void parseChildren(const QDomElement& field);

    //! Determine if this encodable is a primitive, rather than a structure
    virtual bool isPrimitive(void) const {return false;}

    //! True if this encodable has a direct child that uses bitfields
    virtual bool usesBitfields(void ) const {return bitfields;}

    //! True if this encodable has a direct child that needs an iterator
    virtual bool usesEncodeIterator(void) const {return needsEncodeIterator;}

    //! True if this encodable has a direct child that needs an iterator
    virtual bool usesDecodeIterator(void) const {return needsDecodeIterator;}

    //! True if this encodable has a direct child that needs a 2nd iterator
    virtual bool uses2ndEncodeIterator(void) const {return needs2ndEncodeIterator;}

    //! True if this encodable has a direct child that needs a 2nd iterator
    virtual bool uses2ndDecodeIterator(void) const {return needs2ndDecodeIterator;}

    //! True if this encodable has a direct child that uses defaults
    virtual bool usesDefaults(void) const {return defaults;}

    //! Get the number of fields that are encoded
    int getNumberOfEncodes(void) const;

    //! Get the number of encoded fields for which we need parameters
    int getNumberOfEncodeParameters(void) const;

    //! Get the number of decoded fields for which we need parameters
    int getNumberOfDecodeParameters(void) const;

    //! Get the number of fields in memory
    int getNumberInMemory(void) const;

    //! Return the include directives needed for this encodable
    virtual void getIncludeDirectives(QStringList& list) const;

    //! Get the declaration for this structure as a member of another
    virtual QString getDeclaration(void) const;

    //! Get the declaration that goes in the header which declares this structure and all its children
    virtual QString getStructureDeclaration(bool alwaysCreate) const;

    //! Return the string that gives the prototype of the function used to encode this encodable, may be empty
    virtual QString getPrototypeEncodeString(bool isBigEndian, bool includeChildren = true) const;

    //! Return the string that gives the prototype of the function used to decode this encodable, may be empty
    virtual QString getPrototypeDecodeString(bool isBigEndian, bool includeChildren = true) const;

    //! Return the string that gives the function used to encode this encodable, may be empty
    virtual QString getFunctionEncodeString(bool isBigEndian, bool includeChildren = true) const;

    //! Return the string that gives the function used to decode this encodable, may be empty
    virtual QString getFunctionDecodeString(bool isBigEndian, bool includeChildren = true) const;

    //! Return the string that is used to encode this encodable
    virtual QString getEncodeString(bool isBigEndian, int* bitcount, bool isStructureMember) const;

    //! Return the string that is used to decode this encoable
    virtual QString getDecodeString(bool isBigEndian, int* bitcount, bool isStructureMember, bool defaultEnabled = false) const;

    //! Get details needed to produce documentation for this encodable.
    virtual void getDocumentationDetails(QList<int>& outline, QString& startByte, QStringList& bytes, QStringList& names, QStringList& encodings, QStringList& repeats, QStringList& comments) const;

    //! Get details needed to produce documentation for this encodables sub-encodables.
    void getSubDocumentationDetails(QList<int>& outline, QString& startByte, QStringList& bytes, QStringList& names, QStringList& encodings, QStringList& repeats, QStringList& comments) const;

    //! Return true if this structure should be hidden from the documentation
    virtual bool isHidden(void) const {return hidden;}

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
    bool needsEncodeIterator;   //!< True if this structure uses arrays iterators on encode
    bool needsDecodeIterator;   //!< True if this structure uses arrays iterators on decode
    bool needs2ndEncodeIterator;//!< True if this structure uses 2nd arrays iterators on encode
    bool needs2ndDecodeIterator;//!< True if this structure uses 2nd arrays iterators on decode
    bool defaults;              //!< True if this structure uses default values
    bool strings;               //!< True if this structure uses strings
    bool hidden;                //!< True if this structure is to be hidden from the documentation
    QStringList attriblist;     //!< List of all attributes that we understand

};

#endif // PROTOCOLSTRUCTURE_H
