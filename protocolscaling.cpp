#include "protocolscaling.h"
#include "protocolparser.h"


/*!
 * Construct the protocol scaling object
 */
ProtocolScaling::ProtocolScaling(ProtocolSupport sup) :
    support(sup)
{
    if(support.int64)
    {
        typeNames    << "double" << "uint64_t" << "int64_t" << "float" << "uint32_t" << "int32_t" << "uint16_t" << "int16_t" << "uint8_t" << "int8_t";
        typeSigNames << "float64"<< "uint64"   << "int64"   <<"float32"<< "uint32"   << "int32"   << "uint16"   << "int16"   << "uint8"   << "int8";
        typeSizes    <<        8 <<          8 <<         8 <<       4 <<          4 <<         4 <<          2 <<         2 <<         1 <<        1;

        if(support.float64)
        {
            // double and float
            fromIndices  << 0 << 3;
        }
        else
        {
            // just float
            fromIndices  << 3;
        }
    }
    else if(support.float64)
    {
        typeNames    << "double" <<  "float" << "uint32_t" << "int32_t" << "uint16_t" << "int16_t" << "uint8_t" << "int8_t";
        typeSigNames << "float64"<< "float32"<< "uint32"   << "int32"   << "uint16"   << "int16"   << "uint8"   << "int8";
        typeSizes    <<        8 <<        4 <<          4 <<         4 <<          2 <<         2 <<         1 <<        1;

        // double and float
        fromIndices  << 0 << 1;
    }
    else
    {
        typeNames    <<  "float" << "uint32_t" << "int32_t" << "uint16_t" << "int16_t" << "uint8_t" << "int8_t";
        typeSigNames << "float32"<< "uint32"   << "int32"   << "uint16"   << "int16"   << "uint8"   << "int8";
        typeSizes    <<        4 <<          4 <<         4 <<          2 <<         2 <<         1 <<        1;

        // just float
        fromIndices  << 0;

    }

}


/*!
 * Generate the source and header files for protocol scaling
 * \return true if both files are generated
 */
bool ProtocolScaling::generate(void)
{
    if(generateEncodeHeader() && generateEncodeSource() && generateDecodeHeader() && generateDecodeSource())
        return true;
    else
        return false;
}


/*!
 * Generate the header file for protocols caling
 * \return true if the file is generated.
 */
