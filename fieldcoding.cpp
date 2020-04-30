#include "fieldcoding.h"
#include "protocolparser.h"


FieldCoding::FieldCoding(ProtocolSupport sup) :
    ProtocolScaling(sup)
{
    // we use 64-bit integers for floating point
    if(!support.int64)
        support.float64 = false;

    if(support.int64)
    {
        typeNames    <<"uint64_t"<<"int64_t"<<"uint64_t"<<"int64_t"<<"uint64_t"<<"int64_t"<<"uint64_t"<<"int64_t";
        typeSigNames <<"uint64"  <<"int64"  <<"uint56"  <<"int56"  <<"uint48"  <<"int48"  <<"uint40"  <<"int40"  ;
        typeSizes    <<        8 <<       8 <<        7 <<       7 <<        6 <<       6 <<        5 <<       5 ;
        typeUnsigneds<< true     << false   << true     << false   << true     << false   << true     << false   ;
    }

    // These types are always supported
    typeNames    << "float" <<"uint32_t"<<"int32_t"<<"uint32_t"<<"int32_t"<<"uint16_t"<<"int16_t"<<"uint8_t"<<"int8_t";
    typeSigNames <<"float32"<<"uint32"  <<"int32"  <<"uint24"  <<"int24"  <<"uint16"  <<"int16"  <<"uint8"  <<"int8";
    typeSizes    <<       4 <<      4   <<       4 <<        3 <<       3 <<        2 <<       2 <<       1 << 1;
    typeUnsigneds<< false   << true     << false   << true     << false   << true     << false   << true    << false;

    if(support.float64)
    {
        typeNames    << "double";
        typeSigNames <<"float64";
        typeSizes    <<       8 ;
        typeUnsigneds<< false   ;
    }

    if(support.specialFloat)
    {
        typeNames    <<"float"   <<"float"  ;
        typeSigNames <<"float24" <<"float16";
        typeSizes    <<        3 <<       2 ;
        typeUnsigneds<< false    << false   ;
    }

}


/*!
 * Generate the source and header files for protocol scaling
 * \return true if both files are generated
 */
bool FieldCoding::generate(void)
{
    if(generateEncodeHeader() && generateEncodeSource() && generateDecodeHeader() && generateDecodeSource())
        return true;
    else
        return false;
}


/*!
 * Generate the header file for protocol array scaling
 * \return true if the file is generated.
 */
