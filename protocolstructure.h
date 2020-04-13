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
    ProtocolStructure(ProtocolParser* parse, QString Parent, ProtocolSupport supported);

    //! Destroy this protocol structure
    virtual ~ProtocolStructure();

    //! Reset all data to defaults
    virtual void clear(void) Q_DECL_OVERRIDE;

    //! Parse the DOM data for this structure
    virtual void parse(void) Q_DECL_OVERRIDE;

    //! The hierarchical name of this object
    virtual QString getHierarchicalName(void) const Q_DECL_OVERRIDE {return parent + ":" + name;}

    //! Parse the DOM data for this structures children
    void parseChildren(const QDomElement& field);

    //! Determine if this encodable is a primitive, rather than a structure
    virtual bool isPrimitive(void) const Q_DECL_OVERRIDE {return false;}

    //! Get the maximum number of temporary bytes needed for a bitfield group
    virtual void getBitfieldGroupNumBytes(int* num) const Q_DECL_OVERRIDE;

    //! True if this encodable has a direct child that uses bitfields
    virtual bool usesBitfields(void ) const Q_DECL_OVERRIDE {return bitfields;}

    //! True if this encodable needs a temporary buffer for its bitfield during encode
    virtual bool usesEncodeTempBitfield(void) const Q_DECL_OVERRIDE {return usestempencodebitfields;}

    //! True if this encodable needs a temporary buffer for its long bitfield during encode
    virtual bool usesEncodeTempLongBitfield(void) const Q_DECL_OVERRIDE {return usestempencodelongbitfields;}

    //! True if this encodable needs a temporary buffer for its bitfield during decode
    virtual bool usesDecodeTempBitfield(void) const Q_DECL_OVERRIDE {return usestempdecodebitfields;}

    //! True if this encodable needs a temporary buffer for its long bitfield during decode
    virtual bool usesDecodeTempLongBitfield(void) const Q_DECL_OVERRIDE {return usestempdecodelongbitfields;}

    //! True if this encodable has a direct child that needs an iterator
    virtual bool usesEncodeIterator(void) const Q_DECL_OVERRIDE {return needsEncodeIterator;}

    //! True if this encodable has a direct child that needs an iterator
    virtual bool usesDecodeIterator(void) const Q_DECL_OVERRIDE {return needsDecodeIterator;}

    //! True if this encodable has a direct child that needs an iterator for verifying
    virtual bool usesVerifyIterator(void) const Q_DECL_OVERRIDE {return needsVerifyIterator;}

    //! True if this encodable has a direct child that needs an iterator for initializing
    virtual bool usesInitIterator(void) const Q_DECL_OVERRIDE {return needsInitIterator;}

    //! True if this encodable has a direct child that needs a 2nd iterator
    virtual bool uses2ndEncodeIterator(void) const Q_DECL_OVERRIDE {return needs2ndEncodeIterator;}

    //! True if this encodable has a direct child that needs a 2nd iterator
    virtual bool uses2ndDecodeIterator(void) const Q_DECL_OVERRIDE {return needs2ndDecodeIterator;}

    //! True if this encodable has a direct child that needs a 2nd iterator for verifying
    virtual bool uses2ndVerifyIterator(void) const Q_DECL_OVERRIDE {return needs2ndVerifyIterator;}

    //! True if this encodable has a direct child that needs a 2nd iterator for initializing
    virtual bool uses2ndInitIterator(void) const Q_DECL_OVERRIDE {return needs2ndInitIterator;}

    //! True if this encodable has a direct child that uses defaults
    virtual bool usesDefaults(void) const Q_DECL_OVERRIDE {return defaults;}

    //! Return the string that gives the prototype of the function used to initialize this structure
    QString getVerifyFunctionPrototype(bool includeChildren = true) const;

    //! Return the string that gives the function used to initialize this structure
    QString getVerifyFunctionString(bool includeChildren = true) const;

    //! Get the string used for verifying this field.
    virtual QString getVerifyString(bool isStructureMember) const Q_DECL_OVERRIDE;

    //! Return the string that gives the prototype of the function used to compare this structure
    QString getComparisonFunctionPrototype(bool includeChildren = true) const;

    //! Return the string that gives the function used to compare this structure
    QString getComparisonFunctionString(bool includeChildren = true) const;

    //! Get the string used for comparing this structure.
    virtual QString getComparisonString(bool isStructureMember) const Q_DECL_OVERRIDE;

    //! Return the string that gives the prototype of the function used to text print this structure
    QString getTextPrintFunctionPrototype(bool includeChildren = true) const;

    //! Return the string that gives the function used to text print this structure
    QString getTextPrintFunctionString(bool includeChildren = true) const;

    //! Get the string used for text printing this structure.
    virtual QString getTextPrintString(bool isStructureMember) const Q_DECL_OVERRIDE;

    //! Return the string that gives the prototype of the function used to read this structure from text
    QString getTextReadFunctionPrototype(bool includeChildren = true) const;

    //! Return the string that gives the function used to read this structure this structure from text
    QString getTextReadFunctionString(bool includeChildren = true) const;

    //! Get the string used for text reading this structure.
    virtual QString getTextReadString(bool isStructureMember) const Q_DECL_OVERRIDE;

    //! Return the string that gives the prototype of the function used to encode this structure to a map
    QString getMapEncodeFunctionPrototype(bool includeChildren = true) const;

    //! Return the string that gives the function used to encode this structure to a map
    QString getMapEncodeFunctionString(bool includeChildren = true) const;

    //! Return the string used for map encoding this structure
    virtual QString getMapEncodeString(bool isStructureMember) const Q_DECL_OVERRIDE;

    //! Return the string that gives the prototype of the function used to decode this structure from a map
    QString getMapDecodeFunctionPrototype(bool includeChildren = true) const;

    //! Return the string that gives the function used to decode this structure from a map
    QString getMapDecodeFunctionString(bool includeChildren = true) const;

    //! Return the string used for map decoding this structure
    virtual QString getMapDecodeString(bool isStructureMember) const Q_DECL_OVERRIDE;

    //! Return the string that gives the prototype of the function used to initialize this structure
    QString getSetToInitialValueFunctionPrototype(bool includeChildren = true) const;

    //! Return the string that gives the function used to initialize this structure
    QString getSetToInitialValueFunctionString(bool includeChildren = true) const;

    //! Return the string that sets this encodable to its initial value in code
    virtual QString getSetInitialValueString(bool isStructureMember) const Q_DECL_OVERRIDE;

    //! Return the strings that #define initial and variable values
    virtual QString getInitialAndVerifyDefines(bool includeComment = true) const Q_DECL_OVERRIDE;

    //! Get the number of fields that are encoded
    int getNumberOfEncodes(void) const;

    //! Get the number of encoded fields for which we need parameters
    int getNumberOfEncodeParameters(void) const;

    //! Get the number of decoded fields for which we need parameters
    int getNumberOfDecodeParameters(void) const;

    //! Get the number of fields in memory
    int getNumberInMemory(void) const;

    //! Return the include directives needed for this encodable
    virtual void getIncludeDirectives(QStringList& list) const Q_DECL_OVERRIDE;

    //! Return the include directives that go into source code for this encodable
    virtual void getSourceIncludeDirectives(QStringList& list) const Q_DECL_OVERRIDE;

    //! Return the include directives needed for this encodable's init and verify functions
    virtual void getInitAndVerifyIncludeDirectives(QStringList& list) const Q_DECL_OVERRIDE;

    //! Return the include directives needed for this encodable's map functions
    virtual void getMapIncludeDirectives(QStringList& list) const Q_DECL_OVERRIDE;

    //! Return the include directives needed for this encodable's compare functions
    virtual void getCompareIncludeDirectives(QStringList& list) const Q_DECL_OVERRIDE;

    //! Return the include directives needed for this encodable's print functions
    virtual void getPrintIncludeDirectives(QStringList& list) const Q_DECL_OVERRIDE;

    //! Get the declaration for this structure as a member of another
    virtual QString getDeclaration(void) const Q_DECL_OVERRIDE;

    //! Get the declaration that goes in the header which declares this structure and all its children
    virtual QString getStructureDeclaration(bool alwaysCreate) const Q_DECL_OVERRIDE;

    //! Return the string that gives the prototype of the function used to encode this encodable, may be empty
    virtual QString getPrototypeEncodeString(bool isBigEndian, bool includeChildren = true) const;

    //! Return the string that gives the prototype of the function used to decode this encodable, may be empty
    virtual QString getPrototypeDecodeString(bool isBigEndian, bool includeChildren = true) const;

    //! Return the string that gives the function used to encode this encodable, may be empty
    virtual QString getFunctionEncodeString(bool isBigEndian, bool includeChildren = true) const;

    //! Return the string that gives the function used to decode this encodable, may be empty
    virtual QString getFunctionDecodeString(bool isBigEndian, bool includeChildren = true) const;

    //! Return the string that is used to prototype the encode routine for this encodable
    virtual QString getFunctionEncodePrototype() const;

    //! Return the string that is used to prototype the decode routine for this encodable
    virtual QString getFunctionDecodePrototype() const;

    //! Return the string that is used to encode this encodable
    virtual QString getEncodeString(bool isBigEndian, int* bitcount, bool isStructureMember) const Q_DECL_OVERRIDE;

    //! Return the string that is used to decode this encoable
    virtual QString getDecodeString(bool isBigEndian, int* bitcount, bool isStructureMember, bool defaultEnabled = false) const Q_DECL_OVERRIDE;

    //! Get details needed to produce documentation for this encodable.
    virtual void getDocumentationDetails(QList<int>& outline, QString& startByte, QStringList& bytes, QStringList& names, QStringList& encodings, QStringList& repeats, QStringList& comments) const Q_DECL_OVERRIDE;

    //! Get details needed to produce documentation for this encodables sub-encodables.
    void getSubDocumentationDetails(QList<int>& outline, QString& startByte, QStringList& bytes, QStringList& names, QStringList& encodings, QStringList& repeats, QStringList& comments) const;

    //! Return true if this structure should be hidden from the documentation
    virtual bool isHidden(void) const Q_DECL_OVERRIDE {return hidden;}

    //! True if this encodable has verification data
    virtual bool hasVerify(void) const Q_DECL_OVERRIDE {return hasverify;}

    //! True if this encodable has initialization data
    virtual bool hasInit(void) const Q_DECL_OVERRIDE {return hasinit;}

