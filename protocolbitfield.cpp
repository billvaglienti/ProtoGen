#include "protocolbitfield.h"
#include <QFile>

void ProtocolBitfield::generatetest(ProtocolSupport support)
{
    if(!support.bitfieldtest)
        return;

    ProtocolHeaderFile header(support);
    ProtocolSourceFile source(support);

    // We assume that the ProtocolParser has already generated the encode/decode
    // functions in the bitfieldtest module, we just need to exercise them.

    // Prototype for testing bitfields, note the files have already been flushed, so we are appending
    header.setModuleNameAndPath("bitfieldtest", support.outputpath, support.language);
    header.makeLineSeparator();
    header.write("//! Test the bit fields\n");
    header.write("int testBitfield(void);\n");
    header.write("\n");
    header.flush();

    // Now the source code
    source.setModuleNameAndPath("bitfieldtest", support.outputpath, support.language);
    source.makeLineSeparator();
    source.writeIncludeDirective("string.h", QString(), true);
    source.writeIncludeDirective("limits.h", QString(), true);
    source.writeIncludeDirective("math.h", QString(), true);

    if(support.language == ProtocolSupport::c_language)
    {
        source.write("/*!\n");
        source.write(" * Test the bit field encode and decode logic\n");
        source.write(" * \\return 1 if the test passes, else 0\n");
        source.write(" */\n");
        source.write("int testBitfield(void)\n");
        source.write("{\n");
        source.write("    bitfieldtest_t test   = {1, 2, 12, 0xABC, 0, 3, 4, 0xC87654321ULL};\n");
        source.write("    bitfieldtest2_t test2 = {1, 2, 12, 0xABC, 0, 3, 4, 0xC87654321ULL};\n");
        source.write("\n");
        source.write("    bitfieldtest3_t test3 = {12.5f, 12.5f, 3.14159, 0, 0, 50};\n");
        source.write("\n");
        source.write("    uint8_t data[20];\n");
        source.write("    int index = 0;\n");
        source.write("\n");
        source.write("    // Fill the data with 1s so we can be sure the encoder sets all bits correctly\n");
        source.write("    memset(data, UCHAR_MAX, sizeof(data));\n");
        source.write("\n");
        source.write("    encodebitfieldtest_t(data, &index, &test);\n");
        source.write("\n");
        source.write("    // Clear the in-memory data so we can be sure the decoder sets all bits correctly\n");
        source.write("    memset(&test, 0, sizeof(test));\n");
        source.write("\n");
        source.write("    index = 0;\n");
        source.write("    if(!decodebitfieldtest_t(data, &index, &test))\n");
        source.write("        return 0;\n");
        source.write("\n");
        source.write("    if(test.test1 != 1)\n");
        source.write("        return 0;\n");
        source.write("    else if(test.test2 != 2)\n");
        source.write("        return 0;\n");
        source.write("    else if(test.test3 != 7)  // This value was overflow, 7 is the max\n");
        source.write("        return 0;\n");
        source.write("    else if(test.test12 != 0xABC)\n");
        source.write("        return 0;\n");
        source.write("    else if(test.testa != 0)\n");
        source.write("        return 0;\n");
        source.write("    else if(test.testb != 3)\n");
        source.write("        return 0;\n");
        source.write("    else if(test.testc != 4)\n");
        source.write("        return 0;\n");
        source.write("    else if(test.testd != 0xC87654321ULL)\n");
        source.write("        return 0;\n");
        source.write("\n");
        source.write("    // Fill the data with 1s so we can be sure the encoder sets all bits correctly\n");
        source.write("    memset(data, UCHAR_MAX, sizeof(data));\n");
        source.write("\n");
        source.write("    index = 0;\n");
        source.write("    encodebitfieldtest2_t(data, &index, &test2);\n");
        source.write("\n");
        source.write("    // Clear the in-memory data so we can be sure the decoder sets all bits correctly\n");
        source.write("    memset(&test2, 0, sizeof(test2));\n");
        source.write("\n");
        source.write("    index = 0;\n");
        source.write("    if(!decodebitfieldtest2_t(data, &index, &test2))\n");
        source.write("        return 0;\n");
        source.write("\n");
        source.write("    if(test2.test1 != 1)\n");
        source.write("        return 0;\n");
        source.write("    else if(test2.test2 != 2)\n");
        source.write("        return 0;\n");
        source.write("    else if(test2.test3 != 7)  // This value was overflow, 7 is the max\n");
        source.write("        return 0;\n");
        source.write("    else if(test2.test12 != 0xABC)\n");
        source.write("        return 0;\n");
        source.write("    else if(test2.testa != 0)\n");
        source.write("        return 0;\n");
        source.write("    else if(test2.testb != 3)\n");
        source.write("        return 0;\n");
        source.write("    else if(test2.testc != 4)\n");
        source.write("        return 0;\n");
        source.write("    else if(test2.testd != 0xC87654321ULL)\n");
        source.write("        return 0;\n");
        source.write("\n");
        source.write("    // Fill the data with 1s so we can be sure the encoder sets all bits correctly\n");
        source.write("    memset(data, UCHAR_MAX, sizeof(data));\n");
        source.write("\n");
        source.write("    index = 0;\n");
        source.write("    encodebitfieldtest3_t(data, &index, &test3);\n");
        source.write("\n");
        source.write("    // Clear the in-memory data so we can be sure the decoder sets all bits correctly\n");
        source.write("    memset(&test3, 0, sizeof(test3));\n");
        source.write("\n");
        source.write("    index = 0;\n");
        source.write("    if(!decodebitfieldtest3_t(data, &index, &test3))\n");
        source.write("        return 0;\n");
        source.write("\n");
        source.write("    if(fabs(test3.test1 - 25.0f) > 1.0/200.0) // underflow, min is 25\n");
        source.write("        return 0;\n");
        source.write("    else if(fabs(test3.test2 - 12.5f) > 1.0/100.0)\n");
        source.write("        return 0;\n");
        source.write("    else if(fabs(test3.test12 - 3.14159) > 1.0/1024.0)\n");
        source.write("        return 0;\n");
        source.write("    else if(test3.testa != 1)\n");
        source.write("        return 0;\n");
        source.write("    else if(fabs(test3.testc - 3.1415926535898) > 1.0/200.0)\n");
        source.write("        return 0;\n");
        source.write("    else if(test3.testd != 0)\n");
        source.write("        return 0;\n");
        source.write("    else\n");
        source.write("        return 1;\n");
        source.write("\n");
        source.write("}// testBitfield\n");
    }
    else if(support.language == ProtocolSupport::cpp_language)
    {
        source.write("/*!\n");
        source.write(" * Test the bit field encode and decode logic\n");
        source.write(" * \\return 1 if the test passes, else 0\n");
        source.write(" */\n");
        source.write("int testBitfield(void)\n");
        source.write("{\n");
        source.write("    bitfieldtest_t test;\n");
        source.write("    bitfieldtest2_t test2;\n");
        source.write("    bitfieldtest3_t test3;\n");
        source.write("    uint8_t data[20];\n");
        source.write("    int index = 0;\n");
        source.write("\n");
        source.write("    // Fill the data with 1s so we can be sure the encoder sets all bits correctly\n");
        source.write("    memset(data, UCHAR_MAX, sizeof(data));\n");
        source.write("\n");
        source.write("    test.encode(data, &index);\n");
        source.write("\n");
        source.write("    // Clear the in-memory data so we can be sure the decoder sets all bits correctly\n");
        source.write("    memset(&test, 0, sizeof(test));\n");
        source.write("\n");
        source.write("    index = 0;\n");
        source.write("    if(!test.decode(data, &index))\n");
        source.write("        return 0;\n");
        source.write("\n");
        source.write("    if(test.test1 != 1)\n");
        source.write("        return 0;\n");
        source.write("    else if(test.test2 != 2)\n");
        source.write("        return 0;\n");
        source.write("    else if(test.test3 != 7)  // This value was overflow, 7 is the max\n");
        source.write("        return 0;\n");
        source.write("    else if(test.test12 != 0xABC)\n");
        source.write("        return 0;\n");
        source.write("    else if(test.testa != 0)\n");
        source.write("        return 0;\n");
        source.write("    else if(test.testb != 3)\n");
        source.write("        return 0;\n");
        source.write("    else if(test.testc != 4)\n");
        source.write("        return 0;\n");
        source.write("    else if(test.testd != 0xC87654321ULL)\n");
        source.write("        return 0;\n");
        source.write("\n");
        source.write("    // Fill the data with 1s so we can be sure the encoder sets all bits correctly\n");
        source.write("    memset(data, UCHAR_MAX, sizeof(data));\n");
        source.write("\n");
        source.write("    index = 0;\n");
        source.write("    test2.encode(data, &index);\n");
        source.write("\n");
        source.write("    // Clear the in-memory data so we can be sure the decoder sets all bits correctly\n");
        source.write("    memset(&test2, 0, sizeof(test2));\n");
        source.write("\n");
        source.write("    index = 0;\n");
        source.write("    if(!test2.decode(data, &index))\n");
        source.write("        return 0;\n");
        source.write("\n");
        source.write("    if(test2.test1 != 1)\n");
        source.write("        return 0;\n");
        source.write("    else if(test2.test2 != 2)\n");
        source.write("        return 0;\n");
        source.write("    else if(test2.test3 != 7)  // This value was overflow, 7 is the max\n");
        source.write("        return 0;\n");
        source.write("    else if(test2.test12 != 0xABC)\n");
        source.write("        return 0;\n");
        source.write("    else if(test2.testa != 0)\n");
        source.write("        return 0;\n");
        source.write("    else if(test2.testb != 3)\n");
        source.write("        return 0;\n");
        source.write("    else if(test2.testc != 4)\n");
        source.write("        return 0;\n");
        source.write("    else if(test2.testd != 0xC87654321ULL)\n");
        source.write("        return 0;\n");
        source.write("\n");
        source.write("    // Fill the data with 1s so we can be sure the encoder sets all bits correctly\n");
        source.write("    memset(data, UCHAR_MAX, sizeof(data));\n");
        source.write("\n");
        source.write("    index = 0;\n");
        source.write("    test3.encode(data, &index);\n");
        source.write("\n");
        source.write("    // Clear the in-memory data so we can be sure the decoder sets all bits correctly\n");
        source.write("    memset(&test3, 0, sizeof(test3));\n");
        source.write("\n");
        source.write("    index = 0;\n");
        source.write("    if(!test3.decode(data, &index))\n");
        source.write("        return 0;\n");
        source.write("\n");
        source.write("    if(fabs(test3.test1 - 25.0f) > 1.0/200.0) // underflow, min is 25\n");
        source.write("        return 0;\n");
        source.write("    else if(fabs(test3.test2 - 12.5f) > 1.0/100.0)\n");
        source.write("        return 0;\n");
        source.write("    else if(fabs(test3.test12 - 3.14159) > 1.0/1024.0)\n");
        source.write("        return 0;\n");
        source.write("    else if(test3.testa != 1)\n");
        source.write("        return 0;\n");
        source.write("    else if(fabs(test3.testc - 3.1415926535898) > 1.0/200.0)\n");
        source.write("        return 0;\n");
        source.write("    else if(test3.testd != 0)\n");
        source.write("        return 0;\n");
        source.write("    else\n");
        source.write("        return 1;\n");
        source.write("\n");
        source.write("}// testBitfield\n");

    }// else if c++

    source.flush();

}// ProtocolBitfield::generatetest