bool FieldCoding::generateEncodeHeader(void)
{
    header.setModuleNameAndPath("fieldencode", support.outputpath, support.language);

    // Top level comment
    header.write("\
/*!\n\
 * \\file\n\
 * fieldencode provides routines to place numbers into a byte stream.\n\
 *\n\
 * fieldencode provides routines to place numbers in local memory layout into\n\
 * a big or little endian byte stream. The byte stream is simply a sequence of\n\
 * bytes, as might come from the data payload of a packet.\n\
 *\n\
 * Support is included for non-standard types such as unsigned 24. When\n\
 * working with nonstandard types the data in memory are given using the next\n\
 * larger standard type. For example an unsigned 24 is actually a uint32_t in\n\
 * which the most significant byte is clear, and only the least significant\n\
 * three bytes are placed into a byte stream\n\
 *\n\
 * Big or Little Endian refers to the order that a computer architecture will\n\
 * place the bytes of a multi-byte word into successive memory locations. For\n\
 * example the 32-bit number 0x01020304 can be placed in successive memory\n\
 * locations in Big Endian: [0x01][0x02][0x03][0x04]; or in Little Endian:\n\
 * [0x04][0x03][0x02][0x01]. The names \"Big Endian\" and \"Little Endian\" come\n\
 * from Swift's Gulliver's travels, referring to which end of an egg should be\n\
 * opened. The choice of name is made to emphasize the degree to which the\n\
 * choice of memory layout is un-interesting, as long as one stays within the\n\
 * local memory.\n\
 *\n\
 * When transmitting data from one computer to another that assumption no\n\
 * longer holds. In computer-to-computer transmission there are three endians\n\
 * to consider: the endianness of the sender, the receiver, and the protocol\n\
 * between them. A protocol is Big Endian if it sends the most significant\n\
 * byte first and the least significant last. If the computer and the protocol\n\
 * have the same endianness then encoding data from memory into a byte stream\n\
 * is a simple copy. However if the endianness is not the same then bytes must\n\
 * be re-ordered for the data to be interpreted correctly.\n\
 */\n");

    header.write("\n#define __STDC_CONSTANT_MACROS\n");
    header.write("#include <stdint.h>\n");

    if(support.supportbool)
        header.writeIncludeDirective("stdbool.h", "", true);

    header.write("\n");
    header.write("//! Macro to limit a number to be no more than a maximum value\n");
    header.write("#define limitMax(number, max) (((number) > (max)) ? (max) : (number))\n");
    header.write("\n");
    header.write("//! Macro to limit a number to be no less than a minimum value\n");
    header.write("#define limitMin(number, min) (((number) < (min)) ? (min) : (number))\n");
    header.write("\n");
    header.write("//! Macro to limit a number to be no less than a minimum value and no more than a maximum value\n");
    header.write("#define limitBoth(number, min, max) (((number) > (max)) ? (max) : (limitMin((number), (min))))\n");

    header.write("\n");
    header.write("//! Copy a null terminated string\n");
    header.write("void pgstrncpy(char* dst, const char* src, int maxLength);\n");
    header.write("\n");
    header.write("//! Encode a null terminated string on a byte stream\n");
    header.write("void stringToBytes(const char* string, uint8_t* bytes, int* index, int maxLength, int fixedLength);\n");
    header.write("\n");
    header.write("//! Copy an array of bytes to a byte stream without changing the order.\n");
    header.write("void bytesToBeBytes(const uint8_t* data, uint8_t* bytes, int* index, int num);\n");
    header.write("\n");
    header.write("//! Copy an array of bytes to a byte stream while reversing the order.\n");
    header.write("void bytesToLeBytes(const uint8_t* data, uint8_t* bytes, int* index, int num);\n");

    if(support.int64)
    {
        header.write("\n");
        header.write("#ifdef UINT64_MAX\n");
    }

    for(int i = 0; i < typeNames.size(); i++)
    {
        if(support.int64 && (i > 0))
        {
            if((typeSizes.at(i) == 4) && (typeSizes.at(i-1) == 5))
                header.write("\n#endif // UINT64_MAX\n");
        }

        if(typeSizes[i] != 1)
        {
            // big endian
            header.write("\n");
            header.write("//! " + briefEncodeComment(i, true) + "\n");
            header.write(encodeSignature(i, true) + ";\n");
            
            // little endian
            header.write("\n");
            header.write("//! " + briefEncodeComment(i, false) + "\n");
            header.write(encodeSignature(i, false) + ";\n");
        }
        else
        {
            header.write("\n");
            header.write("//! " + briefEncodeComment(i, true) + "\n");
            header.write(encodeSignature(i, true) + "\n");            
        }


    }// for all output byte counts

    header.write("\n");

    return header.flush();

}// FieldCoding::generateEncodeHeader


/*!
 * Generate the source file for protocols caling
 * \return true if the file is generated.
 */
