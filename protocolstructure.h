#ifndef PROTOCOLSTRUCTURE_H
#define PROTOCOLSTRUCTURE_H

#include "encodable.h"
#include "enumcreator.h"
#include <QString>
#include <QList>

// Forward declaration of our descendant
class ProtocolStructureModule;

class ProtocolStructure : public Encodable
{
public:

    //! Default constructor for protocol structure
    ProtocolStructure(ProtocolParser* parse, QString Parent, ProtocolSupport supported);

    //! Reset all data to defaults
    void clear(void) Q_DECL_OVERRIDE;

    //! Parse the DOM data for this structure
    void parse(void) Q_DECL_OVERRIDE;

    //! The hierarchical name of this object
    QString getHierarchicalName(void) const Q_DECL_OVERRIDE {return parent + ":" + name;}

    //! Get the declaration for this structure as a member of another
    QString getDeclaration(void) const Q_DECL_OVERRIDE;

    //! Return the string that is used to encode this encodable
    QString getEncodeString(bool isBigEndian, int* bitcount, bool isStructureMember) const Q_DECL_OVERRIDE;

    //! Return the string that is used to decode this encoable
    QString getDecodeString(bool isBigEndian, int* bitcount, bool isStructureMember, bool defaultEnabled = false) const Q_DECL_OVERRIDE;

    //! Get the string used for verifying this field.
    QString getVerifyString(void) const Q_DECL_OVERRIDE;

    //! Return the string that sets this encodable to its initial value in code
    QString getSetInitialValueString(bool isStructureMember) const Q_DECL_OVERRIDE;

    //! Return the strings that #define initial and variable values
    QString getInitialAndVerifyDefines(bool includeComment = true) const Q_DECL_OVERRIDE;

    //! Get the string used for comparing this structure.
    QString getComparisonString(void) const Q_DECL_OVERRIDE;

    //! Get the string used for text printing this structure.
    QString getTextPrintString(void) const Q_DECL_OVERRIDE;

    //! Get the string used for text reading this structure.
    QString getTextReadString(void) const Q_DECL_OVERRIDE;

    //! Return the string used for map encoding this structure
    QString getMapEncodeString(void) const Q_DECL_OVERRIDE;

    //! Return the string used for map decoding this structure
    QString getMapDecodeString(void) const Q_DECL_OVERRIDE;

    //! Parse the DOM data for this structures children
    void parseChildren(const QDomElement& field);

    //! Set the compare flag for this structure and all children structure
    void setCompare(bool enable);

    //! Set the print flag for this structure and all children structure
    void setPrint(bool enable);

    //! Set the mapEncode flag for this structure and all children structure
    void setMapEncode(bool enable);

    //! Determine if this encodable is a primitive, rather than a structure
    bool isPrimitive(void) const Q_DECL_OVERRIDE {return false;}

    //! Determine if this encodable a string object
    bool isString(void) const Q_DECL_OVERRIDE {return false;}

    //! Get the maximum number of temporary bytes needed for a bitfield group
    void getBitfieldGroupNumBytes(int* num) const Q_DECL_OVERRIDE;

    //! True if this encodable has a direct child that uses bitfields
    bool usesBitfields(void ) const Q_DECL_OVERRIDE {return bitfields;}

    //! True if this encodable needs a temporary buffer for its bitfield during encode
    bool usesEncodeTempBitfield(void) const Q_DECL_OVERRIDE {return usestempencodebitfields;}

    //! True if this encodable needs a temporary buffer for its long bitfield during encode
    bool usesEncodeTempLongBitfield(void) const Q_DECL_OVERRIDE {return usestempencodelongbitfields;}

    //! True if this encodable needs a temporary buffer for its bitfield during decode
    bool usesDecodeTempBitfield(void) const Q_DECL_OVERRIDE {return usestempdecodebitfields;}

    //! True if this encodable needs a temporary buffer for its long bitfield during decode
    bool usesDecodeTempLongBitfield(void) const Q_DECL_OVERRIDE {return usestempdecodelongbitfields;}

    //! True if this encodable has a direct child that needs an iterator
    bool usesEncodeIterator(void) const Q_DECL_OVERRIDE {return needsEncodeIterator;}

    //! True if this encodable has a direct child that needs an iterator
    bool usesDecodeIterator(void) const Q_DECL_OVERRIDE {return needsDecodeIterator;}

    //! True if this encodable has a direct child that needs an iterator for verifying
    bool usesVerifyIterator(void) const Q_DECL_OVERRIDE {return needsVerifyIterator;}

    //! True if this encodable has a direct child that needs an iterator for initializing
    bool usesInitIterator(void) const Q_DECL_OVERRIDE {return needsInitIterator;}

    //! True if this encodable has a direct child that needs a 2nd iterator
    bool uses2ndEncodeIterator(void) const Q_DECL_OVERRIDE {return needs2ndEncodeIterator;}

