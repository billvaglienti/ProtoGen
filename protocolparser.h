#ifndef PROTOCOLPARSER_H
#define PROTOCOLPARSER_H

#include <iostream>
#include "protocolfile.h"
#include "protocolsupport.h"
#include "tinyxml2.h"

using namespace tinyxml2;

// Forward declarations
class ProtocolDocumentation;
class ProtocolStructure;
class ProtocolStructureModule;
class ProtocolPacket;
class EnumCreator;

class ProtocolParser
{
public:
    ProtocolParser();
    ~ProtocolParser();

    //! Configure a documentation path separate to the main protocol output directory
    void setDocsPath(std::string path);

    //! Set the language override option
    void setLanguageOverride(ProtocolSupport::LanguageType lang) {support.setLanguageOverride(lang);}

    //! Set LaTeX support
    void setLaTeXSupport(bool on) {latexEnabled = on;}

    //! Set LaTeX header level
    void setLaTeXLevel(int level) {latexHeader = level;}

    //! Option to disable markdown output
    void disableMarkdown(bool disable) {nomarkdown = disable;}

    //! Option to disable helper file output
    void disableHelperFiles(bool disable) {nohelperfiles = disable;}

    //! Option to disable doxygen output
    void disableDoxygen(bool disable) {nodoxygen = disable;}

    //! Option to disable 'about this ICD' section
    void disableAboutSection(bool disable) { noAboutSection = disable; }

    //! Option to enable table of contents section
    void enableTableOfContents(bool enable) {tableOfContents = enable;}

    //! Option to enable title page section
    void setTitlePage(std::string pagetext) {titlePage = pagetext;}

    //! Return status of 'About this ICD' section
    bool hasAboutSection() const { return !noAboutSection; }

    //! Option to force documentation for hidden items
    void showHiddenItems(bool show) {support.showAllItems = show;}

    //! Option to skip code generation for hidden items
    void omitHiddenItems(bool omit) {support.omitIfHidden = omit;}

    //! Option to disable unrecognized warnings
    void disableUnrecognizedWarnings(bool disable) {support.disableunrecognized = disable;}

    //! Set the inlinee css
    void setInlineCSS(std::string css) {inlinecss = css;}

    //! Disable CSS entirely
    void disableCSS(bool disable) { nocss = disable; }

    //! Parse the DOM from the xml file(s). This kicks off the auto code generation for the protocol
    bool parse(std::string filename, std::string path, std::vector<std::string> otherfiles);

    //! Return a list of XMLNodes that are direct children and have a specific tag
    static std::vector<const XMLElement*> childElementsByTagName(const XMLElement* node, std::string tag, std::string tag2 = std::string(), std::string tag3 = std::string());

    //! Return the value of an attribute from a Dom Element
    static std::string getAttribute(const std::string& name, const XMLAttribute* attr, const std::string& defaultIfNone = std::string());

    //! Output a long string of text which should be wrapped at 80 characters.
    static void outputLongComment(ProtocolFile& file, const std::string& prefix, const std::string& comment);

    //! Parse all enumerations which are direct children of a node
    void parseEnumerations(const std::string& parent, const XMLNode* node);

    //! Parse all enumerations which are direct children of an element
    const EnumCreator* parseEnumeration(const std::string& parent, const XMLElement* element);

    //! Output all includes which are direct children of a node
    void outputIncludes(const std::string& parent, ProtocolFile& file, const XMLNode* node) const;

    //! Output all includes which are direct children of a element
    void outputIncludes(const std::string& parent, ProtocolFile& file, const XMLElement* element) const;

    //! Format a long string of text which should be wrapped at 80 characters.
    static std::string outputLongComment(const std::string& prefix, const std::string& text);

    //! Get a correctly reflowed comment from a DOM
    static std::string getComment(const XMLElement* e);

    //! Take a comment line and reflow it for our needs.
    static std::string reflowComment(const std::string& text, const std::string& prefix = std::string(), std::size_t charlimit = 0);

    //! Find the include name for a specific type
    std::string lookUpIncludeName(const std::string& typeName) const;

    //! Find the enumeration creator for this enum
    const EnumCreator* lookUpEnumeration(const std::string& enumName) const;

    //! Replace any text that matches an enumeration name with the value of that enumeration
    std::string replaceEnumerationNameWithValue(const std::string& text) const;

