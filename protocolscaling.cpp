#include "protocolscaling.h"
#include "protocolparser.h"

/// TODO: make scalers positive


//! Determine if type is signed
bool ProtocolScaling::isTypeSigned(inmemorytypes_t type) const
{
    switch(type)
    {
    default:
        return true;

    case uint64inmemory:
    case uint32inmemory:
    case uint16inmemory:
    case uint8inmemory:
        return false;
    }
}

//! Determine if type is signed
bool ProtocolScaling::isTypeSigned(encodedtypes_t type) const
{
    switch(type)
    {
    default:
        return true;

    case longbitencoded:
    case uint64encoded:
    case uint56encoded:
    case uint48encoded:
    case uint40encoded:
    case bitencoded:
    case uint32encoded:
    case uint24encoded:
    case uint16encoded:
    case uint8encoded:
        return false;
    }
}

//! Determine if type is floating point
bool ProtocolScaling::isTypeFloating(inmemorytypes_t type) const
{
    switch(type)
    {
    default:
        return false;

    case float64inmemory:
    case float32inmemory:
        return true;
    }
}

//! Determine if type is floating point
bool ProtocolScaling::isTypeFloating(encodedtypes_t type) const
{
    (void)type;
    return false;
}

//! Determine if type is bitfield
bool ProtocolScaling::isTypeBitfield(inmemorytypes_t type) const
{
    (void)type;
    return false;
}

//! Determine if type is bitfield
bool ProtocolScaling::isTypeBitfield(encodedtypes_t type) const
{
    switch(type)
    {
    default:
        return false;

    case longbitencoded:
    case bitencoded:
        return true;
    }
}

//! Convert type to signed equivalent
ProtocolScaling::inmemorytypes_t ProtocolScaling::convertTypeToSigned(inmemorytypes_t type) const
{
    switch(type)
    {
    default:           return type;
    case uint64inmemory: return int64inmemory;
    case uint32inmemory: return int32inmemory;
    case uint16inmemory: return int16inmemory;
    case uint8inmemory:  return int8inmemory;
    }
}

//! Convert type to signed equivalent
ProtocolScaling::encodedtypes_t ProtocolScaling::convertTypeToSigned(encodedtypes_t type) const
{
    switch(type)
    {
    default:          return type;
    case longbitencoded: return int64encoded;
    case uint64encoded:  return int64encoded;
    case uint56encoded:  return int56encoded;
    case uint48encoded:  return int48encoded;
    case uint40encoded:  return int40encoded;
    case bitencoded:     return int32encoded;
    case uint32encoded:  return int32encoded;
    case uint24encoded:  return int24encoded;
    case uint16encoded:  return int16encoded;
    case uint8encoded:   return int8encoded;
    }
}

//! Convert type to unsigned equivalent
ProtocolScaling::inmemorytypes_t ProtocolScaling::convertTypeToUnsigned(inmemorytypes_t type) const
{
    switch(type)
    {
    default:          return type;
    case int64inmemory: return uint64inmemory;
    case int32inmemory: return uint32inmemory;
    case int16inmemory: return uint16inmemory;
    case int8inmemory:  return uint8inmemory;
    }
}

//! Convert type to unsigned equivalent
ProtocolScaling::encodedtypes_t ProtocolScaling::convertTypeToUnsigned(encodedtypes_t type) const
{
    switch(type)
    {
    default:        return type;
    case int64encoded: return uint64encoded;
    case int56encoded: return uint56encoded;
    case int48encoded: return uint48encoded;
    case int40encoded: return uint40encoded;
    case int32encoded: return uint32encoded;
    case int24encoded: return uint24encoded;
    case int16encoded: return uint16encoded;
    case int8encoded:  return uint8encoded;
    }
}

//! Determine the encoded length of type
int ProtocolScaling::typeLength(inmemorytypes_t type) const
{
    switch(type)
    {
    default:
    case float64inmemory:
    case uint64inmemory:
    case int64inmemory:
        return 8;

    case float32inmemory:
    case uint32inmemory:
    case int32inmemory:
        return 4;

    case uint16inmemory:
    case int16inmemory:
        return 2;

    case uint8inmemory:
    case int8inmemory:
        return 1;
    }
}

//! Determine the encoded length of type
int ProtocolScaling::typeLength(encodedtypes_t type) const
{
    switch(type)
    {
    default:
    case uint64encoded:
    case int64encoded:
        return 8;

    case uint56encoded:
    case int56encoded:
        return 7;

    case uint48encoded:
    case int48encoded:
        return 6;

    case longbitencoded: // actual bitfield length is variable, it just has to be more than 32 bits
    case uint40encoded:
    case int40encoded:
        return 5;

    case uint32encoded:
    case int32encoded:
        return 4;

    case uint24encoded:
    case int24encoded:
        return 3;

    case uint16encoded:
    case int16encoded:
        return 2;

    case bitencoded:   // actual bitfield length is variable, it just has to be more than 0 bits
    case uint8encoded:
    case int8encoded:
        return 1;
    }
}


/*!
 * Create an in-memory type from discrete choices. Bitfield types not supported
 * \param issigned should be true to create a signed type
 * \param isfloat should be true to create a floating point type
 * \param length is the type length in bytes.
 * \return The in-memory type enumeration
 */
ProtocolScaling::inmemorytypes_t ProtocolScaling::createInMemoryType(bool issigned, bool isfloat, int length) const
{
    if(isfloat)
    {
        if((length > 4) && support.float64)
            return float64inmemory;
        else
            return float32inmemory;
    }
    else if(issigned)
    {
        if((length > 4) && support.int64)
            return int64inmemory;
        else if(length > 3)
            return int32inmemory;
        else if(length == 2)
            return int16inmemory;
        else
            return int8inmemory;
    }
    else
    {
        if((length > 4) && support.int64)
            return uint64inmemory;
        else if(length > 3)
            return uint32inmemory;
        else if(length == 2)
            return uint16inmemory;
        else
            return uint8inmemory;
    }

}// ProtocolScaling::createInMemoryType


/*!
 * Create an encoded type from discrete choices. Bitfield and float types not supported
 * \param issigned should be true to create a signed type
 * \param length is the type length in bytes.
 * \return The encoded type enumeration
 */
ProtocolScaling::encodedtypes_t ProtocolScaling::createEncodedType(bool issigned, int length) const
{
    if(issigned)
    {
        if((length >= 8) && support.int64)
            return int64encoded;
        if((length >= 7) && support.int64)
            return int56encoded;
        if((length >= 6) && support.int64)
            return int48encoded;
        if((length >= 5) && support.int64)
            return int40encoded;
        else if(length >= 4)
            return int32encoded;
        else if(length >= 3)
            return int24encoded;
        else if(length >= 2)
            return int16encoded;
        else
            return int8encoded;
    }
    else
    {
        if((length >= 8) && support.int64)
            return uint64encoded;
        if((length >= 7) && support.int64)
            return uint56encoded;
        if((length >= 6) && support.int64)
            return uint48encoded;
        if((length >= 5) && support.int64)
            return uint40encoded;
        else if(length >= 4)
            return uint32encoded;
        else if(length >= 3)
            return uint24encoded;
        else if(length >= 2)
            return uint16encoded;
        else
            return uint8encoded;
    }

}// ProtocolScaling::createEncodedType