bool FieldCoding::generateEncodeSource(void)
{
    source.setModuleNameAndPath("fieldencode", support.outputpath, support.language);

    if(support.specialFloat)
        source.writeIncludeDirective("floatspecial.h");

    source.write("\n\n");

    // The string functions, these were hand-written and pasted in here
    source.write("\
/*!\n\
 * Copy a null terminated string to a destination whose maximum length (with\n\
 * null terminator) is `maxLength`. The destination string is guaranteed to\n\
 * have a null terminator when this operation is complete. This is a\n\
 * replacement for strncpy().\n\
 * \\param dst receives the string, and is guaranteed to be null terminated.\n\
 * \\param src is the null terminated source string to copy.\n\
 * \\param maxLength is the size of the `dst` buffer.\n\
 */\n\
void pgstrncpy(char* dst, const char* src, int maxLength)\n\
{\n\
    int index = 0;\n\
    stringToBytes(src, (uint8_t*)dst, &index, maxLength, 0);\n\
}\n\
\n\
\n\
/*!\n\
 * Encode a null terminated string on a byte stream\n\
 * \\param string is the null termianted string to encode\n\
 * \\param bytes is a pointer to the byte stream which receives the encoded data.\n\
 * \\param index gives the location of the first byte in the byte stream, and\n\
 *        will be incremented by the number of bytes encoded when this function\n\
 *        is complete.\n\
 * \\param maxLength is the maximum number of bytes that can be encoded. A null\n\
 *        terminator is always included in the encoding.\n\
 * \\param fixedLength should be 1 to force the number of bytes encoded to be\n\
 *        exactly equal to maxLength.\n\
 */\n\
void stringToBytes(const char* string, uint8_t* bytes, int* index, int maxLength, int fixedLength)\n\
{\n\
    int i;\n\
\n\
    // increment byte pointer for starting point\n\
    bytes += (*index);\n\
\n\
    // Reserve the last byte for null termination\n\
    for(i = 0; i < maxLength - 1; i++)\n\
    {\n\
        if(string[i] == 0)\n\
            break;\n\
        else\n\
            bytes[i] = (uint8_t)string[i];\n\
    }\n\
\n\
    // Make sure last byte has null termination\n\
    bytes[i++] = 0;\n\
\n\
    if(fixedLength)\n\
    {\n\
        // Finish with null bytes\n\
        for(; i < maxLength; i++)\n\
            bytes[i] = 0;\n\
    }\n\
\n\
    // Return for the number of bytes we encoded\n\
    (*index) += i;\n\
\n\
}// stringToBytes\n\
\n\
\n\
/*!\n\
 * Copy an array of bytes to a byte stream without changing the order.\n\
 * \\param data is the array of bytes to copy.\n\
 * \\param bytes is a pointer to the byte stream which receives the encoded data.\n\
 * \\param index gives the location of the first byte in the byte stream, and\n\
 *        will be incremented by num when this function is complete.\n\
 * \\param num is the number of bytes to copy\n\
 */\n\
void bytesToBeBytes(const uint8_t* data, uint8_t* bytes, int* index, int num)\n\
{\n\
    // increment byte pointer for starting point\n\
    bytes += (*index);\n\
\n\
    // Increment byte index to indicate number of bytes copied\n\
    (*index) += num;\n\
\n\
    // Copy the bytes without changing the order\n\
    while(num > 0)\n\
    {\n\
        *(bytes++) = *(data++);\n\
        num--;\n\
    }\n\
\n\
}// bytesToBeBytes\n\
\n\
\n\
/*!\n\
 * Copy an array of bytes to a byte stream while reversing the order.\n\
 * \\param data is the array of bytes to copy.\n\
 * \\param bytes is a pointer to the byte stream which receives the encoded data.\n\
 * \\param index gives the location of the first byte in the byte stream, and\n\
 *        will be incremented by num when this function is complete.\n\
 * \\param num is the number of bytes to copy\n\
 */\n\
void bytesToLeBytes(const uint8_t* data, uint8_t* bytes, int* index, int num)\n\
{\n\
    // increment byte pointer for starting point\n\
    bytes += (*index);\n\
\n\
    // Increment byte index to indicate number of bytes copied\n\
    (*index) += num;\n\
\n\
    // To encode as \"little endian bytes\", (a nonsensical statement), reverse the byte order\n\
    bytes += (num - 1);\n\
\n\
    // Copy the bytes, reversing the order\n\
    while(num > 0)\n\
    {\n\
        *(bytes--) = *(data++);\n\
        num--;\n\
    }\n\
\n\
}// bytesToLeBytes\n");

    if(support.int64)
    {
        source.write("\n");
        source.write("#ifdef UINT64_MAX\n");
    }

    for(int i = 0; i < typeNames.size(); i++)
    {
        if(support.int64 && (i > 0))
        {
            if((typeSizes.at(i) == 4) && (typeSizes.at(i-1) == 5))
                source.write("#endif // UINT64_MAX\n");
        }

        if(typeSizes[i] != 1)
        {
            // big endian
            source.write("\n");
            source.write(fullEncodeComment(i, true) + "\n");
            source.write(fullEncodeFunction(i, true) + "\n");
            
            // little endian
            source.write("\n");
            source.write(fullEncodeComment(i, false) + "\n");
            source.write(fullEncodeFunction(i, false) + "\n");
        }

    }

    source.write("\n");

    return source.flush();

}// FieldCoding::generateEncodeSource


/*!
 * Get a human readable type name like "unsigned 3 byte integer".
 * \param type is the type enumeration
 * \return the human readable type name
 */
