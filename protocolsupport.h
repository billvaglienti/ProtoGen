#ifndef PROTOCOLSUPPORT_H
#define PROTOCOLSUPPORT_H

class ProtocolSupport
{
public:
    ProtocolSupport();
    bool int64;         //!< true if support for integers greater than 32 bits is included
    bool float64;       //!< true if support for double precision is included
    bool specialFloat;  //!< true if support for float16 and float24 is included
    bool bitfield;      //!< true if support for bitfields is included

};

#endif // PROTOCOLSUPPORT_H