//! Return the name in code of this type
std::string ProtocolScaling::typeName(inmemorytypes_t type) const
{
    switch(type)
    {
    default:              return "unknown";
    case float64inmemory: return "double";
    case uint64inmemory:  return "uint64_t";
    case int64inmemory:   return "int64_t";
    case float32inmemory: return "float";
    case uint32inmemory:  return "uint32_t";
    case int32inmemory:   return "int32_t";
    case uint16inmemory:  return "uint16_t";
    case int16inmemory:   return "int16_t";
    case uint8inmemory:   return "uint8_t";
    case int8inmemory:    return "int8_t";
    }
}

//! Return the name in code of this type
std::string ProtocolScaling::typeName(encodedtypes_t type) const
{
    switch(type)
    {
    default:
        return "unknown";

    case longbitencoded:
    case uint64encoded:
    case uint56encoded:
    case uint48encoded:
    case uint40encoded:
        return "uint64_t";

    case int64encoded:
    case int56encoded:
    case int48encoded:
    case int40encoded:
        return "int64_t";

    case bitencoded:
        return "unsigned int";

    case uint32encoded:
    case uint24encoded:
        return "uint32_t";

    case int32encoded:
    case int24encoded:
        return "int32_t";

    case uint16encoded:  return "uint16_t";
    case int16encoded:   return "int16_t";
    case uint8encoded:   return "uint8_t";
    case int8encoded:    return "int8_t";
    }
}

//! Return the name in function signature of this type
std::string ProtocolScaling::typeSigName(inmemorytypes_t type) const
{
    switch(type)
    {
    default:              return "unknown";
    case float64inmemory: return "float64";
    case uint64inmemory:  return "uint64";
    case int64inmemory:   return "int64";
    case float32inmemory: return "float32";
    case uint32inmemory:  return "uint32";
    case int32inmemory:   return "int32";
    case uint16inmemory:  return "uint16";
    case int16inmemory:   return "int16";
    case uint8inmemory:   return "uint8";
    case int8inmemory:    return "int8";
    }
}


//! Return the name in function signature of this type
std::string ProtocolScaling::typeSigName(encodedtypes_t type) const
{
    switch(type)
    {
    default:             return "unknown";
    case longbitencoded: return "longBitfield";
    case uint64encoded:  return "uint64";
    case int64encoded:   return "int64";
    case uint56encoded:  return "uint56";
    case int56encoded:   return "int56";
    case uint48encoded:  return "uint48";
    case int48encoded:   return "int48";
    case uint40encoded:  return "uint40";
    case int40encoded:   return "int40";
    case bitencoded:     return "bitfield";
    case uint32encoded:  return "uint32";
    case int32encoded:   return "int32";
    case uint24encoded:  return "uint24";
    case int24encoded:   return "int24";
    case uint16encoded:  return "uint16";
    case int16encoded:   return "int16";
    case uint8encoded:   return "uint8";
    case int8encoded:    return "int8";
    }
}

//! Determine if type is supported by this protocol
bool ProtocolScaling::isTypeSupported(inmemorytypes_t type) const
{
    switch(type)
    {
    default:
        return true;

    case float64inmemory:
        return support.float64;

    case uint64inmemory:
    case int64inmemory:
        return support.int64;
    }
}

//! Determine if type is supported by this protocol
bool ProtocolScaling::isTypeSupported(encodedtypes_t type) const
{
    switch(type)
    {
    default:
        return true;

    case longbitencoded:
        return support.bitfield && support.longbitfield && support.int64;

    case uint64encoded:
    case int64encoded:
    case uint56encoded:
    case int56encoded:
    case uint48encoded:
    case int48encoded:
    case uint40encoded:
    case int40encoded:
        return support.int64;

    case bitencoded:
        return support.bitfield;
    }
}


//! Determine if both types are supported by this protocol
bool ProtocolScaling::areTypesSupported(inmemorytypes_t inmemory, encodedtypes_t encoded) const
{
    return isTypeSupported(inmemory) && isTypeSupported(encoded);
}


/*!
 * Construct the protocol scaling object
 */
ProtocolScaling::ProtocolScaling(ProtocolSupport sup) :
    header(sup),
    source(sup),
    support(sup)
{
    /*
    // for testing
    support.int64 = false;
    support.bitfield = false;
    support.float64 = false;
    */
}


/*!
 * Generate the source and header files for protocol scaling
 * \param fileNameList is appended with the names of the generated files
 * \param filePathList is appended with the paths of the generated files
 * \return true if both modules are generated
 */
bool ProtocolScaling::generate(std::vector<std::string>& fileNameList, std::vector<std::string>& filePathList)
{
    if(generateEncodeHeader())
    {
        fileNameList.push_back(header.fileName());
        filePathList.push_back(header.filePath());
    }
    else
        return false;

    if(generateEncodeSource())
    {
        fileNameList.push_back(source.fileName());
        filePathList.push_back(source.filePath());
    }
    else
        return false;

    if(generateDecodeHeader())
    {
        fileNameList.push_back(header.fileName());
        filePathList.push_back(header.filePath());
    }
    else
        return false;

    if(generateDecodeSource())
    {
        fileNameList.push_back(source.fileName());
        filePathList.push_back(source.filePath());
    }
    else
        return false;

    return true;
}


/*!
 * Generate the header file for protocols caling
 * \return true if the file is generated.
 */
