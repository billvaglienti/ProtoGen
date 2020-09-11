#include "fieldcoding.h"
#include "protocolparser.h"

#include "stdbool.h"
#include <iostream>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <cstdio>

#include <QString>
#include <QFile>
#include <QTextStream>


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
    FieldcodingInterface* generator = nullptr;

    if ( support.language == support.python_language)
        generator = new PythonCoding(support, typeNames, typeSigNames, typeSizes, typeUnsigneds);
    else
        generator = new CandCppCoding(support, typeNames, typeSigNames, typeSizes, typeUnsigneds);

    if ( generator == nullptr)
          return false;

    if(generator->generateEncodeHeader(header))
    {
        fileNameList.push_back(header.fileName());
        filePathList.push_back(header.filePath());
    }
    else
    {
        delete generator;
        return false;
    }
    if(generator->generateEncodeSource(source))
    {
        fileNameList.push_back(source.fileName());
        filePathList.push_back(source.filePath());
    }
    else
    {
        delete generator;
        return false;
    }

    if(generator->generateDecodeHeader(header))
    {
        fileNameList.push_back(header.fileName());
        filePathList.push_back(header.filePath());
    }
    else
    {
        delete generator;
        return false;
    }

    if(generator->generateDecodeSource(source))
    {
        fileNameList.push_back(source.fileName());
        filePathList.push_back(source.filePath());
    }
    else
    {
        delete generator;
        return false;
    }

    delete generator;
    return true;
}


CandCppCoding::CandCppCoding(const ProtocolSupport &sup, const std::vector<std::string> &typeNames, const std::vector<std::string> &typeSigNames,
                           const std::vector<int> &typeSizes, const std::vector<bool> &typeUnsigneds) :
    FieldcodingInterface(sup, typeNames, typeSigNames, typeSizes, typeUnsigneds)
{
}

/*!
 * Generate the header file for protocol array scaling
 * \param pointer to the protocol header file where the output is written
 * \return true if the file is generated.
 */
bool CandCppCoding::generateEncodeHeader(ProtocolHeaderFile &header)
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

}// CandCppCoding::generateEncodeHeader


/*!
 * Generate the source file for protocols caling
 *  \param pointer to the protocol source file where the output is written
 * \return true if the file is generated.
 */
bool CandCppCoding::generateEncodeSource(ProtocolSourceFile &source)
{
    source.setModuleNameAndPath("fieldencode", support.outputpath, support.language);

    if(support.specialFloat)
        source.writeIncludeDirective("floatspecial");

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

}// CandCppCoding::generateEncodeSource


/*!
 * Generate the header file for protocols caling
 * \param pointer to the protocol header file where the output is written
 * \return true if the file is generated.
 */
bool CandCppCoding::generateDecodeHeader(ProtocolHeaderFile &header)
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

}// CandCppCoding::generateDecodeHeader


/*!
 * Generate the source file for protocols caling
 * \param pointer to the protocol source file where the output is written
 * \return true if the file is generated.
 */
bool CandCppCoding::generateDecodeSource(ProtocolSourceFile &source)
{
    source.setModuleNameAndPath("fielddecode", support.outputpath, support.language);

    if(support.specialFloat)
        source.writeIncludeDirective("floatspecial");

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

}// CandCppCoding::generateDecodeSource


/*!
 * Get a human readable type name like "unsigned 3 byte integer".
 * \param type is the type enumeration
 * \return the human readable type name
 */
std::string CandCppCoding::getReadableTypeName(int type)
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

}// CandCppCoding::getReadableTypeName


/*!
 * Create the brief function comment, without doxygen decorations
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return The string that represents the one line function comment.
 */
std::string CandCppCoding::briefEncodeComment(int type, bool bigendian)
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

}// CandCppCoding::briefEncodeComment


/*!
 * Create the full encode function comment, with doxygen decorations
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return The string that represents the full multi-line function comment.
 */
std::string CandCppCoding::fullEncodeComment(int type, bool bigendian)
{
    std::string comment = "/*!\n";

    comment += ProtocolParser::outputLongComment(" * ", briefEncodeComment(type, bigendian)) + "\n";
    comment += " * \\param number is the value to encode.\n";
    comment += " * \\param bytes is a pointer to the byte stream which receives the encoded data.\n";
    comment += " * \\param index gives the location of the first byte in the byte stream, and\n";
    comment += " *        will be incremented by " + std::to_string(typeSizes[type]) + " when this function is complete.\n";
    if(contains(typeSigNames[type], "float24") || contains(typeSigNames[type], "float16"))
        comment += " * \\param sigbits is the number of bits to use in the significand of the float.\n";
    comment += " */";

    return comment;
}// CandCppCoding::fullEncodeComment