QString FieldCoding::getReadableTypeName(int type)
{
    QString name;

    if(typeSigNames[type].contains("float64"))
    {
        name = "8 byte float";
    }
    else if(typeSigNames[type].contains("float32"))
    {
        name = "4 byte float";
    }
    else
    {
        if(typeUnsigneds[type])
            name = "unsigned ";
        else
            name = "signed ";

        name += QString().setNum(typeSizes[type]);

        name += " byte integer";
    }

    return name;

}// FieldCoding::getReadableTypeName


/*!
 * Create the brief function comment, without doxygen decorations
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return The string that represents the one line function comment.
 */
QString FieldCoding::briefEncodeComment(int type, bool bigendian)
{
    QString name = getReadableTypeName(type);

    if(typeSizes[type] == 1)
    {
        // No endian concerns if using only 1 byte
        return QString("Encode a " + name + " on a byte stream.");
    }
    else
    {
        QString endian;

        if(bigendian)
            endian = "big";
        else
            endian = "little";

        return QString("Encode a " + name + " on a " + endian + " endian byte stream.");

    }// If multi-byte

}// FieldCoding::briefEncodeComment


/*!
 * Create the full encode function comment, with doxygen decorations
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return The string that represents the full multi-line function comment.
 */
QString FieldCoding::fullEncodeComment(int type, bool bigendian)
{
    QString comment = "/*!\n";

    comment += ProtocolParser::outputLongComment(" *", briefEncodeComment(type, bigendian)) + "\n";
    comment += " * \\param number is the value to encode.\n";
    comment += " * \\param bytes is a pointer to the byte stream which receives the encoded data.\n";
    comment += " * \\param index gives the location of the first byte in the byte stream, and\n";
    comment += " *        will be incremented by " + QString().setNum(typeSizes[type]) + " when this function is complete.\n";
    if(typeSigNames[type].contains("float24") || typeSigNames[type].contains("float16"))
        comment += " * \\param sigbits is the number of bits to use in the significand of the float.\n";
    comment += " */";

    return comment;
}


/*!
 * Create the one line function signature, without a trailing semicolon
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return The string that represents the function signature, without a trailing semicolon
 */
QString FieldCoding::encodeSignature(int type, bool bigendian)
{
    QString endian;

    // No endian concerns if using only 1 byte
    if(typeSizes[type] > 1)
    {
        if(bigendian)
            endian = "Be";
        else
            endian = "Le";

        if(typeSigNames[type].contains("float24") || typeSigNames[type].contains("float16"))
            return QString("void " + typeSigNames[type] + "To" + endian + "Bytes(" + typeNames[type] + " number, uint8_t* bytes, int* index, int sigbits)");
        else
            return QString("void " + typeSigNames[type] + "To" + endian + "Bytes(" + typeNames[type] + " number, uint8_t* bytes, int* index)");
    }
    else
    {
        return QString("#define " + typeSigNames[type] + "ToBytes(number, bytes, index) (bytes)[(*(index))++] = ((" + typeNames[type] + ")(number))");
    }

}// FieldCoding::encodeSignature


/*!
 * Generate the full encode function output, excluding the comment
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return the function as a string
 */
QString FieldCoding::fullEncodeFunction(int type, bool bigendian)
{
    if(typeSigNames[type].contains("float"))
        return floatEncodeFunction(type, bigendian);
    else
        return integerEncodeFunction(type, bigendian);
}


/*!
 * Generate the full encode function output, excluding the comment, for floating point types
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return the function as a string
 */
QString FieldCoding::floatEncodeFunction(int type, bool bigendian)
{
    QString endian;

    if(bigendian)
        endian = "Be";
    else
        endian = "Le";

    QString function = encodeSignature(type, bigendian) + "\n";
    function += "{\n";

    if((typeSizes[type] == 8) || (typeSizes[type] == 4))
    {
        function += "    union\n";
        function += "    {\n";
        if(typeSizes[type] == 8)
        {
            function += "        double floatValue;\n";
            function += "        uint64_t integerValue;\n";
        }
        else
        {
            function += "        float floatValue;\n";
            function += "        uint32_t integerValue;\n";
        }
        function += "    }field;\n";
        function += "\n";
        function += "    field.floatValue = number;\n";
        function += "\n";
        function += "    uint" + QString().setNum(8*typeSizes[type]) + "To" + endian + "Bytes(field.integerValue, bytes, index);\n";
    }
    else if(typeSizes[type] == 3)
    {
        function += "    uint24To" + endian + "Bytes(float32ToFloat24ex(number, sigbits), bytes, index);\n";
    }
    else
        function += "    uint16To" + endian + "Bytes(float32ToFloat16ex(number, sigbits), bytes, index);\n";

    function += "}\n";

    return function;

}// FieldCoding::floatEncodeFunction