bool ProtocolScaling::generateEncodeHeader(void)
{
    header.setModuleNameAndPath("scaledencode", support.outputpath, support.language);

    header.defineStdC_Constant_Macros();

    // Top level comment
    std::string filecomment = R"(scaledencode routines place scaled numbers into a byte stream.

scaledencode routines place scaled values into a big or little endian byte
stream. The values can be any legitimate type (double, float, uint32_t,
uint16_t, uint8_t, int32_t, int16_t, int8_t), and are encoded as either a
unsigned or signed integer from 1 to 8 bytes in length. Unsigned encodings
allow the caller to specify a minimum and a maximum value, with the only
limitation that the maximum value must be more than the minimum. Signed
encodings only allow the caller to specify a maximum value which gives
maximum absolute value that can be encoded.

An example encoding would be: take a float that represents speed in meters
per second and encode it in two bytes from -200 to 200 meters per second.
In that example the encoding function would be:

float32ScaledTo2SignedBeBytes(speed, bytestream, &index, 200);

This would scale the speed according to (32767/200), and copy the resulting
two bytes to bytestream[index] as a signed 16 bit number in big endian
order. This would result in a velocity resolution of 0.006 m/s.

Another example encoding is: take a double that represents altitude in
meters and encode it in three bytes from -1000 to 49000 meters:

float64ScaledTo3UnsignedLeBytes(alt, bytestream, &index, -1000, 49000);

This would transform the altitude according to (alt *(16777215/50000) + 1000)
and copy the resulting three bytes to bytestream[index] as an unsigned 24
bit number in little endian order. This would result in an altitude
resolution of 0.003 meters.

scaledencode does not include routines that increase the resolution of the
inmemory value. For example the function floatScaledTo5UnsignedBeBytes() does
not exist, because expanding a float to 5 bytes does not make any resolution
improvement over encoding it in 4 bytes. In general the encoded format
must be equal to or less than the number of bytes of the raw data.)";

    // Document the protocol generation options
    filecomment += "\n\nCode generation for this module was affected by these global flags:\n\n";

    if(support.int64)
        filecomment += "- 64-bit integers are supported.\n\n";
    else
        filecomment += "- 64-bit integers are not supported.\n\n";

    if(support.bitfield && support.longbitfield && support.int64)
        filecomment += "- Normal and long bitfields are supported.\n\n";
    else if(support.bitfield)
        filecomment += "- Normal bitfields are supported, long bitfields are not.\n\n";
    else
        filecomment += "- Bitfields are not supported.\n\n";

    if(support.float64)
        filecomment += "- Double precision floating points are supported.\n\n";
    else
        filecomment += "- Double precision floating points are not supported.\n\n";

    header.setFileComment(filecomment);

    bool ifdefopened = false;

    // Iterate all inmemorys to all encodings.
    for(int i = (int)float64inmemory; i <= (int)int8inmemory; i++)
    {
        inmemorytypes_t inmemorytype = (inmemorytypes_t)i;
        for(int j = (int)longbitencoded; j <= (int)int8encoded; j++)
        {
            encodedtypes_t encodedtype = (encodedtypes_t)j;

            // Key concept: the encoded cannot be larger than the inmemory
            if(typeLength(encodedtype) > typeLength(inmemorytype))
                continue;

            // Key concept: the types must be supported in the protocol
            if(!areTypesSupported(inmemorytype, encodedtype))
                continue;

            // If the inmemory or encoded type requires 64-bit support we have
            // to protect it against compilers that cannot handle that
            if((ifdefopened == false) && ((typeLength(encodedtype) > 4) || (typeLength(inmemorytype) > 4)))
            {
                ifdefopened = true;
                header.write("\n#ifdef UINT64_MAX\n");
            }
            else if((ifdefopened == true) && (typeLength(encodedtype) <= 4) && (typeLength(inmemorytype) <= 4))
            {
                ifdefopened = false;
                header.write("\n#endif // UINT64_MAX\n");
            }

            // big endian
            header.write("\n");
            header.write("//! " + briefEncodeComment(inmemorytype, encodedtype, true) + "\n");
            header.write(encodeSignature(inmemorytype, encodedtype, true) + ";\n");

            // little endian
            if((typeLength(encodedtype) > 1) && !isTypeBitfield(encodedtype))
            {
                header.write("\n");
                header.write("//! " + briefEncodeComment(inmemorytype, encodedtype, false) + "\n");
                header.write(encodeSignature(inmemorytype, encodedtype, false) + ";\n");
            }

        }// for all encodeds

    }// for all inmemorys

    header.write("\n");

    if(ifdefopened)
        header.write("\n#endif // UINT64_MAX\n");

    return header.flush();

}// ProtocolScaling::generateEncodeHeader


/*!
 * Generate the source file for protocols caling
 * \return true if the file is generated.
 */
bool ProtocolScaling::generateEncodeSource(void)
{
    source.setModuleNameAndPath("scaledencode", support.outputpath, support.language);

    source.writeIncludeDirective("fieldencode");
    source.write("\n");

    bool ifdefopened = false;

    // Iterate all inmemorys to all encodings.
    for(int i = (int)float64inmemory; i <= (int)int8inmemory; i++)
    {
        inmemorytypes_t inmemorytype = (inmemorytypes_t)i;
        for(int j = (int)longbitencoded; j <= (int)int8encoded; j++)
        {
            encodedtypes_t encodedtype = (encodedtypes_t)j;

            // Key concept: the encoded cannot be larger than the inmemory
            if(typeLength(encodedtype) > typeLength(inmemorytype))
                continue;

            // Key concept: the types must be supported in the protocol
            if(!areTypesSupported(inmemorytype, encodedtype))
                continue;

            // If the inmemory or encoded type requires 64-bit support we have
            // to protect it against compilers that cannot handle that
            if((ifdefopened == false) && ((typeLength(encodedtype) > 4) || (typeLength(inmemorytype) > 4)))
            {
                ifdefopened = true;
                source.write("\n#ifdef UINT64_MAX\n");
            }
            else if((ifdefopened == true) && (typeLength(encodedtype) <= 4) && (typeLength(inmemorytype) <= 4))
            {
                ifdefopened = false;
                source.write("\n#endif // UINT64_MAX\n");
            }

            // big endian
            source.write("\n");
            source.write(fullEncodeComment(inmemorytype, encodedtype, true) + "\n");
            source.write(fullEncodeFunction(inmemorytype, encodedtype, true) + "\n");

            // little endian
            if((typeLength(encodedtype) > 1) && !isTypeBitfield(encodedtype))
            {
                source.write("\n");
                source.write(fullEncodeComment(inmemorytype, encodedtype, false) + "\n");
                source.write(fullEncodeFunction(inmemorytype, encodedtype, false) + "\n");
            }

        }// for all output byte counts

    }// for all input types

    source.write("\n");

    if(ifdefopened)
        source.write("\n#endif // UINT64_MAX\n");

    return source.flush();

}// ProtocolScaling::generateEncodeSource


/*!
 * Create the brief function comment, without doxygen decorations.
 * \param inmemory is the type information for the inmemory (in-memory) data.
 * \param encoded is the type information for the encoded (encoded) data.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return The string that represents the one line function comment.
 */
std::string ProtocolScaling::briefEncodeComment(inmemorytypes_t inmemory, encodedtypes_t encoded, bool bigendian) const
{
    std::string scalingtype;

    if(isTypeFloating(inmemory))
        scalingtype = "floating point";
    else
        scalingtype = "integer";

    if(isTypeBitfield(encoded))
    {
        if(typeLength(encoded) > 4)
            return std::string("Scale a " + typeName(inmemory) + " using " + scalingtype + " scaling to the base integer type used for long bitfields.");
        else
            return std::string("Scale a " + typeName(inmemory) + " using " + scalingtype + " scaling to the base integer type used for bitfields.");
    }
    else
    {
        if(typeLength(encoded) == 1)
        {
            // No endian concerns if using only 1 byte
            if(isTypeSigned(encoded))
                return std::string("Encode a " + typeName(inmemory) + " on a byte stream by " + scalingtype + " scaling to fit in 1 signed byte.");
            else
                return std::string("Encode a " + typeName(inmemory) + " on a byte stream by " + scalingtype + " scaling to fit in 1 unsigned byte.");
        }
        else
        {
            std::string byteLength = std::to_string(typeLength(encoded));
            std::string endian;

            if(bigendian)
                endian = "big";
            else
                endian = "little";

            if(isTypeSigned(encoded))
                return std::string("Encode a " + typeName(inmemory) + " on a byte stream by " + scalingtype + " scaling to fit in " + byteLength + " signed bytes in " + endian +" endian order.");
            else
                return std::string("Encode a " + typeName(inmemory) + " on a byte stream by " + scalingtype + " scaling to fit in " + byteLength + " unsigned bytes in " + endian + " endian order.");

        }// If multi-byte
    }

}// ProtocolScaling::briefEncodeComment