/*!
 * Create the one line function signature, without a trailing semicolon
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return The string that represents the function signature, without a trailing semicolon
 */
std::string CandCppCoding::encodeSignature(int type, bool bigendian)
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

}// CandCppCoding::encodeSignature


/*!
 * Generate the full encode function output, excluding the comment
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return the function as a string
 */
std::string CandCppCoding::fullEncodeFunction(int type, bool bigendian)
{
    if(contains(typeSigNames[type], "float"))
        return floatEncodeFunction(type, bigendian);
    else
        return integerEncodeFunction(type, bigendian);
}// CandCppCoding::fullEncodeFunction

/*!
 * Generate the full encode function output, excluding the comment, for floating point types
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return the function as a string
 */
std::string CandCppCoding::floatEncodeFunction(int type, bool bigendian)
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

}// CandCppCoding::floatEncodeFunction


/*!
 * Generate the full encode function output, excluding the comment, for integer types
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return the function as a string
 */
std::string CandCppCoding::integerEncodeFunction(int type, bool bigendian)
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

}// CandCppCoding::integerEncodeFunction

/*!
 * Create the brief decode function comment, without doxygen decorations
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return The string that represents the one line function comment.
 */
std::string CandCppCoding::briefDecodeComment(int type, bool bigendian)
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

} // CandCppCoding::briefDecodeComment


/*!
 * Create the full decode function comment, with doxygen decorations
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return The string that represents the full multi-line function comment.
 */
std::string CandCppCoding::fullDecodeComment(int type, bool bigendian)
{
    std::string comment= ("/*!\n");

    comment += ProtocolParser::outputLongComment(" * ", briefDecodeComment(type, bigendian)) + "\n";
    comment += " * \\param bytes is a pointer to the byte stream which contains the encoded data.\n";
    comment += " * \\param index gives the location of the first byte in the byte stream, and\n";
    comment += " *        will be incremented by " + std::to_string(typeSizes[type]) + " when this function is complete.\n";

    if(contains(typeSigNames[type], "float24") || contains(typeSigNames[type], "float16"))
        comment += " * \\param sigbits is the number of bits to use in the significand of the float.\n";

    comment += " * \\return the number decoded from the byte stream\n";
    comment += " */";

    return comment;
} //CandCppCoding::fullDecodeComment

/*!
 * Create the one line decode function signature, without a trailing semicolon
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return The string that represents the function signature, without a trailing semicolon
 */
std::string CandCppCoding::decodeSignature(int type, bool bigendian)
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

}// CandCppCoding::decodeSignature


/*!
 * Generate the full decode function output, excluding the comment
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return the function as a string
 */
std::string CandCppCoding::fullDecodeFunction(int type, bool bigendian)
{
    if(contains(typeSigNames[type], "float"))
        return floatDecodeFunction(type, bigendian);
    else
        return integerDecodeFunction(type, bigendian);
} // CandCppCoding::fullDecodeComment

/*!
 * Generate the full decode function output, excluding the comment, for floating point types
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return the function as a string
 */
std::string CandCppCoding::floatDecodeFunction(int type, bool bigendian)
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

}// CandCppCoding::floatDecodeFunction


/*!
 * Generate the full decode function output, excluding the comment, for integer types
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return the function as a string
 */
std::string CandCppCoding::integerDecodeFunction(int type, bool bigendian)
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

}// CandCppCoding::integerDecodeFunction


//! FieldcodingInterface Constructor
FieldcodingInterface::FieldcodingInterface(const ProtocolSupport &sup, const std::vector<std::string> &typeNames, const std::vector<std::string> &typeSigNames,
       const std::vector<int> &typeSizes, const std::vector<bool> &typeUnsigneds) :
            typeNames(typeNames),
            typeSigNames(typeSigNames),
            typeSizes(typeSizes),
            typeUnsigneds(typeUnsigneds),
            support(sup)

{
}

