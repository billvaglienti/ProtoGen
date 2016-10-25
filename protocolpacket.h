#ifndef PROTOCOLPACKET_H
#define PROTOCOLPACKET_H

#include <QDomElement>
#include <QDomNodeList>
#include <QList>
#include "protocolstructuremodule.h"

// Forward declaration of sublass ProtocolDocumentation
class ProtocolDocumentation;

class ProtocolPacket : public ProtocolStructureModule
{
public:
    //! Construct the packet parsing object, with details about the overall protocol
    ProtocolPacket(ProtocolParser* parse, const QString& protocolName, ProtocolSupport supported, const QString& protocolApi, const QString& protocolVersion);

    //! Destroy the protocol packet
    virtual ~ProtocolPacket(void);

    //! Parse a packet from the DOM
    virtual void parse(void);

    //! Clear out any data
    virtual void clear(void);

    //! The hierarchical name of this object
    virtual QString getHierarchicalName(void) const {return parent + ":" + name;}

    //! Return top level markdown documentation for this packet
    virtual QString getTopLevelMarkdown(bool global = false, const QStringList& ids = QStringList()) const;

    //! Get the ID string of this packet
    QString getId(void) const {return id;}

protected:

    //! Create the structure definition code
    void createStructureDefinition(void);

    //! Create the functions that encode and decode the structure
    void createStructurePacketFunctions(void);

    //! Create the functions that encode and decode the parameters
    void createPacketFunctions(void);

    //! Create the function that encodes the structure
    void createStructureEncodeFunction(const QDomElement& e);

    //! Create the functions that encode and decode the structure
    void createUtilityFunctions(const QDomElement& e);

    //! Get the packet encode signature
    QString getPacketEncodeSignature(void) const;

    //! Get the packet decode signature
    QString getPacketDecodeSignature(void) const;

    //! Get the packet encode comment
    QString getPacketEncodeBriefComment(void) const;

    //! Get the packet decode comment
    QString getPacketDecodeBriefComment(void) const;

    //! Get the parameter list part of a encode signature like ", type1 name1, type2 name2 ... "
    QString getDataEncodeParameterList(void) const;

    //! Get the parameter list part of a decode signature like ", type1* name1, type2 name2[3] ... "
    QString getDataDecodeParameterList(void) const;

    //! Get the structure encode comment
    QString getDataEncodeBriefComment(void) const;

    //! Get the structure decode comment
    QString getDataDecodeBriefComment(void) const;

protected:

    //! Packet identifier string
    QString id;

    //! List of document objects
    QList<ProtocolDocumentation*> documentList;
};

#endif // PROTOCOLPACKET_H