/*!
 * Create the full encode function comment, with doxygen decorations
 * \param inmemory is the type information for the inmemory (in-memory) data.
 * \param encoded is the type information for the encoded (encoded) data.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return The string that represents the full multi-line function comment.
 */
std::string ProtocolScaling::fullEncodeComment(inmemorytypes_t inmemory, encodedtypes_t encoded, bool bigendian) const
{
    std::string comment= ("/*!\n");

    if(isTypeBitfield(encoded))
    {
        comment += ProtocolParser::outputLongComment(" * ", briefEncodeComment(inmemory, encoded, bigendian)) + "\n";
        comment += " * \\param value is the number to scale.\n";
        comment += " * \\param min is the minimum value that can be encoded.\n";
        comment += " * \\param scaler is multiplied by value to create the encoded integer.\n";
        comment += " * \\param bits is the number of bits in the bitfield, used to limit the returned value.\n";
        comment += " * \\return (value-min)*scaler.\n";
    }
    else
    {
        comment += ProtocolParser::outputLongComment(" * ", briefEncodeComment(inmemory, encoded, bigendian)) + "\n";
        comment += " * \\param value is the number to encode.\n";
        comment += " * \\param bytes is a pointer to the byte stream which receives the encoded data.\n";
        comment += " * \\param index gives the location of the first byte in the byte stream, and\n";
        comment += " *        will be incremented by " + std::to_string(typeLength(encoded)) + " when this function is complete.\n";

        if(isTypeSigned(encoded))
            comment += " * \\param scaler is multiplied by value to create the encoded integer: encoded = value*scaler.\n";
        else
        {
            comment += " * \\param min is the minimum value that can be encoded.\n";
            comment += " * \\param scaler is multiplied by value to create the encoded integer: encoded = (value-min)*scaler.\n";
        }
    }

    comment += " */";

    return comment;

}// ProtocolScaling::fullEncodeComment


/*!
 * Create the one line function signature, without a trailing semicolon
 * \param inmemory is the type information for the inmemory (in-memory) data.
 * \param encoded is the type information for the encoded (encoded) data.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return The string that represents the function signature, without a trailing semicolon
 */
std::string ProtocolScaling::encodeSignature(inmemorytypes_t inmemory, encodedtypes_t encoded, bool bigendian) const
{
    if(isTypeBitfield(encoded))
    {
        if(typeLength(encoded) > 4)
            return std::string(typeName(encoded) + " " + typeSigName(inmemory) + "ScaledToLongBitfield(" + typeName(inmemory) + " value, " + typeName(convertTypeToSigned(inmemory)) + " min, " + typeName(convertTypeToUnsigned(inmemory)) + " scaler, int bits)");
        else
            return std::string(typeName(encoded) + " " + typeSigName(inmemory) + "ScaledToBitfield(" + typeName(inmemory) + " value, " + typeName(convertTypeToSigned(inmemory)) + " min, " + typeName(convertTypeToUnsigned(inmemory)) + " scaler, int bits)");
    }
    else if(typeLength(encoded) == 1)
    {
        // No endian concerns if using only 1 byte
        if(isTypeSigned(encoded))
            return std::string("void " + typeSigName(inmemory) + "ScaledTo1SignedBytes(" + typeName(inmemory) + " value, uint8_t* bytes, int* index, " + typeName(convertTypeToUnsigned(inmemory)) + " scaler)");
        else
            return std::string("void " + typeSigName(inmemory) + "ScaledTo1UnsignedBytes(" + typeName(inmemory) + " value, uint8_t* bytes, int* index, " + typeName(convertTypeToSigned(inmemory)) + " min, " + typeName(convertTypeToUnsigned(inmemory)) + " scaler)");
    }
    else
    {
        std::string byteLength = std::to_string(typeLength(encoded));
        std::string endian;

        if(bigendian)
            endian = "Be";
        else
            endian = "Le";

        if(isTypeSigned(encoded))
            return std::string("void " + typeSigName(inmemory) + "ScaledTo" + byteLength + "Signed" + endian + "Bytes(" + typeName(inmemory) + " value, uint8_t* bytes, int* index, " + typeName(convertTypeToUnsigned(inmemory)) + " scaler)");
        else
            return std::string("void " + typeSigName(inmemory) + "ScaledTo" + byteLength + "Unsigned" + endian + "Bytes(" + typeName(inmemory) + " value, uint8_t* bytes, int* index, " + typeName(convertTypeToSigned(inmemory)) + " min, " + typeName(convertTypeToUnsigned(inmemory)) + " scaler)");

    }// If multi-byte

}// ProtocolScaling::encodeSignature


/*!
 * Generate the full function output, excluding the comment
 * \param inmemory is the type information for the inmemory (in-memory) data.
 * \param encoded is the type information for the encoded (encoded) data.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return the function as a string
 */
std::string ProtocolScaling::fullEncodeFunction(inmemorytypes_t inmemory, encodedtypes_t encoded, bool bigendian) const
{
    if(isTypeBitfield(encoded))
        return fullBitfieldEncodeFunction(inmemory, encoded, bigendian);
    else if(isTypeFloating(inmemory))
        return fullFloatEncodeFunction(inmemory, encoded, bigendian);
    else
        return fullIntegerEncodeFunction(inmemory, encoded, bigendian);
}


/*!
 * Generate the full bitfield scaling function output, excluding the comment
 * \param inmemory is the type information for the inmemory (in-memory) data.
 * \param encoded is the type information for the encoded (encoded) data.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return the function as a string
 */
