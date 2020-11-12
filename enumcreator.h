#ifndef ENUMCREATOR_H
#define ENUMCREATOR_H

#include "protocolsupport.h"
#include "protocoldocumentation.h"

// Forward declaration of EnumCreator so EnumElement can see it
class EnumCreator;

class EnumElement : public ProtocolDocumentation
{
    // So EnumCreator can access my protected members
    friend class EnumCreator;

public:

    //! Cronstruct an enumeration element
    EnumElement(ProtocolParser* parse, EnumCreator* creator, const std::string& Parent, ProtocolSupport supported);

    //! Parse an enumeration element
    void parse(void) override;

    //! Check the enumeration element against C keywords
    void checkAgainstKeywords(void) override;

    //! Return true if this element is hidden from the documentation
    bool isHidden (void) const override {return hidden;}

    //! The enumeration element name, with or without prefix
    std::string getName() const;

    //! The enumeration element lookup name
    std::string getLookupName() const;

    //! The enumeration element declaration string
    std::string getDeclaration() const;

protected:

    //! The name returned
    std::string lookupName;

    //! The numeration value string
    std::string value;

    //! The numeration number (if it can be computed)
    std::string number;

    //! Indicates if this enumeration element is hidden from documentation
    bool hidden;

    //! Indicates if this enumeration element does not use the enumeration prefix
    bool ignoresPrefix;

    //! Indicates if this enumeration element does not produce lookup code
    bool ignoresLookup;

    //! Pointer to the parent enumeration that owns this element
    EnumCreator* parentEnum;

};

class EnumCreator : public ProtocolDocumentation
{
public:
    //! Construct the enumeration object
    EnumCreator(ProtocolParser* parse, const std::string& parent, ProtocolSupport supported);

    ~EnumCreator(void);

    //! Empty the enumeration list
    void clear(void);

    //! Parse the DOM to fill out the enumeration list
    void parse() override;

    //! Parse the DOM to fill out the enumeration list for a global enum
    void parseGlobal(void);

    //! Check names against the list of C keywords
    void checkAgainstKeywords(void) override;

    //! Get the markdown documentation for this enumeration
    std::string getTopLevelMarkdown(bool global = false, const std::vector<std::string>& ids = std::vector<std::string>()) const override;

    //! Return the enumeration name
    std::string getName(void) const {return name;}

    //! Return the enumeration comment
    std::string getComment(void) const {return comment;}

    //! Return the header file output string
    std::string getOutput(void) const {return output;}

    //! Get the prefix applied to each enumeration element
    std::string getPrefix() const { return prefix; }

    //! Return the source file output string
    std::string getSourceOutput(void) const { return sourceOutput; }

    //! Replace any text that matches an enumeration name with the value of that enumeration
    std::string replaceEnumerationNameWithValue(const std::string& text) const;

    //! Find the enumeration value with this name and return its comment, or an empty string
    std::string getEnumerationValueComment(const std::string& name) const;

    //! Return the name of the first enumeration in the list (used for initial value)
    std::string getFirstEnumerationName(void) const;

    //! Determine if text is an enumeration name
    bool isEnumerationValue(const std::string& text) const;

    //! Return the minimum number of bits needed to encode the enumeration
    int getMinBitWidth(void) const {return minbitwidth;}

    //! Return the maximum value used by the enumeration
    int getMaximumValue(void) const {return maxvalue;}

    //! Return true if this enumeration is hidden from the documentation
    bool isHidden (void) const override {return hidden;}

    //! Return true if this enumeration is never omitted
    bool isNeverOmit (void) const {return neverOmit;}

    //! The hierarchical name of this object
    std::string getHierarchicalName(void) const override {return parent + ":" + name;}

    //! Get the name of the header file (if any) holding this enumeration
    std::string getHeaderFileName(void) const {return file;}

    //! Get the path of the header file (if any) holding this enumeration
    std::string getHeaderFilePath(void) const {return filepath;}

protected:

    //! Parse the enumeration values to build the number list
    void computeNumberList(void);

    //! Split string around math operators
    std::vector<std::string> splitAroundMathOperators(std::string text) const;

    //! Determine if a character is a math operator
    bool isMathOperator(char op) const;

    //! Output file for global enumerations
    std::string file;

    //! Output file path for global enumerations
    std::string filepath;

    //! Output file for source code file (may not be used)
    std::string sourceOutput;

    //! List of all the enumerated values
    std::vector<EnumElement> elements;

    //! A longer description is possible for enums (will be displayed in the documentation)
    std::string description;

    //! A prefix can be specified for each element in the enum
    std::string prefix;

    //! The header file output string of the enumeration
    std::string output;

    //! Minimum number of bits needed to encode the enumeration
    int minbitwidth;

    //! Maximum value of this enumeration
    int maxvalue;

    //! Determines if this enum will be hidden from the documentation
    bool hidden;

    //! Indicates this enumeration should never be omitted, even if hidden and -omit-hidden is set
    bool neverOmit;

    //! Determines if this enumeration will support revese-lookup of label (based on value)
    bool lookup;

    //! Determines if this enumeration will support revese-lookup of title (based on value)
    bool lookupTitle;

    //! Determines if this enumeration will support revese-lookup of comment (based on value)
    bool lookupComment;

    //! Flag set true if parseGlobal() is called
    bool isglobal;

    //! List of document objects
    std::vector<ProtocolDocumentation*> documentList;
};

//! Output a string with specific spacing
std::string spacedString(const std::string& text, std::size_t spacing);

#endif // ENUMCREATOR_H
