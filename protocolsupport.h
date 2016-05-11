#ifndef PROTOCOLSUPPORT_H
#define PROTOCOLSUPPORT_H

#include <QString>

class ProtocolSupport
{
public:
    ProtocolSupport();
    bool int64;         //!< true if support for integers greater than 32 bits is included
    bool float64;       //!< true if support for double precision is included
    bool specialFloat;  //!< true if support for float16 and float24 is included
    bool bitfield;      //!< true if support for bitfields is included
    bool longbitfield;  //!< true to support long bitfields
    bool bitfieldtest;  //!< true to output the bitfield test function
    QString globalFileName; //!< File name to be used if a name is not given
};

#endif // PROTOCOLSUPPORT_H
