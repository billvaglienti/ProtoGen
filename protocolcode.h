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

    //! Reset all data to defaults
    void clear(void) Q_DECL_OVERRIDE;

    //! Parse the DOM element
    void parse(void) Q_DECL_OVERRIDE;

    //! The hierarchical name of this object
    QString getHierarchicalName(void) const Q_DECL_OVERRIDE {return parent + ":" + name;}

    //! Return the string that is used to encode this encodable
    QString getEncodeString(bool isBigEndian, int* bitcount, bool isStructureMember) const Q_DECL_OVERRIDE;

    //! Return the string that is used to decode this encoable
    QString getDecodeString(bool isBigEndian, int* bitcount, bool isStructureMember, bool defaultEnabled = false) const Q_DECL_OVERRIDE;

    //! Return the string that is used to declare this encodable
    QString getDeclaration(void) const Q_DECL_OVERRIDE {return QString();}

    //! Return the include directives that go into source code needed for this encodable
    void getSourceIncludeDirectives(QStringList& list) const Q_DECL_OVERRIDE;

    //! Return the signature of this field in an encode function signature
    QString getEncodeSignature(void) const Q_DECL_OVERRIDE {return QString();}

    //! Return the signature of this field in a decode function signature
    QString getDecodeSignature(void) const Q_DECL_OVERRIDE {return QString();}

    //! Return the string that documents this field as a encode function parameter
    QString getEncodeParameterComment(void) const Q_DECL_OVERRIDE {return QString();}

    //! Return the string that documents this field as a decode function parameter
    QString getDecodeParameterComment(void) const Q_DECL_OVERRIDE {return QString();}

    //! Code tag does not add documentation
    bool hasDocumentation(void) Q_DECL_OVERRIDE {return false;}

    //! Get details needed to produce documentation for this encodable.
    void getDocumentationDetails(QList<int>& outline, QString& startByte, QStringList& bytes, QStringList& names, QStringList& encodings, QStringList& repeats, QStringList& comments) const Q_DECL_OVERRIDE
    {
        Q_UNUSED(outline); Q_UNUSED(startByte); Q_UNUSED(bytes); Q_UNUSED(names); Q_UNUSED(encodings); Q_UNUSED(repeats); Q_UNUSED(comments); return;
    }

    //! Returns true since protocol code is a primitive type
    bool isPrimitive(void) const Q_DECL_OVERRIDE {return true;}

    //! Returns false since protocol code is not a string
    bool isString(void) const Q_DECL_OVERRIDE {return false;}

    //! Returns true since protocol code is not in memory
    bool isNotInMemory(void) const Q_DECL_OVERRIDE {return true;}

    //! True if this encodable has a direct child that uses bitfields
    bool usesBitfields(void ) const Q_DECL_OVERRIDE {return false;}

    //! True if this encodable has a direct child that needs an iterator on encode
    bool usesEncodeIterator(void) const Q_DECL_OVERRIDE {return false;}

    //! True if this encodable has a direct child that needs an iterator on decode
    bool usesDecodeIterator(void) const Q_DECL_OVERRIDE {return false;}

    //! True if this encodable has a direct child that needs an iterator for verifying
    bool usesVerifyIterator(void) const Q_DECL_OVERRIDE {return false;}

    //! True if this encodable has a direct child that needs an iterator for initializing
    bool usesInitIterator(void) const Q_DECL_OVERRIDE {return false;}

    //! True if this encodable has a direct child that needs an iterator on encode
    bool uses2ndEncodeIterator(void) const Q_DECL_OVERRIDE {return false;}

    //! True if this encodable has a direct child that needs an iterator on decode
    bool uses2ndDecodeIterator(void) const Q_DECL_OVERRIDE {return false;}

    //! True if this encodable has a direct child that needs an second iterator for verifying
    bool uses2ndVerifyIterator(void) const Q_DECL_OVERRIDE {return false;}

    //! True if this encodable has a direct child that needs an second iterator for initializing
    bool uses2ndInitIterator(void) const Q_DECL_OVERRIDE {return false;}

    //! True if this encodable has a direct child that uses defaults
    bool usesDefaults(void) const Q_DECL_OVERRIDE {return false;}

    //! True if this encodable invalidates an earlier default
    bool invalidatesPreviousDefault(void) const Q_DECL_OVERRIDE {return false;}

protected:
    QString encode;
    QString decode;
    QString include;
};

#endif // PROTOCOLCODE_H