/*!
 * Generate the full encode function output, excluding the comment, for integer types
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return the function as a string
 */
QString FieldCoding::integerEncodeFunction(int type, bool bigendian)
{
    QString function = encodeSignature(type, bigendian) + "\n";
    function += "{\n";

    if(typeSizes[type] == 1)
        return "// ";
    else
    {
        function += "    // increment byte pointer for starting point\n";

        QString opt;
        if(bigendian)
        {
            function += "    bytes += (*index) + " + QString().setNum(typeSizes[type]-1) + ";\n";
            opt = "--";
        }
        else
        {
            function += "    bytes += (*index);\n";
            opt = "++";
        }

        int offset;
        function += "\n";

        offset = typeSizes[type];
        while(offset > 1)
        {
            function += "    *(bytes" + opt + ") = (uint8_t)(number);\n";
            function += "    number = number >> 8;\n";
            offset--;
        }

        // Finish with the most significant byte
        function += "    *bytes = (uint8_t)(number);\n";
        function += "\n";

        // Update the index value to the user
        function += "    (*index) += " + QString().setNum(typeSizes[type]) + ";\n";

    }// if multi-byte fields

    function += "}\n";

    return function;

}// FieldCoding::integerEncodeFunction


/*!
 * Generate the header file for protocols caling
 * \return true if the file is generated.
 */
bool FieldCoding::generateDecodeHeader(void)
{
    header.setModuleNameAndPath("fielddecode", support.outputpath, support.language);

    // Top level comment
    header.write("\
/*!\n\
 * \\file\n\
 * fielddecode provides routines to pull numbers from a byte stream.\n\
 *\n\
 * fielddecode provides routines to pull numbers in local memory layout from\n\
 * a big or little endian byte stream. It is the opposite operation from the\n\
 * routines contained in fieldencode.h\n\
 *\n\
 * When compressing unsigned numbers (for example 32-bits to 16-bits) the most\n\
 * signficant bytes are discarded and the only requirement is that the value of\n\
 * the number fits in the smaller width. When going the other direction the\n\
 * most significant bytes are simply set to 0x00. However signed two's\n\
 * complement numbers are more complicated.\n\
 *\n\
 * If the signed value is a positive number that fits in the range then the\n\
 * most significant byte will be zero, and we can discard it. If the signed\n\
 * value is negative (in two's complement) then the most significant bytes are\n\
 * 0xFF and again we can throw them away. See the example below\n\
 *\n\
 * 32-bit +100 | 16-bit +100 | 8-bit +100\n\
 *  0x00000064 |      0x0064 |       0x64 <-- notice most significant bit clear\n\
 *\n\
 * 32-bit -100 | 16-bit -100 | 8-bit -100\n\
 *  0xFFFFFF9C |      0xFF9C |       0x9C <-- notice most significant bit set\n\
 *\n\
 * The signed complication comes when going the other way. If the number is\n\
 * positive setting the most significant bytes to zero is correct. However\n\
 * if the number is negative the most significant bytes must be set to 0xFF.\n\
 * This is the process of sign-extension. Typically this is handled by the\n\
 * compiler. For example if a int16_t is assigned to an int32_t the compiler\n\
 * (or the processor instruction) knows to perform the sign extension. However\n\
 * in our case we can decode signed 24-bit numbers (for example) which are\n\
 * returned to the caller as int32_t. In this instance fielddecode performs the\n\
 * sign extension.\n\
 */\n");

    header.write("\n");
    header.write("#define __STDC_CONSTANT_MACROS\n");
    header.write("#include <stdint.h>\n");

    if(support.supportbool)
        header.writeIncludeDirective("stdbool.h", "", true);

    header.write("\n");
    header.write("//! Decode a null terminated string from a byte stream\n");
    header.write("void stringFromBytes(char* string, const uint8_t* bytes, int* index, int maxLength, int fixedLength);\n");
    header.write("\n");
    header.write("//! Copy an array of bytes from a byte stream without changing the order.\n");
    header.write("void bytesFromBeBytes(uint8_t* data, const uint8_t* bytes, int* index, int num);\n");
    header.write("\n");
    header.write("//! Copy an array of bytes from a byte stream while reversing the order.\n");
    header.write("void bytesFromLeBytes(uint8_t* data, const uint8_t* bytes, int* index, int num);\n");

    if(support.int64)
    {
        header.write("\n");
        header.write("#ifdef UINT64_MAX\n");
    }

    for(int type = 0; type < typeNames.size(); type++)
    {
        if(support.int64 && (type > 0))
        {
            if((typeSizes.at(type) == 4) && (typeSizes.at(type-1) == 5))
                header.write("\n#endif // UINT64_MAX\n");
        }

        if(typeSizes[type] != 1)
        {
            header.write("\n");
            header.write("//! " + briefDecodeComment(type, true) + "\n");
            header.write(decodeSignature(type, true) + ";\n");

            header.write("\n");
            header.write("//! " + briefDecodeComment(type, false) + "\n");
            header.write(decodeSignature(type, false) + ";\n");
        }
        else
        {
            header.write("\n");
            header.write("//! " + briefDecodeComment(type, true) + "\n");
            header.write(decodeSignature(type, true) + "\n");
        }


    }// for all input types

    header.write("\n");

    return header.flush();

}// FieldCoding::generateDecodeHeader


