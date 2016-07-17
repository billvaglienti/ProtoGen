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
    virtual void parse(void);

    //! Reset our data contents
    virtual void clear(void);

    //! Destroy the protocol packet
    ~ProtocolStructureModule(void);

    //! Get the name of the header file that encompasses this structure definition
    QString getHeaderFileName(void) const {return header.fileName();}

    //! Get the name of the source file for this structure
    QString getSourceFileName(void) const {return source.fileName();}

protected:

    //! Write data to the source and header files to encode and decode this structure and all its children
    void createStructureFunctions(void);

    //! Create the functions that encode/decode sub stuctures.
    void createSubStructureFunctions(void);

    //! Write data to the source and header files to encode and decode this structure but not its children
    void createTopLevelStructureFunctions(void);

    ProtocolSourceFile source;      //!< The source file (*.c)
    ProtocolHeaderFile header;      //!< The header file (*.c)
    QString api;                    //!< The protocol API enumeration
    QString version;                //!< The version string
    bool isBigEndian;               //!< True if this packets data are encoded in Big Endian
    bool encode;                    //!< True if the encode function is output
    bool decode;                    //!< True if the decode function is output

};

#endif // PROTOCOLSTRUCTUREMODULE_H
