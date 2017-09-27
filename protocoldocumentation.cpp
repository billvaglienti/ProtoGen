#include "protocoldocumentation.h"
#include "protocolparser.h"
#include <QDir>
#include <QFile>

// Initialize convenience strings
const QString ProtocolDocumentation::TAB_IN = "    ";

//! The list of C language keywords, visible to all encodables
QStringList ProtocolDocumentation::keywords = QStringList() << "auto" << "double" << "int" << "struct" << "break" << "else" << "long" << "switch" << "case" << "enum" << "register" << "typedef" << "char" << "extern" << "return" << "union" << "const" << "float" << "short" << "unsigned" << "continue" << "for" << "signed" << "void" << "default" << "goto" << "sizeof" << "volatile" << "do" << "if" << "static" << "while";

//! Construct the document object, with details about the overall protocol
ProtocolDocumentation::ProtocolDocumentation(ProtocolParser* parse, QString Parent, ProtocolSupport supported) :
    support(supported),
    parser(parse),
    parent(Parent),
    outlineLevel(0)
{
}


//! Parse the document from the DOM
void ProtocolDocumentation::parse(void)
{
    // We have two features we care about in the documentation, "name" which
    // gives the paragraph, and "comment" which gives the documentation to add
    QDomNamedNodeMap map = e.attributes();

    name = ProtocolParser::getAttribute("name", map);
    title = ProtocolParser::getAttribute("title", map);
    comment = ProtocolParser::reflowComment(ProtocolParser::getAttribute("comment", map));
    docfile = ProtocolParser::getAttribute("file", map);
    QString outline = ProtocolParser::getAttribute("paragraph", map);

    if(title.isEmpty())
        title = name;

    // Inform the user if there are any problems with the attributes
    testAndWarnAttributes(map, QStringList() << "name" << "title" << "comment" << "file" << "paragraph");

    // The outline level is a number
    if(!outline.isEmpty())
        outlineLevel = outline.toInt();
}


/*!
 * Return top level markdown documentation for this documentation
 * \param global specifies if this output is the sub of another documentation (global == false) or is a top level documentation.
 * \param ids are ignored
 * \return the markdown output string
 */
QString ProtocolDocumentation::getTopLevelMarkdown(bool global, const QStringList& ids) const
{
    Q_UNUSED(ids);

    QString markdown;
    int level = outlineLevel;

    // Make sure the outline level is acceptable
    if((level <= 0) || (level > 10))
    {
        if(global)
            level = 2;
        else
            level = 3;
    }

    if(!title.isEmpty())
    {
        for(int i = 0; i < level; i++)
            markdown += "#";

        markdown += " " + title +"\n\n";
    }

    if(!comment.isEmpty())
        markdown += comment + "\n\n";

    if(!docfile.isEmpty())
    {
        QFile file(parser->getInputPath() + docfile);

        if(file.open(QFile::Text | QFile::ReadOnly))
        {
            markdown += QString(file.readAll()) + "\n\n";
            file.close();
        }

    }

    return markdown;

}// ProtocolDocumentation::getTopLevelMarkdown


/*!
 * Output a warning to stdout. The warning will include the hierarchical name used to describe this objects location in the xml
 * \param warning is the warning text
 * \param subname is the subname to append to the hierarchical name, this can be empty
 */
void ProtocolDocumentation::emitWarning(QString warning, const QString& subname) const
{
    QString name = getHierarchicalName();

    if(!subname.isEmpty())
        name += ":" + subname;

    parser->emitWarning(name, warning);
}


/*!
 * Test the list of attributes and warn if any of them are unrecognized
 * \param map is the list of attributes
 * \param attriblist is the list of attributes that are recognized
 * \param subname is the optional name of a sub-element for which this test is done
 */
void ProtocolDocumentation::testAndWarnAttributes(const QDomNamedNodeMap& map, const QStringList& attriblist, const QString& subname) const
{
    // The only thing we check for is unrecognized attributes
    if(support.disableunrecognized)
        return;

    // Note: I would like to test for repeated attributes,
    // but Qt doesn't make them available (they should be an XML error)

    for(int i = 0; i < map.count(); i++)
    {
        QDomAttr attr = map.item(i).toAttr();
        if(attr.isNull())
            continue;

        // Check to see if the attribute is not in the list of known attributes
        if((attriblist.contains(attr.name(), Qt::CaseInsensitive) == false))
            emitWarning("Unrecognized attribute \"" + attr.name() + "\"", subname);

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
void ProtocolDocumentation::getChildDocuments(ProtocolParser* parse, QString Parent, ProtocolSupport support, const QDomElement& e, QList<ProtocolDocumentation*>& list)
{
    // The list of documentation that goes inside this packet
    QList<QDomNode> documents = ProtocolParser::childElementsByTagName(e, "Documentation");

    // Parse all the document objects
    for(int i = 0; i < documents.count(); i++)
    {
        // Create the document and parse its xml
        ProtocolDocumentation* doc = new ProtocolDocumentation(parse, Parent, support);

        doc->setElement(documents.at(i).toElement());
        doc->parse();

        // Keep it around
        list.push_back(doc);
    }

}// ProtocolDocumentation::getChildDocuments