//! PythonCoding Constructor
PythonCoding::PythonCoding(const ProtocolSupport &sup, const std::vector<std::string> &typeNames, const std::vector<std::string> &typeSigNames,
                           const std::vector<int> &typeSizes, const std::vector<bool> &typeUnsigneds) :
    FieldcodingInterface(sup, typeNames, typeSigNames, typeSizes, typeUnsigneds), specialSize(false)
{
}

/*!
 * Generate the encode source file for protocols caling
 *  \param pointer to the protocol source file where the output is written
 * \return true if the file is generated.
 */
bool PythonCoding::generateEncodeSource(ProtocolSourceFile &source)
{
    source.setModuleNameAndPath("fieldencode", support.outputpath, support.language);

    source.writeIncludeDirective("struct");
    source.writeIncludeDirective("sys", "regular");

    if(support.specialFloat)
        source.writeIncludeDirective("floatspecial");


    if(support.int64)
    {
        source.makeLineSeparator();
        source.write("# supporting 64 bit sizes\n\n"); // TODO: deal with max 64 case properly
    }

    for(int i = 0; i < (int)typeNames.size(); i++)
    {
        if(support.int64 && (i > 0))
        {
            if((typeSizes.at(i) == 4) && (typeSizes.at(i-1) == 5))
               source.write("# end supporting 64 bit sizes\n");
        }

        if(typeSizes[i] != 1)
        {
            // big endian
            source.makeLineSeparator();
            source.write(fullPyEncodeFunction(i, true) + "\n");

            // little endian
            source.makeLineSeparator();
            source.write(fullPyEncodeFunction(i, false) + "\n");
        }
    }

    return source.flush();


} // PythonCoding::generateEncodeSource


/*!
 * Return since python does not have header files
 *  \param pointer to the protocol header file where the output is written
 * \return true if the file is generated.
 */
bool PythonCoding::generateEncodeHeader(ProtocolHeaderFile &header)
{
    (void) header;
    return true;
} // PythonCoding::generateEncodeHeader


/*!
 * Generate the decode source file for protocols caling
 *  \param pointer to the protocol source file where the output is written
 * \return true if the file is generated.
 */
bool PythonCoding::generateDecodeSource(ProtocolSourceFile &source)
{
    source.setModuleNameAndPath("fielddecode", support.outputpath, support.language);

    source.writeIncludeDirective("struct");

    if(support.specialFloat)
        source.writeIncludeDirective("floatspecial");


    if(support.int64)
    {
        source.makeLineSeparator();

        /// TODO: deal with max 64 case properly
        source.write("# supporting 64 bit sizes\n\n");
    }

    for(int i = 0; i < (int)typeNames.size(); i++)
    {
        if(support.int64 && (i > 0))
        {
            if((typeSizes.at(i) == 4) && (typeSizes.at(i-1) == 5))
               source.write("# end supporting 64 bit sizes\n");
        }

        if(typeSizes[i] != 1)
        {
            // big endian
            source.makeLineSeparator();
            source.write(fullPyDecodeFunction(i, true) + "\n");

            // little endian
            source.makeLineSeparator();
            source.write(fullPyDecodeFunction(i, false) + "\n");
        }

    }

    return source.flush();

} // PythonCoding::generateDecodeSource


/*!
 * Return since python does not have header files
 *  \param pointer to the protocol source file where the output is written
 * \return true if the file is generated.
 */
bool PythonCoding::generateDecodeHeader(ProtocolHeaderFile &header)
{
    (void) header;
    return true;
} // PythonCoding::generateDecodeHeader


/*!
 * Create the brief function comment, without doxygen decorations
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return The string that represents the one line function comment.
 */
std::string PythonCoding::briefEncodeComment(int type, bool bigendian)
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

} // PythonCoding::briefEncodeComment

/*!
 * Create the brief decode function comment, without doxygen decorations
 * \param type is the enumerator for the type.
 * \param bigendian should be true if the function outputs big endian byte order.
 * \return The string that represents the one line function comment.
 */
std::string PythonCoding::briefDecodeComment(int type, bool bigendian)
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

} // PythonCoding::briefDecodeComment

/*!
 * Get a human readable type name like "unsigned 3 byte integer".
 * \param type is the type enumeration
 * \return the human readable type name
 */
std::string PythonCoding::getReadableTypeName(int type)
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

} // PythonCoding::getReadableTypeName

/*!
 * Generate the function name
 * \param type is the type enumeration
 * \param bigendian is a boolian which determines big or little endian
 * \return function name
 */
