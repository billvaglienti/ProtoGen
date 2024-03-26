#ifndef PROTOCOLDOCUMENTATION_H
#define PROTOCOLDOCUMENTATION_H

#include <vector>
#include <string>
#include "protocolsupport.h"

class ProtocolParser;

class ProtocolDocumentation
{
public:

    //! Construct the document object
    ProtocolDocumentation(ProtocolParser* parse, std::string Parent, ProtocolSupport supported);

    //! Virtual destructor
    virtual ~ProtocolDocumentation(void) {;}

    //! Set the element from the DOM
    virtual void setElement(const XMLElement* element) {e = element;}

    //! Get the element
    const XMLElement* getElement(void) {return e;}

    //! Parse the document from the DOM
    virtual void parse(bool nocode = false);

    //! Return top level markdown documentation for this packet
    virtual std::string getTopLevelMarkdown(bool global = false, const std::vector<std::string>& ids = std::vector<std::string>()) const;

    //! Documentation is by definition never hidden from the documentation
    virtual bool isHidden(void) const {return false;}

    //! The hierarchical name of this object
    virtual std::string getHierarchicalName(void) const {return parent + ":" + name;}

    //! Check names against the list of C keywords
    virtual void checkAgainstKeywords(void){}

    //! Output a warning
    void emitWarning(const std::string& warning, const std::string& subname = std::string()) const;

    //! Output a warning for an attribute
    void emitWarning(const std::string& warning, const XMLAttribute* a) const;

    //! Output a warning for an attribute to stderr
    static void emitWarning(const std::string& sourcefile, const std::string& hierarchicalName, const std::string& warning, const XMLAttribute* a);

    //! Test the list of attributes and warn if any of them are unrecognized or repeated
    void testAndWarnAttributes(const XMLAttribute* map) const;

    //! Helper function to create a list of ProtocolDocumentation objects
    static void getChildDocuments(ProtocolParser* parse, const std::string& parent, ProtocolSupport support, const XMLElement* e, std::vector<ProtocolDocumentation*>& list);

public:

    //! String used to tab code in (perhaps one day we'll make this user changeable)
    static const std::string TAB_IN;

    std::string name;           //!< The name of this encodable
    std::string title;          //!< The title of this encodable (used for documentation)
    std::string comment;        //!< The comment that goes with this encodable

protected:

    ProtocolSupport support;    //!< Information about what is supported
    ProtocolParser* parser;     //!< The parser object
    std::string parent;         //!< The parent name of this encodable
    const XMLElement* e;        //!< The DOM element which is the source of this object's data

    std::vector<std::string> attriblist;//!< List of all attributes that we understand

    static std::vector<std::string> keywords;     //!< keywords for the C language
    static std::vector<std::string> variablenames;//!< variables used by protogen

private:
    int outlineLevel;           //!< The paragraph outline level
    std::string docfile;        //!< File for external documentation
};

#endif // PROTOCOLDOCUMENTATION_H