std::string ProtocolScaling::fullBitfieldEncodeFunction(inmemorytypes_t inmemory, encodedtypes_t encoded, bool bigendian) const
{
    std::string constantone;

    if(typeLength(encoded) > 4)
        constantone = "0x1ull";
    else if(typeLength(encoded) > 2)
        constantone = "0x1ul";
    else
        constantone = "0x1u";

    std::string function = encodeSignature(inmemory, encoded, bigendian) + "\n";
    function += "{\n";
    function += "    // The largest integer the bitfield can hold\n";
    function += "    " + typeName(encoded) + " max = (" + constantone + " << bits) - 1;\n";
    function += "\n";

    if(isTypeFloating(inmemory))
    {
        function += "    // Protect from underflow\n";
        function += "    if(value < min)\n";
        function += "        return 0;\n";
        function += "\n";
        function += "    // Scale the number\n";
        function += "    value = (value - min)*scaler;\n";
        function += "\n";
        function += "    // Protect from overflow\n";
        function += "    if(value > max)\n";
        function += "        return max;\n";
        function += "\n";

        if(typeLength(inmemory) > 4)
        {
            function += "    // Account for fractional truncation\n";
            function += "    return (" + typeName(encoded) + ")(value + 0.5);\n";

        }// if inmemory data is double precision floating point
        else
        {
            function += "    // Account for fractional truncation\n";
            function += "    return (" + typeName(encoded) + ")(value + 0.5f);\n";

        }// else if inmemory data is single precision floating point

    }// If in-memory type is floating point
    else
    {
        function += "    // Scale the number\n";
        function += "    " + typeName(encoded) + " number = (" + typeName(encoded) + ")((value - min)*scaler);\n";
        function += "\n";
        function += "    // Protect from underflow\n";
        function += "    if(((" + typeName(convertTypeToSigned(encoded)) + ")value) < min)\n";
        function += "        return 0;\n";
        function += "\n";
        function += "    // Protect from overflow\n";
        function += "    if(number > max)\n";
        function += "        return max;\n";
        function += "\n";
        function += "    return number;\n";

    }// else if inmemory type is integer

    function += "}\n";

    return function;

}// ProtocolScaling::fullBitfieldEncodeFunction


/*!
 * Generate the full floating point scaling function output, excluding the comment
 * \param inmemory is the type information for the inmemory (in-memory) data.
 * \param encoded is the type information for the encoded (encoded) data.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return the function as a string
 */
std::string ProtocolScaling::fullFloatEncodeFunction(inmemorytypes_t inmemory, encodedtypes_t encoded, bool bigendian) const
{
    std::string function = encodeSignature(inmemory, encoded, bigendian) + "\n";

    std::string endian;
    if(typeLength(encoded) > 1)
    {
        if(bigendian)
            endian = "Be";
        else
            endian = "Le";
    }

    std::string bitCount = std::to_string(typeLength(encoded)*8);

    std::string halfFraction;
    if(typeLength(inmemory) > 4)
        halfFraction = "0.5";
    else
        halfFraction = "0.5f";

    function += "{\n";
    function += "    // scale the number\n";

    if(isTypeSigned(encoded))
    {
        std::string intmax;
        std::string intmin;
        std::string floatmax;
        std::string floatmin;

        switch(typeLength(encoded))
        {
        default:
        case 1: intmax = "127"; break;
        case 2: intmax = "32767"; break;
        case 3: intmax = "8388607"; break;
        case 4: intmax = "2147483647l"; break;
        case 5: intmax = "549755813887ll"; break;
        case 6: intmax = "140737488355327ll"; break;
        case 7: intmax = "36028797018963967ll"; break;
        case 8: intmax = "9223372036854775807ll"; break;
        }

        switch(typeLength(encoded))
        {
        default:
        case 1: intmin = "(-127 - 1)"; break;
        case 2: intmin = "(-32767 - 1)"; break;
        case 3: intmin = "(-8388607l - 1)"; break;
        case 4: intmin = "(-2147483647l - 1)"; break;
        case 5: intmin = "(-549755813887ll - 1)"; break;
        case 6: intmin = "(-140737488355327ll - 1)"; break;
        case 7: intmin = "(-36028797018963967ll - 1)"; break;
        case 8: intmin = "(-9223372036854775807ll - 1)"; break;
        }

        switch(typeLength(encoded))
        {
        default:
        case 1: floatmax = "127.0"; break;
        case 2: floatmax = "32767.0"; break;
        case 3: floatmax = "8388607.0"; break;
        case 4: floatmax = "2147483647.0"; break;
        case 5: floatmax = "549755813887.0"; break;
        case 6: floatmax = "140737488355327.0"; break;
        case 7: floatmax = "36028797018963967.0"; break;
        case 8: floatmax = "9223372036854775807.0"; break;
        }

        switch(typeLength(encoded))
        {
        default:
        case 1: floatmin = "-128.0"; break;
        case 2: floatmin = "-32768.0"; break;
        case 3: floatmin = "-8388608.0"; break;
        case 4: floatmin = "-2147483648.0"; break;
        case 5: floatmin = "-549755813888.0"; break;
        case 6: floatmin = "-140737488355328.0"; break;
        case 7: floatmin = "-36028797018963968.0"; break;
        case 8: floatmin = "-9223372036854775808.0"; break;
        }

        if(typeLength(inmemory) <= 4)
        {
            floatmin += "f";
            floatmax += "f";
        }

        function += "    " + typeName(inmemory) + " scaledvalue = (" + typeName(inmemory) + ")(value*scaler);\n";
        function += "    " + typeName(encoded) + " number;\n";
        function += "\n";
        function += "    // Make sure number fits in the range\n";
        function += "    if(scaledvalue >= 0)\n";
        function += "    {\n";
        function += "        if(scaledvalue >= " + floatmax + ")\n";
        function += "            number = " + intmax + ";\n";
        function += "        else\n";
        function += "            number = (" + typeName(encoded) + ")(scaledvalue + " + halfFraction + "); // account for fractional truncation\n";
        function += "    }\n";
        function += "    else\n";
        function += "    {\n";
        function += "        if(scaledvalue <= " + floatmin + ")\n";
        function += "            number = " + intmin + ";\n";
        function += "        else\n";
        function += "            number = (" + typeName(encoded) + ")(scaledvalue - " + halfFraction + "); // account for fractional truncation\n";
        function += "    }\n";
        function += "\n";
        function += "    int" + bitCount + "To" + endian + "Bytes" + "(number, bytes, index);\n";
    }
    else
    {
        std::string intmax;
        std::string floatmax;

        switch(typeLength(encoded))
        {
        default:
        case 1: intmax = "255u"; break;
        case 2: intmax = "65535u"; break;
        case 3: intmax = "16777215ul"; break;
        case 4: intmax = "4294967295ul"; break;
        case 5: intmax = "1099511627775ull"; break;
        case 6: intmax = "281474976710655ull"; break;
        case 7: intmax = "72057594037927935ull"; break;
        case 8: intmax = "18446744073709551615ull"; break;
        }

        switch(typeLength(encoded))
        {
        default:
        case 1: floatmax = "255.0"; break;
        case 2: floatmax = "65535.0"; break;
        case 3: floatmax = "16777215.0"; break;
        case 4: floatmax = "4294967295.0"; break;
        case 5: floatmax = "1099511627775.0"; break;
        case 6: floatmax = "281474976710655.0"; break;
        case 7: floatmax = "72057594037927935.0"; break;
        case 8: floatmax = "18446744073709551615.0"; break;
        }

        if(typeLength(inmemory) <= 4)
        {
            floatmax += "f";
        }

        function += "    " + typeName(inmemory) + " scaledvalue = (" + typeName(inmemory) + ")((value - min)*scaler);\n";
        function += "    " + typeName(encoded) + " number;\n";
        function += "\n";
        function += "    // Make sure number fits in the range\n";
        function += "    if(scaledvalue >= " + floatmax + ")\n";
        function += "        number = " + intmax + ";\n";
        function += "    else if(scaledvalue <= 0)\n";
        function += "        number = 0;\n";
        function += "    else\n";
        function += "        number = (" + typeName(encoded) + ")(scaledvalue + " + halfFraction + "); // account for fractional truncation\n";
        function += "\n";
        function += "    uint" + bitCount + "To" + endian + "Bytes" + "(number, bytes, index);\n";
    }

    function += ("}\n");

    return function;

}// ProtocolScaling::fullFloatEncodeFunction


