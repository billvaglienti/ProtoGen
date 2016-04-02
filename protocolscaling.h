#ifndef PROTOCOLSCALING_H
#define PROTOCOLSCALING_H

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

class ProtocolScaling
{
public:
    //! Construct the protocol scaling object
    ProtocolScaling(ProtocolSupport sup);

    //! Perform the generation, writing out the files
    bool generate(void);

protected:

    //! Generate the encode header file
    bool generateEncodeHeader(void);

    //! Generate the encode source file
    bool generateEncodeSource(void);

    //! Generate the decode header file
    bool generateDecodeHeader(void);

    //! Generate the decode source file
    bool generateDecodeSource(void);

    //! Generate the one line brief comment for the encode function
    QString briefEncodeComment(int type, int length, bool bigendian, bool Unsigned);

    //! Generate the full comment for the encode function
    QString fullEncodeComment(int type, int length, bool bigendian, bool Unsigned);

    //! Generate the encode function signature
    QString encodeSignature(int type, int length, bool bigendian, bool Unsigned);

    //! Generate the full encode function
    QString fullEncodeFunction(int type, int length, bool bigendian, bool Unsigned);

    //! Generate the one line brief comment for the decode function
    QString briefDecodeComment(int type, int length, bool bigendian, bool Unsigned);

    //! Generate the full comment for the decode function
    QString fullDecodeComment(int type, int length, bool bigendian, bool Unsigned);

    //! Generate the decode function signature
    QString decodeSignature(int type, int length, bool bigendian, bool Unsigned);

    //! Generate the full decode function
    QString fullDecodeFunction(int type, int length, bool bigendian, bool Unsigned);

    //! Header file output object
    ProtocolHeaderFile header;

    //! Source file output object
    ProtocolSourceFile source;

    //! List of built in type names
    QStringList typeNames;

    //! List of type names in function signature
    QStringList typeSigNames;

    //! Size of built in types
    QList<int> typeSizes;

    //! Indices that we go from (the built in floating point types)
    QList<int> fromIndices;

    //! Whats supported by the protocol
    ProtocolSupport support;
};

#endif // PROTOCOLSCALING_H
