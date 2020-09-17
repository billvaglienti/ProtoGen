#ifndef PROTOCOLSTRUCTURE_H
#define PROTOCOLSTRUCTURE_H

#include "encodable.h"
#include "enumcreator.h"

// Forward declaration of our descendant
class ProtocolStructureModule;

class ProtocolStructure : public Encodable
{
public:

    //! Default constructor for protocol structure
    ProtocolStructure(ProtocolParser* parse, std::string Parent, ProtocolSupport supported);

    //! Reset all data to defaults
    void clear(void) override;

    //! Parse the DOM data for this structure
    void parse(void) override;

    //! Return the struct name, which may be different from typeName
    const std::string& getStructName(void) const {return structName;}

    //! The hierarchical name of this object
    std::string getHierarchicalName(void) const override {return parent + ":" + name;}

    //! Get the declaration for this structure as a member of another
    std::string getDeclaration(void) const override;

    //! Return the string that is used to encode this encodable
    std::string getEncodeString(bool isBigEndian, int* bitcount, bool isStructureMember) const override;

    //! Return the string that is used to decode this encoable
    std::string getDecodeString(bool isBigEndian, int* bitcount, bool isStructureMember, bool defaultEnabled = false) const override;

    //! Get the string used for verifying this field.
    std::string getVerifyString(void) const override;

    //! Return the string that sets this encodable to its initial value in code
    std::string getSetInitialValueString(bool isStructureMember) const override;

    //! Return the strings that #define initial and variable values
    std::string getInitialAndVerifyDefines(bool includeComment = true) const override;

    //! Get the string used for comparing this structure.
    std::string getComparisonString(void) const override;

    //! Get the string used for text printing this structure.
    std::string getTextPrintString(void) const override;

    //! Get the string used for text reading this structure.
    std::string getTextReadString(void) const override;

    //! Return the string used for map encoding this structure
    std::string getMapEncodeString(void) const override;

    //! Return the string used for map decoding this structure
    std::string getMapDecodeString(void) const override;

    //! Parse the DOM data for this structures children
    void parseChildren(const XMLElement* field);

    //! Set the compare flag for this structure and all children structure
    void setCompare(bool enable);

    //! Set the print flag for this structure and all children structure
    void setPrint(bool enable);

    //! Set the mapEncode flag for this structure and all children structure
    void setMapEncode(bool enable);

    //! Determine if this encodable is a primitive, rather than a structure
    bool isPrimitive(void) const override {return false;}

    //! Determine if this encodable a string object
    bool isString(void) const override {return false;}

    //! Get the maximum number of temporary bytes needed for a bitfield group
    void getBitfieldGroupNumBytes(int* num) const override;

    //! True if this encodable has a direct child that uses bitfields
    bool usesBitfields(void ) const override {return bitfields;}

    //! True if this encodable needs a temporary buffer for its bitfield during encode
    bool usesEncodeTempBitfield(void) const override {return usestempencodebitfields;}

    //! True if this encodable needs a temporary buffer for its long bitfield during encode
    bool usesEncodeTempLongBitfield(void) const override {return usestempencodelongbitfields;}

    //! True if this encodable needs a temporary buffer for its bitfield during decode
    bool usesDecodeTempBitfield(void) const override {return usestempdecodebitfields;}

    //! True if this encodable needs a temporary buffer for its long bitfield during decode
    bool usesDecodeTempLongBitfield(void) const override {return usestempdecodelongbitfields;}

    //! True if this encodable has a direct child that needs an iterator
    bool usesEncodeIterator(void) const override {return needsEncodeIterator;}

    //! True if this encodable has a direct child that needs an iterator
    bool usesDecodeIterator(void) const override {return needsDecodeIterator;}

    //! True if this encodable has a direct child that needs an iterator for verifying
    bool usesVerifyIterator(void) const override {return needsVerifyIterator;}

    //! True if this encodable has a direct child that needs an iterator for initializing
    bool usesInitIterator(void) const override {return needsInitIterator;}

    //! True if this encodable has a direct child that needs a 2nd iterator
    bool uses2ndEncodeIterator(void) const override {return needs2ndEncodeIterator;}

    //! True if this encodable has a direct child that needs a 2nd iterator
    bool uses2ndDecodeIterator(void) const override {return needs2ndDecodeIterator;}

