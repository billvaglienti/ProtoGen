#ifndef PROTOCOLCODE_H
#define PROTOCOLCODE_H

#include <stdint.h>
#include <string>
#include "encodable.h"

class ProtocolCode : public Encodable
{
    public:

    //! Construct a field, setting the protocol name and name prefix
    ProtocolCode(ProtocolParser* parse, std::string parent, ProtocolSupport supported);

    //! Reset all data to defaults
    void clear(void) override;

    //! Parse the DOM element
    void parse(void) override;

    //! The hierarchical name of this object
    std::string getHierarchicalName(void) const override {return parent + ":" + name;}

    //! Return the string that is used to encode this encodable
    std::string getEncodeString(bool isBigEndian, int* bitcount, bool isStructureMember) const override;

    //! Return the string that is used to decode this encoable
    std::string getDecodeString(bool isBigEndian, int* bitcount, bool isStructureMember, bool defaultEnabled = false) const override;

    //! Return the string that is used to declare this encodable
    std::string getDeclaration(void) const override {return std::string();}

    //! Return the include directives that go into source code needed for this encodable
    void getSourceIncludeDirectives(std::vector<std::string>& list) const override;

    //! Return the signature of this field in an encode function signature
    std::string getEncodeSignature(void) const override {return std::string();}

    //! Return the signature of this field in a decode function signature
    std::string getDecodeSignature(void) const override {return std::string();}

    //! Return the string that documents this field as a encode function parameter
    std::string getEncodeParameterComment(void) const override {return std::string();}

    //! Return the string that documents this field as a decode function parameter
    std::string getDecodeParameterComment(void) const override {return std::string();}

    //! Code tag does not add documentation
    bool hasDocumentation(void) override {return false;}

    //! Get details needed to produce documentation for this encodable.
    void getDocumentationDetails(std::vector<int>& outline, std::string& startByte, std::vector<std::string>& bytes, std::vector<std::string>& names, std::vector<std::string>& encodings, std::vector<std::string>& repeats, std::vector<std::string>& comments) const override
    {
        (void)outline; (void)startByte; (void)bytes; (void)names; (void)encodings; (void)repeats; (void)comments; return;
    }

    //! Returns true since protocol code is a primitive type
    bool isPrimitive(void) const override {return true;}

    //! Returns false since protocol code is not a string
    bool isString(void) const override {return false;}

    //! Returns true since protocol code is not in memory
    bool isNotInMemory(void) const override {return true;}

    //! True if this encodable has a direct child that uses bitfields
    bool usesBitfields(void ) const override {return false;}

    //! True if this encodable has a direct child that needs an iterator on encode
    bool usesEncodeIterator(void) const override {return false;}

    //! True if this encodable has a direct child that needs an iterator on decode
    bool usesDecodeIterator(void) const override {return false;}

    //! True if this encodable has a direct child that needs an iterator for verifying
    bool usesVerifyIterator(void) const override {return false;}

    //! True if this encodable has a direct child that needs an iterator for initializing
    bool usesInitIterator(void) const override {return false;}

    //! True if this encodable has a direct child that needs an iterator on encode
    bool uses2ndEncodeIterator(void) const override {return false;}

    //! True if this encodable has a direct child that needs an iterator on decode
    bool uses2ndDecodeIterator(void) const override {return false;}

    //! True if this encodable has a direct child that needs an second iterator for verifying
    bool uses2ndVerifyIterator(void) const override {return false;}

    //! True if this encodable has a direct child that needs an second iterator for initializing
    bool uses2ndInitIterator(void) const override {return false;}

    //! True if this encodable has a direct child that uses defaults
    bool usesDefaults(void) const override {return false;}

    //! True if this encodable invalidates an earlier default
    bool invalidatesPreviousDefault(void) const override {return false;}

protected:
    std::string encode;
    std::string decode;
    std::string encodecpp;
    std::string decodecpp;
    std::string encodepython;
    std::string decodepython;
    std::string include;
};

#endif // PROTOCOLCODE_H
