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
        typeNames    = {"uint64_t", "int64_t", "uint64_t", "int64_t", "uint64_t", "int64_t", "uint64_t", "int64_t"};
        typeSigNames = {"uint64"  , "int64"  , "uint56"  , "int56"  , "uint48"  , "int48"  , "uint40"  , "int40"  };
        typeSizes    = {        8 ,        8 ,         7 ,        7 ,         6 ,        6 ,         5 ,       5  };
        typeUnsigneds= { true     ,  false   ,  true     ,  false   ,  true     ,  false   ,  true     ,   false  };
    }

    // These types are always supported

    std::vector<std::string> temp1 = {"float"  , "uint32_t", "int32_t", "uint32_t", "int32_t", "uint16_t", "int16_t", "uint8_t", "int8_t"};
    std::vector<std::string> temp2 = {"float32", "uint32"  , "int32"  , "uint24"  , "int24"  , "uint16"  , "int16"  , "uint8"  , "int8"  };
    std::vector<int>         temp3 = {       4 ,       4   ,        4 ,         3 ,        3 ,         2 ,        2 ,        1 ,       1 };
    std::vector<bool>        temp4 = { false   ,  true     ,  false   ,  true     ,  false   ,  true     ,  false   ,  true    ,  false  };

    typeNames.insert(typeNames.end(), temp1.begin(), temp1.end());
    typeSigNames.insert(typeSigNames.end(), temp2.begin(), temp2.end());
    typeSizes.insert(typeSizes.end(), temp3.begin(), temp3.end());
    typeUnsigneds.insert(typeUnsigneds.end(), temp4.begin(), temp4.end());

    if(support.float64)
    {
        typeNames.push_back("double");
        typeSigNames.push_back("float64");
        typeSizes.push_back(8);
        typeUnsigneds.push_back(false);
    }

    if(support.specialFloat)
    {
        typeNames.push_back("float");      typeNames.push_back("float");
        typeSigNames.push_back("float24"); typeSigNames.push_back("float16");
        typeSizes.push_back(3);            typeSizes.push_back(2);
        typeUnsigneds.push_back(false);    typeUnsigneds.push_back(false);
    }

}


/*!
 * Generate the source and header files for field coding
 * \param fileNameList is appended with the names of the generated files
 * \param filePathList is appended with the paths of the generated files
 * \return true if both modules are generated
 */
bool FieldCoding::generate(std::vector<std::string>& fileNameList, std::vector<std::string>& filePathList)
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
 * Generate the header file for protocol array scaling
 * \return true if the file is generated.
 */