    //! True if this encodable has a direct child that needs a 2nd iterator
    bool uses2ndDecodeIterator(void) const Q_DECL_OVERRIDE {return needs2ndDecodeIterator;}

    //! True if this encodable has a direct child that needs a 2nd iterator for verifying
    bool uses2ndVerifyIterator(void) const Q_DECL_OVERRIDE {return needs2ndVerifyIterator;}

    //! True if this encodable has a direct child that needs a 2nd iterator for initializing
    bool uses2ndInitIterator(void) const Q_DECL_OVERRIDE {return needs2ndInitIterator;}

    //! True if this encodable has a direct child that uses defaults
    bool usesDefaults(void) const Q_DECL_OVERRIDE {return defaults;}


    //! Return the string that is used to prototype the encode routine for this encodable
    virtual QString getEncodeFunctionSignature(bool insource) const;

    //! Return the string that gives the prototype of the function used to encode this encodable, may be empty
    virtual QString getEncodeFunctionPrototype(const QString& spacing = QString(), bool includeChildren = true) const;

    //! Return the string that gives the function used to encode this encodable, may be empty
    virtual QString getEncodeFunctionBody(bool isBigEndian, bool includeChildren = true) const;


    //! Return the string that is used to prototype the decode routine for this encodable
    virtual QString getDecodeFunctionSignature(bool insource) const;

    //! Return the string that gives the prototype of the function used to decode this encodable, may be empty
    virtual QString getDecodeFunctionPrototype(const QString& spacing = QString(), bool includeChildren = true) const;

    //! Return the string that gives the function used to decode this encodable, may be empty
    virtual QString getDecodeFunctionBody(bool isBigEndian, bool includeChildren = true) const;


    //! Return the string that gives the signature of the function used to initialize this structure
    virtual QString getVerifyFunctionSignature(bool insource) const;

    //! Return the string that gives the prototype of the function used to initialize this structure
    virtual QString getVerifyFunctionPrototype(const QString& spacing = QString(), bool includeChildren = true) const;

    //! Return the string that gives the function used to initialize this structure
    virtual QString getVerifyFunctionBody(bool includeChildren = true) const;


    //! Return the string that gives the signature of the function used to compare this structure
    virtual QString getComparisonFunctionSignature(bool insource) const;

    //! Return the string that gives the prototype of the function used to compare this structure
    virtual QString getComparisonFunctionPrototype(const QString& spacing = QString(), bool includeChildren = true) const;

    //! Return the string that gives the function used to compare this structure
    virtual QString getComparisonFunctionBody(bool includeChildren = true) const;


    //! Return the string that gives the signature of the function used to text print this structure
    virtual QString getTextPrintFunctionSignature(bool insource) const;

    //! Return the string that gives the prototype of the function used to text print this structure
    virtual QString getTextPrintFunctionPrototype(const QString& spacing = QString(), bool includeChildren = true) const;

    //! Return the string that gives the function used to text print this structure
    virtual QString getTextPrintFunctionBody(bool includeChildren = true) const;


    //! Return the string that gives the signature of the function used to read this structure from text
    virtual QString getTextReadFunctionSignature(bool insource) const;

    //! Return the string that gives the prototype of the function used to read this structure from text
    virtual QString getTextReadFunctionPrototype(const QString& spacing = QString(), bool includeChildren = true) const;

    //! Return the string that gives the function used to read this structure this structure from text
    virtual QString getTextReadFunctionBody(bool includeChildren = true) const;


    //! Return the string that gives the signature of the function used to encode this structure to a map
    virtual QString getMapEncodeFunctionSignature(bool insource) const;

    //! Return the string that gives the prototype of the function used to encode this structure to a map
    virtual QString getMapEncodeFunctionPrototype(const QString& spacing = QString(), bool includeChildren = true) const;

    //! Return the string that gives the function used to encode this structure to a map
    virtual QString getMapEncodeFunctionBody(bool includeChildren = true) const;


    //! Return the string that gives the signature of the function used to decode this structure from a map
    virtual QString getMapDecodeFunctionSignature(bool insource) const;

    //! Return the string that gives the prototype of the function used to decode this structure from a map
    virtual QString getMapDecodeFunctionPrototype(const QString& spacing = QString(), bool includeChildren = true) const;

    //! Return the string that gives the function used to decode this structure from a map
    virtual QString getMapDecodeFunctionBody(bool includeChildren = true) const;


    //! Return the string that gives the signature of the function used to initialize this structure
    virtual QString getSetToInitialValueFunctionSignature(bool insource) const;

    //! Return the string that gives the prototype of the function used to initialize this structure
    virtual QString getSetToInitialValueFunctionPrototype(const QString& spacing = QString(), bool includeChildren = true) const;

    //! Return the string that gives the function used to initialize this structure
    virtual QString getSetToInitialValueFunctionBody(bool includeChildren = true) const;

    //! Get the number of fields that are encoded
    int getNumberOfEncodes(void) const;

    //! Get the number of encoded fields for which we need parameters
    int getNumberOfEncodeParameters(void) const;