    //! True if this encodable has a direct child that needs a 2nd iterator for verifying
    bool uses2ndVerifyIterator(void) const override {return needs2ndVerifyIterator;}

    //! True if this encodable has a direct child that needs a 2nd iterator for initializing
    bool uses2ndInitIterator(void) const override {return needs2ndInitIterator;}

    //! True if this encodable has a direct child that uses defaults
    bool usesDefaults(void) const override {return defaults;}


    //! Return the string that is used to prototype the encode routine for this encodable
    virtual std::string getEncodeFunctionSignature(bool insource) const;

    //! Return the string that gives the prototype of the function used to encode this encodable, may be empty
    virtual std::string getEncodeFunctionPrototype(const std::string& spacing = std::string(), bool includeChildren = true) const;

    //! Return the string that gives the function used to encode this encodable, may be empty
    virtual std::string getEncodeFunctionBody(bool isBigEndian, bool includeChildren = true) const;


    //! Return the string that is used to prototype the decode routine for this encodable
    virtual std::string getDecodeFunctionSignature(bool insource) const;

    //! Return the string that gives the prototype of the function used to decode this encodable, may be empty
    virtual std::string getDecodeFunctionPrototype(const std::string& spacing = std::string(), bool includeChildren = true) const;

    //! Return the string that gives the function used to decode this encodable, may be empty
    virtual std::string getDecodeFunctionBody(bool isBigEndian, bool includeChildren = true) const;


    //! Return the string that gives the signature of the function used to initialize this structure
    virtual std::string getVerifyFunctionSignature(bool insource) const;

    //! Return the string that gives the prototype of the function used to initialize this structure
    virtual std::string getVerifyFunctionPrototype(const std::string& spacing = std::string(), bool includeChildren = true) const;

    //! Return the string that gives the function used to initialize this structure
    virtual std::string getVerifyFunctionBody(bool includeChildren = true) const;


    //! Return the string that gives the signature of the function used to compare this structure
    virtual std::string getComparisonFunctionSignature(bool insource) const;

    //! Return the string that gives the prototype of the function used to compare this structure
    virtual std::string getComparisonFunctionPrototype(const std::string& spacing = std::string(), bool includeChildren = true) const;

    //! Return the string that gives the function used to compare this structure
    virtual std::string getComparisonFunctionBody(bool includeChildren = true) const;


    //! Return the string that gives the signature of the function used to text print this structure
    virtual std::string getTextPrintFunctionSignature(bool insource) const;

    //! Return the string that gives the prototype of the function used to text print this structure
    virtual std::string getTextPrintFunctionPrototype(const std::string& spacing = std::string(), bool includeChildren = true) const;

    //! Return the string that gives the function used to text print this structure
    virtual std::string getTextPrintFunctionBody(bool includeChildren = true) const;


    //! Return the string that gives the signature of the function used to read this structure from text
    virtual std::string getTextReadFunctionSignature(bool insource) const;

    //! Return the string that gives the prototype of the function used to read this structure from text
    virtual std::string getTextReadFunctionPrototype(const std::string& spacing = std::string(), bool includeChildren = true) const;

    //! Return the string that gives the function used to read this structure this structure from text
    virtual std::string getTextReadFunctionBody(bool includeChildren = true) const;


    //! Return the string that gives the signature of the function used to encode this structure to a map
    virtual std::string getMapEncodeFunctionSignature(bool insource) const;

    //! Return the string that gives the prototype of the function used to encode this structure to a map
    virtual std::string getMapEncodeFunctionPrototype(const std::string& spacing = std::string(), bool includeChildren = true) const;

    //! Return the string that gives the function used to encode this structure to a map
    virtual std::string getMapEncodeFunctionBody(bool includeChildren = true) const;


    //! Return the string that gives the signature of the function used to decode this structure from a map
    virtual std::string getMapDecodeFunctionSignature(bool insource) const;

    //! Return the string that gives the prototype of the function used to decode this structure from a map
    virtual std::string getMapDecodeFunctionPrototype(const std::string& spacing = std::string(), bool includeChildren = true) const;

    //! Return the string that gives the function used to decode this structure from a map
    virtual std::string getMapDecodeFunctionBody(bool includeChildren = true) const;


