#include "protocolbitfield.h"
#include "protocolstructuremodule.h"
#include <QFile>

void ProtocolBitfield::generatetest(ProtocolParser* parse, ProtocolSupport support)
{
    if(!support.bitfieldtest)
        return;

    QDomDocument doc;
    QString errorMsg;
    int errorLine;
    int errorCol;
    QFile file(":/files/prebuiltSources/bitfieldtest.xml");

    if(!file.open(QFile::ReadOnly))
        return;

    if(!doc.setContent(file.readAll(), false, &errorMsg, &errorLine, &errorCol))
    {
        file.close();
        QString warning = ":/files/prebuiltSources/bitfieldtest.xml:" + QString::number(errorLine) + ":" + QString::number(errorCol) + " error: " + errorMsg;
        std::cerr << warning.toStdString() << std::endl;
        return;
    }

    file.close();

    // The outer most element
    QDomElement docElem = doc.documentElement();

    // All of the top level Structures, which stand alone in their own modules
    QList<QDomNode> structlist = parse->childElementsByTagName(docElem, "Structure");

    // We only care about the first one
    if(structlist.count() < 1)
        return;

    // We force the filename
    support.globalFileName = "bitfieldtest";

    // Create the module object
    ProtocolStructureModule* module = new ProtocolStructureModule(parse, support, QString(), QString());

    // Remember its xml
    module->setElement(structlist.at(0).toElement());

    // Parse its xml data
    module->parse();

    // Prototype for testing bitfields, note the files have already been flushed, so we are appending
    module->header.setModuleNameAndPath("bitfieldtest", support.outputpath);
    module->header.makeLineSeparator();
    module->header.write("//! Test the bit fields\n");
    module->header.write("int testBitfield(void);\n");
    module->header.write("\n");
    module->header.flush();

    // Now the source code
    module->source.setModuleNameAndPath("bitfieldtest", support.outputpath);
    module->source.makeLineSeparator();
    module->source.writeIncludeDirective("string.h", QString(), true);
    module->source.write("/*!\n");
    module->source.write(" * Test the bit field encode and decode logic\n");
    module->source.write(" * \\return 1 if the test passes, else 0\n");
    module->source.write(" */\n");
    module->source.write("int testBitfield(void)\n");
    module->source.write("{\n");
    module->source.write("    bitfieldtest_t test = {1, 2, 12, 0xABC, 0, 3, 4, 0xC87654321};\n");
    module->source.write("    uint8_t data[20];\n");
    module->source.write("    int index = 0;\n");
    module->source.write("\n");
    module->source.write("    encodebitfieldtest_t(data, &index, &test);\n");
    module->source.write("\n");
    module->source.write("    memset(&test, 0, sizeof(test));\n");
    module->source.write("\n");
    module->source.write("    index = 0;\n");
    module->source.write("    if(!decodebitfieldtest_t(data, &index, &test))\n");
    module->source.write("        return 0;\n");
    module->source.write("\n");
    module->source.write("    if(test.test1 != 1)\n");
    module->source.write("        return 0;\n");
    module->source.write("    else if(test.test2 != 2)\n");
    module->source.write("        return 0;\n");
    module->source.write("    else if(test.test3 != 7)\n");
    module->source.write("        return 0;\n");
    module->source.write("    else if(test.test12 != 0xABC)\n");
    module->source.write("        return 0;\n");
    module->source.write("    else if(test.testa != 0)\n");
    module->source.write("        return 0;\n");
    module->source.write("    else if(test.testb != 3)\n");
    module->source.write("        return 0;\n");
    module->source.write("    else if(test.testc != 4)\n");
    module->source.write("        return 0;\n");
    module->source.write("    else if(test.testd != 0xC87654321)\n");
    module->source.write("        return 0;\n");
    module->source.write("    else\n");
    module->source.write("        return 1;\n");
    module->source.write("\n");
    module->source.write("}// testBitfield\n");
    module->source.flush();

}


/*!
 * Compute the maximum value of a field
 * \param numbits is the bit width of the field
 * \return the maximum value, which will be zero if numbits is zero
 */
uint64_t ProtocolBitfield::maxvalueoffield(int numbits)
{
    // The maximum value that can be stored in a uint64_t
    uint64_t max = UINT64_MAX;

    // The maximum value that can be stored in numbits
    max = max >> (CHAR_BIT*sizeof(uint64_t) - numbits);

    return max;
}


/*!
 * Get the decode string for a bitfield, which may or may not cross byte boundaries
 * \param spacing is the spacing at the start of each line
 * \param argument is the string describing the field of bits
 * \param dataname is the string describing the array of bytes
 * \param dataindex is the string describing the index into the array of bytes
 * \param bitcount is the current bitcount of this field
 * \param numbits is the number of bits in this field
 * \return the string that is the encoding code
 */
QString ProtocolBitfield::getDecodeString(QString spacing, QString argument, QString dataname, QString dataindex, int bitcount, int numbits)
{
    if((numbits > 1) && (((bitcount % 8) + numbits) > 8))
        return getComplexDecodeString(spacing, argument, dataname, dataindex, bitcount, numbits);
    else
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
            return spacing + argument + " = " + dataname + "[" + dataindex + offset + "];\n";
        else if(mask.isEmpty())
            return spacing + argument + " = (" + dataname + "[" + dataindex + offset + "]" + rightshift + ");\n";
        else
            return spacing + argument + " = ((" + dataname + "[" + dataindex + offset + "]" + rightshift + ")" + mask + ");\n";
    }

}// ProtocolBitfield::getDecodeString


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
            return spacing + dataname + "[" + dataindex + offset + "] = (uint8_t)" + argument + leftshift + ";\n";
        else
            return spacing + dataname + "[" + dataindex + offset + "] |= (uint8_t)" + argument + leftshift + ";\n";

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

        // The least significant bits of value, encoded in the most
        // significant bits of the last byte we are going to use.
        output += spacing + dataname + "[" + dataindex + offset + "] = (uint8_t)("+argument+" << "+QString().setNum(8-remainder)+");\n\n";

        // Discard these bits, we have encoded them
        numbits -= remainder;

        // Shift the field down for the next byte of bits
        if(numbits > 0)
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

        output += spacing + dataname + "[" + dataindex + offset + "] = (uint8_t)"+argument+";\n\n";
        byteoffset--;
        numbits -= 8;

        // Shift the field down for the next byte of bits
        if(numbits > 0)
            output += spacing + argument + " >>= 8;\n";

    }// while still bits to encode

    // Finally finish any remaining most significant bits. There are
    // some valid bits in the most signficant bit locations.
    if(numbits > 0)
    {
        QString offset;

        if(byteoffset > 0)
            offset = " + " + QString().setNum(byteoffset);

        output += spacing + dataname + "[" + dataindex + offset + "] |= (uint8_t)" + argument + ";\n";
    }

    return output;

}// ProtocolBitfield::getEncodeString