bool ProtocolScaling::generateEncodeHeader(void)
{
    header.setModuleName("scaledencode");

    // Make sure empty
    header.clear();

    // Top level comment
    header.write(
"/*!\n\
 * \\file\n\
 * scaledencode routines place scaled numbers into a byte stream.\n\
 *\n\
 * scaledencode routines place scaled values into a big or little endian byte\n\
 * stream. The values can be any legitimate type (double, float, uint32_t,\n\
 * uint16_t, uint8_t, int32_t, int16_t, int8_t), and are encoded as either a\n\
 * unsigned or signed integer from 1 to 8 bytes in length. Unsigned encodings\n\
 * allow the caller to specify a minimum and a maximum value, with the only\n\
 * limitation that the maximum value must be more than the minimum. Signed\n\
 * encodings only allow the caller to specify a maximum value which gives\n\
 * maximum absolute value that can be encoded.\n\
 *\n\
 * An example encoding would be: take a float that represents speed in meters\n\
 * per second and encode it in two bytes from -200 to 200 meters per second.\n\
 * In that example the encoding function would be:\n\
 *\n\
 * floatScaledTo2SignedBeBytes(speed, bytestream, &index, 200);\n\
 *\n\
 * This would scale the speed according to (32767/200), and copy the resulting\n\
 * two bytes to bytestream[index] as a signed 16 bit number in big endian\n\
 * order. This would result in a velocity resolution of 0.006 m/s.\n\
 *\n\
 * Another example encoding is: take a double that represents altitude in\n\
 * meters and encode it in three bytes from -1000 to 49000 meters:\n\
 *\n\
 * doubleScaledTo3UnsignedLeBytes(alt, bytestream, &index, -1000, 49000);\n\
 *\n\
 * This would transform the altitude according to (alt *(16777215/50000) + 1000)\n\
 * and copy the resulting three bytes to bytestream[index] as an unsigned 24\n\
 * bit number in little endian order. This would result in an altitude\n\
 * resolution of 0.003 meters.\n\
 * \n\
 * scaledencode does not include routines that increase the resolution of the\n\
 * source value. For example the function floatScaledTo5UnsignedBeBytes() does\n\
 * not exist, because expanding a float to 5 bytes does not make any resolution\n\
 * improvement over encoding it in 4 bytes. In general the encoded format\n\
 * must be equal to or less than the number of bytes of the raw data.\n\
 */\n");

    header.write("\n");
    header.write("#define __STDC_CONSTANT_MACROS\n");
    header.write("#include <stdint.h>\n");

    for(int typeindex = 0; typeindex < fromIndices.size(); typeindex++)
    {
        bool ifdefopened = false;

        int type = fromIndices.at(typeindex);

        for(int length = typeSizes.at(type); length >= 1; length--)
        {
            // Protect against compilers that cannot support 64-bit operations
            if(support.int64)
            {
                if((ifdefopened == false) && (length > 4))
                {
                    ifdefopened = true;
                    header.write("\n#ifdef UINT64_MAX\n");
                }
                else if((ifdefopened == true) && (length <= 4))
                {
                    ifdefopened = false;
                    header.write("\n#endif // UINT64_MAX\n");
                }
            }
            else if(length > 4) // We don't always do 64-bit
                continue;

            // big endian unsigned
            header.write("\n");
            header.write("//! " + briefEncodeComment(type, length, true, true) + "\n");
            header.write(encodeSignature(type, length, true, true) + ";\n");

            // little endian unsigned
            if(length != 1)
            {
                header.write("\n");
                header.write("//! " + briefEncodeComment(type, length, false, true) + "\n");
                header.write(encodeSignature(type, length, false, true) + ";\n");
            }

            // big endian signed
            header.write("\n");
            header.write("//! " + briefEncodeComment(type, length, true, false) + "\n");
            header.write(encodeSignature(type, length, true, false) + ";\n");

            // little endian signed
            if(length != 1)
            {
                header.write("\n");
                header.write("//! " + briefEncodeComment(type, length, false, false) + "\n");
                header.write(encodeSignature(type, length, false, false) + ";\n");
            }

        }// for all output byte counts

    }// for all input types

    header.write("\n");

    return header.flush();

}// ProtocolScaling::generateEncodeHeader


/*!
 * Generate the source file for protocols caling
 * \return true if the file is generated.
 */
bool ProtocolScaling::generateEncodeSource(void)
{
    // Make sure empty
    source.clear();

    source.setModuleName("scaledencode");
    source.write("#include \"fieldencode.h\"\n");
    source.write("\n");

    for(int typeindex = 0; typeindex < fromIndices.size(); typeindex++)
    {
        bool ifdefopened = false;

        int type = fromIndices.at(typeindex);

        for(int length = typeSizes.at(type); length >= 1; length--)
        {
            // Protect against compilers that cannot support 64-bit operations
            if(support.int64)
            {
                if((ifdefopened == false) && (length > 4))
                {
                    ifdefopened = true;
                    source.write("#ifdef UINT64_MAX\n");
                }
                else if((ifdefopened == true) && (length <= 4))
                {
                    ifdefopened = false;
                    source.write("#endif // UINT64_MAX\n");
                }
            }
            else if(length > 4) // We don't always do 64-bit
                continue;

            // big endian unsigned
            source.write("\n");
            source.write(fullEncodeComment(type, length, true, true) + "\n");
            source.write(fullEncodeFunction(type, length, true, true) + "\n");

            // little endian unsigned
            if(length != 1)
            {
                source.write("\n");
                source.write(fullEncodeComment(type, length, false, true) + "\n");
                source.write(fullEncodeFunction(type, length, false, true) + "\n");
            }

            // big endian signed
            source.write("\n");
            source.write(fullEncodeComment(type, length, true, false) + "\n");
            source.write(fullEncodeFunction(type, length, true, false) + "\n");

            // little endian signed
            if(length != 1)
            {
                source.write("\n");
                source.write(fullEncodeComment(type, length, false, false) + "\n");
                source.write(fullEncodeFunction(type, length, false, false) + "\n");
            }

        }// for all output byte counts

    }// for all input types

    source.write("\n");

    return source.flush();

}// ProtocolScaling::generateEncodeSource