/*!
 * Generate the full integer scaling function output, excluding the comment
 * \param inmemory is the type information for the inmemory (in-memory) data.
 * \param encoded is the type information for the encoded (encoded) data.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return the function as a string
 */
std::string ProtocolScaling::fullIntegerEncodeFunction(inmemorytypes_t inmemory, encodedtypes_t encoded, bool bigendian) const
{
    std::string function = encodeSignature(inmemory, encoded, bigendian) + "\n";

    std::string endian;
    if(typeLength(encoded) > 1)
    {
        if(bigendian)
            endian = "Be";
        else
            endian = "Le";
    }

    std::string bitCount = std::to_string(typeLength(encoded)*8);

    function += "{\n";
    function += "    // scale the number\n";

    // We need a local type which has the same sign as the encoding, but uses the longer length of the encoding or in-memory
    int longestlength = typeLength(inmemory);
    if(typeLength(encoded) > longestlength)
        longestlength = typeLength(encoded);

    inmemorytypes_t local = createInMemoryType(isTypeSigned(encoded), false, longestlength);

    if(isTypeSigned(encoded))
    {
        std::string max;
        std::string min;
        switch(typeLength(encoded))
        {
        default:
        case 1: max = "127"; break;
        case 2: max = "32767"; break;
        case 3: max = "8388607l"; break;
        case 4: max = "2147483647l"; break;
        case 5: max = "549755813887ll"; break;
        case 6: max = "140737488355327ll"; break;
        case 7: max = "36028797018963967ll"; break;
        case 8: max = "9223372036854775807ll"; break;
        }

        switch(typeLength(encoded))
        {
        default:
        case 1: min = "(-127 - 1)"; break;
        case 2: min = "(-32767 - 1)"; break;
        case 3: min = "(-8388607l - 1)"; break;
        case 4: min = "(-2147483647l - 1)"; break;
        case 5: min = "(-549755813887ll - 1)"; break;
        case 6: min = "(-140737488355327ll - 1)"; break;
        case 7: min = "(-36028797018963967ll - 1)"; break;
        case 8: min = "(-9223372036854775807ll - 1)"; break;
        }

        function += "    " + typeName(local) + " number = (" + typeName(local) + ")(value*scaler);\n";
        function += "\n";

        // We don't need to test the range of the local number if it is the
        // same length as the encoding - by definition it cannot exceed the range
        if(typeLength(local) > typeLength(encoded))
        {
            function += "    // Make sure number fits in the range\n";
            function += "    if(number > " + max + ")\n";
            function += "        number = " + max + ";\n";
            if(isTypeSigned(local))
            {
                function += "    else if(number < " + min + ")\n";
                function += "        number = " + min + ";\n";
            }
            function += "\n";
        }

        // If the local type is the same or smaller size, and the same sign, we don't need a cast
        if((isTypeSigned(local) != isTypeSigned(encoded)) || (typeLength(local) > typeLength(encoded)))
            function += "    int" + bitCount + "To" + endian + "Bytes" + "((" + typeName(encoded)+ ")number, bytes, index);\n";
        else
            function += "    int" + bitCount + "To" + endian + "Bytes" + "(number, bytes, index);\n";
    }
    else
    {
        std::string max;

        switch(typeLength(encoded))
        {
        default:
        case 1: max = "255u"; break;
        case 2: max = "65535u"; break;
        case 3: max = "16777215ul"; break;
        case 4: max = "4294967295ul"; break;
        case 5: max = "1099511627775ull"; break;
        case 6: max = "281474976710655ull"; break;
        case 7: max = "72057594037927935ull"; break;
        case 8: max = "18446744073709551615ull"; break;
        }

        function += "    " + typeName(local) + " number = 0;\n";
        function += "\n";
        function += "    // Make sure number fits in the range\n";

        // The min value is signed, if the local value is unsigned then we need a cast to avoid the warning
        if(isTypeSigned(local))
            function += "    if(value > min)\n";
        else
            function += "    if(((" + typeName(convertTypeToSigned(local)) + ")value) > min)\n";

        function += "    {\n";
        function += "        number = (" + typeName(local) + ")((value - min)*scaler);\n";

        // We don't need to test the range of the local number if it is the
        // same length as the encoding - by definition it cannot exceed the range
        if(typeLength(local) > typeLength(encoded))
        {
            function += "        if(number > " + max + ")\n";
            function += "            number = " + max + ";\n";
        }

        function += "    }\n";
        function += "\n";

        // If the local type is the same or smaller size, and the same sign, we don't need a cast
        if((isTypeSigned(local) != isTypeSigned(encoded)) || (typeLength(local) > typeLength(encoded)))
            function += "    uint" + bitCount + "To" + endian + "Bytes" + "((" + typeName(encoded)+ ")number, bytes, index);\n";
        else
            function += "    uint" + bitCount + "To" + endian + "Bytes" + "(number, bytes, index);\n";
    }

    function += ("}\n");

    return function;

}// ProtocolScaling::fullIntegerEncodeFunction


/*!
 * Generate the header file for protocols caling
 * \return true if the file is generated.
 */