std::string PythonCoding::pySignature(int type, bool bigendian, bool encode)
{
    std::string endian    = "";
    std::string to_from    = "";

    // determine the endian of the type
    if (bigendian)
        endian = "Be";
    else
        endian = "Le";

    // determine if encoding or decoding
    if (encode)
        to_from = "To";
    else
        to_from = "From";

    return typeSigNames[type] + to_from + endian + "Bytes";

} // PythonCoding::pySignature

/*!
 * generate the format string for based on size and signed value
 * \param type is the type enumeration
 * \param bigendian is a boolian which determines big or little endian
 * \return format string
 */
std::string PythonCoding::pyFormat(int type, bool bigendian)
{
    //    char      - b     longlong  - q
    //    uchar     - B     ulonglong - Q
    //    short     - h     float     - f
    //    ushort    - H     double    - d
    //    int       - i     bool      - ?
    //    uint      - I


    std::string format = "";
    std::string letter = "";
    std::string endian = "";

    std::string quote  = "'";

    bool isUnsigned  = typeUnsigneds[type];
    int size         = typeSizes[type];
    int coeficient   = 0;

    if (bigendian)
        endian = ">";
    else
        endian = "<";

    if(contains(typeSigNames[type], "float"))
    {
        if (size == 8)
            letter = "d";
        if (size == 4)
            letter = "f";
    }
    else
    {
        // integer
        switch (size)
        {
            case 1:
                letter = "b";
                break;
            case 2:
                letter = "h";
                break;
            case 4:
                letter = "i";
                break;
            case 8:
                letter = "q";
                break;
            default:
                letter = "B";
                coeficient = size;
                specialSize = true;
                break;
        }
    }

    // Check if the type is signed - unsigned is capital letters
    if (isUnsigned)
        letter = toUpper(letter);

    // check that we have a valid type or just bytes
    if ( coeficient != 0)
        format = quote + endian + std::to_string(coeficient) + letter + quote;

    else
        format = quote + endian + letter + quote;

    // return the string with the endian symbol and the letter corresponding to type
    return format;

} // PythonCoding::pyFormat

/*!
 * Generate the type promoted format string for non standard sizes
 * \param type is the type enumeration
 * \param bigendian is a boolian which determines big or little endian
 * \param fullsize is the type promotion of non standard sizes
 * \return type promoted format string
 */
std::string PythonCoding::secondFormat(int fullSize, bool bigendian, bool unSigned)
{
    std::string format = "";
    std::string letter = "";
    std::string endian = "";
    std::string quote  ="'";


    if (bigendian)
        endian = ">";
    else
        endian = "<";

    switch (fullSize)
    {
        case 1:
            letter = "b";
            break;
        case 2:
            letter = "h";
            break;
        case 4:
            letter = "i";
            break;
        case 8:
            letter = "q";
            break;
        default:
            break;
    }

    // Check if the type is signed - unsigned is capital letters
    if (unSigned)
        letter = toUpper(letter);

    format = quote + endian + letter + quote;
    return format;
} // PythonCoding::secondFormat


/*!
 * generate the encode functions and all the peices
 * \param type is the type enumeration
 * \param bigendian is a boolian which determines big or little endian
 * \return entire encode function for size and endian
 */
std::string PythonCoding::fullPyEncodeFunction(int type, bool bigendian)
{
    bool encode = true;

    std::string signature = pySignature(type, bigendian, encode);
    std::string comment   = pyEncodeComment(type, bigendian);
    std::string format    = pyFormat(type, bigendian);

    std::string function = pyEncodeFunction(signature, comment, format, type, bigendian);

    return function;
} // PythonCoding::fullPyEncodeFunction


/*!
 * Write the encode python docstring including summary, args, and return
 * \param type is the type enumeration
 * \param bigendian is a boolian which determines big or little endian
 * \return docstring
 */
std::string PythonCoding::pyEncodeComment(int type, bool bigendian)
{
    std::string summary = briefEncodeComment(type, bigendian) + "\n\n";
    std::string    args = R"(    Args:
        number (int): the value to encode
        byteA  (byteArray): The byte stream where the data is encoded
        index  (int): Gives the location of the first byte in the byte stream)";

    if(contains(typeSigNames[type], "float24") || contains(typeSigNames[type], "float16"))
        args += "\n        sigbits (int): the number of bits to use in the significand of the float.\n\n";
    else
        args += "\n\n";

    std::string returns = R"(    Returns:
        index (int): The incremented index increased by )";
        returns += std::to_string(typeSizes[type]) + "\n"; // NOTE: incorrect cause of no xml

    std::string quotes = R"(    """)";
    std::string comment = quotes + summary + args + returns + quotes + "\n";

    return comment;

} // PythonCoding::pyEncodeComment


