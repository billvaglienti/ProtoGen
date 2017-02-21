#ifndef PROTOCOLSUPPORT_H
#define PROTOCOLSUPPORT_H

#include <QString>
#include <QDomElement>

class ProtocolSupport
{
public:
    ProtocolSupport();

    //! Parse attributes from the ProtocolTag
    void parse(const QDomNamedNodeMap& map);

    //! Return the list of attributes understood by ProtocolSupport
    QStringList getAttriblist(void) const;

    int maxdatasize;                //!< Maximum number of data bytes in a packet, 0 if no limit
    bool int64;                     //!< true if support for integers greater than 32 bits is included
    bool float64;                   //!< true if support for double precision is included
    bool specialFloat;              //!< true if support for float16 and float24 is included
    bool bitfield;                  //!< true if support for bitfields is included
    bool longbitfield;              //!< true to support long bitfields
    bool bitfieldtest;              //!< true to output the bitfield test function
    bool disableunrecognized;       //!< true to disable warnings about unrecognized attributes
    bool bigendian;                 //!< Protocol bigendian flag
    QString globalFileName;         //!< File name to be used if a name is not given
    QString outputpath;             //!< path to output files to
    QString packetStructureSuffix;  //!< Name to use at end of encode/decode Packet structure functions
    QString packetParameterSuffix;  //!< Name to use at end of encode/decode Packet parameter functions
    QString protoName;              //!< Name of the protocol
    QString prefix;                 //!< Prefix name
    QString pktPrefix;              //!< Packet prefix name
};

#endif // PROTOCOLSUPPORT_H