/*!
 * Compute the maximum value of a field
 * \param numbits is the bit width of the field
 * \return the maximum value, which will be zero if numbits is zero
 */
uint64_t ProtocolBitfield::maxvalueoffield(int numbits)
{
    // Do not omit the "ull" or you will be limited to shifting the number of bits in an int
    return (0x1ull << numbits) - 1;
}


/*!
 * Get the decode string for a bitfield, which may or may not cross byte boundaries
 * \param spacing is the spacing at the start of each line
 * \param argument is the string describing the field of bits
 * \param cast is the string used to cast to the arguments type. This can be empty
 * \param dataname is the string describing the array of bytes
 * \param dataindex is the string describing the index into the array of bytes
 * \param bitcount is the current bitcount of this field
 * \param numbits is the number of bits in this field
 * \return the string that is the decoding code
 */
QString ProtocolBitfield::getDecodeString(QString spacing, QString argument, QString cast, QString dataname, QString dataindex, int bitcount, int numbits)
{
    if((numbits > 1) && (((bitcount % 8) + numbits) > 8))
        return getComplexDecodeString(spacing, argument, dataname, dataindex, bitcount, numbits);
    else
        return spacing + argument + " = " + cast + getInnerDecodeString(dataname, dataindex, bitcount, numbits) + ";\n";

}// ProtocolBitfield::getDecodeString