/*!
 * Generate the encode function
 * \param type is the type enumeration
 * \param bigendian is a boolian which determines big or little endian
 * \param signature is the function name
 * \param comment is the docstring for the python function
 * \param format is the string that specifies the type to be packed orunpacked
 * \return string with the encode function
 */
std::string PythonCoding::pyEncodeFunction(std::string signature, std::string comment, std::string format, int type, bool bigendian)
{
    std::string max = "0";
    std::string min = "0";

    if (typeUnsigneds[type])
    {

        switch(typeSizes[type])
        {
        default:
        case 1: max = "255"; break;
        case 2: max = "65535"; break;
        case 3: max = "16777215"; break;
        case 4: max = "4294967295"; break;
        case 5: max = "1099511627775"; break;
        case 6: max = "281474976710655"; break;
        case 7: max = "72057594037927935"; break;
        case 8: max = "18446744073709551615"; break;
        }
    }
    else
    {

        switch(typeSizes[type])
        {
        default:
        case 1: max = "127"; break;
        case 2: max = "32767"; break;
        case 3: max = "8388607"; break;
        case 4: max = "2147483647"; break;
        case 5: max = "549755813887"; break;
        case 6: max = "140737488355327"; break;
        case 7: max = "36028797018963967"; break;
        case 8: max = "sys.maxsize"; break;
        }

        switch(typeSizes[type])
        {
        default:
        case 1: min = "(-128)"; break;
        case 2: min = "(-32768)"; break;
        case 3: min = "(-8388608)"; break;
        case 4: min = "(-2147483648)"; break;
        case 5: min = "(-549755813888)"; break;
        case 6: min = "(-140737488355328)"; break;
        case 7: min = "(-36028797018963968)"; break;
        case 8: min = "(-sys.maxsize)"; break;       // 9223372036854775807
        }

    }

    std::string tab = "    ";
    std::string bytes = std::to_string(typeSizes[type]);
    std::string function_args = "byteA: bytearray, index: int, number: ";
    std::string pack_args = "byteA, index, number";


    std::string function = "def " + signature + "(" + function_args;

    if (contains(typeSigNames[type], "float"))
    {
        if (contains(typeSigNames[type], "float16") or contains(typeSigNames[type], "float24"))
        {
            function += "float, sigbits: int) -> int:\n" + comment;
            function = pyEncodeSpecialFloat(function, type, bigendian);
            return function;
        }
        else
        {
            function += "float) -> int:\n";
            function += comment;
        }
    }
    else
    {

        function += "int) -> int:\n" + comment;
        function += tab + "if number > " + max + ":\n" + tab + tab + "number = " + max + "\n";
        function += tab + "if number < " + min + ":\n" + tab + tab + "number = " + min + "\n\n";
    }

    if (specialSize) {
        specialSize = false;
        function = pyEncodeSpecialSize(function, type, bigendian);
        return function;
    }

    function += tab + "pack_into(" + format + ", "  + pack_args + ")\n";
    function += tab + "index += " + bytes + "\n" + tab + "return index\n\n";

    return function;

} // PythonCoding::pyEncodeFunction


/*!
 * encode the non standard size floats including 16, 24, 40, 48, and 56
 * \param type is the type enumeration
 * \param bigendian is a boolian which determines big or little endian
 * \param format is the string that specifies the type to be packed orunpacked
 * \param function is the whole function up to this point
 * \return special size function based on size and endian
 */
std::string PythonCoding::pyEncodeSpecialSize(std::string function, int type, bool bigendian)
{

    std::string sign   = "";
    std::string endian = "";

    // Determine the if the type is signed
    if(typeUnsigneds[type])
        sign = "False";
    else
        sign = "True";

    // determine if the type is be/le
    if (bigendian)
        endian = "'big', ";
    else
        endian = "'little', ";


    // turn the number into a bytes
    function += "    n_byte = number.to_bytes(" + std::to_string(typeSizes[type]) + ", " + endian + "signed=" + sign + ")\n\n";


    // assign the bytes from the number to the bytearray
    for (int i = 0; i < typeSizes[type]; i++)
    {
        function += "    byteA[index + " + std::to_string(i) + "] = n_byte[" + std::to_string(i) + "]\n";
    }

    // update the index
    function += "    index += " + std::to_string(typeSizes[type]) + "\n    return index\n\n";

    return function;
} // PythonCoding::pyEncodeSpecialSize