    //! Get the number of decoded fields for which we need parameters
    int getNumberOfDecodeParameters(void) const;

    //! Get the number of fields in memory
    int getNumberInMemory(void) const;

    //! Return the include directives needed for this encodable
    void getIncludeDirectives(QStringList& list) const Q_DECL_OVERRIDE;

    //! Return the include directives that go into source code for this encodable
    void getSourceIncludeDirectives(QStringList& list) const Q_DECL_OVERRIDE;

    //! Return the include directives needed for this encodable's init and verify functions
    void getInitAndVerifyIncludeDirectives(QStringList& list) const Q_DECL_OVERRIDE;

    //! Return the include directives needed for this encodable's map functions
    void getMapIncludeDirectives(QStringList& list) const Q_DECL_OVERRIDE;

    //! Return the include directives needed for this encodable's compare functions
    void getCompareIncludeDirectives(QStringList& list) const Q_DECL_OVERRIDE;

    //! Return the include directives needed for this encodable's print functions
    void getPrintIncludeDirectives(QStringList& list) const Q_DECL_OVERRIDE;




    //! Get details needed to produce documentation for this encodable.
    void getDocumentationDetails(QList<int>& outline, QString& startByte, QStringList& bytes, QStringList& names, QStringList& encodings, QStringList& repeats, QStringList& comments) const Q_DECL_OVERRIDE;

    //! Get details needed to produce documentation for this encodables sub-encodables.
    void getSubDocumentationDetails(QList<int>& outline, QString& startByte, QStringList& bytes, QStringList& names, QStringList& encodings, QStringList& repeats, QStringList& comments) const;

    //! Return true if this structure should be hidden from the documentation
    bool isHidden(void) const Q_DECL_OVERRIDE {return hidden;}

    //! True if this encodable has verification data
    bool hasVerify(void) const Q_DECL_OVERRIDE {return hasverify;}

    //! True if this encodable has initialization data
    bool hasInit(void) const Q_DECL_OVERRIDE {return hasinit;}

protected:

    //! Get the declaration that goes in the header which declares this structure and all its children
    QString getStructureDeclaration(bool alwaysCreate) const;

    //! Get the structure declaration, for this structure only (not its children) for the C language.
    virtual QString getStructureDeclaration_C(bool alwaysCreate) const;

    //! Get the class declaration, for this structure only (not its children) for the C++ language.
    virtual QString getClassDeclaration_CPP(void) const;

    //! Make a structure output be prettily aligned
    QString alignStructureData(const QString& structure) const;

    //! Parse all enumerations which are direct children of a DomNode
    void parseEnumerations(const QDomNode& node);

    //! This list of all children encodables
    QList<Encodable*> encodables;

    //! The list of our enumerations
    QList<const EnumCreator*> enumList;

    int  numbitfieldgroupbytes;         //!< Number of temporary bytes needed by children with bitfield groups
    bool bitfields;                     //!< True if this structure uses bitfields
    bool usestempencodebitfields;       //!< True if this structure uses a temporary bitfield for encoding
    bool usestempencodelongbitfields;   //!< True if this structure uses a temporary long bitfield for encoding
    bool usestempdecodebitfields;       //!< True if this structure uses a temporary bitfield for decoding
    bool usestempdecodelongbitfields;   //!< True if this structure uses a temporary long bitfield for decoding
    bool needsEncodeIterator;           //!< True if this structure uses arrays iterators on encode
    bool needsDecodeIterator;           //!< True if this structure uses arrays iterators on decode
    bool needsInitIterator;             //!< True if this structure uses arrays iterators on initialization
    bool needsVerifyIterator;           //!< True if this structure uses arrays iterators on verification
    bool needs2ndEncodeIterator;        //!< True if this structure uses 2nd arrays iterators on encode
    bool needs2ndDecodeIterator;        //!< True if this structure uses 2nd arrays iterators on decode
    bool needs2ndInitIterator;          //!< True if this structure uses 2nd arrays iterators on initialization
    bool needs2ndVerifyIterator;        //!< True if this structure uses 2nd arrays iterators on verification
    bool defaults;                      //!< True if this structure uses default values
    bool strings;                       //!< True if this structure uses strings
    bool hidden;                        //!< True if this structure is to be hidden from the documentation
    bool hasinit;                       //!< True if any children of this structure have initialization data
    bool hasverify;                     //!< True if any children of this structure have verify data
    QStringList attriblist;             //!< List of all attributes that we understand
    QString structName;                 //!< Name of the structure (usually the same as typeName)
    bool encode;                        //!< True if the encode function is output
    bool decode;                        //!< True if the decode function is output
    bool compare;                       //!< True if the comparison function is output
    bool print;                         //!< True if the textPrint function is output
    bool mapEncode;                     //!< True if the mapEncode function is output
    const ProtocolStructureModule* redefines; //!< Pointer to a structure that we are redefining

};

#endif // PROTOCOLSTRUCTURE_H
