#ifndef PROTOCOLSTRUCTUREMODULE_H
#define PROTOCOLSTRUCTUREMODULE_H

#include "protocolstructure.h"
#include "protocolfile.h"
#include "enumcreator.h"
#include <QString>
#include <QDomElement>

class ProtocolStructureModule : public ProtocolStructure
{
public:
    //! Construct the structure parsing object, with details about the overall protocol
    ProtocolStructureModule(const QString& protocolName, const QString& protocolPrefix, ProtocolSupport supported, const QString& protocolApi, const QString& protocolVersion, bool bigendian);

    //! Parse a packet from the DOM
    virtual void parse(const QDomElement& e);

    //! Reset our data contents
    virtual void clear(void);

    //! Destroy the protocol packet
    ~ProtocolStructureModule(void);

    //! Get the name of the header file that encompasses this structure definition
    QString getHeaderFileName(void) const {return header.fileName();}

    //! Output the markdown documentation for this structure and its children
    virtual QString getMarkdown(QString prefix) const;

    /*!
     * Output the top level markdown documentation for the this structure and its children
     * \param indent is the markdown indentation level
     * \return the markdown documentation for this structure and its children
     */
    QString getTopLevelMarkdown(QString indent) const;

protected:

    //! Write data to the source and header files to encode and decode this structure and all its children
    void createStructureFunctions(void);

    //! Write data to the source and header files to encode and decode this structure but not its children
    void createTopLevelStructureFunctions(void);

    //! Parse and output all enumerations which are direct children of a DomNode
    void parseEnumerations(const QDomNode& node);

    QList<EnumCreator*> enumList;   //!< The list of our enumerations

    ProtocolSourceFile source;      //!< The source file (*.c)
    ProtocolHeaderFile header;      //!< The header file (*.c)
    QString api;                    //!< The protocol API enumeration
    QString version;                //!< The version string
    bool isBigEndian;               //!< True if this packets data are encoded in Big Endian

};

#endif // PROTOCOLSTRUCTUREMODULE_H