    //! Determine if text is part of an enumeration.
    std::string getEnumerationNameForEnumValue(const std::string& text) const;

    //! Find the enumeration with this name and return its comment, or an empty string
    std::string getEnumerationValueComment(const std::string& name) const;

    //! Find the global structure point for a specific type
    const ProtocolStructureModule* lookUpStructure(const std::string& typeName) const;

    //! Get the documentation details for a specific global structure type
    void getStructureSubDocumentationDetails(std::string typeName, std::vector<int>& outline, std::string& startByte, std::vector<std::string>& bytes, std::vector<std::string>& names, std::vector<std::string>& encodings, std::vector<std::string>& repeats, std::vector<std::string>& comments) const;

    //! The version of the protocol generator software
    static const std::string genVersion;

    //! Get the string used for inline css.
    static std::string getDefaultInlinCSS(void);

    //! Return the path of the xml source file
    std::string getInputPath(void) {return inputpath;}

    //! Return true if the element has a particular attribute set to {'true','yes','1'}
    static bool isFieldSet(const XMLElement* e, const std::string& label);

    //! Return true if the value set to {'true','yes','1'}
    static bool isFieldSet(const std::string& value);

    //! Return true if the value of an attribute is 'true', 'yes', or '1'
    static bool isFieldSet(const std::string& attribname, const XMLAttribute* firstattrib);

    //! Return true if the element has a particular attribute set to {'false','no','0'}
    static bool isFieldClear(const XMLElement* e, const std::string& label);

    //! Return true if the value is set to {'false','no','0'}
    static bool isFieldClear(const std::string& value);

    //! Determine if the value of an attribute is either {'false','no','0'}
    static bool isFieldClear(const std::string& attribname, const XMLAttribute* firstattrib);

    //! Set the license text
    void setLicenseText(const std::string text) { support.licenseText = text; }

    std::string getLicenseText() const { return support.licenseText; }

protected:

    //! Parses a single XML file handling any require tags to flatten a file
    bool parseFile(std::string xmlFilename);

    //! Create markdown documentation
    void outputMarkdown(bool isBigEndian, std::string inlinecss);

    //! Get the table of contents, based on the file contents
    std::string getTableOfContents(const std::string& filecontents);

    //! Get the "About this ICD" section to file
    std::string getAboutSection(bool isBigEndian);

    //! Output the doxygen HTML documentation
    void outputDoxygen(void);

    //! Protocol support information
    ProtocolSupport support;

    //! The list of xml documents we created by loading files
    std::vector<XMLDocument*> xmldocs;

    //! The document currently being parsed
    XMLDocument* currentxml;

    //! The protocol header file (*.h)
    ProtocolHeaderFile* header;

    std::string name;   //!< Base name of the protocol
    std::string title;  //!< Title name of the protocol used in documentation
    std::string comment;//!< Comment description of the protocol
    std::string version;//!< The version string
    std::string api;    //!< The protocol API enumeration

    std::string docsDir;    //!< Directory target for storing documentation markdown

    int latexHeader;    //!< Top heading level for LaTeX output
    bool latexEnabled;  //!< Generate LaTeX markdown automagically
    bool nomarkdown;    //!< Disable markdown output
    bool nohelperfiles; //!< Disable helper file output
    bool nodoxygen;     //!< Disable doxygen output
    bool noAboutSection;//!< Disable extra 'about' section in the generated documentation

    std::string inlinecss;  //!< CSS used for markdown output
    bool nocss;         //!< Disable all CSS output
    bool tableOfContents;//!< Enable table of contents
    std::string titlePage;     //!< Title page information

    std::vector<std::string> filesparsed;
    std::vector<ProtocolDocumentation*> alldocumentsinorder;
    std::vector<ProtocolDocumentation*> documents;
    std::vector<ProtocolStructureModule*> structures;
    std::vector<ProtocolPacket*> packets;
    std::vector<EnumCreator*> enums;
    std::vector<EnumCreator*> globalEnums;
    std::string inputpath;
    std::string inputfile;

private:

    //! Create the header file for the top level module of the protocol
    void createProtocolHeader(const XMLElement* docElem);

    //! Finish the protocol header file
    void finishProtocolHeader(void);

};

#endif // PROTOCOLPARSER_H