bool ProtocolScaling::generateDecodeHeader(void)
{
    header.setModuleNameAndPath("scaleddecode", support.outputpath, support.language);

    header.defineStdC_Constant_Macros();

    // Top level comment
    std::string filecomment = R"(scaleddecode routines extract scaled numbers from a byte stream.

scaleddecode routines extract scaled numbers from a byte stream. The routines
in this module are the reverse operation of the routines in scaledencode.)";

    // Document the protocol generation options
    filecomment += "\n\nCode generation for this module was affected by these global flags:\n\n";

    if(support.int64)
        filecomment += "- 64-bit integers are supported.\n\n";
    else
        filecomment += "- 64-bit integers are not supported.\n\n";

    if(support.bitfield && support.longbitfield && support.int64)
        filecomment += "- Normal and long bitfields are supported.\n\n";
    else if(support.bitfield)
        filecomment += "- Normal bitfields are supported, long bitfields are not.\n\n";
    else
        filecomment += "- Bitfields are not supported.\n\n";

    if(support.float64)
        filecomment += "- Double precision floating points are supported.\n\n";
    else
        filecomment += "- Double precision floating points are not supported.\n\n";

    header.setFileComment(filecomment);

    bool ifdefopened = false;

    // Iterate all inmemorys to all encodings.
    for(int i = (int)float64inmemory; i <= (int)int8inmemory; i++)
    {
        inmemorytypes_t inmemorytype = (inmemorytypes_t)i;
        for(int j = (int)longbitencoded; j <= (int)int8encoded; j++)
        {
            encodedtypes_t encodedtype = (encodedtypes_t)j;

            // Key concept: the encoded cannot be larger than the inmemory
            if(typeLength(encodedtype) > typeLength(inmemorytype))
                continue;

            // Key concept: the types must be supported in the protocol
            if(!areTypesSupported(inmemorytype, encodedtype))
                continue;

            // If the inmemory or encoded type requires 64-bit support we have
            // to protect it against compilers that cannot handle that
            if((ifdefopened == false) && ((typeLength(encodedtype) > 4) || (typeLength(inmemorytype) > 4)))
            {
                ifdefopened = true;
                header.write("\n#ifdef UINT64_MAX\n");
            }
            else if((ifdefopened == true) && (typeLength(encodedtype) <= 4) && (typeLength(inmemorytype) <= 4))
            {
                ifdefopened = false;
                header.write("\n#endif // UINT64_MAX\n");
            }

            // big endian
            header.write("\n");
            header.write("//! " + briefDecodeComment(inmemorytype, encodedtype, true) + "\n");
            header.write(decodeSignature(inmemorytype, encodedtype, true) + ";\n");

            // little endian
            if((typeLength(encodedtype) > 1) && !isTypeBitfield(encodedtype))
            {
                header.write("\n");
                header.write("//! " + briefDecodeComment(inmemorytype, encodedtype, false) + "\n");
                header.write(decodeSignature(inmemorytype, encodedtype, false) + ";\n");
            }

        }// for all encodeds

    }// for all inmemorys

    header.write("\n");

    if(ifdefopened)
        header.write("\n#endif // UINT64_MAX\n");

    return header.flush();

}// ProtocolScaling::generateDecodeHeader


/*!
 * Generate the source file for protocols caling
 * \return true if the file is generated.
 */
bool ProtocolScaling::generateDecodeSource(void)
{
    source.setModuleNameAndPath("scaleddecode", support.outputpath, support.language);

    source.writeIncludeDirective("fielddecode");
    source.write("\n");

    bool ifdefopened = false;

    // Iterate all inmemorys to all encodings.
    for(int i = (int)float64inmemory; i <= (int)int8inmemory; i++)
    {
        inmemorytypes_t inmemorytype = (inmemorytypes_t)i;
        for(int j = (int)longbitencoded; j <= (int)int8encoded; j++)
        {
            encodedtypes_t encodedtype = (encodedtypes_t)j;

            // Key concept: the encoded cannot be larger than the inmemory
            if(typeLength(encodedtype) > typeLength(inmemorytype))
                continue;

            // Key concept: the types must be supported in the protocol
            if(!areTypesSupported(inmemorytype, encodedtype))
                continue;

            // If the inmemory or encoded type requires 64-bit support we have
            // to protect it against compilers that cannot handle that
            if((ifdefopened == false) && ((typeLength(encodedtype) > 4) || (typeLength(inmemorytype) > 4)))
            {
                ifdefopened = true;
                source.write("\n#ifdef UINT64_MAX\n");
            }
            else if((ifdefopened == true) && (typeLength(encodedtype) <= 4) && (typeLength(inmemorytype) <= 4))
            {
                ifdefopened = false;
                source.write("\n#endif // UINT64_MAX\n");
            }

            // big endian
            source.write("\n");
            source.write(fullDecodeComment(inmemorytype, encodedtype, true) + "\n");
            source.write(fullDecodeFunction(inmemorytype, encodedtype, true) + "\n");

            // little endian
            if((typeLength(encodedtype) > 1) && !isTypeBitfield(encodedtype))
            {
                source.write("\n");
                source.write(fullDecodeComment(inmemorytype, encodedtype, false) + "\n");
                source.write(fullDecodeFunction(inmemorytype, encodedtype, false) + "\n");
            }

        }// for all output byte counts

    }// for all input types

    source.write("\n");

    if(ifdefopened)
        source.write("\n#endif // UINT64_MAX\n");

    return source.flush();

}// ProtocolScaling::generateDecodeSource


/*!
 * Create the brief decode function comment, without doxygen decorations
 * \param inmemory is the type information for the inmemory (in-memory) data.
 * \param encoded is the type information for the encoded (encoded) data.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return The string that represents the one line function comment.
 */
std::string ProtocolScaling::briefDecodeComment(inmemorytypes_t inmemory, encodedtypes_t encoded, bool bigendian) const
{
    std::string scalingtype;

    if(isTypeFloating(inmemory))
        scalingtype = "floating point";
    else
        scalingtype = "integer";

    if(isTypeBitfield(encoded))
    {
        if(typeLength(encoded) > 4)
            return std::string("Compute a " + typeName(inmemory) + " using invese " + scalingtype + " scaling from the base integer type used for long bitfields.");
        else
            return std::string("Compute a " + typeName(inmemory) + " using inverse " + scalingtype + " scaling from the base integer type used for bitfields.");
    }
    else
    {
        if(typeLength(encoded) == 1)
        {
            // No endian concerns if using only 1 byte
            if(isTypeSigned(encoded))
                return std::string("Decode a " + typeName(inmemory) + " from a byte stream by inverse " + scalingtype + " scaling from 1 signed byte.");
            else
                return std::string("Decode a " + typeName(inmemory) + " from a byte stream by inverse " + scalingtype + " scaling from 1 unsigned byte.");
        }
        else
        {
            std::string byteLength = std::to_string(typeLength(encoded));
            std::string endian;

            if(bigendian)
                endian = "big";
            else
                endian = "little";

            if(isTypeSigned(encoded))
                return std::string("Decode a " + typeName(inmemory) + " from a byte stream by inverse " + scalingtype + " scaling from " + byteLength + " signed bytes in " + endian +" endian order.");
            else
                return std::string("Decode a " + typeName(inmemory) + " from a byte stream by inverse " + scalingtype + " scaling from " + byteLength + " unsigned bytes in " + endian + " endian order.");

        }// If multi-byte
    }

}// ProtocolScaling::briefDecodeComment


/*!
 * Create the full decode function comment, with doxygen decorations
 * \param inmemory is the type information for the inmemory (in-memory) data.
 * \param encoded is the type information for the encoded (encoded) data.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return The string that represents the full multi-line function comment.
 */