/*!
 * encode the non standard size floats including 16 and 24
 * \param type is the type enumeration
 * \param bigendian is a boolian which determines big or little endian
 * \param function is the whole function up to this point
 * \return special float function based on size and endian
 */
std::string PythonCoding::pyEncodeSpecialFloat(std::string function, int type, bool bigendian)
{
    std::string endian = "";

    if (bigendian)
        endian = "Be";
    else
        endian = "Le";

    if(typeSizes[type] == 3)
    {
        function += "    index = uint24To" + endian + "Bytes(byteA, index, float32ToFloat24(number, sigbits))\n";
        function += "    return index\n\n";
        return function;
    }
    else
    {
        function += "    index = uint16To" + endian + "Bytes(byteA, index, float32ToFloat16(number, sigbits))\n";
        function += "    return index\n\n";
        return function;
    }

} // PythonCoding::pyEncodeSpecialFloat


/*!
 * Generate the whole decode funcion and the peices
 * \param type is the type enumeration
 * \param bigendian is a boolian which determines big or little endian
 * \return decode function based of size and endian
 */
std::string PythonCoding::fullPyDecodeFunction(int type, bool bigendian)
{
    bool encode = false;

    std::string signature = pySignature(type, bigendian, encode);
    std::string comment   = pyDecodeComment(type, bigendian);
    std::string format    = pyFormat(type, bigendian);

    std::string function = pyDecodeFunction(signature, comment, format, type, bigendian);

    return function;
} // PythonCoding::fullPyDecodeFunction


/*!
 * Creates the decode function from its peices
 * \param type is the type enumeration
 * \param bigendian is a boolian which determines big or little endian
 * \param signature is the function name
 * \param comment is the docstring for the python function
 * \param format is the string that specifies the type to be packed orunpacked
 * \return decode function
 */
std::string PythonCoding::pyDecodeFunction(std::string signature, std::string comment, std::string format, int type, bool bigendian)
{
    std::string function_args = "byteA: bytearray, index: int";
    std::string unpack_args   = format + ", byteA, offset=index[0]";

    std::string function = "def " + signature + "(" + function_args;

    if (contains(typeSigNames[type], "float"))
    {

        if (contains(typeSigNames[type], "float16") or contains(typeSigNames[type], "float24"))
        {
            function += ", sigbits: int) -> float:\n" + comment;
            function = pyDecodeSpecialFloat(function, type, bigendian);
            return function;
        }
        else
            function += ") -> float:\n" + comment;
    }
    else
        function += ") -> int:\n" + comment;

    if (specialSize)
    {
        specialSize = false;
        function = pyDecodeSpecialSize(function, type, bigendian);
        return function;
    }

    function += "    number = unpack_from(" + unpack_args + ")\n";
    function += "    index[0] = index[0] + " + std::to_string(typeSizes[type]) + "\n\n";

    if(contains(typeSigNames[type], "float"))
    {
        std::string bits = std::to_string(typeSizes[type] * 8);

        function += "    # Verify that the unpacked float is valid\n";
        function += "    if isFloat" + bits + "Valid(float" + bits + "ToInt(number[0])) is True:\n";
        function += "        return number[0]\n    else:\n        return 0\n\n\n";
        return function;
    }

    function += "    return number[0]\n\n\n";
    return function;


} // PythonCoding::pyDecodeFunction


/*!
 * Decode non standard size types including 16, 24, 40, 48, and 56
 * \param type is the type enumeration
 * \param bigendian is a boolian which determines big or little endian
 * \param format is the string that specifies the type to be packed orunpacked
 * \param function is the whole function up to this point
 * \return decode function for non standard size
 */