/*!
 * Create the brief function comment, without doxygen decorations
 * \param type is the enumerator for the functions input type.
 * \param length is the number of bytes in the encoded format.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \param Unsigned should be true if the function outputs unsigned bytes.
 * \return The string that represents the one line function comment.
 */
QString ProtocolScaling::briefEncodeComment(int type, int length, bool bigendian, bool Unsigned)
{
    QString typeName = typeNames[type];
    typeName.remove("_t", Qt::CaseInsensitive);

    if(length == 1)
    {
        // No endian concerns if using only 1 byte
        if(Unsigned)
            return QString("Encode a " + typeNames[type] + " on a byte stream by scaling to fit in 1 unsigned byte.");
        else
            return QString("Encode a " + typeNames[type] + " on a byte stream by scaling to fit in 1 signed byte.");
    }
    else
    {
        QString byteLength;
        byteLength.setNum(length);
        QString endian;

        if(bigendian)
            endian = "big";
        else
            endian = "little";

        if(Unsigned)
            return QString("Encode a " + typeNames[type] + " on a byte stream by scaling to fit in " + byteLength + " unsigned bytes in " + endian + " endian order.");
        else
            return QString("Encode a " + typeNames[type] + " on a byte stream by scaling to fit in " + byteLength + " signed bytes in " + endian +" endian order.");

    }// If multi-byte

}// ProtocolScaling::briefEncodeComment


/*!
 * Create the full encode function comment, with doxygen decorations
 * \param type is the enumerator for the functions input type.
 * \param length is the number of bytes in the encoded format.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \param Unsigned should be true if the function outputs unsigned bytes.
 * \return The string that represents the full multi-line function comment.
 */
QString ProtocolScaling::fullEncodeComment(int type, int length, bool bigendian, bool Unsigned)
{
    QString comment= ("/*!\n");

    comment += ProtocolParser::outputLongComment(" *", briefEncodeComment(type, length, bigendian, Unsigned)) + "\n";
    comment += " * \\param value is the number to encode.\n";
    comment += " * \\param bytes is a pointer to the byte stream which receives the encoded data.\n";
    comment += " * \\param index gives the location of the first byte in the byte stream, and\n";
    comment += " *        will be incremented by " + QString().setNum(length) + " when this function is complete.\n";

    if(Unsigned)
    {
        comment += " * \\param min is the minimum value that can be encoded.\n";
        comment += " * \\param scaler is multiplied by value to create the encoded integer: encoded = (value-min)*scaler.\n";
    }
    else
        comment += " * \\param scaler is multiplied by value to create the encoded integer: encoded = value*scaler.\n";

    comment += " */";

    return comment;
}


/*!
 * Create the one line function signature, without a trailing semicolon
 * \param type is the enumerator for the functions input type.
 * \param length is the number of bytes in the encoded format.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \param Unsigned should be true if the function outputs unsigned bytes.
 * \return The string that represents the function signature, without a trailing semicolon
 */