/*!
 * Get the inner string that does a simple bitfield decode
 * \param dataname is the string describing the array of bytes
 * \param dataindex is the string describing the index into the array of bytes
 * \param bitcount is the current bitcount of this field
 * \param numbits is the number of bits in this field
 * \return the string that is the decoding code
 */
QString ProtocolBitfield::getInnerDecodeString(QString dataname, QString dataindex, int bitcount, int numbits)
{
    // This is the easiest case, we can just decode it directly
    QString rightshift;
    QString offset;
    QString mask;

    // Don't do shifting by zero bits
    int right = 8 - (bitcount%8 + numbits);
    if(right > 0)
        rightshift = " >> " + QString().setNum(8 - (bitcount%8 + numbits));

    // This mask protects against any other bits we don't want. We don't
    // need the mask if we are grabbing the most significant bit of this byte
    if((numbits + right) < 8)
        mask = " & 0x" + QString().setNum(maxvalueoffield(numbits), 16).toUpper();

    // The value of the bit count after moving all the bits
    int bitoffset = bitcount + numbits;

    // The byte offset of this bitfield
    int byteoffset = (bitoffset-1) >> 3;

    if(byteoffset > 0)
        offset = " + " + QString().setNum(byteoffset);

    if(mask.isEmpty() && rightshift.isEmpty())
        return dataname + "[" + dataindex + offset + "]";
    else if(mask.isEmpty())
        return "(" + dataname + "[" + dataindex + offset + "]" + rightshift + ")";
    else
        return "((" + dataname + "[" + dataindex + offset + "]" + rightshift + ")" + mask + ")";
}