std::string PythonCoding::pyDecodeSpecialSize(std::string function, int type, bool bigendian)
{
    int size = typeSizes[type];
    int fullSize;
    std::string format2 = "";

    if (size == 3)
        fullSize = 4;               ///TODO: check that if fits for the usigned case or promote to 8
    if ( size > 4 and size < 8)
        fullSize = 8;

    // determine the format of the in memory type
    format2 = secondFormat(fullSize, bigendian, typeUnsigneds[type]);

    std::string s_size = std::to_string(size);
    std::string s_fullSize = std::to_string(fullSize);
    std::string pad = "0";
    std::string extention_byte;

    function += "    offset = index[0]\n\n";
    function += "    # declare enough bytes for the next largest standard size\n";
    function += "    full_bytes = bytearray(" + s_fullSize + ")\n\n";

    // if signed perform sign extenstion
    if(!typeUnsigneds[type]) {

        // based on the endian determine if the sign is the 0th byte or the final byte extracted
        if (bigendian)
            extention_byte = "0";
        else
            extention_byte = "offset + " + std::to_string(size - 1);

        function += "    # determine byte extesion value\n    pad = 0\n    if (byteA[" + extention_byte + "] >> 7) == 1:\n";
        function += "        pad = 255\n\n";
        pad = "pad";
    }

    // extract the bytes directly from the bytearray into a fullsize sign extended temporary array
    if (bigendian)
    {
        function += "    # transfer the bytes from the bytearray into the fullsized type\n";
        for ( int i = 0; i < fullSize; i++)
        {
            if ( i < (fullSize - size))
                function += "    full_bytes[" + std::to_string(i) + "] = " + pad + "\n";
            else
            {
                function += "    full_bytes[" + std::to_string(i) + "] = byteA[offset + ";
                function += std::to_string(i - (fullSize - size)) + "]\n";
            }
        }
        function += "\n";
    }
    else
    {
        function += "    # transfer the bytes from the bytearray into the fullsized type\n";
        for ( int i = fullSize - 1; i >= 0; i-- )
        {
            if ( i >= size)
                function += "    full_bytes[" + std::to_string(i) + "] = " + pad + "\n";
            else
            {
                function += "    full_bytes[" + std::to_string(i) + "] = byteA[offset + " + std::to_string(i) + "]\n";
            }
        }
        function += "\n";
    }

    // turn the full sized temporary array bytes into a number
    function += "    # unpack the full sized value\n    number = unpack_from(" + format2 + ", full_bytes, 0)\n\n";
    function += "    # update the index and return the first element of the tuple\n    index[0] = index[0] + " + s_size + "\n    return number[0]\n";

    return function;

} // PythonCoding::pyDecodeSpecialSize


/*!
 * Decode the non standard size floats including 16 and 24
 * \param type is the type enumeration
 * \param bigendian is a boolian which determines big or little endian
 * \param function is the whole function up to this point
 * \return decode function for non standard float sizes
 */
std::string PythonCoding::pyDecodeSpecialFloat(std::string function, int type, bool bigendian)
{
    std::string endian = "";

    if (bigendian)
        endian = "Be";
    else
        endian = "Le";

    if(typeSizes[type] == 3)
        return function += "    return float24ToFloat32(uint24From" + endian + "Bytes(byteA, index), sigbits)\n";
    else
        return function += "    return float16ToFloat32(uint16From" + endian + "Bytes(byteA, index), sigbits)\n";


} // PythonCoding::pyDecodeSpecialFloat


/*!
 * Create the python docstring with summary, args, and return
 * \param type is the type enumeration
 * \param bigendian is a boolian which determines big or little endian
 * \return return python docstring for decode functions
 */
std::string PythonCoding::pyDecodeComment(int type, bool bigendian)
{
    std::string summary = briefDecodeComment(type, bigendian) + "\n\n";

    std::string    args = R"(    Args:
        byteA  (byteArray): The byte stream which contains the encodes data
        index  (list): a list where index 0 is the offset to the first
            byte in the byte stream and will be incremented by )";
                    args += std::to_string(typeSizes[type]);
                    args += "\n            * this gurantees that the index will be updated\n";
                    args += "              since you cannot pass an integer by reference\n";

    if(contains(typeSigNames[type], "float24") || contains(typeSigNames[type], "float16"))
        args += "        sigbits (int): the number of bits to use in the significand of the float.\n\n";
    else
        args += "\n\n";

    std::string returns = R"(    Returns:
        number (int): return the number decoded from the byte stream)";
                returns += "\n";

    std::string quotes = R"(    """)";
    std::string comment = quotes + summary + args + returns + quotes + "\n";

    return comment;

} // PythonCoding::pyDecodeComment














