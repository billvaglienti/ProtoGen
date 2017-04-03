#ifndef PROTOCOLPARSER_H
#define PROTOCOLPARSER_H

#include <QDomDocument>
#include <QFile>
#include <QList>
#include <iostream>
#include "protocolfile.h"
#include "xmllinelocator.h"
#include "protocolsupport.h"

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
    void setDocsPath(QString path);

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

    //! Option to force documentation for hidden items
    void showHiddenItems(bool show) {showAllItems = show;}

    //! Option to disable unrecognized warnings
    void disableUnrecognizedWarnings(bool disable) {support.disableunrecognized = disable;}

    //! Set the inlinee css
    void setInlineCSS(QString css) {inlinecss = css;}

    //! Parse the DOM from the xml file. This kicks off the auto code generation for the protocol
    bool parse(QString filename, QString path);

    //! Get the line number from a hierarchical name
    int getLineNumber(const QString& hierarchicalName) {return line.getLineNumber(hierarchicalName);}

    //! Output a warning
    void emitWarning(QString warning) const;

    //! Return a list of QDomNodes that are direct children and have a specific tag
    static QList<QDomNode> childElementsByTagName(const QDomNode& node, QString tag, QString tag2 = QString(), QString tag3 = QString());

    //! Return the value of an attribute from a Dom Element
    static QString getAttribute(QString name, const QDomNamedNodeMap& map, QString defaultIfNone = QString());

    //! Output a long string of text which should be wrapped at 80 characters.
    static void outputLongComment(ProtocolFile& file, const QString& prefix, const QString& comment);

    //! Parse all enumerations which are direct children of a DomNode
    void parseEnumerations(QString parent, const QDomNode& node);

    //! Parse all enumerations which are direct children of a DomNode
    const EnumCreator* parseEnumeration(QString parent, const QDomElement& element);

    //! Output all includes which are direct children of a DomNode
    void outputIncludes(QString parent, ProtocolFile& file, const QDomNode& node) const;

    //! Format a long string of text which should be wrapped at 80 characters.
    static QString outputLongComment(const QString& prefix, const QString& text);

    //! Get a correctly reflowed comment from a DOM
    static QString getComment(const QDomElement& e);

    //! Take a comment line and reflow it for our needs.
    static QString reflowComment(QString comment, QString prefix = QString(), int charlimit = 0);

    //! Find the include name for a specific type
    QString lookUpIncludeName(const QString& typeName) const;

    //! Find the enumeration creator for this enum
    const EnumCreator* lookUpEnumeration(const QString& enumName) const;

    //! Replace any text that matches an enumeration name with the value of that enumeration
    QString& replaceEnumerationNameWithValue(QString& text) const;

    //! Determine if text is part of an enumeration.
    QString getEnumerationNameForEnumValue(const QString& text) const;

    //! Find the global structure point for a specific type
    const ProtocolStructure* lookUpStructure(const QString& typeName) const;

    //! Get the documentation details for a specific global structure type
    void getStructureSubDocumentationDetails(QString typeName, QList<int>& outline, QString& startByte, QStringList& bytes, QStringList& names, QStringList& encodings, QStringList& repeats, QStringList& comments) const;

    //! The version of the protocol generator software
    static const QString genVersion;

    //! Get the string used for inline css.
    static QString getDefaultInlinCSS(void);

    //! Return the path of the xml source file
    QString getInputPath(void) {return inputpath;}

    //! Return true if the element has a particular attribute set to {'true','yes','1'}
    static bool isFieldSet(const QDomElement &e, QString label);

    //! Return true if the value set to {'true','yes','1'}
    static bool isFieldSet(QString value);

    static bool isFieldSet(QString value, QDomNamedNodeMap map);

    //! Return true if the element has a particular attribute set to {'false','no','0'}
    static bool isFieldClear(const QDomElement &e, QString label);

    //! Return true if the value is set to {'false','no','0'}
    static bool isFieldClear(QString value);


    /* Functions for converting a string to a numerical value */

    static bool isDecNum(QString text, int& value);
    static bool isHexNum(QString text, int& value);
    static bool isBinNum(QString text, int& value);

    static bool isNumber(QString text, int& value);

    //! Take a sum of numbers (e.g. an enumeration value) and attempt to compress it
    static QString compressSum(QString text);

protected:

    //! Create the source and header files that represent a packet
    bool createPacketFiles(const QDomElement& packet);

    //! Create markdown documentation
    void outputMarkdown(bool isBigEndian, QString inlinecss);

    //! Output the doxygen HTML documentation
    void outputDoxygen(void);

    //! Protocol support information
    ProtocolSupport support;

    QDomDocument doc;
    XMLLineLocator line;

    ProtocolHeaderFile header;   //!< The header file (*.h)
    QString name;   //!< Base name of the protocol
    QString comment;//!< Comment description of the protocol
    QString version;//!< The version string
    QString api;    //!< The protocol API enumeration

    QString docsDir;    //!< Directory target for storing documentation markdown

    int latexHeader;    //!< Top heading level for LaTeX output
    bool latexEnabled;  //!< Generate LaTeX markdown automagically
    bool nomarkdown;    //!< Disable markdown output
    bool nohelperfiles; //!< Disable helper file output
    bool nodoxygen;     //!< Disable doxygen output
    bool showAllItems;  //!< Generate documentation even for elements with 'hidden="true"'
    QString inlinecss;  //!< CSS used for markdown output

    QList<ProtocolDocumentation*> alldocumentsinorder;
    QList<ProtocolDocumentation*> documents;
    QList<ProtocolStructureModule*> structures;
    QList<ProtocolPacket*> packets;
    QList<EnumCreator*> enums;
    QList<EnumCreator*> globalEnums;
    QString inputpath;
    QString inputfile;

private:

    //! Create the source and header files for the top level module of the protocol
    void createProtocolFiles(const QDomElement& docElem);

};

#endif // PROTOCOLPARSER_H