std::string ProtocolScaling::fullDecodeComment(inmemorytypes_t inmemory, encodedtypes_t encoded, bool bigendian) const
{
    std::string comment= ("/*!\n");

    if(isTypeBitfield(encoded))
    {
        comment += ProtocolParser::outputLongComment(" * ", briefDecodeComment(inmemory, encoded, bigendian)) + "\n";
        comment += " * \\param value is the integer bitfield number to inverse scale\n";
        comment += " * \\param min is the minimum value that can be represented.\n";

        if(isTypeFloating(inmemory))
        {
            comment += " * \\param invscaler is multiplied by the integer to create the return value.\n";
            comment += " *        invscaler should be the inverse of the scaler given to the scaling function.\n";
            comment += " * \\return the correctly scaled decoded value: return = min + value*invscaler.\n";
        }
        else
        {
            comment += " * \\param divisor is divided into the encoded integer to create the return value.\n";
            comment += " * \\return the correctly scaled decoded value: return = min + encoded/divisor.\n";
        }
    }
    else
    {
        comment += ProtocolParser::outputLongComment(" * ", briefDecodeComment(inmemory, encoded, bigendian)) + "\n";
        comment += " * \\param bytes is a pointer to the byte stream to decode.\n";
        comment += " * \\param index gives the location of the first byte in the byte stream, and\n";
        comment += " *        will be incremented by " + std::to_string(typeLength(encoded)) + " when this function is complete.\n";

        if(isTypeFloating(inmemory))
        {
            if(!isTypeSigned(encoded))
            {
                comment += " * \\param invscaler is multiplied by the decoded integer to create the return value.\n";
                comment += " *        invscaler should be the inverse of the scaler given to the encode function.\n";
                comment += " * \\return the correctly scaled decoded value: return = encoded*invscaler.\n";
            }
            else
            {
                comment += " * \\param min is the minimum value that can be decoded.\n";
                comment += " * \\param invscaler is multiplied by the decoded integer to create the return value.\n";
                comment += " *        invscaler should be the inverse of the scaler given to the encode function.\n";
                comment += " * \\return the correctly scaled decoded value: return = min + encoded*invscaler.\n";
            }
        }
        else
        {
            if(isTypeSigned(encoded))
            {
                comment += " * \\param divisor is divided into the encoded integer to create the return value.\n";
                comment += " * \\return the correctly scaled decoded value: return = encoded/divisor.\n";
            }
            else
            {
                comment += " * \\param min is the minimum value that can be decoded.\n";
                comment += " * \\param divisor is divided into the encoded integer to create the return value.\n";
                comment += " * \\return the correctly scaled decoded value: return = min + encoded/divisor.\n";
            }
        }
    }

    comment += " */";

    return comment;

}// ProtocolScaling::fullDecodeComment


/*!
 * Create the one line decode function signature, without a trailing semicolon
 * \param inmemory is the type information for the inmemory (in-memory) data.
 * \param encoded is the type information for the encoded (encoded) data.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return The string that represents the function signature, without a trailing semicolon
 */
std::string ProtocolScaling::decodeSignature(inmemorytypes_t inmemory, encodedtypes_t encoded, bool bigendian) const
{
    if(isTypeBitfield(encoded))
    {
        std::string longbit;

        if(typeLength(encoded) > 4)
            longbit = "Long";

        if(isTypeFloating(inmemory))
            return std::string(typeName(inmemory) + " " + typeSigName(inmemory) + "ScaledFrom" + longbit + "Bitfield(" + typeName(encoded) + " value, " + typeName(convertTypeToSigned(inmemory)) + " min, " + typeName(convertTypeToUnsigned(inmemory)) + " invscaler)");
        else
            return std::string(typeName(inmemory) + " " + typeSigName(inmemory) + "ScaledFrom" + longbit + "Bitfield(" + typeName(encoded) + " value, " + typeName(convertTypeToSigned(inmemory)) + " min, " + typeName(convertTypeToUnsigned(inmemory)) + " divisor)");
    }
    else
    {
        std::string byteLength = std::to_string(typeLength(encoded));
        std::string endian;

        if(typeLength(encoded) > 1)
        {
            if(bigendian)
                endian = "Be";
            else
                endian = "Le";
        }

        if(isTypeFloating(inmemory))
        {
            if(isTypeSigned(encoded))
                return std::string(typeName(inmemory) + " " + typeSigName(inmemory) + "ScaledFrom" + byteLength + "Signed" + endian + "Bytes(const uint8_t* bytes, int* index, " + typeName(convertTypeToUnsigned(inmemory)) + " invscaler)");
            else
                return std::string(typeName(inmemory) + " " + typeSigName(inmemory) + "ScaledFrom" + byteLength + "Unsigned" + endian + "Bytes(const uint8_t* bytes, int* index, " + typeName(convertTypeToSigned(inmemory)) + " min, " + typeName(convertTypeToUnsigned(inmemory)) + " invscaler)");
        }
        else
        {
            if(isTypeSigned(encoded))
                return std::string(typeName(inmemory) + " " + typeSigName(inmemory) + "ScaledFrom" + byteLength + "Signed" + endian + "Bytes(const uint8_t* bytes, int* index, " + typeName(convertTypeToUnsigned(inmemory)) + " divisor)");
            else
                return std::string(typeName(inmemory) + " " + typeSigName(inmemory) + "ScaledFrom" + byteLength + "Unsigned" + endian + "Bytes(const uint8_t* bytes, int* index, " + typeName(convertTypeToSigned(inmemory)) + " min, " + typeName(convertTypeToUnsigned(inmemory)) + " divisor)");
        }

    }// If multi-byte

}// ProtocolScaling::decodeSignature


/*!
 * Generate the full function output, excluding the comment
 * \param inmemory is the type information for the inmemory (in-memory) data.
 * \param encoded is the type information for the encoded (encoded) data.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return the function as a string
 */
std::string ProtocolScaling::fullDecodeFunction(inmemorytypes_t inmemory, encodedtypes_t encoded, bool bigendian) const
{
    std::string function = decodeSignature(inmemory, encoded, bigendian) + "\n";
    function += "{\n";

    if(isTypeBitfield(encoded))
    {
        if(isTypeFloating(inmemory))
            function += "    return (" + typeName(inmemory) + ")(min + invscaler*value);\n";
        else
            function += "    return (" + typeName(inmemory) + ")(min + value/divisor);\n";
    }
    else
    {
        std::string endian;
        if(typeLength(encoded) > 1)
        {
            if(bigendian)
                endian = "Be";
            else
                endian = "Le";
        }

        std::string bitCount = std::to_string(typeLength(encoded)*8);

        if(isTypeFloating(inmemory))
        {
            if(isTypeSigned(encoded))
                function += "    return (" + typeName(inmemory) + ")(invscaler*int" + bitCount + "From" + endian + "Bytes(bytes, index));\n";
            else
                function += "    return (" + typeName(inmemory) + ")(min + invscaler*uint" + bitCount + "From" + endian + "Bytes(bytes, index));\n";
        }
        else
        {
            if(isTypeSigned(encoded))
                function += "    return (" + typeName(inmemory) + ")(int" + bitCount + "From" + endian + "Bytes(bytes, index)/divisor);\n";
            else
                function += "    return (" + typeName(inmemory) + ")(min + uint" + bitCount + "From" + endian + "Bytes(bytes, index)/divisor);\n";
        }
    }

    function += ("}\n");

    return function;

}// ProtocolScaling::fullDecodeFunction