/*!
 * Get the decode string for a complex bitfield (crossing byte boundaries)
 * \param spacing is the spacing at the start of each line
 * \param argument is the string describing the field of bits
 * \param dataname is the string describing the array of bytes
 * \param dataindex is the string describing the index into the array of bytes
 * \param bitcount is the current bitcount of this field
 * \param numbits is the number of bits in this field
 * \return the string that is the encoding code
 */
QString ProtocolBitfield::getComplexDecodeString(QString spacing, QString argument, QString dataname, QString dataindex, int bitcount, int numbits)
{
    QString output;

    // Bits are encoded left-to-right from most-significant to least-significant.
    // The most significant bits are moved first, as that allows us to keep the
    // shifts to 8 bits or less, which some compilers need.

    // The byteoffset of the most significant block of 8 bits
    int byteoffset = bitcount>>3;

    int leadingbits = (8 - (bitcount % 8)%8);

    // First get any most significant bits that are partially encoded in the previous byte
    if(leadingbits)
    {
        QString offset;
        if(byteoffset)
            offset = " + " + QString().setNum(byteoffset);

        // This mask protects against any other bits we don't want
        QString mask = " & 0x" + QString().setNum(maxvalueoffield(leadingbits), 16).toUpper();

        output += spacing + argument + " = (" + dataname + "[" + dataindex + offset + "]" + mask + ");\n\n";

        // These bits are done
        numbits -= leadingbits;
        byteoffset++;

        // Shift the argument up for the next decode
        if(numbits >= 8)
            output += spacing + argument + " <<= 8;\n";
        else if(numbits)
            output += spacing + argument + " <<= " + QString().setNum(numbits) + ";\n";
    }

    while(numbits >= 8)
    {
        QString offset;
        if(byteoffset)
            offset = " + " + QString().setNum(byteoffset);

        // Make space in the argument for the next most significant 8 bits
        output += spacing + argument + " |= " + dataname + "[" + dataindex + offset + "];\n\n";

        byteoffset++;
        numbits -= 8;

        // Shift the argument up for the next decode
        if(numbits >= 8)
            output += spacing + argument + " <<= 8;\n";
        else if(numbits)
            output += spacing + argument + " <<= " + QString().setNum(numbits) + ";\n";
    }

    // Handle the final remainder bits
    if(numbits)
    {
        QString offset;

        if(byteoffset > 0)
            offset = " + " + QString().setNum(byteoffset);

        // The least significant bits of value, encoded in the most
        // significant bits of the last byte we are going to use. By
        // Definition we don't need a mask because we are grabbing the
        // most significant bit in this case.
        output += spacing + argument + " |= (" + dataname + "[" + dataindex + offset + "] >> " + QString().setNum(8-numbits) + ");\n\n";
    }

    return output;

}// ProtocolBitfield::getComplexDecodeString


/*!
 * Get the encode string for a bitfield, which may or may not cross byte boundaries
 * \param spacing is the spacing at the start of each line
 * \param argument is the string describing the field of bits
 * \param dataname is the string describing the array of bytes
 * \param dataindex is the string describing the index into the array of bytes
 * \param bitcount is the current bitcount of this field
 * \param numbits is the number of bits in this field
 * \return the string that is the encoding code
 */
