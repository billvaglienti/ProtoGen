#ifndef PROTOCOLPARSER_H
#define PROTOCOLPARSER_H

#include <QDomDocument>
#include <QFile>
#include <QList>
#include <iostream>
#include "protocolfile.h"
#include "protocolstructuremodule.h"
#include "protocolpacket.h"
#include "enumcreator.h"

class ProtocolParser
{
public:
    ProtocolParser();
    ~ProtocolParser();

    //! Parse the DOM from the xml file. This kicks off the auto code generation for the protocol
    bool parse(const QDomDocument& doc, bool nodoxygen = false, bool nomarkdown = false, bool nohelperfiles = false, QString inlinecss = "", bool disableunrecognized = false);

    //! Output a warning
    static void emitWarning(QString warning) {std::cerr << warning.toStdString() << std::endl;}

    //! Return a list of QDomNodes that are direct children and have a specific tag
    static QList<QDomNode> childElementsByTagName(const QDomNode& node, QString tag, QString tag2 = QString(), QString tag3 = QString());

    //! Return the value of an attribute from a Dom Element
    static QString getAttribute(QString name, const QDomNamedNodeMap& map);

    //! Output a long string of text which should be wrapped at 80 characters.
    static void outputLongComment(ProtocolFile& file, const QString& prefix, const QString& comment);

    //! Parse all enumerations which are direct children of a DomNode
    static void parseEnumerations(QString parent, const QDomNode& node);

    //! Parse all enumerations which are direct children of a DomNode
    static const EnumCreator* parseEnumeration(QString parent, const QDomElement& element);

    //! Output all includes which are direct children of a DomNode
    static void outputIncludes(QString parent, ProtocolFile& file, const QDomNode& node);

    //! Format a long string of text which should be wrapped at 80 characters.
    static QString outputLongComment(const QString& prefix, const QString& text);

    //! Get a correctly reflowed comment from a DOM
    static QString getComment(const QDomElement& e);

    //! Take a comment line and reflow it for our needs.
    static QString reflowComment(QString comment, QString prefix = QString(), int charlimit = 0);

    //! Find the include name for a specific type
    static QString lookUpIncludeName(const QString& typeName);

    //! Find the enumeration creator for this enum
    static const EnumCreator* lookUpEnumeration(const QString& enumName);

    //! Replace any text that matches an enumeration name with the value of that enumeration
    static QString& replaceEnumerationNameWithValue(QString& text);

    //! Determine if text is part of an enumeration.
    static QString getEnumerationNameForEnumValue(const QString& text);

    //! Find the global structure point for a specific type
    static const ProtocolStructure* lookUpStructure(const QString& typeName);

    //! Get the documentation details for a specific global structure type
    static void getStructureSubDocumentationDetails(QString typeName, QList<int>& outline, QString& startByte, QStringList& bytes, QStringList& names, QStringList& encodings, QStringList& repeats, QStringList& comments);

    //! The version of the protocol generator software
    static const QString genVersion;

    //! Get the string used for inline css.
    static QString getDefaultInlinCSS(void);

    //! The path of the xml source file
    static void setInputPath(QString path) {inputpath = path;}

    //! Return the path of the xml source file
    static QString getInputPath(void) {return inputpath;}

    //! Configure a documentation path separate to the main protocol output directory
    void setDocsPath(QString path);

    //! Set LaTeX support
    void setLaTeXSupport(bool on) {latexEnabled = on;}

    //! Return true if the element has a particular attribute set to {'true','yes','1'}
    static bool isFieldSet(const QDomElement &e, QString label);

    //! Return true if the value set to {'true','yes','1'}
    static bool isFieldSet(QString value);

    //! Return true if the element has a particular attribute set to {'false','no','0'}
    static bool isFieldClear(const QDomElement &e, QString label);

    //! Return true if the value is set to {'false','no','0'}
    static bool isFieldClear(QString value);

protected:

    //! Create the source and header files that represent a packet
    bool createPacketFiles(const QDomElement& packet);

    //! Create markdown documentation
    void outputMarkdown(bool isBigEndian, QString inlinecss);

    //! Output the doxygen HTML documentation
    void outputDoxygen(void);

    //! Protocol support information
    static ProtocolSupport support;

    ProtocolHeaderFile header;   //!< The header file (*.h)
    QString name;   //!< Base name of the protocol
    QString prefix; //!< Naming prefix
    QString comment;//!< Comment description of the protocol
    QString version;//!< The version string
    QString api;    //!< The protocol API enumeration

    QString docsDir;    //!< Directory target for storing documentation markdown

    bool latexEnabled;  //!< Generate LaTeX markdown automagically

    QList<ProtocolDocumentation*> documents;
    static QList<ProtocolStructureModule*> structures;
    static QList<ProtocolPacket*> packets;
    static QList<EnumCreator*> enums;
    static QList<EnumCreator*> globalEnums;
    static QString inputpath;


private:

    //! Create the source and header files for the top level module of the protocol
    void createProtocolFiles(const QDomElement& docElem);

};

#endif // PROTOCOLPARSER_H
