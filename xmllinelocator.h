#ifndef XMLLINELOCATOR_H
#define XMLLINELOCATOR_H

#include <QString>
#include <QList>

class XMLContent
{
    friend class XMLLineLocator;

protected:

    //! decleare XMLContent with starting line number and outline level
    XMLContent(void) :
        mylinenumber(0),
        outline(0)
    {}

    //! declare XMLContent with line number and outline level
    XMLContent(int line, int level) :
        mylinenumber(line),
        outline(level)
    {}

    //! Override name for the contens
    void overrideName(QString newname) {name = newname;}

    //! Set the contents of this XMLContent, including subs
    void setXMLContents(const QString& text, int& startindex, int& linenumber);

    //! Get the line number (if any) that matches the hierarchical name
    int getMatchingLineNumber(const QStringList& names, int level) const;

    //! Determine the "name" attribute
    void parseNameFromContents(void);

    //! Determine an attribute value
    static QString parseAttribute(const QString& label, const QString& text);

    //! The list of sub-contents
    QList<XMLContent> subs;

    //! Content of this XML
    QString contents;

    //! "name" attribute of this XML
    QString name;

    //! Line number at the start of this XML
    int mylinenumber;

    //! Outline level of this XML (0 is outermost)
    int outline;
};


class XMLLineLocator
{
public:
    XMLLineLocator(){}

    //! Input the contents of the XML file, this will trigger a parse operation
    void setXMLContents(QString text, QString path, QString file, QString topname = QString());

    //! Find the line number given a hierarchical name
    int getLineNumber(QString hierarchicalName) const;

    //! Output a warning
    bool emitWarning(QString hierarchicalName, QString warning) const;

protected:

    //! The path to the file this locator represents
    QString inputpath;

    //! The name of the file this locator represents
    QString inputfile;

    //! The contents of the file used for line number lookups
    XMLContent contents;
};


#endif // XMLLINELOCATOR_H