/*!
 * Generate the source file for protocols caling
 * \return true if the file is generated.
 */
bool FieldCoding::generateDecodeSource(void)
{
    source.setModuleNameAndPath("fielddecode", support.outputpath, support.language);

    if(support.specialFloat)
        source.writeIncludeDirective("floatspecial.h");

    source.write("\n\n");

    source.write("\
/*!\n\
 * Decode a null terminated string from a byte stream\n\
 * \\param string receives the deocded null-terminated string.\n\
 * \\param bytes is a pointer to the byte stream to be decoded.\n\
 * \\param index gives the location of the first byte in the byte stream, and\n\
 *        will be incremented by the number of bytes decoded when this function\n\
 *        is complete.\n\
 * \\param maxLength is the maximum number of bytes that can be decoded.\n\
 *        maxLength includes the null terminator, which is always applied.\n\
 * \\param fixedLength should be 1 to force the number of bytes decoded to be\n\
 *        exactly equal to maxLength.\n\
 */\n\
void stringFromBytes(char* string, const uint8_t* bytes, int* index, int maxLength, int fixedLength)\n\
{\n\
    int i;\n\
\n\
    // increment byte pointer for starting point\n\
    bytes += *index;\n\
\n\
    for(i = 0; i < maxLength - 1; i++)\n\
    {\n\
        if(bytes[i] == 0)\n\
            break;\n\
        else\n\
            string[i] = (char)bytes[i];\n\
    }\n\
\n\
    // Make sure we include null terminator\n\
    string[i++] = 0;\n\
\n\
    if(fixedLength)\n\
        (*index) += maxLength;\n\
    else\n\
        (*index) += i;\n\
\n\
}// stringFromBytes\n\
\n\
\n\
/*!\n\
 * Copy an array of bytes from a byte stream without changing the order.\n\
 * \\param data receives the copied bytes\n\
 * \\param bytes is a pointer to the byte stream to be copied from.\n\
 * \\param index gives the location of the first byte in the byte stream, and\n\
 *        will be incremented by num when this function is complete.\n\
 * \\param num is the number of bytes to copy\n\
 */\n\
void bytesFromBeBytes(uint8_t* data, const uint8_t* bytes, int* index, int num)\n\
{\n\
    // increment byte pointer for starting point\n\
    bytes += (*index);\n\
\n\
    // Increment byte index to indicate number of bytes copied\n\
    (*index) += num;\n\
\n\
    // Copy the bytes without changing the order\n\
    while(num > 0)\n\
    {\n\
        *(data++) = *(bytes++);\n\
        num--;\n\
    }\n\
\n\
}// bytesFromBeBytes\n\
\n\
\n\
/*!\n\
 * Copy an array of bytes from a byte stream, reversing the order.\n\
 * \\param data receives the copied bytes\n\
 * \\param bytes is a pointer to the byte stream to be copied.\n\
 * \\param index gives the location of the first byte in the byte stream, and\n\
 *        will be incremented by num when this function is complete.\n\
 * \\param num is the number of bytes to copy\n\
 */\n\
void bytesFromLeBytes(uint8_t* data, const uint8_t* bytes, int* index, int num)\n\
{\n\
    // increment byte pointer for starting point\n\
    bytes += (*index);\n\
\n\
    // Increment byte index to indicate number of bytes copied\n\
    (*index) += num;\n\
\n\
    // To encode as \"little endian bytes\", (a nonsensical statement), reverse the byte order\n\
    bytes += (num - 1);\n\
\n\
    // Copy the bytes, reversing the order\n\
    while(num > 0)\n\
    {\n\
        *(data++) = *(bytes--);\n\
        num--;\n\
    }\n\
\n\
}// bytesFromLeBytes\n");

    if(support.int64)
    {
        source.write("\n");
        source.write("#ifdef UINT64_MAX\n");
    }

    for(int type = 0; type < typeNames.size(); type++)
    {
        if(support.int64 && (type > 0))
        {
            if((typeSizes.at(type) == 4) && (typeSizes.at(type-1) == 5))
                source.write("#endif // UINT64_MAX\n");
        }

        if(typeSizes[type] != 1)
        {
            // big endian unsigned
            source.write("\n");
            source.write(fullDecodeComment(type, true) + "\n");
            source.write(fullDecodeFunction(type, true) + "\n");

            // little endian unsigned
            source.write("\n");
            source.write(fullDecodeComment(type, false) + "\n");
            source.write(fullDecodeFunction(type, false) + "\n");
        }

    }// for all input types

    source.write("\n");

    return source.flush();

}// FieldCoding::generateDecodeSource