QString ProtocolBitfield::getEncodeString(QString spacing, QString argument, QString dataname, QString dataindex, int bitcount, int numbits)
{
    if((numbits > 1) && (((bitcount % 8) + numbits) > 8))
        return getComplexEncodeString(spacing, argument, dataname, dataindex, bitcount, numbits);
    else
    {
        // This is the easiest case, we can just encode it directly
        QString leftshift;
        QString offset;

        // Don't do any shifting by zero bits
        int left = 8 - (bitcount%8 + numbits);
        if(left > 0)
            leftshift = " << " + QString().setNum(8 - (bitcount%8 + numbits));

        // The value of the bit count after moving all the bits
        int bitoffset = bitcount + numbits;

        // The byte offset of this bitfield
        int byteoffset = (bitoffset-1) >> 3;

        if(byteoffset > 0)
            offset = " + " + QString().setNum(byteoffset);

        // If this is the first bit of this byte then we assign rather than or-equal
        if((bitcount%8) == 0)
        {
            // If the argument is the string "0" then we don't need to be shifting
            if(argument == "0")
                return spacing + dataname + "[" + dataindex + offset + "] = 0;\n";
            else
                return spacing + dataname + "[" + dataindex + offset + "] = (uint8_t)" + argument + leftshift + ";\n";
        }
        else
        {
            // If the thing we are or-equaling is the string "0" then we can just skip the entire line
            if(argument == "0")
                return QString();
            else
                return spacing + dataname + "[" + dataindex + offset + "] |= (uint8_t)" + argument + leftshift + ";\n";
        }

    }

}// ProtocolBitfield::getEncodeString


/*!
 * Get the encode string for a complex bitfield (crossing byte boundaries)
 * \param spacing is the spacing at the start of each line
 * \param argument is the string describing the field of bits
 * \param dataname is the string describing the array of bytes
 * \param dataindex is the string describing the index into the array of bytes
 * \param bitcount is the current bitcount of this field
 * \param numbits is the number of bits in this field
 * \return the string that is the encoding code
 */
QString ProtocolBitfield::getComplexEncodeString(QString spacing, QString argument, QString dataname, QString dataindex, int bitcount, int numbits)
{
    QString output;

    // Bits are encoded left-to-right from most-significant to least-significant.
    // The least significant bits are moved first, as that allows us to keep the
    // shifts to 8 bits or less, which some compilers need.

    // The value of the bit count after moving all the bits
    int bitoffset = bitcount + numbits;

    // The byte offset of the least significant block of 8 bits to move
    int byteoffset = (bitoffset-1) >> 3;

    // The remainder bits (modulo 8) which are the least significant bits to move
    int remainder = bitoffset & 0x07;

    if(remainder)
    {
        QString offset;

        if(byteoffset > 0)
            offset = " + " + QString().setNum(byteoffset);

        if(argument == "0")
        {
            // If the argument is the constant string "0" then shifting is not needed
            output += spacing + dataname + "[" + dataindex + offset + "] = 0;\n\n";
        }
        else
        {
            // The least significant bits of value, encoded in the most
            // significant bits of the last byte we are going to use.
            output += spacing + dataname + "[" + dataindex + offset + "] = (uint8_t)("+argument+" << "+QString().setNum(8-remainder)+");\n\n";
        }

        // Discard these bits, we have encoded them
        numbits -= remainder;

        // Shift the field down for the next byte of bits
        if((numbits > 0) && (argument != "0"))
            output += spacing + argument + " >>= " + QString().setNum(remainder) + ";\n";

        remainder = 0;
        byteoffset--;

    }

    // Now aligned on a byte boundary, move full bytes
    while(numbits >= 8)
    {
        QString offset;

        if(byteoffset > 0)
            offset = " + " + QString().setNum(byteoffset);

        if(argument == "0")
        {
            // If the argument is the constant string "0" then shifting is not needed
            output += spacing + dataname + "[" + dataindex + offset + "] = 0;\n\n";
            byteoffset--;
            numbits -= 8;
        }
        else
        {
            output += spacing + dataname + "[" + dataindex + offset + "] = (uint8_t)"+argument+";\n\n";
            byteoffset--;
            numbits -= 8;

            // Shift the field down for the next byte of bits
            if(numbits > 0)
                output += spacing + argument + " >>= 8;\n";
        }

    }// while still bits to encode

    // Finally finish any remaining most significant bits. There are
    // some valid bits in the most signficant bit locations.
    if(numbits > 0)
    {
        QString offset;

        if(byteoffset > 0)
            offset = " + " + QString().setNum(byteoffset);

        // If the thing we are or-equaling is the string "0" then we can just skip the entire line
        if(argument != "0")
            output += spacing + dataname + "[" + dataindex + offset + "] |= (uint8_t)" + argument + ";\n";
    }

    return output;

}// ProtocolBitfield::getEncodeString