bool FieldCoding::generateEncodeHeader(void)
{
    header.setModuleNameAndPath("fieldencode", support.outputpath, support.language);

// Raw string magic
header.setFileComment(R"(fieldencode provides routines to place numbers into a byte stream.

fieldencode provides routines to place numbers in local memory layout into
a big or little endian byte stream. The byte stream is simply a sequence of
bytes, as might come from the data payload of a packet.

Support is included for non-standard types such as unsigned 24. When
working with nonstandard types the data in memory are given using the next
larger standard type. For example an unsigned 24 is actually a uint32_t in
which the most significant byte is clear, and only the least significant
three bytes are placed into a byte stream

Big or Little Endian refers to the order that a computer architecture will
place the bytes of a multi-byte word into successive memory locations. For
example the 32-bit number 0x01020304 can be placed in successive memory
locations in Big Endian: [0x01][0x02][0x03][0x04]; or in Little Endian:
[0x04][0x03][0x02][0x01]. The names "Big Endian" and "Little Endian" come
from Swift's Gulliver's travels, referring to which end of an egg should be
opened. The choice of name is made to emphasize the degree to which the
choice of memory layout is un-interesting, as long as one stays within the
local memory.

When transmitting data from one computer to another that assumption no
longer holds. In computer-to-computer transmission there are three endians
to consider: the endianness of the sender, the receiver, and the protocol
between them. A protocol is Big Endian if it sends the most significant
byte first and the least significant last. If the computer and the protocol
have the same endianness then encoding data from memory into a byte stream
is a simple copy. However if the endianness is not the same then bytes must
be re-ordered for the data to be interpreted correctly.)");

    header.makeLineSeparator();
    header.write("\n#define __STDC_CONSTANT_MACROS\n");
    header.write("#include <stdint.h>\n");

    if(support.supportbool)
        header.writeIncludeDirective("stdbool.h", "", true);

    header.makeLineSeparator();

// Raw string magic
header.write(R"(//! Macro to limit a number to be no more than a maximum value
#define limitMax(number, max) (((number) > (max)) ? (max) : (number))

//! Macro to limit a number to be no less than a minimum value
#define limitMin(number, min) (((number) < (min)) ? (min) : (number))

//! Macro to limit a number to be no less than a minimum value and no more than a maximum value
#define limitBoth(number, min, max) (((number) > (max)) ? (max) : (limitMin((number), (min))))

//! Copy a null terminated string
void pgstrncpy(char* dst, const char* src, int maxLength);

//! Encode a null terminated string on a byte stream
void stringToBytes(const char* string, uint8_t* bytes, int* index, int maxLength, int fixedLength);

//! Copy an array of bytes to a byte stream without changing the order.
void bytesToBeBytes(const uint8_t* data, uint8_t* bytes, int* index, int num);

//! Copy an array of bytes to a byte stream while reversing the order.
void bytesToLeBytes(const uint8_t* data, uint8_t* bytes, int* index, int num);)");

    if(support.int64)
    {
        header.makeLineSeparator();
        header.write("#ifdef UINT64_MAX\n");
    }

    for(int i = 0; i < (int)typeNames.size(); i++)
    {
        if(support.int64 && (i > 0))
        {
            if((typeSizes.at(i) == 4) && (typeSizes.at(i-1) == 5))
                header.write("\n#endif // UINT64_MAX\n");
        }

        if(typeSizes[i] != 1)
        {
            // big endian
            header.makeLineSeparator();
            header.write("//! " + briefEncodeComment(i, true) + "\n");
            header.write(encodeSignature(i, true) + ";\n");
            
            // little endian
            header.makeLineSeparator();
            header.write("//! " + briefEncodeComment(i, false) + "\n");
            header.write(encodeSignature(i, false) + ";\n");
        }
        else
        {
            header.makeLineSeparator();
            header.write("//! " + briefEncodeComment(i, true) + "\n");
            header.write(encodeSignature(i, true) + "\n");            
        }


    }// for all output byte counts

    header.makeLineSeparator();

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

    source.makeLineSeparator();

source.write(R"(/*!
 * Copy a null terminated string to a destination whose maximum length (with
 * null terminator) is `maxLength`. The destination string is guaranteed to
 * have a null terminator when this operation is complete. This is a
 * replacement for strncpy().
 * \param dst receives the string, and is guaranteed to be null terminated.
 * \param src is the null terminated source string to copy.
 * \param maxLength is the size of the `dst` buffer.
 */
void pgstrncpy(char* dst, const char* src, int maxLength)
{
    int index = 0;
    stringToBytes(src, (uint8_t*)dst, &index, maxLength, 0);
}


/*!
 * Encode a null terminated string on a byte stream
 * \param string is the null termianted string to encode
 * \param bytes is a pointer to the byte stream which receives the encoded data.
 * \param index gives the location of the first byte in the byte stream, and
 *        will be incremented by the number of bytes encoded when this function
 *        is complete.
 * \param maxLength is the maximum number of bytes that can be encoded. A null
 *        terminator is always included in the encoding.
 * \param fixedLength should be 1 to force the number of bytes encoded to be
 *        exactly equal to maxLength.
 */
void stringToBytes(const char* string, uint8_t* bytes, int* index, int maxLength, int fixedLength)
{
    int i;

    // increment byte pointer for starting point
    bytes += (*index);

    // Reserve the last byte for null termination
    for(i = 0; i < maxLength - 1; i++)
    {
        if(string[i] == 0)
            break;
        else
            bytes[i] = (uint8_t)string[i];
    }

    // Make sure last byte has null termination
    bytes[i++] = 0;

    if(fixedLength)
    {
        // Finish with null bytes
        for(; i < maxLength; i++)
            bytes[i] = 0;
    }

    // Return for the number of bytes we encoded
    (*index) += i;

}// stringToBytes


/*!
 * Copy an array of bytes to a byte stream without changing the order.
 * \param data is the array of bytes to copy.
 * \param bytes is a pointer to the byte stream which receives the encoded data.
 * \param index gives the location of the first byte in the byte stream, and
 *        will be incremented by num when this function is complete.
 * \param num is the number of bytes to copy
 */
void bytesToBeBytes(const uint8_t* data, uint8_t* bytes, int* index, int num)
{
    // increment byte pointer for starting point
    bytes += (*index);

    // Increment byte index to indicate number of bytes copied
    (*index) += num;

    // Copy the bytes without changing the order
    while(num > 0)
    {
        *(bytes++) = *(data++);
        num--;
    }

}// bytesToBeBytes


/*!
 * Copy an array of bytes to a byte stream while reversing the order.
 * \param data is the array of bytes to copy.
 * \param bytes is a pointer to the byte stream which receives the encoded data.
 * \param index gives the location of the first byte in the byte stream, and
 *        will be incremented by num when this function is complete.
 * \param num is the number of bytes to copy
 */
void bytesToLeBytes(const uint8_t* data, uint8_t* bytes, int* index, int num)
{
    // increment byte pointer for starting point
    bytes += (*index);

    // Increment byte index to indicate number of bytes copied
    (*index) += num;

    // To encode as "little endian bytes", (a nonsensical statement), reverse the byte order
    bytes += (num - 1);

    // Copy the bytes, reversing the order
    while(num > 0)
    {
        *(bytes--) = *(data++);
        num--;
    }

}// bytesToLeBytes)");

    source.makeLineSeparator();

    if(support.int64)
    {
        source.makeLineSeparator();
        source.write("#ifdef UINT64_MAX\n");
    }

    for(int i = 0; i < (int)typeNames.size(); i++)
    {
        if(support.int64 && (i > 0))
        {
            if((typeSizes.at(i) == 4) && (typeSizes.at(i-1) == 5))
                source.write("#endif // UINT64_MAX\n");
        }

        if(typeSizes[i] != 1)
        {
            // big endian
            source.makeLineSeparator();
            source.write(fullEncodeComment(i, true) + "\n");
            source.write(fullEncodeFunction(i, true) + "\n");
            
            // little endian
            source.makeLineSeparator();
            source.write(fullEncodeComment(i, false) + "\n");
            source.write(fullEncodeFunction(i, false) + "\n");
        }

    }

    source.makeLineSeparator();

    return source.flush();

}// FieldCoding::generateEncodeSource


/*!
 * Get a human readable type name like "unsigned 3 byte integer".
 * \param type is the type enumeration
 * \return the human readable type name
 */
std::string FieldCoding::getReadableTypeName(int type)
{
    std::string name;

    if(contains(typeSigNames.at(type), "float64"))
    {
        name = "8 byte float";
    }
    else if(contains(typeSigNames.at(type), "float32"))
    {
        name = "4 byte float";
    }
    else
    {
        if(typeUnsigneds.at(type))
            name = "unsigned ";
        else
            name = "signed ";

        name += std::to_string(typeSizes.at(type));

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
std::string FieldCoding::briefEncodeComment(int type, bool bigendian)
{
    std::string name = getReadableTypeName(type);

    if(typeSizes[type] == 1)
    {
        // No endian concerns if using only 1 byte
        return "Encode a " + name + " on a byte stream.";
    }
    else
    {
        std::string endian;

        if(bigendian)
            endian = "big";
        else
            endian = "little";

        return "Encode a " + name + " on a " + endian + " endian byte stream.";

    }// If multi-byte

}// FieldCoding::briefEncodeComment


/*!
 * Create the full encode function comment, with doxygen decorations
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return The string that represents the full multi-line function comment.
 */
std::string FieldCoding::fullEncodeComment(int type, bool bigendian)
{
    std::string comment = "/*!\n";

    comment += ProtocolParser::outputLongComment(" *", briefEncodeComment(type, bigendian)) + "\n";
    comment += " * \\param number is the value to encode.\n";
    comment += " * \\param bytes is a pointer to the byte stream which receives the encoded data.\n";
    comment += " * \\param index gives the location of the first byte in the byte stream, and\n";
    comment += " *        will be incremented by " + std::to_string(typeSizes[type]) + " when this function is complete.\n";
    if(contains(typeSigNames[type], "float24") || contains(typeSigNames[type], "float16"))
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
std::string FieldCoding::encodeSignature(int type, bool bigendian)
{
    std::string endian;

    // No endian concerns if using only 1 byte
    if(typeSizes[type] > 1)
    {
        if(bigendian)
            endian = "Be";
        else
            endian = "Le";

        if(contains(typeSigNames[type], "float24") || contains(typeSigNames[type], "float16"))
            return std::string("void " + typeSigNames[type] + "To" + endian + "Bytes(" + typeNames[type] + " number, uint8_t* bytes, int* index, int sigbits)");
        else
            return std::string("void " + typeSigNames[type] + "To" + endian + "Bytes(" + typeNames[type] + " number, uint8_t* bytes, int* index)");
    }
    else
    {
        return std::string("#define " + typeSigNames[type] + "ToBytes(number, bytes, index) (bytes)[(*(index))++] = ((" + typeNames[type] + ")(number))");
    }

}// FieldCoding::encodeSignature


/*!
 * Generate the full encode function output, excluding the comment
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return the function as a string
 */
std::string FieldCoding::fullEncodeFunction(int type, bool bigendian)
{
    if(contains(typeSigNames[type], "float"))
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
std::string FieldCoding::floatEncodeFunction(int type, bool bigendian)
{
    std::string endian;

    if(bigendian)
        endian = "Be";
    else
        endian = "Le";

    std::string function = encodeSignature(type, bigendian) + "\n";
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
        function += "    uint" + std::to_string(8*typeSizes[type]) + "To" + endian + "Bytes(field.integerValue, bytes, index);\n";
    }
    else if(typeSizes[type] == 3)
    {
        function += "    uint24To" + endian + "Bytes(float32ToFloat24(number, sigbits), bytes, index);\n";
    }
    else
        function += "    uint16To" + endian + "Bytes(float32ToFloat16(number, sigbits), bytes, index);\n";

    function += "}\n";

    return function;

}// FieldCoding::floatEncodeFunction


/*!
 * Generate the full encode function output, excluding the comment, for integer types
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return the function as a string
 */
std::string FieldCoding::integerEncodeFunction(int type, bool bigendian)
{
    std::string function = encodeSignature(type, bigendian) + "\n";
    function += "{\n";

    if(typeSizes[type] == 1)
        return "// ";
    else
    {
        function += "    // increment byte pointer for starting point\n";

        std::string opt;
        if(bigendian)
        {
            function += "    bytes += (*index) + " + std::to_string(typeSizes[type]-1) + ";\n";
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
        function += "    (*index) += " + std::to_string(typeSizes[type]) + ";\n";

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
header.setFileComment(R"(fielddecode provides routines to pull numbers from a byte stream.

fielddecode provides routines to pull numbers in local memory layout from
a big or little endian byte stream. It is the opposite operation from the
routines contained in fieldencode.h

When compressing unsigned numbers (for example 32-bits to 16-bits) the most
signficant bytes are discarded and the only requirement is that the value of
the number fits in the smaller width. When going the other direction the
most significant bytes are simply set to 0x00. However signed two's
complement numbers are more complicated.

If the signed value is a positive number that fits in the range then the
most significant byte will be zero, and we can discard it. If the signed
value is negative (in two's complement) then the most significant bytes are
0xFF and again we can throw them away. See the example below

32-bit +100 | 16-bit +100 | 8-bit +100
 0x00000064 |      0x0064 |       0x64 <-- notice most significant bit clear

32-bit -100 | 16-bit -100 | 8-bit -100
 0xFFFFFF9C |      0xFF9C |       0x9C <-- notice most significant bit set

The signed complication comes when going the other way. If the number is
positive setting the most significant bytes to zero is correct. However
if the number is negative the most significant bytes must be set to 0xFF.
This is the process of sign-extension. Typically this is handled by the
compiler. For example if a int16_t is assigned to an int32_t the compiler
(or the processor instruction) knows to perform the sign extension. However
in our case we can decode signed 24-bit numbers (for example) which are
returned to the caller as int32_t. In this instance fielddecode performs the
sign extension.)");

    header.write("\n");
    header.write("#define __STDC_CONSTANT_MACROS\n");
    header.write("#include <stdint.h>\n");

    if(support.supportbool)
        header.writeIncludeDirective("stdbool.h", "", true);

    header.makeLineSeparator();

header.write(R"(//! Decode a null terminated string from a byte stream
void stringFromBytes(char* string, const uint8_t* bytes, int* index, int maxLength, int fixedLength);

//! Copy an array of bytes from a byte stream without changing the order.
void bytesFromBeBytes(uint8_t* data, const uint8_t* bytes, int* index, int num);

//! Copy an array of bytes from a byte stream while reversing the order.
void bytesFromLeBytes(uint8_t* data, const uint8_t* bytes, int* index, int num);)");


    if(support.int64)
    {
        header.makeLineSeparator();
        header.write("#ifdef UINT64_MAX\n");
    }

    for(int type = 0; type < (int)typeNames.size(); type++)
    {
        if(support.int64 && (type > 0))
        {
            if((typeSizes.at(type) == 4) && (typeSizes.at(type-1) == 5))
                header.write("\n#endif // UINT64_MAX\n");
        }

        if(typeSizes[type] != 1)
        {
            header.makeLineSeparator();
            header.write("//! " + briefDecodeComment(type, true) + "\n");
            header.write(decodeSignature(type, true) + ";\n");

            header.makeLineSeparator();
            header.write("//! " + briefDecodeComment(type, false) + "\n");
            header.write(decodeSignature(type, false) + ";\n");
        }
        else
        {
            header.makeLineSeparator();
            header.write("//! " + briefDecodeComment(type, true) + "\n");
            header.write(decodeSignature(type, true) + "\n");
        }


    }// for all input types

    header.makeLineSeparator();

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

    source.makeLineSeparator();

// Raw string magic
source.write(R"(/*!
 * Decode a null terminated string from a byte stream
 * \param string receives the deocded null-terminated string.
 * \param bytes is a pointer to the byte stream to be decoded.
 * \param index gives the location of the first byte in the byte stream, and
 *        will be incremented by the number of bytes decoded when this function
 *        is complete.
 * \param maxLength is the maximum number of bytes that can be decoded.
 *        maxLength includes the null terminator, which is always applied.
 * \param fixedLength should be 1 to force the number of bytes decoded to be
 *        exactly equal to maxLength.
 */
void stringFromBytes(char* string, const uint8_t* bytes, int* index, int maxLength, int fixedLength)
{
    int i;

    // increment byte pointer for starting point
    bytes += *index;

    for(i = 0; i < maxLength - 1; i++)
    {
        if(bytes[i] == 0)
            break;
        else
            string[i] = (char)bytes[i];
    }

    // Make sure we include null terminator
    string[i++] = 0;

    if(fixedLength)
        (*index) += maxLength;
    else
        (*index) += i;

}// stringFromBytes


/*!
 * Copy an array of bytes from a byte stream without changing the order.
 * \param data receives the copied bytes
 * \param bytes is a pointer to the byte stream to be copied from.
 * \param index gives the location of the first byte in the byte stream, and
 *        will be incremented by num when this function is complete.
 * \param num is the number of bytes to copy
 */
void bytesFromBeBytes(uint8_t* data, const uint8_t* bytes, int* index, int num)
{
    // increment byte pointer for starting point
    bytes += (*index);

    // Increment byte index to indicate number of bytes copied
    (*index) += num;

    // Copy the bytes without changing the order
    while(num > 0)
    {
        *(data++) = *(bytes++);
        num--;
    }

}// bytesFromBeBytes


/*!
 * Copy an array of bytes from a byte stream, reversing the order.
 * \param data receives the copied bytes
 * \param bytes is a pointer to the byte stream to be copied.
 * \param index gives the location of the first byte in the byte stream, and
 *        will be incremented by num when this function is complete.
 * \param num is the number of bytes to copy
 */
void bytesFromLeBytes(uint8_t* data, const uint8_t* bytes, int* index, int num)
{
    // increment byte pointer for starting point
    bytes += (*index);

    // Increment byte index to indicate number of bytes copied
    (*index) += num;

    // To encode as "little endian bytes", (a nonsensical statement), reverse the byte order
    bytes += (num - 1);

    // Copy the bytes, reversing the order
    while(num > 0)
    {
        *(data++) = *(bytes--);
        num--;
    }

}// bytesFromLeBytes)");

    if(support.int64)
    {
        source.makeLineSeparator();
        source.write("#ifdef UINT64_MAX\n");
    }

    for(int type = 0; type < (int)typeNames.size(); type++)
    {
        if(support.int64 && (type > 0))
        {
            if((typeSizes.at(type) == 4) && (typeSizes.at(type-1) == 5))
                source.write("#endif // UINT64_MAX\n");
        }

        if(typeSizes[type] != 1)
        {
            // big endian unsigned
            source.makeLineSeparator();
            source.write(fullDecodeComment(type, true) + "\n");
            source.write(fullDecodeFunction(type, true) + "\n");

            // little endian unsigned
            source.makeLineSeparator();
            source.write(fullDecodeComment(type, false) + "\n");
            source.write(fullDecodeFunction(type, false) + "\n");
        }

    }// for all input types

    source.makeLineSeparator();

    return source.flush();

}// FieldCoding::generateDecodeSource


/*!
 * Create the brief decode function comment, without doxygen decorations
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return The string that represents the one line function comment.
 */
std::string FieldCoding::briefDecodeComment(int type, bool bigendian)
{
    std::string name = getReadableTypeName(type);

    if(typeSizes[type] == 1)
    {
        // No endian concerns if using only 1 byte
        return std::string("Decode a " + name + " from a byte stream.");
    }
    else
    {
        std::string endian;

        if(bigendian)
            endian = "big";
        else
            endian = "little";

        return std::string("Decode a " + name + " from a " + endian + " endian byte stream.");

    }// If multi-byte

}// FieldCoding::briefDecodeComment


/*!
 * Create the full decode function comment, with doxygen decorations
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return The string that represents the full multi-line function comment.
 */
std::string FieldCoding::fullDecodeComment(int type, bool bigendian)
{
    std::string comment= ("/*!\n");

    comment += ProtocolParser::outputLongComment(" *", briefDecodeComment(type, bigendian)) + "\n";
    comment += " * \\param bytes is a pointer to the byte stream which contains the encoded data.\n";
    comment += " * \\param index gives the location of the first byte in the byte stream, and\n";
    comment += " *        will be incremented by " + std::to_string(typeSizes[type]) + " when this function is complete.\n";

    if(contains(typeSigNames[type], "float24") || contains(typeSigNames[type], "float16"))
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
std::string FieldCoding::decodeSignature(int type, bool bigendian)
{
    std::string endian;

    // No endian concerns if using only 1 byte
    if(typeSizes[type] > 1)
    {
        if(bigendian)
            endian = "Be";
        else
            endian = "Le";

        if(contains(typeSigNames[type], "float24") || contains(typeSigNames[type], "float16"))
            return std::string(typeNames[type] + " " + typeSigNames[type] + "From" + endian + "Bytes(const uint8_t* bytes, int* index, int sigbits)");
        else
            return std::string(typeNames[type] + " " + typeSigNames[type] + "From" + endian + "Bytes(const uint8_t* bytes, int* index)");
    }
    else
    {
        return std::string("#define " + typeSigNames[type] + "FromBytes(bytes, index) (" + typeNames[type] + ")((bytes)[(*(index))++])");
    }

}// FieldCoding::decodeSignature


/*!
 * Generate the full decode function output, excluding the comment
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return the function as a string
 */
std::string FieldCoding::fullDecodeFunction(int type, bool bigendian)
{
    if(contains(typeSigNames[type], "float"))
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
std::string FieldCoding::floatDecodeFunction(int type, bool bigendian)
{
    std::string endian;

    if(bigendian)
        endian = "Be";
    else
        endian = "Le";

    std::string function = decodeSignature(type, bigendian) + "\n";
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
        function += "    field.integerValue = uint" + std::to_string(8*typeSizes[type]) + "From" + endian + "Bytes(bytes, index);\n";
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
        function += "    return float24ToFloat32(uint24From" + endian + "Bytes(bytes, index), sigbits);\n";
    }
    else
        function += "    return float16ToFloat32(uint16From" + endian + "Bytes(bytes, index), sigbits);\n";

    function += "}\n";

    return function;

}// FieldCoding::floatDecodeFunction


/*!
 * Generate the full decode function output, excluding the comment, for integer types
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return the function as a string
 */
std::string FieldCoding::integerDecodeFunction(int type, bool bigendian)
{
    std::string function = decodeSignature(type, bigendian) + "\n";
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
            function += "    bytes += (*index) + " + std::to_string(typeSizes[type]-1) + ";\n";

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
        function += "    (*index) += " + std::to_string(typeSizes[type]) + ";\n";

        function += "\n";

        if(signextend)
            function += "    return (number ^ m) - m;\n";
        else
            function += "    return number;\n";

    }// if multi-byte fields

    function += "}\n";

    return function;

}// FieldCoding::integerDecodeFunction

