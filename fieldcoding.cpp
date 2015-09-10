#include "fieldcoding.h"
#include "protocolparser.h"


FieldCoding::FieldCoding(ProtocolSupport sup) :
    ProtocolScaling(sup)
{
    typeNames.clear();
    typeSigNames.clear();
    typeSizes.clear();
    typeUnsigneds.clear();

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

    if(support.float64)
    {
        typeNames    << "double";
        typeSigNames <<"float64";
        typeSizes    <<       8 ;
        typeUnsigneds<< false   ;
    }

    // These types are always supported
    typeNames    << "float" <<"uint32_t"<<"int32_t"<<"uint32_t"<<"int32_t"<<"uint16_t"<<"int16_t"<<"uint8_t"<<"int8_t";
    typeSigNames <<"float32"<<"uint32"  <<"int32"  <<"uint24"  <<"int24"  <<"uint16"  <<"int16"  <<"uint8"  <<"int8";
    typeSizes    <<       4 <<      4   <<       4 <<        3 <<       3 <<        2 <<       2 <<       1 << 1;
    typeUnsigneds<< false   << true     << false   << true     << false   << true     << false   << true    << false;

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
    header.setModuleName("fieldencode");
    header.setVersionOnly(true);

    // Make sure empty
    header.clear();

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

    header.write("\n#include <stdint.h>\n");

    header.write("\n\
    //! Encode a null terminated string on a byte stream\n\
    void stringToBytes(const char* string, uint8_t* bytes, int* index, int maxLength, int fixedLength);\n");

    for(int i = 0; i < typeNames.size(); i++)
    {
        // big endian
        header.write("\n");
        header.write("//! " + briefEncodeComment(i, true) + "\n");
        header.write(encodeSignature(i, true) + ";\n");

        // little endian
        if(typeSizes[i] != 1)
        {
            header.write("\n");
            header.write("//! " + briefEncodeComment(i, false) + "\n");
            header.write(encodeSignature(i, false) + ";\n");
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
    source.setModuleName("fieldencode");
    source.setVersionOnly(true);

    // Make sure empty
    source.clear();

    if(support.specialFloat)
        source.write("#include \"floatspecial.h\"\n");

    source.write("\n\n");

    // The string function, this was hand-written and pasted in here
    source.write("\
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
}// stringToBytes\n");


    for(int i = 0; i < typeNames.size(); i++)
    {
        // big endian
        source.write("\n");
        source.write(fullEncodeComment(i, true) + "\n");
        source.write(fullEncodeFunction(i, true) + "\n");

        // little endian
        if(typeSizes[i] != 1)
        {
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
    }

    return QString("void " + typeSigNames[type] + "To" + endian + "Bytes(" + typeNames[type] + " number, uint8_t* bytes, int* index)");

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
        function += "    uint24To" + endian + "Bytes(float32ToFloat24((float)number), bytes, index);\n";
    }
    else
        function += "    uint16To" + endian + "Bytes(float32ToFloat16((float)number), bytes, index);\n";

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
        function += "    bytes[(*index)++] = (uint8_t)(number);\n";
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
    header.setModuleName("fielddecode");
    header.setVersionOnly(true);

    // Make sure empty
    header.clear();

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

    header.write("\n#include <stdint.h>\n");

    header.write("\n\
    //! Decode a null terminated string from a byte stream\n\
    void stringFromBytes(char* string, const uint8_t* bytes, int* index, int maxLength, int fixedLength);\n");

    for(int type = 0; type < typeNames.size(); type++)
    {
        header.write("\n");
        header.write("//! " + briefDecodeComment(type, true) + "\n");
        header.write(decodeSignature(type, true) + ";\n");

        if(typeSizes[type] != 1)
        {
            header.write("\n");
            header.write("//! " + briefDecodeComment(type, false) + "\n");
            header.write(decodeSignature(type, false) + ";\n");
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
    source.setModuleName("fielddecode");
    source.setVersionOnly(true);

    // Make sure empty
    source.clear();

    if(support.specialFloat)
        source.write("#include \"floatspecial.h\"\n");

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
}// stringFromBytes\n");

    for(int type = 0; type < typeNames.size(); type++)
    {
        // big endian unsigned
        source.write("\n");
        source.write(fullDecodeComment(type, true) + "\n");
        source.write(fullDecodeFunction(type, true) + "\n");

        // little endian unsigned
        if(typeSizes[type] != 1)
        {
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

    comment += ProtocolParser::outputLongComment(" *", briefEncodeComment(type, bigendian)) + "\n";
    comment += " * \\param bytes is a pointer to the byte stream which contains the encoded data.\n";
    comment += " * \\param index gives the location of the first byte in the byte stream, and\n";
    comment += " *        will be incremented by " + QString().setNum(typeSizes[type]) + " when this function is complete.\n";
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
    }

    return QString(typeNames[type] + " " + typeSigNames[type] + "From" + endian + "Bytes(const uint8_t* bytes, int* index)");

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
        function += "    return float24ToFloat32(uint24From" + endian + "Bytes(bytes, index));\n";
    }
    else
        function += "    return float16ToFloat32(uint16From" + endian + "Bytes(bytes, index));\n";

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
        QString variable;

        // We have to perform sign extension for signed types that are nonstandard lengths
        if(typeUnsigneds[type] == false)
            if((typeSizes[type] == 7) || (typeSizes[type] == 6) || (typeSizes[type] == 5) || (typeSizes[type] == 3))
                signextend = true;

        if(signextend)
        {
            function += "    // A bitfield is a fast portable way to get the compiler to sign extend a non standard size\n";
            function += "    // Courtesty of: https://graphics.stanford.edu/~seander/bithacks.html\n";
            function += "    struct{" + typeNames[type] + " number: " + QString().setNum(typeSizes[type]*8) + ";}s;\n";
            variable = "s.number";
        }
        else
        {
            function += "    " + typeNames[type] + " number;\n";
            variable = "number";
        }

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
            function += "    " + variable + " = *(bytes++);\n";

            offset = 1;
            while(offset < typeSizes[type] - 1)
            {
                function += "    " + variable + " = (" + variable + " << 8) | *(bytes++);\n";
                offset++;
            }

            function += "    " + variable + " = (" + variable + " << 8) | *bytes;\n";

        }// if big endian encoding
        else
        {
            offset = typeSizes[type] - 1;

            // Start with the most significant byte
            function += "    " + variable + " = *(bytes--);\n";
            offset--;

            while(offset > 0)
            {
                function += "    " + variable + " = (" + variable + " << 8) | *(bytes--);\n";
                offset--;
            }

            // Finish with the least significant byte
            function += "    " + variable + " = (" + variable + " << 8) | *bytes;\n";

        }// if little endian encoding

        function += "\n";
        function += "    (*index) += " + QString().setNum(typeSizes[type]) + ";\n";

        function += "\n";
        function += "    return " + variable + ";\n";

    }// if multi-byte fields

    function += "}\n";

    return function;

}// FieldCoding::integerDecodeFunction