/*!
 * Create the brief decode function comment, without doxygen decorations
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return The string that represents the one line function comment.
 */
QString FieldCoding::briefDecodeComment(int type, bool bigendian)
{
    QString name = getReadableTypeName(type);

    if(typeSizes[type] == 1)
    {
        // No endian concerns if using only 1 byte
        return QString("Decode a " + name + " from a byte stream.");
    }
    else
    {
        QString endian;

        if(bigendian)
            endian = "big";
        else
            endian = "little";

        return QString("Decode a " + name + " from a " + endian + " endian byte stream.");

    }// If multi-byte

}// FieldCoding::briefDecodeComment


/*!
 * Create the full decode function comment, with doxygen decorations
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return The string that represents the full multi-line function comment.
 */
QString FieldCoding::fullDecodeComment(int type, bool bigendian)
{
    QString comment= ("/*!\n");

    comment += ProtocolParser::outputLongComment(" *", briefDecodeComment(type, bigendian)) + "\n";
    comment += " * \\param bytes is a pointer to the byte stream which contains the encoded data.\n";
    comment += " * \\param index gives the location of the first byte in the byte stream, and\n";
    comment += " *        will be incremented by " + QString().setNum(typeSizes[type]) + " when this function is complete.\n";

    if(typeSigNames[type].contains("float24") || typeSigNames[type].contains("float16"))
        comment += " * \\param sigbits is the number of bits to use in the significand of the float.\n";

    comment += " * \\return the number decoded from the byte stream\n";
    comment += " */";

    return comment;
}

/*!
 * Create the one line decode function signature, without a trailing semicolon
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return The string that represents the function signature, without a trailing semicolon
 */
QString FieldCoding::decodeSignature(int type, bool bigendian)
{
    QString endian;

    // No endian concerns if using only 1 byte
    if(typeSizes[type] > 1)
    {
        if(bigendian)
            endian = "Be";
        else
            endian = "Le";

        if(typeSigNames[type].contains("float24") || typeSigNames[type].contains("float16"))
            return QString(typeNames[type] + " " + typeSigNames[type] + "From" + endian + "Bytes(const uint8_t* bytes, int* index, int sigbits)");
        else
            return QString(typeNames[type] + " " + typeSigNames[type] + "From" + endian + "Bytes(const uint8_t* bytes, int* index)");
    }
    else
    {
        return QString("#define " + typeSigNames[type] + "FromBytes(bytes, index) (" + typeNames[type] + ")((bytes)[(*(index))++])");
    }

}// FieldCoding::decodeSignature


/*!
 * Generate the full decode function output, excluding the comment
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return the function as a string
 */
QString FieldCoding::fullDecodeFunction(int type, bool bigendian)
{
    if(typeSigNames[type].contains("float"))
        return floatDecodeFunction(type, bigendian);
    else
        return integerDecodeFunction(type, bigendian);
}


/*!
 * Generate the full decode function output, excluding the comment, for floating point types
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return the function as a string
 */