protected:

    //! Make a structure output be prettily aligned
    QString alignStructureData(const QString& structure) const;

    //! Parse all enumerations which are direct children of a DomNode
    void parseEnumerations(const QDomNode& node);

    //! This list of all children encodables
    QList<Encodable*> encodables;

    //! The list of our enumerations
    QList<const EnumCreator*> enumList;

    int  numbitfieldgroupbytes; //!< Number of temporary bytes needed by children with bitfield groups
    bool bitfields;             //!< True if this structure uses bitfields
    bool usestempencodebitfields;     //!< True if this structure uses a temporary bitfield for encoding
    bool usestempencodelongbitfields; //!< True if this structure uses a temporary long bitfield for encoding
    bool usestempdecodebitfields;     //!< True if this structure uses a temporary bitfield for decoding
    bool usestempdecodelongbitfields; //!< True if this structure uses a temporary long bitfield for decoding
    bool needsEncodeIterator;   //!< True if this structure uses arrays iterators on encode
    bool needsDecodeIterator;   //!< True if this structure uses arrays iterators on decode
    bool needsInitIterator;     //!< True if this structure uses arrays iterators on initialization
    bool needsVerifyIterator;   //!< True if this structure uses arrays iterators on verification
    bool needs2ndEncodeIterator;//!< True if this structure uses 2nd arrays iterators on encode
    bool needs2ndDecodeIterator;//!< True if this structure uses 2nd arrays iterators on decode
    bool needs2ndInitIterator;  //!< True if this structure uses 2nd arrays iterators on initialization
    bool needs2ndVerifyIterator;//!< True if this structure uses 2nd arrays iterators on verification
    bool defaults;              //!< True if this structure uses default values
    bool strings;               //!< True if this structure uses strings
    bool hidden;                //!< True if this structure is to be hidden from the documentation
    bool hasinit;               //!< True if any children of this structure have initialization data
    bool hasverify;             //!< True if any children of this structure have verify data
    QStringList attriblist;     //!< List of all attributes that we understand
    QString structName;         //!< Name of the structure (usually the same as typeName)

};

#endif // PROTOCOLSTRUCTURE_H