QString ProtocolScaling::encodeSignature(int type, int length, bool bigendian, bool Unsigned)
{
    if(length == 1)
    {
        // No endian concerns if using only 1 byte
        if(Unsigned)
        {
            if(typeSizes[type] > 4)
                return QString("void " + typeSigNames[type] + "ScaledTo1UnsignedBytes(" + typeNames[type] + " value, uint8_t* bytes, int* index, double min, double scaler)");
            else
                return QString("void " + typeSigNames[type] + "ScaledTo1UnsignedBytes(" + typeNames[type] + " value, uint8_t* bytes, int* index, float min, float scaler)");
        }
        else
        {
            if(typeSizes[type] > 4)
                return QString("void " + typeSigNames[type] + "ScaledTo1SignedBytes(" + typeNames[type] + " value, uint8_t* bytes, int* index, double scaler)");
            else
                return QString("void " + typeSigNames[type] + "ScaledTo1SignedBytes(" + typeNames[type] + " value, uint8_t* bytes, int* index, float scaler)");
        }
    }
    else
    {
        QString byteLength;
        byteLength.setNum(length);
        QString endian;

        if(bigendian)
            endian = "Be";
        else
            endian = "Le";

        if(Unsigned)
        {
            if(typeSizes[type] > 4)
                return QString("void " + typeSigNames[type] + "ScaledTo" + byteLength + "Unsigned" + endian + "Bytes(" + typeNames[type] + " value, uint8_t* bytes, int* index, double min, double scaler)");
            else
                return QString("void " + typeSigNames[type] + "ScaledTo" + byteLength + "Unsigned" + endian + "Bytes(" + typeNames[type] + " value, uint8_t* bytes, int* index, float min, float scaler)");
        }
        else
        {
            if(typeSizes[type] > 4)
                return QString("void " + typeSigNames[type] + "ScaledTo" + byteLength + "Signed" + endian + "Bytes(" + typeNames[type] + " value, uint8_t* bytes, int* index, double scaler)");
            else
                return QString("void " + typeSigNames[type] + "ScaledTo" + byteLength + "Signed" + endian + "Bytes(" + typeNames[type] + " value, uint8_t* bytes, int* index, float scaler)");
        }

    }// If multi-byte

}// ProtocolScaling::encodeSignature


/*!
 * Generate the full function output, excluding the comment
 * \param type is the input type to be encoded.
 * \param length is the number of bytes to use in the encoding
 * \param bigendian should be true for bigendian encoding
 * \param Unsigned should be true for unsigned encoding
 * \return the function as a string
 */
QString ProtocolScaling::fullEncodeFunction(int type, int length, bool bigendian, bool Unsigned)
{
    QString function = encodeSignature(type, length, bigendian, Unsigned) + "\n";

    QString endian;
    if(length > 1)
    {
        if(bigendian)
            endian = "Be";
        else
            endian = "Le";
    }

    QString bitCount;
    bitCount.setNum(length*8);

    QString numberType;
    if(Unsigned)
    {
        if(length > 4)
            numberType = "uint64_t";
        else if(length > 2)
            numberType = "uint32_t";
        else if(length > 1)
            numberType = "uint16_t";
        else
            numberType = "uint8_t";
    }
    else
    {
        if(length > 4)
            numberType = "int64_t";
        else if(length > 2)
            numberType = "int32_t";
        else if(length > 1)
            numberType = "int16_t";
        else
            numberType = "int8_t";
    }

    QString floatType;
    QString halfFraction;
    if(typeSizes[type] > 4)
    {
        halfFraction = "0.5";
        floatType = "double";
    }
    else
    {
        halfFraction = "0.5f";
        floatType = "float";
    }

    function += "{\n";
    function += "    // scale the number\n";

    if(Unsigned)
    {
        QString max;

        switch(length)
        {
        default:
        case 1: max = "255u"; break;
        case 2: max = "65535u"; break;
        case 3: max = "16777215u"; break;
        case 4: max = "4294967295uL"; break;
        case 5: max = "1099511627775ull"; break;
        case 6: max = "281474976710655ull"; break;
        case 7: max = "72057594037927935ull"; break;
        case 8: max = "18446744073709551615ull"; break;
        }

        function += "    " + floatType + " scaledvalue = (" + floatType + ")((value - min)*scaler);\n";
        function += "    " + numberType + " number;\n";
        function += "\n";
        function += "    // Make sure number fits in the range\n";
        function += "    if(scaledvalue >= " + max + ")\n";
        function += "        number = " + max + ";\n";
        function += "    else if(scaledvalue <= 0)\n";
        function += "        number = 0;\n";
        function += "    else\n";
        function += "        number = (" + numberType + ")(scaledvalue + " + halfFraction + "); // account for fractional truncation\n";
        function += "\n";
        function += "    uint" + bitCount + "To" + endian + "Bytes" + "((" + numberType + ")number, bytes, index);\n";
    }
    else
    {
        QString max;
        QString min;
        switch(length)
        {
        default:
        case 1: max = "127"; break;
        case 2: max = "32767"; break;
        case 3: max = "8388607"; break;
        case 4: max = "2147483647"; break;
        case 5: max = "549755813887ll"; break;
        case 6: max = "140737488355327ll"; break;
        case 7: max = "36028797018963967ll"; break;
        case 8: max = "9223372036854775807ll"; break;
        }

        switch(length)
        {
        default:
        case 1: min = "(-127 - 1)"; break;
        case 2: min = "(-32767 - 1)"; break;
        case 3: min = "(-8388607 - 1)"; break;
        case 4: min = "(-2147483647 - 1)"; break;
        case 5: min = "(-549755813887ll - 1)"; break;
        case 6: min = "(-140737488355327ll - 1)"; break;
        case 7: min = "(-36028797018963967ll - 1)"; break;
        case 8: min = "(-9223372036854775807ll - 1)"; break;
        }

        function += "    " + floatType + " scaledvalue = (" + floatType + ")(value*scaler);\n";
        function += "    " + numberType + " number;\n";
        function += "\n";
        function += "    // Make sure number fits in the range\n";
        function += "    if(scaledvalue >= 0)\n";
        function += "    {\n";
        function += "        if(scaledvalue >= " + max + ")\n";
        function += "            number = " + max + ";\n";
        function += "        else\n";
        function += "            number = (" + numberType + ")(scaledvalue + " + halfFraction + "); // account for fractional truncation\n";
        function += "    }\n";
        function += "    else\n";
        function += "    {\n";
        function += "        if(scaledvalue <= " + min + ")\n";
        function += "            number = " + min + ";\n";
        function += "        else\n";
        function += "            number = (" + numberType + ")(scaledvalue - " + halfFraction + "); // account for fractional truncation\n";
        function += "    }\n";
        function += "\n";
        function += "    int" + bitCount + "To" + endian + "Bytes" + "((" + numberType + ")number, bytes, index);\n";
    }

    function += ("}\n");

    return function;

}// ProtocolScaling::encodeFullFunction


