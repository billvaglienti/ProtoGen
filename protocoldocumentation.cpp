#include "protocoldocumentation.h"
#include "protocolparser.h"
#include <fstream>

// Initialize convenience strings
const std::string ProtocolDocumentation::TAB_IN = "    ";

//! The list of C language keywords, visible to all encodables
std::vector<std::string> ProtocolDocumentation::keywords = {"auto", "double", "int", "struct", "break", "else", "long", "switch", "case", "enum", "register", "typedef", "char", "extern", "return", "union", "const", "float", "short", "unsigned", "continue", "for", "signed", "void", "default", "goto", "sizeof", "volatile", "do", "if", "static", "while", "bool" };

//! The list of protogen variable names, visible to all encodables
std::vector<std::string> ProtocolDocumentation::variablenames = {"_pg_user", "_pg_user1", "_pg_user2", "_pg_data", "_pg_i", "_pg_j", "_pg_byteindex", "_pg_bytecount", "_pg_numBytes", "_pg_bitfieldbytes", "_pg_tempbitfield", "_pg_templongbitfield", "_pg_bitfieldindex", "_pg_good", "_pg_struct1", "_pg_struct2", "_pg_prename", "_pg_report"};

//! Construct the document object, with details about the overall protocol
ProtocolDocumentation::ProtocolDocumentation(ProtocolParser* parse, std::string Parent, ProtocolSupport supported) :
    support(supported),
    parser(parse),
    parent(Parent),
    e(nullptr),
    attriblist({"name", "title", "comment", "file", "paragraph"}),
    outlineLevel(0)
{
}


//! Parse the document from the DOM
void ProtocolDocumentation::parse(void)
{
    if(e == nullptr)
        return;

    // We have two features we care about in the documentation, "name" which
    // gives the paragraph, and "comment" which gives the documentation to add
    const XMLAttribute* map = e->FirstAttribute();

    name = ProtocolParser::getAttribute("name", map);
    title = ProtocolParser::getAttribute("title", map);
    comment = ProtocolParser::reflowComment(ProtocolParser::getAttribute("comment", map));
    docfile = ProtocolParser::getAttribute("file", map);
    std::string outline = ProtocolParser::getAttribute("paragraph", map);

    if(title.empty())
        title = name;

    // Inform the user if there are any problems with the attributes
    testAndWarnAttributes(map);

    // The outline level is a number
    if(!outline.empty())
        outlineLevel = std::stoi(outline);
}


/*!
 * Return top level markdown documentation for this documentation
 * \param global specifies if this output is the sub of another documentation (global == false) or is a top level documentation.
 * \param ids are ignored
 * \return the markdown output string
 */
std::string ProtocolDocumentation::getTopLevelMarkdown(bool global, const std::vector<std::string>& ids) const
{
    (void)ids;

    std::string markdown;
    int level = outlineLevel;

    // Make sure the outline level is acceptable
    if((level <= 0) || (level > 10))
    {
        if(global)
            level = 2;
        else
            level = 3;
    }

    if(!title.empty())
    {
        for(int i = 0; i < level; i++)
            markdown += "#";

        markdown += " " + title +"\n\n";
    }

    if(!comment.empty())
        markdown += comment + "\n\n";

    if(!docfile.empty())
    {
        std::fstream file(parser->getInputPath() + docfile, std::ios_base::in);

        if(file.is_open())
        {
            markdown += std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            markdown += "\n\n";
            file.close();
        }

    }

    return markdown;

}// ProtocolDocumentation::getTopLevelMarkdown


/*!
 * Output a warning to stderr. The warning will include the hierarchical name.
 * used to describe this objects location in the xml.
 * \param warning is the warning text.
 * \param subname is the subname to append to the hierarchical name, this can
 *        be empty.
 */
void ProtocolDocumentation::emitWarning(const std::string& warning, const std::string& subname) const
{
    std::string name = getHierarchicalName();

    if(!subname.empty())
        name += ":" + subname;

    int line = e->GetLineNum();

    std::cerr << support.sourcefile << "(" << line << "): warning: " << name << ": " << warning << std::endl;
}


/*!
 * Output a warning to stderr. The warning will include the hierarchical name.
 * used to describe this objects location in the xml.
 * \param warning is the warning text.
 * \param a is the offending attribute, which will be used to determine the
 *        name and line number in the warning.
 */
void ProtocolDocumentation::emitWarning(const std::string& warning, const XMLAttribute* a) const
{
    emitWarning(support.sourcefile, getHierarchicalName(), warning, a);
}


/*!
 * Output a warning to stderr.
 * \param hierarchicalName is the name of the object that owns the offending
 *        attribute
 * \param warning is the warning text.
 * \param a is the offending attribute, which will be used to determine the
 *        name and line number in the warning.
 */
void ProtocolDocumentation::emitWarning(const std::string& sourcefile, const std::string& hierarchicalName, const std::string& warning, const XMLAttribute* a)
{
    std::string name;
    if(hierarchicalName.empty())
        name = a->Name();
    else
        name = hierarchicalName + ":" + a->Name();

    int line = a->GetLineNum();

    std::cerr << sourcefile << "(" << line << "): warning: " << name << ": " << warning << std::endl;
}


/*!
 * Test the list of attributes and warn if any of them are unrecognized
 * \param map is the list of attributes
 * \param subname is the optional name of a sub-element for which this test is done
 */
void ProtocolDocumentation::testAndWarnAttributes(const XMLAttribute* map) const
{
    // The only thing we check for is unrecognized attributes
    if(support.disableunrecognized)
        return;

    /// TODO: test for repeated attributes
    for(const XMLAttribute* a = map; a != nullptr; a = a->Next())
    {
        // Check to see if the attribute is not in the list of known attributes
        if((contains(attriblist, a->Name()) == false))
            emitWarning("Unrecognized attribute", a);

    }// for all attributes

}// ProtocolDocumentation::testAndWarnAttributes


/*!
 * Helper function to create a list of ProtocolDocumentation objects based upon the DOM
 * \param parse points to the global protocol parser that owns everything
 * \param Parent is the name of the parent object that owns the created objects
 * \param support is the protocol support object that gives protocol options
 * \param e is the DOM element which may have documentation children
 * \param list receives the list of allocated objects.
 */
void ProtocolDocumentation::getChildDocuments(ProtocolParser* parse, const std::string& Parent, ProtocolSupport support, const XMLElement* e, std::vector<ProtocolDocumentation*>& list)
{
    // The list of documentation that goes inside this packet
    std::vector<const XMLElement*> documents = ProtocolParser::childElementsByTagName(e, "Document");

    // Parse all the document objects
    for(std::size_t i = 0; i < documents.size(); i++)
    {
        // Create the document and parse its xml
        ProtocolDocumentation* doc = new ProtocolDocumentation(parse, Parent, support);

        doc->setElement(documents.at(i));
        doc->parse();

        // Keep it around
        list.push_back(doc);
    }

}// ProtocolDocumentation::getChildDocuments
