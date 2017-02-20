#ifndef PROTOCOLCODE_H
#define PROTOCOLCODE_H

#include <stdint.h>
#include <QString>
#include <QDomElement>
#include "encodable.h"

class ProtocolCode : public Encodable
{
    public:

    //! Construct a field, setting the protocol name and name prefix
    ProtocolCode(ProtocolParser* parse, QString Parent, ProtocolSupport supported);

    //! Destroy the protocol field
    virtual ~ProtocolCode(){}

    //! Reset all data to defaults
    virtual void clear(void);

    //! Parse the DOM element
    virtual void parse(void);

    //! The hierarchical name of this object
    virtual QString getHierarchicalName(void) const {return parent + ":" + name;}

    //! Return the string that is used to encode this encodable
    virtual QString getEncodeString(bool isBigEndian, int* bitcount, bool isStructureMember) const;

    //! Return the string that is used to decode this encoable
    virtual QString getDecodeString(bool isBigEndian, int* bitcount, bool isStructureMember, bool defaultEnabled = false) const;

    //! Return the string that is used to declare this encodable
    virtual QString getDeclaration(void) const {return QString();}

    //! Return the signature of this field in an encode function signature
    virtual QString getEncodeSignature(void) const {return QString();}

    //! Return the signature of this field in a decode function signature
    virtual QString getDecodeSignature(void) const {return QString();}

    //! Return the string that documents this field as a encode function parameter
    virtual QString getEncodeParameterComment(void) const {return QString();}

    //! Return the string that documents this field as a decode function parameter
    virtual QString getDecodeParameterComment(void) const {return QString();}

    //! Code tag does not add documentation
    virtual bool hasDocumentation(void) {return false;}

    //! Get details needed to produce documentation for this encodable.
    virtual void getDocumentationDetails(QList<int>& outline, QString& startByte, QStringList& bytes, QStringList& names, QStringList& encodings, QStringList& repeats, QStringList& comments) const {return;}

    //! Returns true since protocol code is a primitive type
    virtual bool isPrimitive(void) const {return true;}

    //! Returns true since protocol code is not in memory
    virtual bool isNotInMemory(void) const {return true;}

    //! True if this encodable has a direct child that uses bitfields
    virtual bool usesBitfields(void ) const {return false;}

    //! True if this encodable has a direct child that needs an iterator on encode
    virtual bool usesEncodeIterator(void) const {return false;}

    //! True if this encodable has a direct child that needs an iterator on decode
    virtual bool usesDecodeIterator(void) const {return false;}

    //! True if this encodable has a direct child that needs an iterator on encode
    virtual bool uses2ndEncodeIterator(void) const {return false;}

    //! True if this encodable has a direct child that needs an iterator on decode
    virtual bool uses2ndDecodeIterator(void) const {return false;}

    //! True if this encodable has a direct child that uses defaults
    virtual bool usesDefaults(void) const {return false;}

    //! True if this encodable invalidates an earlier default
    virtual bool invalidatesPreviousDefault(void) const {return false;}

protected:
    QString encode;
    QString decode;
};

#endif // PROTOCOLCODE_H
