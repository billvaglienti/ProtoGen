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
    virtual void clear(void) Q_DECL_OVERRIDE;

    //! Parse the DOM element
    virtual void parse(void) Q_DECL_OVERRIDE;

    //! The hierarchical name of this object
    virtual QString getHierarchicalName(void) const Q_DECL_OVERRIDE {return parent + ":" + name;}

    //! Return the string that is used to encode this encodable
    virtual QString getEncodeString(bool isBigEndian, int* bitcount, bool isStructureMember) const Q_DECL_OVERRIDE;

    //! Return the string that is used to decode this encoable
    virtual QString getDecodeString(bool isBigEndian, int* bitcount, bool isStructureMember, bool defaultEnabled = false) const Q_DECL_OVERRIDE;

    //! Return the string that is used to declare this encodable
    virtual QString getDeclaration(void) const Q_DECL_OVERRIDE {return QString();}

    //! Return the include directives that go into source code needed for this encodable
    virtual void getSourceIncludeDirectives(QStringList& list) const Q_DECL_OVERRIDE;

    //! Return the signature of this field in an encode function signature
    virtual QString getEncodeSignature(void) const Q_DECL_OVERRIDE {return QString();}

    //! Return the signature of this field in a decode function signature
    virtual QString getDecodeSignature(void) const Q_DECL_OVERRIDE {return QString();}

    //! Return the string that documents this field as a encode function parameter
    virtual QString getEncodeParameterComment(void) const Q_DECL_OVERRIDE {return QString();}

    //! Return the string that documents this field as a decode function parameter
    virtual QString getDecodeParameterComment(void) const Q_DECL_OVERRIDE {return QString();}

    //! Code tag does not add documentation
    virtual bool hasDocumentation(void) Q_DECL_OVERRIDE {return false;}

    //! Get details needed to produce documentation for this encodable.
    virtual void getDocumentationDetails(QList<int>& outline, QString& startByte, QStringList& bytes, QStringList& names, QStringList& encodings, QStringList& repeats, QStringList& comments) const Q_DECL_OVERRIDE
    {
        Q_UNUSED(outline); Q_UNUSED(startByte); Q_UNUSED(bytes); Q_UNUSED(names); Q_UNUSED(encodings); Q_UNUSED(repeats); Q_UNUSED(comments); return;
    }

    //! Returns true since protocol code is a primitive type
    virtual bool isPrimitive(void) const Q_DECL_OVERRIDE {return true;}

    //! Returns true since protocol code is not in memory
    virtual bool isNotInMemory(void) const Q_DECL_OVERRIDE {return true;}

    //! True if this encodable has a direct child that uses bitfields
    virtual bool usesBitfields(void ) const Q_DECL_OVERRIDE {return false;}

    //! True if this encodable has a direct child that needs an iterator on encode
    virtual bool usesEncodeIterator(void) const Q_DECL_OVERRIDE {return false;}

    //! True if this encodable has a direct child that needs an iterator on decode
    virtual bool usesDecodeIterator(void) const Q_DECL_OVERRIDE {return false;}

    //! True if this encodable has a direct child that needs an iterator for verifying
    virtual bool usesVerifyIterator(void) const Q_DECL_OVERRIDE {return false;}

    //! True if this encodable has a direct child that needs an iterator for initializing
    virtual bool usesInitIterator(void) const Q_DECL_OVERRIDE {return false;}

    //! True if this encodable has a direct child that needs an iterator on encode
    virtual bool uses2ndEncodeIterator(void) const Q_DECL_OVERRIDE {return false;}

    //! True if this encodable has a direct child that needs an iterator on decode
    virtual bool uses2ndDecodeIterator(void) const Q_DECL_OVERRIDE {return false;}

    //! True if this encodable has a direct child that needs an second iterator for verifying
    virtual bool uses2ndVerifyIterator(void) const Q_DECL_OVERRIDE {return false;}

    //! True if this encodable has a direct child that needs an second iterator for initializing
    virtual bool uses2ndInitIterator(void) const Q_DECL_OVERRIDE {return false;}

    //! True if this encodable has a direct child that uses defaults
    virtual bool usesDefaults(void) const Q_DECL_OVERRIDE {return false;}

    //! True if this encodable invalidates an earlier default
    virtual bool invalidatesPreviousDefault(void) const Q_DECL_OVERRIDE {return false;}

protected:
    QString encode;
    QString decode;
    QString include;
};

#endif // PROTOCOLCODE_H
