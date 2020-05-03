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
    ProtocolPacket(ProtocolParser* parse, ProtocolSupport supported, const QString& protocolApi, const QString& protocolVersion);

    ~ProtocolPacket();

    //! Parse a packet from the DOM
    void parse(void) Q_DECL_OVERRIDE;

    //! Clear out any data
    void clear(void) Q_DECL_OVERRIDE;

    //! The hierarchical name of this object
    QString getHierarchicalName(void) const Q_DECL_OVERRIDE {return parent + ":" + name;}

    //! Return top level markdown documentation for this packet
    QString getTopLevelMarkdown(bool global = false, const QStringList& ids = QStringList()) const Q_DECL_OVERRIDE;

    //! Get all the ID strings of this packet
    void appendIds(QStringList& list) const {list.append(ids);}

    //! Return the extended packet name
    QString extendedName() const { return support.prefix + this->name + support.packetStructureSuffix; }

protected:

    //! Get the class declaration, for this packet only (not its children) for the C++ language
    QString getClassDeclaration_CPP(void) const Q_DECL_OVERRIDE;

    //! Create the functions that encode and decode the structure
    void createStructurePacketFunctions(void);

    //! Create the functions that encode and decode the parameters
    void createPacketFunctions(void);

    //! Create the functions that encode and decode the structure
    void createUtilityFunctions(const QDomElement& e);

    //! Get the signature of the packet structure encode function
    QString getStructurePacketEncodeSignature(bool insource) const;

    //! Get the prototype for the structure packet encode function
    QString getStructurePacketEncodePrototype(const QString& spacing) const;

    //! Get the prototype for the structure packet encode function
    QString getStructurePacketEncodeBody(void) const;

    //! Get the signature of the packet structure decode function
    QString getStructurePacketDecodeSignature(bool insource) const;

    //! Get the prototype for the structure packet decode function
    QString getStructurePacketDecodePrototype(const QString& spacing) const;

    //! Get the prototype for the structure packet decode function
    QString getStructurePacketDecodeBody(void) const;

    //! Get the packet encode signature
    QString getParameterPacketEncodeSignature(bool insource) const;

    //! Get the prototype for the parameter packet encode function
    QString getParameterPacketEncodePrototype(const QString& spacing) const;

    //! Get the prototype for the parameter packet encode function
    QString getParameterPacketEncodeBody(void) const;

    //! Get the packet decode signature
    QString getParameterPacketDecodeSignature(bool insource) const;

    //! Get the prototype for the parameter packet decode function
    QString getParameterPacketDecodePrototype(const QString& spacing) const;

    //! Get the prototype for the parameter packet decode function
    QString getParameterPacketDecodeBody(void) const;

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

    //! Flag to treat this packet as a structure that other structures can reference
    bool useInOtherPackets;

    //! Flag to output parameter functions
    bool parameterFunctions;

    //! Flag to output structure functions
    bool structureFunctions;

    //! Packet identifier string
    QStringList ids;

    //! List of document objects
    QList<ProtocolDocumentation*> documentList;
};

#endif // PROTOCOLPACKET_H
