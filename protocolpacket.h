#ifndef PROTOCOLPACKET_H
#define PROTOCOLPACKET_H

#include "protocolstructuremodule.h"

// Forward declaration of sublass ProtocolDocumentation
class ProtocolDocumentation;

class ProtocolPacket : public ProtocolStructureModule
{
public:
    //! Construct the packet parsing object, with details about the overall protocol
    ProtocolPacket(ProtocolParser* parse, ProtocolSupport supported);

    ~ProtocolPacket();

    //! Parse a packet from the DOM
    void parse(bool nocode = false) override;

    //! Clear out any data
    void clear(void) override;

    //! The hierarchical name of this object
    std::string getHierarchicalName(void) const override {return parent + ":" + name;}

    //! Return top level markdown documentation for this packet
    std::string getTopLevelMarkdown(bool global = false, const std::vector<std::string>& ids = std::vector<std::string>()) const override;

    //! Get the string which identifies this encodable in a CAN DBC file
    std::string getDBCMessageString(uint32_t baseid, uint8_t typeshift) const;

    //! Get the string which comments this encodable in a CAN DBC file
    std::string getDBCMessageComment(uint32_t baseid, uint8_t typeshift) const;

    //! Get the string which comments this encodables enumerations in a CAN DBC file
    std::string getDBCMessageEnum(uint32_t baseid, uint8_t typeshift) const;

    //! Get all the ID strings of this packet
    void appendIds(std::vector<std::string>& list) const {list.insert(list.end(), ids.begin(), ids.end());}

    //! Return the extended packet name
    std::string extendedName() const { return support.prefix + this->name + support.packetStructureSuffix; }

    //! Return the flag indicating if this packet has DBC transmit turned on
    bool dbctx(void) const {return dbctxon;}

    //! Return the flag indicating if this packet has DBC receive turned on
    bool dbcrx(void) const {return dbcrxon;}

protected:

    //! Get the class declaration, for this packet only (not its children) for the C++ language
    std::string getClassDeclaration_CPP(void) const override;

    //! Create the helper functions for the packet
    std::string createUtilityFunctions(const std::string& spacing) const override;

    //! Write the top level initialize / constructor function.
    void createTopLevelInitializeFunction(void);

    //! Write data to the source and header for structure functions that do not use a packet.
    void createTopLevelStructureFunctions(void);

    //! Create the functions that encode and decode the structure
    void createStructurePacketFunctions(void);

    //! Create the functions that encode and decode the parameters
    void createPacketFunctions(void);

    //! Get the signature of the packet structure encode function
    std::string getStructurePacketEncodeSignature(bool insource) const;

    //! Get the prototype for the structure packet encode function
    std::string getStructurePacketEncodePrototype(const std::string& spacing) const;

    //! Get the prototype for the structure packet encode function
    std::string getStructurePacketEncodeBody(void) const;

    //! Get the signature of the packet structure decode function
    std::string getStructurePacketDecodeSignature(bool insource) const;

    //! Get the prototype for the structure packet decode function
    std::string getStructurePacketDecodePrototype(const std::string& spacing) const;

    //! Get the prototype for the structure packet decode function
    std::string getStructurePacketDecodeBody(void) const;

    //! Get the packet encode signature
    std::string getParameterPacketEncodeSignature(bool insource) const;

    //! Get the prototype for the parameter packet encode function
    std::string getParameterPacketEncodePrototype(const std::string& spacing) const;

    //! Get the prototype for the parameter packet encode function
    std::string getParameterPacketEncodeBody(void) const;

    //! Get the packet decode signature
    std::string getParameterPacketDecodeSignature(bool insource) const;

    //! Get the prototype for the parameter packet decode function
    std::string getParameterPacketDecodePrototype(const std::string& spacing) const;

    //! Get the prototype for the parameter packet decode function
    std::string getParameterPacketDecodeBody(void) const;

    //! Get the packet encode comment
    std::string getPacketEncodeBriefComment(void) const;

    //! Get the packet decode comment
    std::string getPacketDecodeBriefComment(void) const;

    //! Get the parameter list part of a encode signature like ", type1 name1, type2 name2 ... "
    std::string getDataEncodeParameterList(void) const;

    //! Get the parameter list part of a decode signature like ", type1* name1, type2 name2[3] ... "
    std::string getDataDecodeParameterList(void) const;

    //! Get the structure encode comment
    std::string getDataEncodeBriefComment(void) const;

    //! Get the structure decode comment
    std::string getDataDecodeBriefComment(void) const;

protected:

    //! Flag to treat this packet as a structure that other structures can reference
    bool useInOtherPackets;

    //! Flag to output parameter functions
    bool parameterFunctions;

    //! Flag to output structure functions
    bool structureFunctions;

    //! Flag for DBC transmit turned on
    bool dbctxon;

    //! Flag for DBC receive turned on
    bool dbcrxon;

    //! Packet identifier string
    std::vector<std::string> ids;

    //! List of document objects
    std::vector<ProtocolDocumentation*> documentList;
};

#endif // PROTOCOLPACKET_H