    //! Return the string that gives the signature of the function used to initialize this structure
    virtual std::string getSetToInitialValueFunctionSignature(bool insource) const;

    //! Return the string that gives the prototype of the function used to initialize this structure
    virtual std::string getSetToInitialValueFunctionPrototype(const std::string& spacing = std::string(), bool includeChildren = true) const;

    //! Return the string that gives the function used to initialize this structure
    virtual std::string getSetToInitialValueFunctionBody(bool includeChildren = true) const;

    //! Get the number of fields that are encoded
    int getNumberOfEncodes(void) const;

    //! Get the number of encoded fields for which we need parameters
    int getNumberOfEncodeParameters(void) const;

    //! Get the number of decoded fields for which we need parameters
    int getNumberOfDecodeParameters(void) const;

    //! Get the number of fields in memory
    int getNumberInMemory(void) const;

    //! Return the include directives needed for this encodable
    void getIncludeDirectives(std::vector<std::string>& list) const override;

    //! Return the include directives that go into source code for this encodable
    void getSourceIncludeDirectives(std::vector<std::string>& list) const override;

    //! Return the include directives needed for this encodable's init and verify functions
    void getInitAndVerifyIncludeDirectives(std::vector<std::string>& list) const override;

    //! Return the include directives needed for this encodable's map functions
    void getMapIncludeDirectives(std::vector<std::string>& list) const override;

    //! Return the include directives needed for this encodable's compare functions
    void getCompareIncludeDirectives(std::vector<std::string>& list) const override;

    //! Return the include directives needed for this encodable's print functions
    void getPrintIncludeDirectives(std::vector<std::string>& list) const override;




    //! Get details needed to produce documentation for this encodable.
    void getDocumentationDetails(std::vector<int>& outline, std::string& startByte, std::vector<std::string>& bytes, std::vector<std::string>& names, std::vector<std::string>& encodings, std::vector<std::string>& repeats, std::vector<std::string>& comments) const override;

    //! Get details needed to produce documentation for this encodables sub-encodables.
    void getSubDocumentationDetails(std::vector<int>& outline, std::string& startByte, std::vector<std::string>& bytes, std::vector<std::string>& names, std::vector<std::string>& encodings, std::vector<std::string>& repeats, std::vector<std::string>& comments) const;

    //! Return true if this structure should be hidden from the documentation
    bool isHidden(void) const override {return hidden;}

    //! True if this encodable has verification data
    bool hasVerify(void) const override {return hasverify;}

    //! True if this encodable has initialization data
    bool hasInit(void) const override {return hasinit;}

protected:

    //! Get the declaration that goes in the header which declares this structure and all its children
    std::string getStructureDeclaration(bool alwaysCreate) const;

    //! Get the structure declaration, for this structure only (not its children) for the C language.
    virtual std::string getStructureDeclaration_C(bool alwaysCreate) const;

    //! Get the class declaration, for this structure only (not its children) for the C++ language.
    virtual std::string getClassDeclaration_CPP(void) const;

    //! Create utility functions for structure lengths
    virtual std::string createUtilityFunctions(const std::string& spacing) const {(void)spacing; return std::string();}

    //! Make a structure output be prettily aligned
    std::string alignStructureData(const std::string& structure) const;

    //! Parse all enumerations which are direct children of a DomNode
    void parseEnumerations(const XMLNode* node);

    //! This list of all children encodables
    std::vector<Encodable*> encodables;

    //! The list of our enumerations
    std::vector<const EnumCreator*> enumList;

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
    bool neverOmit;                     //!< True if this structure should never be omitted, even if hidden and -omit-hidden is set
    bool hasinit;                       //!< True if this structure or its children have initialization data
    bool hasverify;                     //!< True if this structure or its children have verify data
    std::string structName;             //!< Name of the structure (usually the same as typeName)
    bool encode;                        //!< True if the encode function is output
    bool decode;                        //!< True if the decode function is output
    bool compare;                       //!< True if the comparison function is output
    bool print;                         //!< True if the textPrint function is output
    bool mapEncode;                     //!< True if the mapEncode function is output
    const ProtocolStructureModule* redefines; //!< Pointer to a structure that we are redefining

};

#endif // PROTOCOLSTRUCTURE_H
