#ifndef PROTOCOLBITFIELD_H
#define PROTOCOLBITFIELD_H


/*!
 * \file
 * Auto magically generate the plethora of scaling functions
 *
 * Scaling functions convert all built in float types (double, float
 * float) to different integer encodings (signed, unsigned, big endian,
 * littl endian, and byte lengths from 1 to 8). That's a lot of functions,
 * hence the desire to auto-generate them.
 */


#include "protocolfile.h"
#include "protocolsupport.h"
#include <QList>
#include <QStringList>

class ProtocolBitfield
{
public:
    //! Construct the protocol bitfield object
    ProtocolBitfield(ProtocolSupport sup);

    //! Perform the generation, writing out the files
    void generate(void);

private:

    //! The header for bitfield special
    void generateHeader(void);

    //! The source for bitfield special, not counting the long versions
    void generateSource(void);

    //! The long version (64-bit fields)
    void generateLongSource(void);

    //! The encode source function
    void generateEncodeBitfield(void);

    //! The decode source function
    void generateDecodeBitfield(void);

    //! The long encode source function
    void generateEncodeLongBitfield(void);

    //! The long decode source function
    void generateDecodeLongBitfield(void);

    //! The test code
    void generateTest(void);

    //! Header file output object
    ProtocolHeaderFile header;

    //! Source file output object
    ProtocolSourceFile source;

    //! Whats supported by the protocol
    ProtocolSupport support;

};

#endif // PROTOCOLBITFIELD_H
