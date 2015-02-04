#ifndef PROTOCOLPARSER_H
#define PROTOCOLPARSER_H

#include <QDomDocument>
#include <QFile>
#include <QList>
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
    bool parse(const QDomDocument& doc, bool nodoxygen = false, bool nomarkdown = false, bool nohelperfiles = false);

    //! Return a list of QDomNodes that are direct children and have a specific tag
    static QList<QDomNode> childElementsByTagName(const QDomNode& node, QString tag);

    //! Output a long string of text which should be wrapped at 80 characters.
    static void outputLongComment(ProtocolFile& file, const QString& prefix, const QString& comment);

    //! Output all enumerations which are direct children of a DomNode
    static void outputEnumerations(ProtocolFile& file, const QDomNode& node);

    //! Output all includes which are direct children of a DomNode
    static void outputIncludes(ProtocolFile& file, const QDomNode& node);

    //! Format a long string of text which should be wrapped at 80 characters.
    static QString outputLongComment(const QString& prefix, const QString& text);

    //! Get a correctly reflowed comment from a DOM
    static QString getComment(const QDomElement& e);

    //! Take a comment line and reflow it for our needs.
    static QString reflowComment(const QString& comment);

    //! Find the include name for a specific type
    static QString lookUpIncludeName(const QString& typeName);

    //! Get the markdown documentation for a specific global structure type
    static QString getStructureSubMarkdown(const QString& typeName, QString indent);

    //! The version of the protocol generator software
    static const QString genVersion;

protected:

    //! Wipe any data, including static data
    void clear(void);

    //! Create the source and header files that represent a packet
    bool createPacketFiles(const QDomElement& packet);

    //! Create markdown documentation
    void outputMarkdown(bool isBigEndian);

    //! Output the doxygen HTML documentation
    void outputDoxygen(void);

    ProtocolHeaderFile header;   //!< The header file (*.h)
    QString name;   //!< Base name of the protocol
    QString prefix; //!< Naming prefix
    QString comment;//!< Comment description of the protocol
    QString version;//!< The version string
    QString api;    //!< The protocol API enumeration

    static QList<ProtocolStructureModule*> structures;
    static QList<ProtocolPacket*> packets;
    static QList<EnumCreator*> enums;

private:

    //! Create the source and header files for the top level module of the protocol
    bool createProtocolFiles(const QDomElement& docElem);

};

#endif // PROTOCOLPARSER_H
