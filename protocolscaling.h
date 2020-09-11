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
#include <string>

class ProtocolScaling
{
    // friend class ScalingInterface;

public:
    //! Construct the protocol scaling object
    ProtocolScaling(ProtocolSupport sup);

    //! Perform the generation, writing out the files
    bool generate(std::vector<std::string>& fileNameList, std::vector<std::string>& filePathList);

protected:

    //! Header file output object
    ProtocolHeaderFile header;

    //! Source file output object
    ProtocolSourceFile source;

    //! Whats supported by the protocol
    ProtocolSupport support;
};


class ScalingInterface
{

public:

    ScalingInterface(const ProtocolSupport &sup);

    virtual ~ScalingInterface() {;}

    //! Generate the encode header file
    virtual bool generateEncodeHeader(ProtocolHeaderFile &header) = 0;

    //! Generate the encode source file
    virtual bool generateEncodeSource(ProtocolSourceFile &source) = 0;

    //! Generate the decode header file
    virtual bool generateDecodeHeader(ProtocolHeaderFile &header) = 0;

    //! Generate the decode source file
    virtual bool generateDecodeSource(ProtocolSourceFile &source) = 0;

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
    std::string typeName(inmemorytypes_t type) const;

    //! Return the name in code of this type
    std::string typeName(encodedtypes_t type) const;

    //! Return the name in function signature of this type
    std::string typeSigName(inmemorytypes_t type) const;

    //! Return the name in function signature of this type
    std::string typeSigName(encodedtypes_t type) const;

    //! Return the name in function signature of this type
    std::string ctypeName(encodedtypes_t type) const;

    //! Determine if type is supported by this protocol
    bool isTypeSupported(inmemorytypes_t type) const;

    //! Determine if type is supported by this protocol
    bool isTypeSupported(encodedtypes_t type) const;

    //! Determine if both types are supported by this protocol
    bool areTypesSupported(inmemorytypes_t source, encodedtypes_t encoded) const;

    //! Key information
    const ProtocolSupport &support;


};


class CandCppScaledCoding : public ScalingInterface
{
public:

    CandCppScaledCoding(const ProtocolSupport &sup) : ScalingInterface(sup) {;}

    //! Generate the encode header file
    bool generateEncodeHeader(ProtocolHeaderFile &header);

    //! Generate the encode source file
    bool generateEncodeSource(ProtocolSourceFile &source);

    //! Generate the decode header file
    bool generateDecodeHeader(ProtocolHeaderFile &header);

    //! Generate the decode source file
    bool generateDecodeSource(ProtocolSourceFile &source);

protected:

    //! Generate the one line brief comment for the encode function
    std::string briefEncodeComment(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Generate the full comment for the encode function
    std::string fullEncodeComment(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Generate the encode function signature
    std::string encodeSignature(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Generate the full encode function
    std::string fullEncodeFunction(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Generate the full encode function for integer scaling
    std::string fullBitfieldEncodeFunction(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Generate the full encode function for floating point scaling
    std::string fullFloatEncodeFunction(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const ;

    //! Generate the full encode function for integer scaling
    std::string fullIntegerEncodeFunction(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Generate the one line brief comment for the decode function
    std::string briefDecodeComment(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Generate the full comment for the decode function
    std::string fullDecodeComment(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Generate the decode function signature
    std::string decodeSignature(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Generate the full decode function
    std::string fullDecodeFunction(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

};



class PythonScaledCoding : public ScalingInterface
{
public:

    PythonScaledCoding(const ProtocolSupport &sup) : ScalingInterface(sup) {;}

    //! Generate the encode header file
    bool generateEncodeHeader(ProtocolHeaderFile &header);

    //! Generate the encode source file
    bool generateEncodeSource(ProtocolSourceFile &source);

    //! Generate the decode header file
    bool generateDecodeHeader(ProtocolHeaderFile &header);

    //! Generate the decode source file
    bool generateDecodeSource(ProtocolSourceFile &source);

protected:
    //! Generate the one line brief comment for the encode function
    std::string briefEncodeComment(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Generate the full comment for the encode function
    std::string fullEncodeComment(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Generate the encode function signature
    std::string encodeSignature(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Generate the full encode function
    std::string fullEncodeFunction(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Find the max value for a type
    std::string findMax(encodedtypes_t encoded) const;

    std::string findMin(encodedtypes_t encoded) const;

    //! Generate the full encode function for integer scaling
    std::string fullBitfieldEncodeFunction(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Generate the full encode function for floating point scaling
    std::string fullFloatEncodeFunction(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const ;

    //! Generate the full encode function for integer scaling
    std::string fullIntegerEncodeFunction(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Generate the one line brief comment for the decode function
    std::string briefDecodeComment(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Generate the full comment for the decode function
    std::string fullDecodeComment(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Generate the decode function signature
    std::string decodeSignature(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;

    //! Generate the full decode function
    std::string fullDecodeFunction(inmemorytypes_t source, encodedtypes_t encoded, bool bigendian) const;


};


#endif // PROTOCOLSCALING_H