/*!
 * Generate the header file for protocols caling
 * \return true if the file is generated.
 */
bool ProtocolScaling::generateDecodeHeader(void)
{
    header.setModuleName("scaleddecode");

    // Make sure empty
    header.clear();

    // Top level comment
    header.write(
"/*!\n\
 * \\file\n\
 * scaleddecode routines extract scaled numbers from a byte stream.\n\
 *\n\
 * scaleddecode routines extract scaled numbers from a byte stream. The routines\n\
 * in this module are the reverse operation of the routines in scaledencode.\n\
 */");

    header.write("\n");
    header.write("#define __STDC_CONSTANT_MACROS\n");
    header.write("#include <stdint.h>\n");

    for(int typeindex = 0; typeindex < fromIndices.size(); typeindex++)
    {
        bool ifdefopened = false;

        int type = fromIndices.at(typeindex);

        for(int length = typeSizes.at(type); length >= 1; length--)
        {
            // Protect against compilers that cannot support 64-bit operations
            if(support.int64)
            {
                if((ifdefopened == false) && (length > 4))
                {
                    ifdefopened = true;
                    header.write("\n#ifdef UINT64_MAX\n");
                }
                else if((ifdefopened == true) && (length <= 4))
                {
                    ifdefopened = false;
                    header.write("\n#endif // UINT64_MAX\n");
                }
            }
            else if(length > 4) // We don't always do 64-bit
                continue;

            // big endian unsigned
            header.write("\n");
            header.write("//! " + briefDecodeComment(type, length, true, true) + "\n");
            header.write(decodeSignature(type, length, true, true) + ";\n");

            // little endian unsigned
            if(length != 1)
            {
                header.write("\n");
                header.write("//! " + briefDecodeComment(type, length, false, true) + "\n");
                header.write(decodeSignature(type, length, false, true) + ";\n");
            }

            // big endian signed
            header.write("\n");
            header.write("//! " + briefDecodeComment(type, length, true, false) + "\n");
            header.write(decodeSignature(type, length, true, false) + ";\n");


            // little endian signed
            if(length != 1)
            {
                header.write("\n");
                header.write("//! " + briefDecodeComment(type, length, false, false) + "\n");
                header.write(decodeSignature(type, length, false, false) + ";\n");
            }


        }// for all output byte counts

    }// for all input types

    header.write("\n");

    return header.flush();

}// ProtocolScaling::generateDecodeHeader


