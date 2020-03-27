#ifndef PROTOCOLSCALING_H
#define PROTOCOLSCALING_H

/*!
 * \file
 * Auto magically generate the plethora of scaling functions
 *
 * Scaling functions convert all built in in-memory types to different
 * to different integer encodings (signed, unsigned, big and little endian,
 * and byte lengths from 1 to 8). That's a lot of functions, hence the desire
 * to auto-generate them.
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

    //! Enumeration for types that can exist in-memory
    typedef enum
    {
        float64inmemory,
        uint64inmemory,
        int64inmemory,
        float32inmemory,
        uint32inmemory,
        int32inmemory,
        uint16inmemory,
        int16inmemory,
        uint8inmemory,
        int8inmemory

    }inmemorytypes_t;

    //! Enumeration for types that can be scaled to or from encodings
    typedef enum
    {
        longbitencoded,
        uint64encoded,
        int64encoded,
        uint56encoded,
        int56encoded,
        uint48encoded,
        int48encoded,
        uint40encoded,
        int40encoded,
        bitencoded,
        uint32encoded,
        int32encoded,
        uint24encoded,
        int24encoded,
        uint16encoded,
        int16encoded,
        uint8encoded,
        int8encoded,

    }encodedtypes_t;

    //! Determine if type is signed
    bool isTypeSigned(inmemorytypes_t type) const;

    //! Determine if type is signed
    bool isTypeSigned(encodedtypes_t type) const;

    //! Determine if type is floating point
    bool isTypeFloating(inmemorytypes_t type) const;

    //! Determine if type is floating point
    bool isTypeFloating(encodedtypes_t type) const;

    //! Determine if type is bitfield
    bool isTypeBitfield(inmemorytypes_t type) const;

    //! Determine if type is bitfield
    bool isTypeBitfield(encodedtypes_t type) const;

    //! Convert type to signed equivalent
    inmemorytypes_t convertTypeToSigned(inmemorytypes_t type) const;

    //! Convert type to signed equivalent
    encodedtypes_t convertTypeToSigned(encodedtypes_t type) const;

    //! Convert type to unsigned equivalent
    inmemorytypes_t convertTypeToUnsigned(inmemorytypes_t type) const;

    //! Convert type to unsigned equivalent
    encodedtypes_t convertTypeToUnsigned(encodedtypes_t type) const;

    //! Determine the encoded length of type
    int typeLength(inmemorytypes_t type) const;

    //! Determine the encoded length of type
    int typeLength(encodedtypes_t type) const;

    //! Create an in-memory type
    inmemorytypes_t createInMemoryType(bool issigned, bool isfloat, int length) const;

    //! Create an encoded type
    encodedtypes_t createEncodedType(bool issigned, int length) const;

    //! Return the name in code of this type
    QString typeName(inmemorytypes_t type) const;

    //! Return the name in code of this type
    QString typeName(encodedtypes_t type) const;

    //! Return the name in function signature of this type
    QString typeSigName(inmemorytypes_t type) const;

    //! Return the name in function signature of this type
    QString typeSigName(encodedtypes_t type) const;

    //! Determine if type is supported by this protocol
    bool isTypeSupported(inmemorytypes_t type) const;

    //! Determine if type is supported by this protocol
    bool isTypeSupported(encodedtypes_t type) const;

    //! Determine if both types are supported by this protocol
    bool areTypesSupported(inmemorytypes_t source, encodedtypes_t encoded) const;

    //! Generate the encode header file
    bool generateEncodeHeader(void);

    //! Generate the encode source file
    bool generateEncodeSource(void);

    //! Generate the one line brief comment for the encode function
    QString briefEncodeComment(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Generate the full comment for the encode function
    QString fullEncodeComment(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Generate the encode function signature
    QString encodeSignature(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Generate the full encode function
    QString fullEncodeFunction(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Generate the full encode function for integer scaling
    QString fullBitfieldEncodeFunction(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Generate the full encode function for floating point scaling
    QString fullFloatEncodeFunction(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const ;

    //! Generate the full encode function for integer scaling
    QString fullIntegerEncodeFunction(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Generate the decode header file
    bool generateDecodeHeader(void);

    //! Generate the decode source file
    bool generateDecodeSource(void);

    //! Generate the one line brief comment for the decode function
    QString briefDecodeComment(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Generate the full comment for the decode function
    QString fullDecodeComment(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Generate the decode function signature
    QString decodeSignature(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Generate the full decode function
    QString fullDecodeFunction(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Generate the full decode function for integer scaling
    QString fullBitfieldDecodeFunction(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Generate the full decode function for floating point scaling
    QString fullFloatDecodeFunction(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const ;

    //! Generate the full decode function for integer scaling
    QString fullIntegerDecodeFunction(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Header file output object
    ProtocolHeaderFile header;

    //! Source file output object
    ProtocolSourceFile source;

    //! Whats supported by the protocol
    ProtocolSupport support;
};

#endif // PROTOCOLSCALING_H