QString FieldCoding::floatDecodeFunction(int type, bool bigendian)
{
    QString endian;

    if(bigendian)
        endian = "Be";
    else
        endian = "Le";

    QString function = decodeSignature(type, bigendian) + "\n";
    function += "{\n";

    if((typeSizes[type] == 8) || (typeSizes[type] == 4))
    {
        function += "    union\n";
        function += "    {\n";
        if(typeSizes[type] == 8)
        {
            function += "        double floatValue;\n";
            function += "        uint64_t integerValue;\n";
        }
        else
        {
            function += "        float floatValue;\n";
            function += "        uint32_t integerValue;\n";
        }
        function += "    }field;\n";
        function += "\n";
        function += "    field.integerValue = uint" + QString().setNum(8*typeSizes[type]) + "From" + endian + "Bytes(bytes, index);\n";
        function += "\n";

        if(support.specialFloat)
        {
            if(typeSizes[type] == 8)
                function += "    if(isFloat64Valid(field.integerValue))\n";
            else
                function += "    if(isFloat32Valid(field.integerValue))\n";
            function += "        return field.floatValue;\n";
            function += "    else\n";
            function += "        return 0;\n";
        }
        else
            function += "    return field.floatValue;\n";

    }
    else if(typeSizes[type] == 3)
    {
        function += "    return float24ToFloat32ex(uint24From" + endian + "Bytes(bytes, index), sigbits);\n";
    }
    else
        function += "    return float16ToFloat32ex(uint16From" + endian + "Bytes(bytes, index), sigbits);\n";

    function += "}\n";

    return function;

}// FieldCoding::floatDecodeFunction


/*!
 * Generate the full decode function output, excluding the comment, for integer types
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return the function as a string
 */
QString FieldCoding::integerDecodeFunction(int type, bool bigendian)
{
    QString function = decodeSignature(type, bigendian) + "\n";
    function += "{\n";

    if(typeSizes[type] == 1)
        function += "    return (" + typeNames[type] + ")bytes[(*index)++];\n";
    else
    {
        bool signextend = false;

        // We have to perform sign extension for signed types that are nonstandard lengths
        if(typeUnsigneds[type] == false)
            if((typeSizes[type] == 7) || (typeSizes[type] == 6) || (typeSizes[type] == 5) || (typeSizes[type] == 3))
                signextend = true;

        if(signextend)
        {
            function += "    // Signed value in non-native size, requires sign extension. Algorithm\n";
            function += "    // courtesty of: https://graphics.stanford.edu/~seander/bithacks.html\n";

            function += "    const " + typeNames[type] + " m = ";
            switch(typeSizes[type])
            {
            default:
            case 3: function += "0x00800000;\n"; break;
            case 5: function += "0x0000008000000000LL;\n"; break;
            case 6: function += "0x0000800000000000LL;\n"; break;
            case 7: function += "0x0080000000000000LL;\n"; break;
            }
        }

        function += "    " + typeNames[type] + " number;\n";

        function += "\n";

        function += "    // increment byte pointer for starting point\n";

        // The byte pointer starts at different points depending on the byte order
        if(bigendian)
            function += "    bytes += *index;\n";
        else
            function += "    bytes += (*index) + " + QString().setNum(typeSizes[type]-1) + ";\n";

        function += "\n";

        int offset;
        if(bigendian)
        {
            // Start with the most significant byte
            function += "    number = *(bytes++);\n";

            offset = 1;
            while(offset < typeSizes[type] - 1)
            {
                function += "    number = (number << 8) | *(bytes++);\n";
                offset++;
            }

            function += "    number = (number << 8) | *bytes;\n";

        }// if big endian encoding
        else
        {
            offset = typeSizes[type] - 1;

            // Start with the most significant byte
            function += "    number = *(bytes--);\n";
            offset--;

            while(offset > 0)
            {
                function += "    number = (number << 8) | *(bytes--);\n";
                offset--;
            }

            // Finish with the least significant byte
            function += "    number = (number << 8) | *bytes;\n";

        }// if little endian encoding

        function += "\n";
        function += "    (*index) += " + QString().setNum(typeSizes[type]) + ";\n";

        function += "\n";

        if(signextend)
            function += "    return (number ^ m) - m;\n";
        else
            function += "    return number;\n";

    }// if multi-byte fields

    function += "}\n";

    return function;

}// FieldCoding::integerDecodeFunction