/*!
 * Generate the source file for protocols caling
 * \return true if the file is generated.
 */
bool ProtocolScaling::generateDecodeSource(void)
{
    // Make sure empty
    source.clear();

    source.setModuleName("scaleddecode");
    source.write("#include \"fielddecode.h\"\n");
    source.write("\n");

    for(int typeindex = 0; typeindex < fromIndices.size(); typeindex++)
    {
        bool ifdefopened = false;

        int type = fromIndices.at(typeindex);

        for(int length = typeSizes.at(type); length >= 1; length--)
        {
            // Protect against compilers that cannot support 64-bit operations
            if(support.int64)
            {
                if((ifdefopened == false) && (length > 4))
                {
                    ifdefopened = true;
                    source.write("#ifdef UINT64_MAX\n");
                }
                else if((ifdefopened == true) && (length <= 4))
                {
                    ifdefopened = false;
                    source.write("#endif // UINT64_MAX\n");
                }
            }
            else if(length > 4) // We don't always do 64-bit
                continue;

            // big endian unsigned
            source.write("\n");
            source.write(fullDecodeComment(type, length, true, true) + "\n");
            source.write(fullDecodeFunction(type, length, true, true) + "\n");

            // little endian unsigned
            if(length != 1)
            {
                source.write("\n");
                source.write(fullDecodeComment(type, length, false, true) + "\n");
                source.write(fullDecodeFunction(type, length, false, true) + "\n");
            }

            // big endian signed
            source.write("\n");
            source.write(fullDecodeComment(type, length, true, false) + "\n");
            source.write(fullDecodeFunction(type, length, true, false) + "\n");

            // little endian signed
            if(length != 1)
            {
                source.write("\n");
                source.write(fullDecodeComment(type, length, false, false) + "\n");
                source.write(fullDecodeFunction(type, length, false, false) + "\n");
            }

        }// for all output byte counts

    }// for all input types

    source.write("\n");

    return source.flush();

}// ProtocolScaling::generateDecodeSource


/*!
 * Create the brief decode function comment, without doxygen decorations
 * \param type is the enumerator for the functions input type.
 * \param length is the number of bytes in the encoded format.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \param Unsigned should be true if the function outputs unsigned bytes.
 * \return The string that represents the one line function comment.
 */
QString ProtocolScaling::briefDecodeComment(int type, int length, bool bigendian, bool Unsigned)
{
    QString typeName = typeNames[type];
    typeName.remove("_t", Qt::CaseInsensitive);
    QString sign;

    if(Unsigned)
        sign = "unsigned";
    else
        sign = "signed";

    if(length == 1)
    {
        // No endian concerns if using only 1 byte
        return QString("Compute a " + typeNames[type] + " scaled from 1 " + sign + " byte.");
    }
    else
    {
        QString byteLength;
        byteLength.setNum(length);
        QString endian;

        if(bigendian)
            endian = "big";
        else
            endian = "little";

        return QString("Compute a " + typeNames[type] + " scaled from " + byteLength + " " + sign + " bytes in " + endian + " endian order.");

    }// If multi-byte

}// ProtocolScaling::briefDecodeComment


/*!
 * Create the full decode function comment, with doxygen decorations
 * \param type is the enumerator for the functions input type.
 * \param length is the number of bytes in the decoded format.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \param Unsigned should be true if the function outputs unsigned bytes.
 * \return The string that represents the full multi-line function comment.
 */
QString ProtocolScaling::fullDecodeComment(int type, int length, bool bigendian, bool Unsigned)
{
    QString comment= ("/*!\n");

    comment += ProtocolParser::outputLongComment(" *", briefDecodeComment(type, length, bigendian, Unsigned)) + "\n";
    comment += " * \\param bytes is a pointer to the byte stream to decode.\n";
    comment += " * \\param index gives the location of the first byte in the byte stream, and\n";
    comment += " *        will be incremented by " + QString().setNum(length) + " when this function is complete.\n";

    if(Unsigned)
    {
        comment += " * \\param min is the minimum value that can be decoded.\n";
        comment += " * \\param invscaler is multiplied by the encoded integer to create the return value.\n";
        comment += " *        invscaler should be the inverse of the scaler given to the encode function.\n";
        comment += " * \\return the correctly scaled decoded value. return = min + encoded*invscaler.\n";
    }
    else
    {
        comment += " * \\param invscaler is multiplied by the encoded integer to create the return value.\n";
        comment += " *        invscaler should be the inverse of the scaler given to the encode function.\n";
        comment += " * \\return the correctly scaled decoded value. return = encoded*invscaler.\n";
    }
    comment += " */";

    return comment;
}

