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
    ProtocolStructureModule(ProtocolParser* parse, ProtocolSupport supported, const QString& protocolApi, const QString& protocolVersion);

    //! Parse a packet from the DOM
    virtual void parse(void);

    //! Reset our data contents
    virtual void clear(void);

    //! Destroy the protocol packet
    ~ProtocolStructureModule(void);

    //! Get the name of the header file that encompasses this structure definition
    QString getDefinitionFileName(void) const;

    //! Get the name of the header file that encompasses this structure interface functions
    QString getHeaderFileName(void) const {return header.fileName();}

    //! Get the name of the source file for this structure
    QString getSourceFileName(void) const {return source.fileName();}

    //! Get the path of the header file that encompasses this structure definition
    QString getDefinitionFilePath(void) const;

    //! Get the path of the header file that encompasses this structure interface functions
    QString getHeaderFilePath(void) const {return header.filePath();}

    //! Get the path of the source file for this structure
    QString getSourceFilePath(void) const {return source.filePath();}

protected:

    //! Issue warnings for the structure module.
    void issueWarnings(const QDomNamedNodeMap& map);

    //! Write data to the source and header files to encode and decode this structure and all its children
    void createStructureFunctions(void);

    //! Create the functions that encode/decode sub stuctures.
    void createSubStructureFunctions(void);

    //! Write data to the source and header files to encode and decode this structure but not its children
    void createTopLevelStructureFunctions(void);

    ProtocolSourceFile source;      //!< The source file (*.c)
    ProtocolHeaderFile header;      //!< The header file (*.h)
    ProtocolHeaderFile defheader;   //!< The header file name for the structure definition
    QString api;                    //!< The protocol API enumeration
    QString version;                //!< The version string
    bool encode;                    //!< True if the encode function is output
    bool decode;                    //!< True if the decode function is output

};

#endif // PROTOCOLSTRUCTUREMODULE_H