/*!
 * Create the one line decode function signature, without a trailing semicolon
 * \param type is the enumerator for the functions input type.
 * \param length is the number of bytes in the encoded format.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \param Unsigned should be true if the function outputs unsigned bytes.
 * \return The string that represents the function signature, without a trailing semicolon
 */
QString ProtocolScaling::decodeSignature(int type, int length, bool bigendian, bool Unsigned)
{
    if(length == 1)
    {
        // No endian concerns if using only 1 byte
        if(Unsigned)
        {
            if(typeSizes[type] > 4)
                return QString(typeNames[type] + " " + typeSigNames[type] + "ScaledFrom1UnsignedBytes(const uint8_t* bytes, int* index, double min, double invscaler)");
            else
                return QString(typeNames[type] + " " + typeSigNames[type] + "ScaledFrom1UnsignedBytes(const uint8_t* bytes, int* index, float min, float invscaler)");
        }
        else
        {
            if(typeSizes[type] > 4)
                return QString(typeNames[type] + " " + typeSigNames[type] + "ScaledFrom1SignedBytes(const uint8_t* bytes, int* index, double invscaler)");
            else
                return QString(typeNames[type] + " " + typeSigNames[type] + "ScaledFrom1SignedBytes(const uint8_t* bytes, int* index, float invscaler)");
        }
    }
    else
    {
        QString byteLength;
        byteLength.setNum(length);
        QString endian;

        if(bigendian)
            endian = "Be";
        else
            endian = "Le";

        if(Unsigned)
        {
            if(typeSizes[type] > 4)
                return QString(typeNames[type] + " " + typeSigNames[type] + "ScaledFrom" + byteLength + "Unsigned" + endian + "Bytes(const uint8_t* bytes, int* index, double min, double invscaler)");
            else
                return QString(typeNames[type] + " " + typeSigNames[type] + "ScaledFrom" + byteLength + "Unsigned" + endian + "Bytes(const uint8_t* bytes, int* index, float min, float invscaler)");
        }
        else
        {
            if(typeSizes[type] > 4)
                return QString(typeNames[type] + " " + typeSigNames[type] + "ScaledFrom" + byteLength + "Signed" + endian + "Bytes(const uint8_t* bytes, int* index, double invscaler)");
            else
                return QString(typeNames[type] + " " + typeSigNames[type] + "ScaledFrom" + byteLength + "Signed" + endian + "Bytes(const uint8_t* bytes, int* index, float invscaler)");
        }

    }// If multi-byte

}// ProtocolScaling::decodeSignature


/*!
 * Generate the full function output, excluding the comment
 * \param type is the input type to be decoded.
 * \param length is the number of bytes to use in the encoding
 * \param bigendian should be true for bigendian encoding
 * \param Unsigned should be true for unsigned encoding
 * \return the function as a string
 */
QString ProtocolScaling::fullDecodeFunction(int type, int length, bool bigendian, bool Unsigned)
{
    QString function = decodeSignature(type, length, bigendian, Unsigned) + "\n";
    function += "{\n";

    QString endian;
    if(length > 1)
    {
        if(bigendian)
            endian = "Be";
        else
            endian = "Le";
    }

    QString bitCount;
    bitCount.setNum(length*8);

    if(Unsigned)
    {
        function += "    return (" + typeNames[type] + ")(min + invscaler*uint" + bitCount + "From" + endian + "Bytes(bytes, index));\n";

    }// if unsigned encoding
    else
    {
        function += "    return (" + typeNames[type] + ")(invscaler*int" + bitCount + "From" + endian + "Bytes(bytes, index));\n";

    }// else if signed encoding

    function += ("}\n");

    return function;

}// ProtocolScaling::decodeFullFunction
