#include "protocolfield.h"
#include "protocolparser.h"
#include "shuntingyard.h"
#include "enumcreator.h"
#include "protocolstructuremodule.h"
#include "protocolbitfield.h"
#include <QString>
#include <QDomElement>
#include <math.h>

TypeData::TypeData(ProtocolSupport sup) :
    isStruct(false),
    isSigned(false),
    isBitfield(false),
    isFloat(false),
    isEnum(false),
    isString(false),
    isFixedString(false),
    isNull(false),
    bits(8),
    sigbits(0),
    support(sup)
{
}


TypeData::TypeData(const TypeData& that) :
    isStruct(that.isStruct),
    isSigned(that.isSigned),
    isBitfield(that.isBitfield),
    isFloat(that.isFloat),
    isEnum(that.isEnum),
    isString(that.isString),
    isFixedString(that.isFixedString),
    isNull(that.isNull),
    bits(that.bits),
    sigbits(that.sigbits),
    support(that.support)
{
}


/*!
 * Reset all members to defaults except the protocol support
 */
void TypeData::clear(void)
{
    isStruct = false;
    isSigned = false;
    isBitfield = false;
    isFloat = false;
    isEnum = false;
    isString = false;
    isFixedString = false;
    isNull = false;
    bits = 8;
    sigbits = 0;
}


/*!
 * Pull a positive integer value from a string
 * \param string is the source string, which can contain a decimal or hexadecimal (0x) value
 * \param ok receives false if there was a problem with the decode
 * \return the positive integer value or 0 if there was a problem
 */
int TypeData::extractPositiveInt(const QString& string, bool* ok)
{
    QString number = string;

    if(number.contains("0x", Qt::CaseInsensitive))
    {
        QRegExp rx("[^0123456789AaBbCcDdEeFf]");
        number.replace(rx, "");
        return number.toInt(ok, 16);
    }
    else
    {
        QRegExp rx("[^0123456789]");
        number.replace(rx, "");
        return number.toInt(ok, 10);
    }

}


/*!
 * Pull a double precision value from a string
 * \param string is the source string
 * \param ok receives false if there was a problem with the decode
 * \return the double precision value or 0 if there was a problem
 */
double TypeData::extractDouble(const QString& string, bool* ok)
{
    QString number = string;

    if(number.contains("0x", Qt::CaseInsensitive) && !number.contains("."))
    {
        QRegExp rx("[^0123456789AaBbCcDdEeFf]");
        number.replace(rx, "");
        return (double)number.toInt(ok, 16);
    }
    else
    {
        QRegExp rx("[^0123456789-\\.]");
        number.replace(rx, "");
        return number.toDouble(ok);
    }
}


QString TypeData::toTypeString(QString enumName, QString structName) const
{
    QString typeName;

    if(isString)
        typeName = "char";
    else if(isBitfield)
    {
        if((bits > 32) && (support.longbitfield))
            typeName = "uint64_t";
        else
            typeName = "unsigned";
    }
    else if(isEnum)
    {
        typeName = enumName;
    }
    else if(isStruct)
    {
        typeName = structName;

        // Make sure it ends with "_t";
        if(!typeName.endsWith("_t"))
        {
            typeName += "_t";
        }
    }
    else
    {
        // create the type name
        if(isFloat)
        {
            if(bits > 32)
                typeName = "double";
            else
                typeName = "float";

        }// if floating point type
        else
        {
            if(isSigned)
                typeName = "int";
            else
                typeName = "uint";

            // Add the bits, we use only valid native type widths
            if(bits > 32)
                typeName += "64_t";
            else if(bits > 16)
                typeName += "32_t";
            else if(bits > 8)
                typeName += "16_t";
            else
                typeName += "8_t";

        }// else if integer type
    }

    return typeName;

}// toTypeString


/*!
 * Construct a blank protocol field
 * \param parse points to the global protocol parser that owns everything
 * \param parent is the hierarchical name of the parent object
 * \param supported indicates what the protocol can support
 */
ProtocolField::ProtocolField(ProtocolParser* parse, QString parent, ProtocolSupport supported):
    Encodable(parse, parent, supported),
    encodedMin(0),
    encodedMax(0),
    scaler(1),
    checkConstant(false),
    overridesPrevious(false),
    isOverriden(false),
    inMemoryType(supported),
    encodedType(supported),
    prevField(0),
    hidden(false),
    mapOptions(MAP_BOTH)
{
}


/*!
 * Reset all data to defaults
 */
void ProtocolField::clear(void)
{
    Encodable::clear();

    enumName.clear();
    encodedMin = encodedMax = 0;
    scaler = 1;
    defaultString.clear();
    defaultStringForDisplay.clear();
    constantString.clear();
    constantStringForDisplay.clear();
    checkConstant = false;
    overridesPrevious = false;
    isOverriden = false;
    encodedType = inMemoryType = TypeData(support);
    bitfieldData.clear();
    scalerString.clear();
    printScalerString.clear();
    readScalerString.clear();
    minString.clear();
    maxString.clear();
    prevField = 0;
    extraInfoNames.clear();
    extraInfoValues.clear();
    hidden = false;
    initialValueString.clear();
    verifyMinString.clear();
    verifyMaxString.clear();
    initialValueStringForDisplay.clear();
    verifyMinStringForDisplay.clear();
    verifyMaxStringForDisplay.clear();

}// ProtocolField::clear


//! Provide the pointer to a previous encodable in the list
void ProtocolField::setPreviousEncodable(Encodable* prev)
{
    prevField = NULL;

    // We need to know when the bitfields end
    if(prev != NULL)
    {
        // Does prev point to a ProtocolField?
        prevField = dynamic_cast<ProtocolField*>(prev);
    }

    if(prevField == NULL)
        return;

    // Are we the start of part of a new bitfield group (or are we not a bitfield at all)?
    // Which means the previous field terminates that group (if any)
    if(bitfieldData.groupStart || !encodedType.isBitfield)
        prevField->setTerminatesBitfield(true);

    if(prevField->isBitfield() && (encodedType.isBitfield))
    {
        // Are we part of a bitfield group?
        if(!bitfieldData.groupStart)
        {
            // We did not start a group, we might be a member of a previous group
            bitfieldData.groupMember = prevField->bitfieldData.groupMember;

            // Previous bitfield does not terminate the bitfields
            prevField->setTerminatesBitfield(false);

            // We start at some nonzero bitcount that continues from the previous
            setStartingBitCount(prevField->getEndingBitCount());

            // Now that we know our starting bitcount we can apply the bitfield defaults warning
            if(!defaultString.isEmpty() && (defaultString != "0") && (bitfieldData.startingBitCount != 0) && !bitfieldCrossesByteBoundary())
            {
                // The key concept is this: bitfields can have defaults, but if
                // the bitfield does not start a new byte it is possible (not
                // guaranteed) that the default will be overwritten because the
                // previous bitfield will have caused the packet length to
                // satisfy the length requirement. As an example:
                //
                // Old packet = 1 byte + 1 bit.
                // New augmented packet = 1 byte + 2 bits (second bit has a default).
                //
                // The length of the old and new packets is the same (2 bytes)
                // which means the default value for the augmented bit will
                // always be overwritten with zero.
                emitWarning("Bitfield with non-zero default which does not start a new byte; default may not be obeyed");
            }

        }

    }// if previous and us are bitfields

    computeEncodedLength();

}// ProtocolField::setPreviousEncodable


/*!
 * Get overriden type information.
 * \param prev is the previous encodable to test if its the source of the data being overriden by this encodable. Can be null
 * \return true if prev is the source of data being overriden
 */
bool ProtocolField::getOverriddenTypeData(ProtocolField* prev)
{
    // If we are overriding then this function is not interesting
    if(!overridesPrevious)
        return false;

    // Check to make sure that this previous actually exists
    if(prev == NULL)
        return false;

    // Must have the same name if we are overriding it
    if(prev->name != name)
        return false;

    // Must exist in memory, or we can't be overriding it
    if(prev->isNotInMemory())
        return false;

    // Let the previous one know that we are overriding it
    prev->isOverriden = true;

    // If we get here, then this is our baby. Update the data being overriden.
    inMemoryType = prev->inMemoryType;

    if(!enumName.isEmpty())
        emitWarning("Enumeration name ignored for overridden field");
    enumName = prev->enumName;

    if(!array.isEmpty())
        emitWarning("Array information ignored for overridden field");
    array = prev->array;

    if(!array2d.isEmpty())
        emitWarning("2D Array information ignored for overridden field");
    array2d = prev->array2d;

    // This information can be modified, but is typically taken from the original.
    if(variableArray.isEmpty())
        variableArray = prev->variableArray;

    if(variable2dArray.isEmpty())
        variable2dArray = prev->variable2dArray;

    if(dependsOn.isEmpty())
        dependsOn = prev->dependsOn;

    if(comment.isEmpty())
        comment = prev->comment;

    // Recompute the length now that the array data are up to date
    computeEncodedLength();

    return true;

}// ProtocolField::getOverriddenTypeData


//! Get the maximum number of temporary bytes needed for a bitfield group of our children
void ProtocolField::getBitfieldGroupNumBytes(int* num) const
{
    if(encodedType.isBitfield && bitfieldData.lastBitfield && bitfieldData.groupMember)
    {
        int length = ((bitfieldData.groupBits+7)/8);

        if(length > (*num))
            (*num) = length;
    }

}


/*!
 * Extract the type information from the type string, for in memory types
 * \param _pg_data holds the extracted type
 * \param type is the type string
 * \param name is the name of this field, used for warnings
 * \param inMemory is true if this is an in-memory type string, else encoded
 */
void ProtocolField::extractType(TypeData& data, const QString& typeString, const QString& name, bool inMemory)
{
    Q_UNUSED(name);

    QString type(typeString);

    data.clear();

    if(type.startsWith("n", Qt::CaseInsensitive))
        data.isNull = true;
    else if(type.startsWith("over", Qt::CaseInsensitive) && inMemory)
    {
        overridesPrevious = true;

        // This is just a place holder, it will get overriden later
        data.bits = 32;
    }
    else if(type.startsWith("stru", Qt::CaseInsensitive))
    {
        if(inMemory)
            data.isStruct = true;
        else
            return;
    }
    else if(type.startsWith("string", Qt::CaseInsensitive))
    {
        data.isString = true;
        data.isFixedString = false;

        data.bits = 8;
    }
    else if(type.startsWith("fixedstring", Qt::CaseInsensitive))
    {
        data.isString = true;
        data.isFixedString = true;

        data.bits = 8;
    }
    else if(type.startsWith("b", Qt::CaseInsensitive))
    {
        // Get the number of bits, between 1 and 32 inclusive
        data.bits = data.extractPositiveInt(type);

        if(support.bitfield == false)
        {
            emitWarning("bitfield support is disabled in this protocol");

            // if bits is 1, then it becomes 8. If it is 8 then it
            // becomes 8, if its 9 it becomes 16, etc.
            data.bits = 8*((data.bits + 7)/8);
        }
        else
        {
            // Bitfields cannot be floats and cannot be signed
            data.isBitfield = true;

            // Bitfields must have at least one bit, and less than 33 bits
            if(data.bits < 1)
            {
                data.bits = 1;
                emitWarning("bitfields must have a bit width of at least one");
            }
            else if((data.bits > 32) && (support.longbitfield == false))
            {
                emitWarning("bitfields must have a bit width of 32 or less");
                data.bits = 32;
            }
            else if(data.bits > 64)
            {
                emitWarning("bitfields must have a bit width of 64 or less");
                data.bits = 64;
            }
        }
    }
    else if(type.startsWith("e", Qt::CaseInsensitive))
    {
        // enumeration types are only for in-memory, never encoded
        if(inMemory)
        {
            data.isEnum = true;
        }
        else
        {
            data.isEnum = false;
        }

        data.bits = 8;
    }
    else
    {
        data.bits = data.extractPositiveInt(type);

        if(type.startsWith("u", Qt::CaseInsensitive))
        {
            data.isSigned = false;
        }
        else
        {
            data.isSigned = true;

            if(type.startsWith("f", Qt::CaseInsensitive))
            {
                // we want to handle the cas where the user types "float16:10" to specify the number of significands
                if(type.contains(":"))
                {
                    QStringList list = type.split(":", QString::SkipEmptyParts);

                    if(list.count() >= 2)
                    {
                        data.bits = data.extractPositiveInt(list.at(0));
                        data.sigbits = data.extractPositiveInt(list.at(1));
                    }

                }

                data.isFloat = true;

                // "float" is not a warning
                if(data.bits == 0)
                    data.bits = 32;
            }
            else if(type.startsWith("d", Qt::CaseInsensitive))
            {
                data.isFloat = true;

                // "double" is not a warning
                if(data.bits == 0)
                    data.bits = 64;
            }
            else if(!type.startsWith("s", Qt::CaseInsensitive) && !type.startsWith("i", Qt::CaseInsensitive))
            {
                emitWarning("in memory type name not understood, signed integer assumed");
            }
        }

        if(data.isFloat)
        {
            if(inMemory)
            {
                if((data.bits != 32) && (data.bits != 64))
                {
                    emitWarning("in memory float types must be 32 or 64 bits");

                    if(data.bits < 32)
                        data.bits = 32;
                    else
                        data.bits = 64;
                }

                if(data.sigbits != 0)
                {
                    data.sigbits = 0;
                    emitWarning("in memory float types do not have variable resolution");
                }
            }
            else
            {
                if((data.bits != 16) && (data.bits != 24) && (data.bits != 32) && (data.bits != 64))
                {
                    emitWarning("encoded float types must be 16, 24, 32, or 64 bits");

                    if(data.bits < 16)
                        data.bits = 16;
                    else if(data.bits < 24)
                        data.bits = 24;
                    else if(data.bits < 32)
                        data.bits = 32;
                    else
                        data.bits = 64;

                    if((data.bits < 32) && (support.specialFloat == false))
                    {
                        emitWarning("non-standard float bit widths are disabled in this protocol");
                        data.bits = 32;
                        data.sigbits = 0;
                    }
                }

                if(data.sigbits != 0)
                {
                    if(data.bits >= 32)
                    {
                        emitWarning("float type must be 16 or 24 bit to specify resolution");
                        data.sigbits = 0;
                    }
                    else if(data.bits == 24)
                    {
                        if((data.sigbits < 4) || (data.sigbits > 20))
                        {
                            emitWarning("significand (resolution) of float24 must be between 4 and 20 bits inclusive, defaulted to 15");
                            data.sigbits = 15;
                        }
                    }
                    else if(data.bits == 16)
                    {
                        if((data.sigbits < 4) || (data.sigbits > 12))
                        {
                            emitWarning("significand (resolution) of float16 must be between 4 and 12 bits inclusive, defaulted to 9");
                            data.sigbits = 9;
                        }
                    }

                }// if user specified significant bits
                else
                {
                    // Default significand bits for float16 and float24
                    if(data.bits == 16)
                        data.sigbits = 9;
                    else if(data.bits == 24)
                        data.sigbits = 15;
                }

            }// else if not in memory float

            if((data.bits > 32) && (support.float64 == false))
            {
                emitWarning("64 bit float support is disabled in this protocol");
                data.bits = 32;
            }

        }// if float
        else
        {
            if(inMemory)
            {
                if((data.bits != 8) && (data.bits != 16) && (data.bits != 32) && (data.bits != 64))
                {
                    emitWarning("in memory integer types must be 8, 16, 32, or 64 bits");

                    if(data.bits > 32)
                        data.bits = 64;
                    else if(data.bits > 16)
                        data.bits = 32;
                    else if(data.bits > 8)
                        data.bits = 16;
                    else
                        data.bits = 8;
                }
            }
            else if(((data.bits % 8) != 0) || (data.bits > 64))
            {
                emitWarning("encoded integer types must be 8, 16, 24, 32, 40, 48, 56, or 64 bits");

                if(data.bits > 56)
                    data.bits = 64;
                else if(data.bits > 48)
                    data.bits = 56;
                else if(data.bits > 40)
                    data.bits = 48;
                else if(data.bits > 32)
                    data.bits = 40;
                else if(data.bits > 24)
                    data.bits = 32;
                else if(data.bits > 16)
                    data.bits = 24;
                else if(data.bits > 8)
                    data.bits = 16;
                else
                    data.bits = 8;
            }

            if((data.bits > 32) && (support.int64 == false))
            {
                emitWarning("Integers greater than 32 bits are disabled in this protocol");
                data.bits = 32;
            }

        }// if integer

    }// else if float or integer

}// ProtocolField::extractType


/*!
 * Parse the DOM to determine the details of this ProtocolField
 */
void ProtocolField::parse(void)
{
    QString memoryTypeString;
    QString encodedTypeString;
    QString structName;

    clear();

    QDomNamedNodeMap map = e.attributes();

    // We use name as part of our debug outputs, so its good to have it first.
    name = ProtocolParser::getAttribute("name", map);

    // Tell the user of attribute problems
    testAndWarnAttributes(map, QStringList() << "name"
                                             << "title"
                                             << "inMemoryType"
                                             << "encodedType"
                                             << "struct"
                                             << "max"
                                             << "min"
                                             << "scaler"
                                             << "printscaler"
                                             << "array"
                                             << "variableArray"
                                             << "array2d"
                                             << "variable2dArray"
                                             << "dependsOn"
                                             << "enum"
                                             << "default"
                                             << "constant"
                                             << "checkConstant"
                                             << "comment"
                                             << "Units"
                                             << "Range"
                                             << "Notes"
                                             << "bitfieldGroup"
                                             << "hidden"
                                             << "initialValue"
                                             << "verifyMinValue"
                                             << "verifyMaxValue"
                                             << "map");

    for(int i = 0; i < map.count(); i++)
    {
        QDomAttr attr = map.item(i).toAttr();
        if(attr.isNull())
            continue;

        QString attrname = attr.name();

        if(attrname.compare("title", Qt::CaseInsensitive) == 0)
            title = attr.value().trimmed();
        if(attrname.compare("inMemoryType", Qt::CaseInsensitive) == 0)
            memoryTypeString = attr.value().trimmed();
        else if(attrname.compare("encodedType", Qt::CaseInsensitive) == 0)
            encodedTypeString = attr.value().trimmed();
        else if(attrname.compare("struct", Qt::CaseInsensitive) == 0)
            structName = attr.value().trimmed();
        else if(attrname.compare("max", Qt::CaseInsensitive) == 0)
            maxString = attr.value().trimmed();
        else if(attrname.compare("min", Qt::CaseInsensitive) == 0)
            minString = attr.value().trimmed();
        else if(attrname.compare("scaler", Qt::CaseInsensitive) == 0)
            scalerString = attr.value().trimmed();
        else if(attrname.compare("printscaler", Qt::CaseInsensitive) == 0)
            printScalerString = attr.value().trimmed();
        else if(attrname.compare("array", Qt::CaseInsensitive) == 0)
            array = attr.value().trimmed();
        else if(attrname.compare("variableArray", Qt::CaseInsensitive) == 0)
            variableArray = attr.value().trimmed();
        else if(attrname.compare("array2d", Qt::CaseInsensitive) == 0)
            array2d = attr.value().trimmed();
        else if(attrname.compare("variable2dArray", Qt::CaseInsensitive) == 0)
            variable2dArray = attr.value().trimmed();
        else if(attrname.compare("dependsOn", Qt::CaseInsensitive) == 0)
            dependsOn = attr.value().trimmed();
        else if(attrname.compare("enum", Qt::CaseInsensitive) == 0)
            enumName = attr.value().trimmed();
        else if(attrname.compare("default", Qt::CaseInsensitive) == 0)
            defaultString = attr.value().trimmed();
        else if(attrname.compare("constant", Qt::CaseInsensitive) == 0)
            constantString = attr.value().trimmed();
        else if(attrname.compare("checkConstant", Qt::CaseInsensitive) == 0)
            checkConstant = ProtocolParser::isFieldSet(attr.value().trimmed());
        else if(attrname.compare("comment", Qt::CaseInsensitive) == 0)
            comment = ProtocolParser::reflowComment(attr.value().trimmed());
        else if(attrname.compare("Units", Qt::CaseInsensitive) == 0)
        {
            extraInfoNames.append("Units");
            extraInfoValues.append(attr.value());
        }
        else if(attrname.compare("Range", Qt::CaseInsensitive) == 0)
        {
            extraInfoNames.append("Range");
            extraInfoValues.append(attr.value());
        }
        else if(attrname.compare("Notes", Qt::CaseInsensitive) == 0)
        {
            extraInfoNames.append("Notes");
            extraInfoValues.append(attr.value());
        }
        else if(attrname.compare("bitfieldGroup", Qt::CaseInsensitive) == 0)
            bitfieldData.groupMember = bitfieldData.groupStart = ProtocolParser::isFieldSet(attr.value().trimmed());
        else if(attrname.compare("hidden", Qt::CaseInsensitive) == 0)
            hidden = ProtocolParser::isFieldSet(attr.value().trimmed());
        else if(attrname.compare("initialValue", Qt::CaseInsensitive) == 0)
            initialValueString = attr.value().trimmed();
        else if(attrname.compare("verifyMinValue", Qt::CaseInsensitive) == 0)
            verifyMinString = attr.value().trimmed();
        else if(attrname.compare("verifyMaxValue", Qt::CaseInsensitive) == 0)
            verifyMaxString = attr.value().trimmed();
        else if(attrname.compare("map", Qt::CaseInsensitive) == 0)
        {
            QString v = attr.value().trimmed().toLower();

            if(v.compare("encode") == 0)
                mapOptions = MAP_ENCODE;
            else if(v.compare("decode") == 0)
                mapOptions = MAP_DECODE;
            else if(ProtocolParser::isFieldSet(v))
                mapOptions = MAP_BOTH;
            else if (ProtocolParser::isFieldClear(v))
                mapOptions = MAP_NONE;
            else
                emitWarning("Value for 'map' field is incorrect: '" + v + "'");
        }

    }// for all attributes

    if(name.isEmpty() && (memoryTypeString != "null"))
    {
        emitWarning("Data tag without a name: " + e.text());
    }

    if(title.isEmpty())
        title = name;

    // maybe its an enum or a external struct?
    if(memoryTypeString.isEmpty())
    {
        // Maybe its an enum?
        if(!e.attribute("enum").isEmpty())
            memoryTypeString = "enum";
        else if(!e.attribute("struct").isEmpty())
            memoryTypeString = "struct";
        else
        {
            memoryTypeString = "null";
            emitWarning("failed to find inMemoryType attribute, \"null\" assumed.");
        }
    }

    // Extract the in memory type
    extractType(inMemoryType, memoryTypeString, name, true);

    // The encoded type string, this can be empty which implies encoded is same as memory
    if(encodedTypeString.isEmpty())
    {
        if(overridesPrevious)
            emitWarning("encodedType cannot be empty if inMemoryType is override");

        encodedType = inMemoryType;

        // Encoded types are never enums
        if(encodedType.isEnum)
            encodedType.isEnum = false;
    }
    else
    {
        extractType(encodedType, encodedTypeString, name, false);

        // This is just a warning pacifier, we won't learn until later what the
        // in memory type is
        if(overridesPrevious)
            inMemoryType = encodedType;
    }

    if(inMemoryType.isNull)
    {
        // Null types are not in memory, therefore cannot have defaults or variable arrays
        /// TODO: is this strictly true? It seems we could relax this requirement somewhat
        variableArray.clear();
        variable2dArray.clear();
        defaultString.clear();
        overridesPrevious = false;

        // A special case, where we use the encoded type data in place of the
        // in memory type. This handles cases where (for example) we want to
        // encode a constant bitfield, or a string, but have no data in memory
        if(!encodedType.isNull)
        {
            inMemoryType = encodedType;
            inMemoryType.isNull = true;
        }
        else
        {
            emitWarning("both in-memory and encoded types are \"null\", nothing to do.");
            return;
        }
    }

    if(inMemoryType.isEnum)
    {
        if(enumName.isEmpty())
        {
            emitWarning("enumeration name is missing, type changed to unsigned");
            inMemoryType.isEnum = encodedType.isEnum = false;
        }
        else
        {
            int minbits = 8;

            // Figure out the minimum number of bits for the enumeration
            const EnumCreator* creator = parser->lookUpEnumeration(enumName);
            if(creator != 0)
                minbits = creator->getMinBitWidth();

            if(encodedTypeString.isEmpty())
            {
                // Make it a multiple of 8 bits. The only way to have something
                // different is to encode as a bitfield, which means the
                // encoded string won't be empty
                if(minbits % 8)
                    minbits = ((minbits/8)+1)*8;

                encodedType.bits = minbits;

            }// if we don't have encoded length data
            else
            {
                // make sure the encoded length data is large enough
                if(encodedType.bits < minbits)
                {
                    emitWarning("enumeration needs at least " + QString::number(minbits) + " bits. Encoded bit length changed.");
                    if(!encodedType.isBitfield)
                    {
                        // Make it a multiple of 8 bits
                        if(minbits % 8)
                            minbits = ((minbits/8)+1)*8;
                    }

                    encodedType.bits = minbits;

                }// if not enough encoded bits

            }// else we do have encoded length data

        }// if we have the enumeration name

    }// if in memory data is enumeration

    if(inMemoryType.isStruct)
    {
        if(structName.isEmpty())
        {
            emitWarning("struct name is missing, struct name \"unknown\" used, probable compile failure");
            structName = "unknown";
        }

        if(!constantString.isEmpty() || checkConstant)
        {
            constantString.clear();
            checkConstant = false;
            emitWarning("structure cannot be a constant");
        }

        if(overridesPrevious)
        {
            overridesPrevious = false;
            emitWarning("structure cannot override a previous field");
        }

    }

    if(inMemoryType.isBitfield)
    {
        if(!encodedType.isNull)
        {
            if(!encodedTypeString.isEmpty() && !encodedType.isBitfield)
                emitWarning("encoded type ignored because in memory type is bitfield");

            // make the encoded type follow the in memory type for bit fields
            encodedType.isBitfield = true;
            encodedType.bits = inMemoryType.bits;
        }
    }

    // It is possible for the in memory type to not be a bit field, but the
    // encoding could be. The most common case for this would be an in-memory
    // enumeration in which the maximum enumeration fits in fewer than 8 bits
    if(encodedType.isBitfield)
    {
        // Do we start a bitfield group?
        if(bitfieldData.groupMember)
        {
            if(!defaultString.isEmpty())
            {
                emitWarning("bitfield groups cannot have default values");
                defaultString.clear();
            }

        }

        if(!dependsOn.isEmpty())
        {
            emitWarning("bitfields cannot use dependsOn");
            dependsOn.clear();
        }

        if(!array.isEmpty())
        {
            emitWarning("bitfields encodings cannot use arrays");
            array.clear();
            variableArray.clear();
            array2d.clear();
            variable2dArray.clear();
        }

        // We assume we are the last member of the bitfield, until we learn otherwise
        bitfieldData.lastBitfield = true;
    }
    else
    {
        if(bitfieldData.groupMember || bitfieldData.groupStart)
        {
            emitWarning("bitfieldGroup applied to non-bitfield, ignored");
            bitfieldData.groupBits = 0;
            bitfieldData.groupStart = false;
            bitfieldData.groupMember = false;
        }
    }

    // if either type says string, than they both are string
    if(inMemoryType.isString != encodedType.isString)
    {
        if(!inMemoryType.isNull && !encodedType.isNull)
            emitWarning("String type requires that inMemory and encoded types both be strings");

        inMemoryType.isString = encodedType.isString = true;
        inMemoryType.bits = encodedType.bits = 8;
    }

    // if either type says fixed string, than they both are fixed string
    if(inMemoryType.isFixedString != encodedType.isFixedString)
    {
        inMemoryType.isString = encodedType.isString = true;
        inMemoryType.isFixedString = encodedType.isFixedString = true;
        inMemoryType.bits = encodedType.bits = 8;
    }

    if(array.isEmpty() && !variableArray.isEmpty())
    {
        emitWarning("Must specify array length to specify variable array length");
        variableArray.clear();
    }

    if(array.isEmpty() && !array2d.isEmpty())
    {
        emitWarning("Must specify array length to specify second dimension array length");
        array2d.clear();
    }

    if(array2d.isEmpty() && !variable2dArray.isEmpty())
    {
        emitWarning("Must specify array 2d length to specify variable 2d array length");
        variable2dArray.clear();
    }

    if(!scalerString.isEmpty() && !maxString.isEmpty())
    {
        emitWarning("scaler ignored because max is provided");
        scalerString.clear();
    }

    // handle the scaler used for printing and comparing
    if(!printScalerString.isEmpty())
    {
        bool ok;
        double printScaler = ShuntingYard::computeInfix(printScalerString, &ok);

        if(ok)
        {
            printScalerString = "*" + QString::number(printScaler, 'g', 16);
            readScalerString = "/" + QString::number(printScaler, 'g', 16);
        }
        else
        {
            emitWarning("Print scaler is not a number, scaling during printing or comparing will not be done");
            printScalerString = "";
            readScalerString = "";
        }

    }

    if(!maxString.isEmpty() || !minString.isEmpty() || !scalerString.isEmpty())
    {
        if(inMemoryType.isStruct || inMemoryType.isString || encodedType.isNull)
        {
            emitWarning("min, max, and scaler do not apply to this type data");
            maxString.clear();
            minString.clear();
            scalerString.clear();
        }

    }

    if(!maxString.isEmpty() || !minString.isEmpty())
    {
        if(encodedType.isFloat)
        {
            emitWarning("min, max, are ignored because encoded type is float");
            maxString.clear();
            minString.clear();
        }

    }

    if(constantString.isEmpty() && checkConstant)
    {
        emitWarning("\"checkConstant\" cannot be applied unless the field is constant");
        checkConstant = false;
    }

    if(inMemoryType.isString)
    {
        // Strings have to be arrays, default to 64 characters
        if(array.isEmpty())
        {
            emitWarning("string length not provided, assuming 64");
            array = "64";
        }

        // Strings are always variable length, through null termination
        if(!variableArray.isEmpty())
        {
            emitWarning("strings cannot use variableAray attribute, they are always variable length through null termination (unless fixedstring)");
            variableArray.clear();
        }

        if(!array2d.isEmpty())
        {
            emitWarning("2d arrays not allowed for strings");
            array2d.clear();
            variable2dArray.clear();
        }

        if(!dependsOn.isEmpty())
        {
            emitWarning("strings cannot use dependsOn");
            dependsOn.clear();
        }

    }// if string
    else if(!array.isEmpty())
    {
        if(checkConstant)
        {
            emitWarning("\"checkConstant\" cannot be applied to arrays (except strings) ");
            checkConstant = false;
        }

    }// if not string

    if(encodedType.isNull)
    {
        if(!constantString.isEmpty())
        {
            emitWarning("constant value does not make sense for types that are not encoded (null)");
            constantString.clear();
            checkConstant = false;
        }

        if(!variableArray.isEmpty() || !variable2dArray.isEmpty())
        {
            emitWarning("variable length arrays do not make sense for types that are not encoded (null)");
            variableArray.clear();
            variable2dArray.clear();
        }

        if(!dependsOn.isEmpty())
        {
            emitWarning("dependsOn does not make sense for types that are not encoded (null)");
            dependsOn.clear();
        }
    }

    bool ok;

    if(!minString.isEmpty())
    {
        if(encodedType.isSigned)
        {
            emitWarning("min value ignored because encoded type is signed");
            minString.clear();
        }
        else
        {
            encodedMin = ShuntingYard::computeInfix(minString, &ok);

            if(!ok)
            {
                emitWarning("min is not a number, 0.0 assumed");
                minString = "0";
            }
            else
            {
                // Special case handling here, if the user just wants a min value, that implies that scaler is 1.0
                if(maxString.isEmpty() && scalerString.isEmpty())
                    scalerString = "1";
            }
        }
    }

    if(!maxString.isEmpty())
    {
        encodedMax = ShuntingYard::computeInfix(maxString, &ok);
        if(!ok)
        {
            emitWarning("max is not a number, 1.0 assumed");
            encodedMax = 1.0;
            maxString = "1";
        }

        if(encodedType.isSigned)
        {
            scaler = (powf(2.0, encodedType.bits-1) - 1.0)/encodedMax;

            scalerString = QString().setNum(pow2(encodedType.bits-1)-1)+ "/(" + maxString + ")";

            // This is not exactly true, there is one more bit that could be used,
            // but this makes conciser commenting, and is clearer to the user
            encodedMin = -encodedMax;
            minString = "-" + maxString;
        }
        else
        {
            scaler = (powf(2.0, encodedType.bits) - 1.0)/(encodedMax - encodedMin);

            if(encodedMin == 0.0)
            {
                minString = "0";
                scalerString = QString().setNum(pow2(encodedType.bits)-1)+ "/(" + maxString + ")";
            }
            else
            {
                // If the user gives us something like 145 for max and -5 for min, I would rather just put 150 in the documentation
                QString denominator = "(" + maxString + " - " + minString + ")";

                // Documentation is only improved if maxString and minString are simple numbers, not formulas
                if(ShuntingYard::isNumber(maxString) && ShuntingYard::isNumber(minString))
                {
                    bool ok;
                    double number = ShuntingYard::computeInfix(denominator, &ok);
                    if(ok)
                        denominator = getNumberString(number);
                }

                scalerString = QString().setNum(pow2(encodedType.bits)-1)+ "/" + denominator;
            }
        }
    }
    else if(!scalerString.isEmpty())
    {
        scaler = ShuntingYard::computeInfix(scalerString, &ok);

        if(!ok)
        {
            emitWarning("scaler is not a number, 1.0 assumed");
            scaler = 1.0;
            scalerString = "1.0";
        }
        else if(scaler <= 0.0)
        {
            emitWarning("scaler must be greater than zero, 1.0 used");
            scaler = 1.0;
            scalerString = "1.0";
        }

        if(encodedType.isFloat)
        {
            // Floating point scaling does not have min and max.
            encodedMax = encodedMin = 0;
            minString = maxString = "";
        }
        else if(encodedType.isSigned)
        {
            encodedMax = (powf(2.0, encodedType.bits-1) - 1.0)/scaler;
            maxString = QString().setNum(pow2(encodedType.bits-1)-1)+ "/(" + scalerString + ")";

            // This is not exactly true, there is one more bit that could be used,
            // but this makes conciser commenting, and is clearer to the user
            encodedMin = -encodedMax;
            minString = "-" + maxString;
        }
        else
        {
            encodedMax = encodedMin + (powf(2.0, encodedType.bits) - 1.0)/scaler;

            // Make sure minString isn't empty
            if(encodedMin == 0)
            {
                minString = "0";
                maxString = QString().setNum(pow2(encodedType.bits)-1)+ "/(" + scalerString + ")";
            }
            else
                maxString = minString + " + " + QString().setNum(pow2(encodedType.bits)-1)+ "/(" + scalerString + ")";
        }

    }// if scaler string

    // Max must be larger than minimum
    if(encodedMin > encodedMax)
    {
        encodedMin = encodedMax = 0.0;
        minString.clear();
        maxString.clear();
        scalerString.clear();
        scaler = 1.0;
        emitWarning("max is not more than min, encoding not scaled");
    }

    if(inMemoryType.isFloat && !encodedType.isFloat && !inMemoryType.isNull && !encodedType.isNull)
    {
        // If the user wants to convert a float to an integer they should be
        // applying a scaler. If they don't then there is the potential for
        // truncation and overflow problems. However its possible they actually
        // *want* the truncation, hence warning without fixing.
        if(encodedMin == encodedMax)
        {
            emitWarning("Casting float to integer without truncation, consider setting scaler=\"1.0\"");
        }
    }

    // Just the type data
    typeName = inMemoryType.toTypeString(enumName, support.prefix + structName);

    if(!constantString.isEmpty())
    {
        if(inMemoryType.isStruct)
        {
            emitWarning("structure cannot have a constant value");
            constantString.clear();
        }
        else if(!defaultString.isEmpty())
        {
            emitWarning("fields with default values cannot also be constant");
            constantString.clear();
        }
    }

    // Make sure no keyword conflicts
    checkAgainstKeywords();

    // Check if the verify max string is "auto" and handle it
    if(verifyMaxString.compare("auto", Qt::CaseInsensitive) == 0)
    {
        // Automatically figure out the value of the max
        if(maxString.isEmpty())
        {
            // In this case we don't already have scaling information, so we need to work out the value ourselves
            if(encodedType.isSigned)
                verifyMaxString = QString().setNum(pow2(encodedType.bits-1)-1);
            else
                verifyMaxString = QString().setNum(pow2(encodedType.bits)-1);
        }
        else
            verifyMaxString = getNumberString(encodedMax, encodedType.bits);

    }// if we need to automagically determine the max verify string

    // Check if the verify min string is "auto" and handle it
    if(verifyMinString.compare("auto", Qt::CaseInsensitive) == 0)
    {
        // Automatically figure out the value of the min
        if(minString.isEmpty())
        {
            // In this case we don't already have scaling information, so we need to work out the value ourselves
            if(encodedType.isSigned)
                verifyMinString = QString().setNum(-1*pow2(encodedType.bits-1));
            else
                verifyMinString = "0";
        }
        else
            verifyMinString = getNumberString(encodedMin, encodedType.bits);

    }// if we need to automagically determine the min verify string

    // Support the case where a numeric string uses "pi" or "e".
    defaultStringForDisplay = handleNumericConstants(defaultString);
    constantStringForDisplay = handleNumericConstants(constantString);
    verifyMaxStringForDisplay = handleNumericConstants(verifyMaxString);
    verifyMinStringForDisplay = handleNumericConstants(verifyMinString);
    initialValueStringForDisplay = handleNumericConstants(initialValueString);

    // Update these strings for display purposes
    maxString = handleNumericConstants(maxString);
    minString = handleNumericConstants(minString);
    scalerString = handleNumericConstants(scalerString);

    // Compute the data length
    computeEncodedLength();

}// ProtocolField::parse


/*!
 * Take an input string, which may be a number, and handle instances of "pi" or
 * "e", modifying the input string, and generating an output string which is
 * suitable for documentation purposes.
 * \param input is the input string, which may be modified
 * \return a documentation version of input
 */
QString ProtocolField::handleNumericConstants(QString& input) const
{
    if(input.isEmpty())
        return input;

    bool ok;
    QString display = input;

    // Determine if the input string is a number, which might have numeric constants (pi or e)
    ShuntingYard::computeInfix(input, &ok);

    if(ok)
    {
        // For the input string, we replace the symbols "pi" and "e" with their
        // numeric values so the code which uses this string will compile. We
        // cannot do this replacement without first knowing input is a number,
        // otherwise we'll just be screwing up the name of something
        ShuntingYard::replacePie(input);

        // For the display string we replace the symbol "pi" with the
        // appropriate value for html outputs
        display.replace("pi", "&pi;", Qt::CaseInsensitive);

        // Change to get rid of * multiply symbol, which plays havoc with markdown
        // We put spaces around the multiply symbol, so that the html tables can
        // better reflow the resulting text
        display.replace("*", " &times; ");
    }

    return display;

}// ProtocolField::handleNumericConstants


/*!
 * Compute the encoded length of this field
 */
void ProtocolField::computeEncodedLength(void)
{
    encodedLength.clear();

    if(encodedType.isNull)
        return;

    if(encodedType.isBitfield)
    {
        if(bitfieldData.groupMember)
        {
            // If we are a group member, we need to figure out the number of bits in
            // the group, we can only do this if we are the last member of the group
            if(bitfieldData.lastBitfield)
            {
                int bits = 0;
                ProtocolField* prev = this;

                // Count backwards until our bitfield group ends in order to
                // determine the number of bits in the group
                while(prev != NULL)
                {
                    if(prev->encodedType.isBitfield && prev->bitfieldData.groupMember)
                    {
                        bits += prev->encodedType.bits;

                        if(prev->bitfieldData.groupStart)
                            break;
                        else
                            prev = prev->prevField;
                    }
                    else
                        break;
                }

                // Now we know the group bits, apply this to all members of the group
                prev = this;
                while(prev != NULL)
                {
                    if(prev->encodedType.isBitfield && prev->bitfieldData.groupMember)
                    {
                        prev->bitfieldData.groupBits = bits;

                        if(prev->bitfieldData.groupStart)
                            break;
                        else
                            prev = prev->prevField;
                    }
                    else
                        break;
                }

                // groupBits is visible to all fields in the group, but we only
                // want to count it once, so we only count for the lastBitfield
                encodedLength.addToLength(QString().setNum((bitfieldData.groupBits+7)/8), false, false, false, !defaultString.isEmpty());

            }// if we are the last member of the group

        }// if we are a group member
        else
        {
            int length = 0;

            // As a bitfield our length in bytes is given by the number of 8 bit boundaries we cross
            int bitcount = bitfieldData.startingBitCount;
            while(bitcount < getEndingBitCount())
            {
                bitcount++;
                if((bitcount % 8) == 0)
                    length++;
            }

            // If we are the last bitfield, and if we have any bits left, then add a byte
            if(bitfieldData.lastBitfield && ((bitcount % 8) != 0))
                length++;

            encodedLength.addToLength(QString().setNum(length), false, false, false, !defaultString.isEmpty());
        }
    }
    else if(inMemoryType.isString)
        encodedLength.addToLength(array, !inMemoryType.isFixedString, false, !dependsOn.isEmpty(), !defaultString.isEmpty());
    else if(inMemoryType.isStruct)
    {
        encodedLength.clear();

        const ProtocolStructure* struc = parser->lookUpStructure(typeName);

        // Account for array, variable array, and depends on
        if(struc != NULL)
            encodedLength.addToLength(struc->encodedLength, array, !variableArray.isEmpty() || !variable2dArray.isEmpty(), !dependsOn.isEmpty(), array2d);
        else
        {
            if(is2dArray())
                encodedLength.addToLength("getMinLengthOf" + typeName + "()*" + array + "*" + array2d, false, !variableArray.isEmpty() || !variable2dArray.isEmpty(), !dependsOn.isEmpty(), (!defaultString.isEmpty()) || overridesPrevious);
            else if(isArray())
                encodedLength.addToLength("getMinLengthOf" + typeName + "()*" + array, false, !variableArray.isEmpty(), !dependsOn.isEmpty(), (!defaultString.isEmpty()) || overridesPrevious);
            else
                encodedLength.addToLength("getMinLengthOf" + typeName + "()"         , false, false,                    !dependsOn.isEmpty(), (!defaultString.isEmpty()) || overridesPrevious);
        }
    }
    else
    {
        QString lengthString;

        lengthString.setNum(encodedType.bits / 8);

        // Remember that we could be encoding an array
        if(isArray())
            lengthString += "*" + array;

        if(is2dArray())
            lengthString += "*" + array2d;

        encodedLength.addToLength(lengthString, false, !variableArray.isEmpty() || !variable2dArray.isEmpty(), !dependsOn.isEmpty(), (!defaultString.isEmpty()) || overridesPrevious);

    }

}// ProtocolField::computeEncodedLength


//! Check names against the list of C keywords
void ProtocolField::checkAgainstKeywords(void)
{
    Encodable::checkAgainstKeywords();

    if(keywords.contains(enumName))
    {
        emitWarning("enum name matches C keyword, changed to _name");
        enumName = "_" + enumName;
    }

    if(keywords.contains(maxString))
    {
        emitWarning("max value matches C keyword, changed to _max");
        maxString = "_" + maxString;
    }

    if(keywords.contains(minString))
    {
        emitWarning("min value matches C keyword, changed to _min");
        minString = "_" + minString;
    }

    if(keywords.contains(scalerString))
    {
        emitWarning("scaler value matches C keyword, changed to _scaler");
        scalerString = "_" + scalerString;
    }

    if(keywords.contains(defaultString))
    {
        emitWarning("default value matches C keyword, changed to _default");
        defaultString = "_" + defaultString;
    }

    if(keywords.contains(constantString))
    {
        emitWarning("constant value matches C keyword, changed to _constant");
        constantString = "_" + constantString;
    }

}// ProtocolField::checkAgainstKeywords


/*!
 * Get the declaration for this field as a member of a structure
 * \return the declaration string
 */
QString ProtocolField::getDeclaration(void) const
{
    QString output;

    if(isNotInMemory())
        return output;

    output = "    " + typeName + " " + name;

    if(inMemoryType.isBitfield)
        output += " : " + QString().setNum(inMemoryType.bits);
    else if(is2dArray())
        output += "[" + array + "][" + array2d + "]";
    else if(isArray())
        output += "[" + array + "]";

    output += ";";

    if(comment.isEmpty())
    {
        if(!constantString.isEmpty())
            output += " //!< Field is encoded constant.";
    }
    else
    {
        output += " //!< " + comment;
        if(!constantString.isEmpty())
            output += ". Field is encoded constant.";
    }

    output += "\n";

    return output;

}// ProtocolField::getDeclaration


/*!
 * Append the include directives needed for this encodable. Mostly this is empty,
 * but for external structures or enumerations we need to bring in the include file
 * \param list is appended with any directives this encodable requires.
 */
void ProtocolField::getIncludeDirectives(QStringList& list) const
{
    QString include;

    // Array sizes could be enumerations that need an include directive
    if(!array.isEmpty())
    {
        include = parser->lookUpIncludeName(array);
        if(!include.isEmpty())
            list.append(include);

        // Array sizes could be enumerations that need an include directive
        if(!array2d.isEmpty())
        {
            include = parser->lookUpIncludeName(array2d);
            if(!include.isEmpty())
                list.append(include);
        }
    }

    if(inMemoryType.isEnum || inMemoryType.isStruct)
    {
        include = parser->lookUpIncludeName(typeName);
        if(include.isEmpty())
        {
            // we don't warn for enumeration include failures, they are (potentially) less serious
            if(inMemoryType.isStruct)
                emitWarning("unknown include for \"" + typeName + "\"");
        }
        else
            list.append(include);

    }// if enum or struct

    // Only need one of each include
    list.removeDuplicates();

}// ProtocolField::getIncludeDirectives


/*!
 * Append the include directives needed for this encodable. Mostly this is empty,
 * but for external structures or enumerations we need to bring in the include file
 * \param list is appended with any directives this encodable requires.
 */
void ProtocolField::getInitAndVerifyIncludeDirectives(QStringList& list) const
{
    if(inMemoryType.isStruct)
    {
        const ProtocolStructureModule* struc = parser->lookUpStructure(typeName);

        if(struc != NULL)
            struc->getInitAndVerifyIncludeDirectives(list);

    }// else if struct

}// ProtocolField::getInitAndVerifyIncludeDirectives


/*!
 * Return the signature of this field in an encode function signature
 * \return The encode signature of this field
 */
QString ProtocolField::getEncodeSignature(void) const
{
    if(isNotEncoded() || isNotInMemory() || isConstant())
        return "";
    else if(is2dArray())
        return ", const " + typeName + " " + name + "[" + array + "][" + array2d + "]";
    else if(isArray())
        return ", const " + typeName + " " + name + "[" + array + "]";
    else if(!inMemoryType.isStruct)
        return ", " + typeName + " " + name;
    else
        return ", const " + typeName + "* " + name;
}


/*!
 * Get details needed to produce documentation for this encodable.
 * \param outline gives the paragraph numbers for all the fields up to this one.
 * \param startByte is the starting byte location of this encodable, which will be updated for the following encodable.
 * \param bytes is appended for the byte range of this encodable.
 * \param names is appended for the name of this encodable.
 * \param encodings is appended for the encoding of this encodable.
 * \param repeats is appended for the array information of this encodable.
 * \param comments is appended for the description of this encodable.
 */
void ProtocolField::getDocumentationDetails(QList<int>& outline, QString& startByte, QStringList& bytes, QStringList& names, QStringList& encodings, QStringList& repeats, QStringList& comments) const
{
    QString description;
    QString maxEncodedLength = encodedLength.maxEncodedLength;

    if(encodedType.isNull || hidden)
        return;

    // See if we can replace any enumeration names with values
    parser->replaceEnumerationNameWithValue(maxEncodedLength);

    // The byte after this one
    QString nextStartByte = EncodedLength::collapseLengthString(startByte + "+" + maxEncodedLength);

    // The length data
    if(encodedType.isBitfield)
    {
        QString range;

        // the starting bit count is the full count, not the count in the byte
        int startCount = bitfieldData.startingBitCount % 8;

        if(startByte.isEmpty())
            range = "0:" + QString().setNum(7 - startCount);
        else
            range = startByte + ":" + QString().setNum(7 - startCount);

        if(encodedType.bits > 1)
        {
            int endCount = (startCount + encodedType.bits - 1);

            int byteCount = endCount / 8;

            QString endByte = EncodedLength::collapseLengthString(startByte + "+" + QString().setNum(byteCount), true);

            range += "..." + endByte + ":" + QString().setNum(7 - (endCount%8));
        }

        bytes.append(range);
    }
    else
    {
        if(maxEncodedLength.isEmpty() || (maxEncodedLength.compare("1") == 0))
            bytes.append(startByte);
        else
        {
            QString endByte = EncodedLength::subtractOneFromLengthString(nextStartByte);

            // The range of the data
            bytes.append(startByte + "..." + endByte);
        }
    }

    // The name information
    outline.last() += 1;
    QString outlineString;
    outlineString.setNum(outline.at(0));
    for(int i = 1; i < outline.size(); i++)
        outlineString += "." + QString().setNum(outline.at(i));

    if(inMemoryType.isEnum)
    {
        // Link to the enumeration
        outlineString += ")[" + title +"](#" + enumName + ")";
    }
    else
        outlineString += ")" + title;

    names.append(outlineString);

    if(inMemoryType.isStruct)
    {
        // Encoding is blank for structures
        encodings.append(QString());

        // Repeats
        repeats.append(getRepeatsDocumentationDetails());

        // Fourth column is the commenting
        description += comment;

        if(!dependsOn.isEmpty())
        {
            if(!description.endsWith('.'))
                description += ".";

            description += " Only included if " + dependsOn + " is non-zero.";
        }

        if(description.isEmpty())
            comments.append(QString());
        else
            comments.append(description);

        QString subStartByte = startByte;

        // Now go get the substructure stuff
        parser->getStructureSubDocumentationDetails(typeName, outline, subStartByte, bytes, names, encodings, repeats, comments);

    }// if structure
    else
    {
        // The encoding
        if(encodedType.isFixedString)
        {
            encodings.append("Zero terminated string of exactly " + array + " bytes");
            repeats.append(QString());
        }
        else if( encodedType.isString)
        {
            encodings.append("Zero-terminated string up to " + array + " bytes");
            repeats.append(QString());
        }
        else
        {
            if(encodedType.isBitfield)
                encodings.append("B" + QString().setNum(encodedType.bits));
            else if(encodedType.isFloat)
            {
                if(encodedType.bits < 32)
                    encodings.append("F" + QString().setNum(encodedType.bits) + ":" + QString::number(encodedType.sigbits));
                else
                    encodings.append("F" + QString().setNum(encodedType.bits));
            }
            else if(encodedType.isSigned)
                encodings.append("I" + QString().setNum(encodedType.bits));
            else
                encodings.append("U" + QString().setNum(encodedType.bits));

            // Third column is the repeat/array column
            repeats.append(getRepeatsDocumentationDetails());
        }

        // Fourth column is the commenting
        if(inMemoryType.isNull)
        {
            if(!comment.isEmpty())
                description += comment;
            else
            {
                if(encodedType.isBitfield)
                    description += "Reserved bits in the packet.";
                else
                    description += "Reserved bytes in the packet.";
            }
        }
        else
            description += comment;

        if(!description.isEmpty() && !description.endsWith('.'))
            description += ".";

        if((encodedMax > encodedMin) && (encodedType.isFloat))
            description += "<br>Scaled by " + scalerString + ".";
        else if(isFloatScaling() || isIntegerScaling())
            description += "<br>Scaled by " + scalerString + " from " + getDisplayNumberString(encodedMin) + " to " + getDisplayNumberString(encodedMax) + ".";

        if(!constantString.isEmpty())
            description += "<br>Data are given constant value on encode " + constantStringForDisplay + ".";

        if(!dependsOn.isEmpty())
            description += "<br>Only included if " + dependsOn + " is non-zero.";

        if(!defaultString.isEmpty())
            description += "<br>This field is optional. If it is not included then the value is assumed to be " + defaultStringForDisplay + ".";

        if(overridesPrevious)
            description += "<br>This field overrides the previous field of the same name, if the packet is long enough.";

        for (int i=0;i<extraInfoNames.count();i++)
        {
            if ((extraInfoValues.count() > i) && (!extraInfoValues.at(i).isEmpty()))
                description += "<br>" + extraInfoNames.at(i) + ": " + extraInfoValues.at(i) + ".";
        }

        // StringList cannot be empty
        if(description.isEmpty())
            comments.append(QString());
        else
            comments.append(description);

    }// else if not structure

    // Update startByte for following encodables
    startByte = nextStartByte;

}// ProtocolField::getDocumentationDetails


/*!
 * Get the next lines(s) of source coded needed to encode this field
 * \param isBigEndian should be true for big endian encoding.
 * \param bitcount points to the running count of bits in a bitfields and should persist between calls
 * \param isStructureMember should be true if the left hand side is a
 *        member of a user structure, else the left hand side is a pointer
 *        to the inMemoryType
 * \return The string to add to the source file that encodes this field.
 */
QString ProtocolField::getEncodeString(bool isBigEndian, int* bitcount, bool isStructureMember) const
{
    QString output;

    if(encodedType.isBitfield)
    {
        output = getEncodeStringForBitfield(bitcount, isStructureMember);
    }
    else
    {
        if(inMemoryType.isString)
            output += getEncodeStringForString(isStructureMember);
        else if(inMemoryType.isStruct)
            output += getEncodeStringForStructure(isStructureMember);
        else
            output += getEncodeStringForField(isBigEndian, isStructureMember);
    }

    return output;

}


/*!
 * Get the next lines(s) of source coded needed to decode this field
 * \param isBigEndian should be true for big endian encoding.
 * \param bitcount points to the running count of bits in a bitfields and should persist between calls
 * \param isStructureMember should be true if the left hand side is a
 *        member of a user structure, else the left hand side is a pointer
 *        to the inMemoryType
 * \param defaultEnabled should be true to handle defaults
 * \return The string to add to the source file that encodes this field.
 */
QString ProtocolField::getDecodeString(bool isBigEndian, int* bitcount, bool isStructureMember, bool defaultEnabled) const
{
    QString output;

    if(encodedType.isBitfield)
        output += getDecodeStringForBitfield(bitcount, isStructureMember, defaultEnabled);
    else if(inMemoryType.isString)
        output += getDecodeStringForString(isStructureMember, defaultEnabled);
    else if(inMemoryType.isStruct)
        output += getDecodeStringForStructure(isStructureMember);
    else
        output += getDecodeStringForField(isBigEndian, isStructureMember, defaultEnabled);

    return output;
}


//! True if this encodable has verification data
bool ProtocolField::hasVerify(void) const
{
    if(inMemoryType.isStruct)
    {
        const ProtocolStructure* struc = parser->lookUpStructure(typeName);
        if(struc)
            return struc->hasVerify();
        else
            return false;
    }
    else
    {
        if(verifyMaxString.isEmpty() && verifyMinString.isEmpty())
            return false;
        else
            return true;
    }

}// ProtocolField::hasVerify


//! True if this encodable has initialization data
bool ProtocolField::hasInit(void) const
{
    if(inMemoryType.isStruct)
    {
        const ProtocolStructure* struc = parser->lookUpStructure(typeName);
        if(struc)
            return struc->hasInit();
        else
            return false;
    }
    else
    {
        if(initialValueString.isEmpty())
            return false;
        else
            return true;
    }

}// ProtocolField::hasInit


/*!
 * Get the string used for verifying this field. If there is no verification data the string will be empty
 * \param isStructureMember should be true if this member is referenced by structure
 * \return the string used for verifying this field, which may be blank
 */
QString ProtocolField::getVerifyString(bool isStructureMember) const
{
    if(!hasVerify())
        return QString();

    // No verify for null or string
    if(inMemoryType.isNull || inMemoryType.isString)
        return QString();

    if(inMemoryType.isStruct)
    {
        QString output;
        QString access;

        if(!comment.isEmpty())
            output += TAB_IN + "// " + comment + "\n";

        if(isArray())
        {
            QString spacing;
            output += TAB_IN + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";

            if(isStructureMember)
                access = "&_pg_user->" + name + "[_pg_i]";
            else
                access = "&" + name + "[_pg_i]";

            // Handle 2D array
            if(is2dArray())
            {
                access += "[_pg_j]";
                spacing += TAB_IN;
                output += TAB_IN + TAB_IN + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";
            }

            output += TAB_IN + TAB_IN + spacing + "if(!verify" + typeName + "(" + access + "))\n";
            output += TAB_IN + TAB_IN + spacing + TAB_IN + "_pg_good = 0;\n";
        }
        else
        {
            if(isStructureMember)
                access = "&_pg_user->" + name;
            else
                access = name;  // in this case, name is already pointer, so we don't need "&"

            output += TAB_IN + "if(!verify" + typeName + "(" + access + "))\n";
            output += TAB_IN + TAB_IN + "_pg_good = 0;\n";
        }

        return output;

    }
    else
    {
        if(verifyMaxString.isEmpty() && verifyMinString.isEmpty())
            return QString();

        QString output;
        QString access;

        if(!comment.isEmpty())
            output += TAB_IN + "// " + comment + "\n";

        if(isArray())
        {
            QString spacing;
            output += TAB_IN + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";

            if(isStructureMember)
                access = "_pg_user->" + name + "[_pg_i]";
            else
                access = name + "[_pg_i]";

            // Handle 2D array
            if(is2dArray())
            {
                access += "[_pg_j]";
                spacing += TAB_IN;
                output += TAB_IN + TAB_IN + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";
            }

            output += TAB_IN + spacing + "{\n";
            if(!verifyMinString.isEmpty())
            {
                output += TAB_IN + TAB_IN + spacing + "if(" + access + " < " + verifyMinString + ")\n";
                output += TAB_IN + TAB_IN + spacing + "{\n";
                output += TAB_IN + TAB_IN + spacing + TAB_IN + access + " = " + verifyMinString + ";\n";
                output += TAB_IN + TAB_IN + spacing + TAB_IN + "_pg_good = 0;\n";
                output += TAB_IN + TAB_IN + spacing + "}\n";
            }

            if(!verifyMaxString.isEmpty())
            {
                QString choice = "else if(";
                if(verifyMinString.isEmpty())
                    choice = "if(";

                output += TAB_IN + TAB_IN + spacing + choice + access + " > " + verifyMaxString + ")\n";
                output += TAB_IN + TAB_IN + spacing + "{\n";
                output += TAB_IN + TAB_IN + spacing + TAB_IN + access + " = " + verifyMaxString + ";\n";
                output += TAB_IN + TAB_IN + spacing + TAB_IN + "_pg_good = 0;\n";
                output += TAB_IN + TAB_IN + spacing + "}\n";
            }

            output += TAB_IN + spacing + "}\n";

        }// if field is array
        else
        {
            if(isStructureMember)
                access = "_pg_user->" + name;
            else
                access = "*" + name;

            if(!verifyMinString.isEmpty())
            {
                output += TAB_IN + "if(" + access + " < " + verifyMinString + ")\n";
                output += TAB_IN + "{\n";
                output += TAB_IN + TAB_IN + access + " = " + verifyMinString + ";\n";
                output += TAB_IN + TAB_IN + "_pg_good = 0;\n";
                output += TAB_IN + "}\n";
            }

            if(!verifyMaxString.isEmpty())
            {
                QString choice = "else if(";
                if(verifyMinString.isEmpty())
                    choice = "if(";

                output += TAB_IN + choice + access + " > " + verifyMaxString + ")\n";
                output += TAB_IN + "{\n";
                output += TAB_IN + TAB_IN + access + " = " + verifyMaxString + ";\n";
                output += TAB_IN + TAB_IN + "_pg_good = 0;\n";
                output += TAB_IN + "}\n";
            }

        }// else if not array

        return output;

    }// else if not struct

}// ProtocolField::getVerifyString


/*!
* Get the string used for coparing this field.
* \param isStructureMember should be true to indicate this field is accessed as a member structure
* \return the string used to compare this field, which may be empty
*/
QString ProtocolField::getComparisonString(bool isStructureMember) const
{
    QString output;

    // No comparison if nothing is in memory or if not encoded
    if(inMemoryType.isNull || encodedType.isNull)
        return output;

    QString access1, access2;

    if(isStructureMember)
    {
        access1 = "_pg_user1->" + name;
        access2 = "_pg_user2->" + name;
    }
    else
    {
        access1 = name + "_1";
        access2 = name + "_2";
    }

    if(inMemoryType.isString)
    {
        output += TAB_IN + "if(QString::compare(" + access1 + ", " + access2 + ") != 0)\n";
        output += TAB_IN + TAB_IN + "_pg_report += _pg_prename + \":" + name + " strings differ\\n\";\n";
    }
    else
    {
        QString spacing = TAB_IN;
        bool closearraytest = false;
        bool closearraytest2 = false;
        bool closeforloop = false;
        bool closeforloop2 = false;

        if(isArray())
        {
            if(!variableArray.isEmpty() && isStructureMember)
            {
                output += spacing + "if(_pg_user1->" + variableArray + " == _pg_user2->" + variableArray + ")\n";
                output += spacing + "{\n";
                spacing += TAB_IN;
                closearraytest = true;

                output += spacing + "for(_pg_i = 0; (_pg_i < " + array + ") && (_pg_i < (unsigned)_pg_user1->" + variableArray + "); _pg_i++)\n";
                output += spacing + "{\n";
                spacing += TAB_IN;
                closeforloop = true;
            }
            else
            {
                output += spacing + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";
                spacing += TAB_IN;
            }

            access1 = access1 + "[_pg_i]";
            access2 = access2 + "[_pg_i]";
        }

        if(is2dArray())
        {
            if(!variable2dArray.isEmpty() && isStructureMember)
            {
                output += spacing + "if(_pg_user1->" + variable2dArray + " == _pg_user2->" + variable2dArray + ")\n";
                output += spacing + "{\n";
                spacing += TAB_IN;
                closearraytest2 = true;

                output += spacing + "for(_pg_j = 0; (_pg_j < " + array2d + ") && (_pg_j < (unsigned)_pg_user1->" + variable2dArray + "); _pg_j++)\n";
                output += spacing + "{\n";
                spacing += TAB_IN;
                closeforloop2 = true;
            }
            else
            {
                output += spacing + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";
                spacing += TAB_IN;
            }

            access1 = access1 + "[_pg_j]";
            access2 = access2 + "[_pg_j]";
        }

        if(inMemoryType.isStruct)
        {
            output += spacing + "_pg_report += compare" + typeName + "(_pg_prename + \":" + name + "\"";

            if(isArray())
                output += " + \"[\" + QString::number(_pg_i) + \"]\"";

            if(is2dArray())
                output += " + \"[\" + QString::number(_pg_j) + \"]\"";

            // Structure compare we need to pass the address of the structure, not the object
            output += ", &" + access1 + ", &" + access2 + ");\n";
        }
        else
        {
            // Simple value comparison to generate the _pg_report
            output += spacing + "if(" + access1 + " != " + access2 + ")\n";

            // The _pg_report includes the _pg_prename and the name
            output += spacing + TAB_IN + "_pg_report += _pg_prename + \":" + name + "\" ";

            // The _pg_report needs to include the array indices
            if(isArray())
                output += " + \"[\" + QString::number(_pg_i) + \"]\"";

            if(is2dArray())
                output += " + \"[\" + QString::number(_pg_j) + \"]\"";

            // And finally the values that go into the _pg_report
            if(inMemoryType.isFloat || !printScalerString.isEmpty())
                output += " + \" '\" + QString::number(" + access1 + printScalerString + ", 'g', 16) + \"' '\" + QString::number(" + access2 + printScalerString + ", 'g', 16) + \"'\\n\";\n";
            else
                output += " + \" '\" + QString::number(" + access1 + ") + \"' '\" + QString::number(" + access2 + ") + \"'\\n\";\n";

        }// else not a struct

        if(closeforloop2)
        {
            spacing.remove(spacing.lastIndexOf(TAB_IN), TAB_IN.count());
            output += spacing + "}\n";
        }

        if(closearraytest2)
        {
            spacing.remove(spacing.lastIndexOf(TAB_IN), TAB_IN.count());
            output += spacing + "}\n";
            output += spacing + "else\n";
            output += spacing + TAB_IN + "_pg_report += _pg_prename + \":" + name + " 2nd array dimension differs, array not compared\\n\";\n";
        }

        if(closeforloop)
        {
            spacing.remove(spacing.lastIndexOf(TAB_IN), TAB_IN.count());
            output += spacing + "}\n";
        }

        if(closearraytest)
        {
            spacing.remove(spacing.lastIndexOf(TAB_IN), TAB_IN.count());
            output += spacing + "}\n";
            output += spacing + "else\n";
            output += spacing + TAB_IN + "_pg_report += _pg_prename + \":" + name + " array dimension differs, array not compared\\n\";\n";
        }

    }// else numeric output

    return output;

}// ProtocolField::getComparisonString


/*!
 * Get the string used for text printing this field.
 * \param isStructureMember should be true to indicate this field is accessed as a member structure
 * \return the string used to print this field as text, which may be empty
 */
QString ProtocolField::getTextPrintString(bool isStructureMember) const
{
    QString output;

    // No print if nothing is in memory or if not encoded
    if(inMemoryType.isNull || encodedType.isNull)
        return output;

    QString access;

    if(isStructureMember)
        access = "_pg_user->" + name;
    else
        access = name;

    if(inMemoryType.isString)
    {
        output += TAB_IN + "_pg_report += _pg_prename + \":" + name + " '\" + QString(" + access + ") + \"'\\n\";\n";
    }
    else
    {
        QString spacing = TAB_IN;
        bool closeforloop = false;
        bool closeforloop2 = false;

        if(isArray())
        {
            if(!variableArray.isEmpty() && isStructureMember)
            {
                output += spacing + "for(_pg_i = 0; (_pg_i < " + array + ") && (_pg_i < (unsigned)_pg_user->" + variableArray + "); _pg_i++)\n";
                output += spacing + "{\n";
                spacing += TAB_IN;
                closeforloop = true;
            }
            else
            {
                output += spacing + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";
                spacing += TAB_IN;
            }

            access = access + "[_pg_i]";
        }

        if(is2dArray())
        {
            if(!variable2dArray.isEmpty() && isStructureMember)
            {
                output += spacing + "for(_pg_j = 0; (_pg_j < " + array2d + ") && (_pg_j < (unsigned)_pg_user->" + variable2dArray + "); _pg_j++)\n";
                output += spacing + "{\n";
                spacing += TAB_IN;
                closeforloop2 = true;
            }
            else
            {
                output += spacing + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";
                spacing += TAB_IN;
            }

            access = access + "[_pg_j]";
        }

        if(inMemoryType.isStruct)
        {
            output += spacing + "_pg_report += textPrint" + typeName + "(_pg_prename + \":" + name + "\"";

            if(isArray())
                output += " + \"[\" + QString::number(_pg_i) + \"]\"";

            if(is2dArray())
                output += " + \"[\" + QString::number(_pg_j) + \"]\"";

            // Structure print we need to pass the address of the structure, not the object
            output += ", &" + access + ");\n";
        }
        else
        {
            // The _pg_report includes the _pg_prename and the name
            output += spacing + "_pg_report += _pg_prename + \":" + name + "\"";

            // The _pg_report needs to include the array indices
            if(isArray())
                output += " + \"[\" + QString::number(_pg_i) + \"]\"";

            if(is2dArray())
                output += " + \"[\" + QString::number(_pg_j) + \"]\"";

            // And finally the values go into the _pg_report
            if(inMemoryType.isFloat || !printScalerString.isEmpty())
                output += " + \" '\" + QString::number(" + access + printScalerString + ", 'g', 16) + \"'\\n\";\n";
            else
                output += " + \" '\" + QString::number(" + access + ") + \"'\\n\";\n";

        }// else not a struct

        if(closeforloop2)
        {
            spacing.remove(spacing.lastIndexOf(TAB_IN), TAB_IN.count());
            output += spacing + "}\n";
        }

        if(closeforloop)
        {
            spacing.remove(spacing.lastIndexOf(TAB_IN), TAB_IN.count());
            output += spacing + "}\n";
        }

    }// else numeric output

    return output;

}// ProtocolField::getTextPrintString


/*!
 * Get the string used for text reading this field.
 * \param isStructureMember should be true to indicate this field is accessed as a member structure
 * \return the string used to read this field as text, which may be empty
 */
QString ProtocolField::getTextReadString(bool isStructureMember) const
{
    QString output;

    // No print if nothing is in memory or if not encoded
    if(inMemoryType.isNull || encodedType.isNull)
        return output;

    QString access;

    if(isStructureMember)
        access = "_pg_user->" + name;
    else
        access = name;

    if(inMemoryType.isString)
    {
        output += TAB_IN + "strncpy(" + access + ", extractText(_pg_prename + \":" + name + "\", _pg_source, &_pg_fieldcount).toLatin1().constData(), " + array + ");\n";
    }
    else
    {
        QString spacing = TAB_IN;
        bool closeforloop = false;
        bool closeforloop2 = false;

        if(isArray())
        {
            closeforloop = true;

            if(!variableArray.isEmpty() && isStructureMember)
                output += spacing + "for(_pg_i = 0; (_pg_i < " + array + ") && (_pg_i < (unsigned)_pg_user->" + variableArray + "); _pg_i++)\n";
            else
                output += spacing + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";

            output += spacing + "{\n";
            spacing += TAB_IN;
            access = access + "[_pg_i]";
        }

        if(is2dArray())
        {
            closeforloop2 = true;

            if(!variable2dArray.isEmpty() && isStructureMember)
            {
                output += spacing + "for(_pg_j = 0; (_pg_j < " + array2d + ") && (_pg_j < (unsigned)_pg_user->" + variable2dArray + "); _pg_j++)\n";
                output += spacing + "{\n";
                spacing += TAB_IN;
            }
            else
            {
                output += spacing + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";
                output += spacing + "{\n";
                spacing += TAB_IN;
            }

            access = access + "[_pg_j]";
        }

        if(inMemoryType.isStruct)
        {
            output += spacing + "_pg_fieldcount += textRead" + typeName + "(_pg_prename + \":" + name + "\"";

            if(isArray())
                output += " + \"[\" + QString::number(_pg_i) + \"]\"";

            if(is2dArray())
                output += " + \"[\" + QString::number(_pg_j) + \"]\"";

            // Structure read, we need to pass the address of the structure, not the object
            output += ", _pg_source, &" + access + ");\n";
        }
        else
        {
            // First get the text
            output += spacing + "_pg_text = extractText(_pg_prename + \":" + name + "\"";

            // The array indices are part of the key text
            if(isArray())
                output += " + \"[\" + QString::number(_pg_i) + \"]\"";

            if(is2dArray())
                output += " + \"[\" + QString::number(_pg_j) + \"]\"";

            output += ", _pg_source, &_pg_fieldcount);\n";

            // Check the text and get a result if it is not empty
            output += spacing + "if(!_pg_text.isEmpty())\n";

            if(inMemoryType.isFloat || !readScalerString.isEmpty())
                output += spacing + TAB_IN + access + " = (" + typeName + ")(_pg_text.toDouble()" + readScalerString + ");\n";
            else if(inMemoryType.isSigned)
            {
                if(inMemoryType.bits > 32)
                    output += spacing + TAB_IN + access + " = (" + typeName + ")(_pg_text.toLongLong());\n";
                else
                    output += spacing + TAB_IN + access + " = (" + typeName + ")(_pg_text.toInt());\n";
            }
            else
            {
                if(inMemoryType.bits > 32)
                    output += spacing + TAB_IN + access + " = (" + typeName + ")(_pg_text.toULongLong());\n";
                else
                    output += spacing + TAB_IN + access + " = (" + typeName + ")(_pg_text.toUInt());\n";
            }

        }// else not a struct

        if(closeforloop2)
        {
            spacing.remove(spacing.lastIndexOf(TAB_IN), TAB_IN.count());
            output += spacing + "}\n";
        }

        if(closeforloop)
        {
            spacing.remove(spacing.lastIndexOf(TAB_IN), TAB_IN.count());
            output += spacing + "}\n";
        }

    }// else numeric output

    return output;

}// ProtocolField::getTextReadString


/*!
 * Get the string used for storing this field in a map.
 * \param isStructureMember should be true to indicate this field is accessed as a member structure
 * \return the string used to read this field as text, which may be empty
 */
QString ProtocolField::getMapEncodeString(bool isStructureMember) const
{  
    QString output;

    // Cannot encode to a null memory type
    if(inMemoryType.isNull)
        return output;

    // Return empty string if this field should not be encoded to map
    if((mapOptions & MAP_ENCODE) == 0)
        return output;

    if(inMemoryType.isNull || encodedType.isNull)
        return output;

    QString access;

    if(isStructureMember)
        access = "_pg_user->" + name;
    else
        access = name;

    if(!comment.isEmpty())
        output += TAB_IN + "// " + comment + "\n";

    if(inMemoryType.isString)
    {
        output += TAB_IN + "_pg_map[_pg_prename + \":" + name + "\"] = QString(" + access + ");\n";
    }
    else
    {
        QString spacing = TAB_IN;
        bool closeforloop = false;
        bool closeforloop2 = false;

        if(isArray())
        {
            closeforloop = true;

            if(!variableArray.isEmpty() && isStructureMember)
                output += spacing + "for(_pg_i = 0; (_pg_i < " + array + ") && (_pg_i < (unsigned)_pg_user->" + variableArray + "); _pg_i++)\n";
            else
                output += spacing + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";

            output += spacing + "{\n";
            spacing += TAB_IN;
            access = access + "[_pg_i]";
        }

        if(is2dArray())
        {
            closeforloop2 = true;

            if(!variable2dArray.isEmpty() && isStructureMember)
            {
                output += spacing + "for(_pg_j = 0; (_pg_j < " + array2d + ") && (_pg_j < (unsigned)_pg_user->" + variable2dArray + "); _pg_j++)\n";
                output += spacing + "{\n";
                spacing += TAB_IN;
            }
            else
            {
                output += spacing + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";
                output += spacing + "{\n";
                spacing += TAB_IN;
            }

            access = access + "[_pg_j]";
        }

        if(inMemoryType.isStruct)
        {
            output += spacing + "mapEncode" + typeName + "(_pg_prename + \":" + name + "\"";

            if(isArray())
                output += " + \"[\" + QString::number(_pg_i) + \"]\"";
            if(is2dArray())
                output += " + \"[\" + QString::number(_pg_j) + \"]\"";

            output += ", _pg_map, &" + access + ");\n";
        }// data type is a struct
        else
        {
            output += spacing + "_pg_map[_pg_prename + \":" + name + "\"";

            if(isArray())
                output += " + \"[\" + QString::number(_pg_i) + \"]\"";
            if(is2dArray())
                output += " + \"[\" + QString::number(_pg_j) + \"]\"";

            output += "] = ";

            // Numeric values are automatically converted to correct QVariant types
            if(inMemoryType.isFloat || !printScalerString.isEmpty())
                output += access + printScalerString;
            else
                output += access;

            output += ";\n";

        }// else not a struct

        if(closeforloop2)
        {
            spacing.remove(spacing.lastIndexOf(TAB_IN), TAB_IN.count());
            output += spacing + "}\n";
        }

        if(closeforloop)
        {
            spacing.remove(spacing.lastIndexOf(TAB_IN), TAB_IN.count());
            output += spacing + "}\n";
        }
    }// else numeric output

    return output;
}// ProtocolField::getMapEncodeString


/*!
 * Get the string used for extracting this field from a map.
 * \param isStructureMember should be true to indicate this field is accessed as a member structure
 * \return the string used to read this field as text, which may be empty
 */
QString ProtocolField::getMapDecodeString(bool isStructureMember) const
{
    QString key;
    QString output;
    QString decode;

    // Cannot decode to a null memory type
    if(inMemoryType.isNull)
        return output;

    // Return empty string if this field is not to be decoded
    if((mapOptions & MAP_DECODE) == 0)
        return output;

    QString access;

    if(isStructureMember)
        access = "_pg_user->" + name;
    else
        access = name;

    if(!comment.isEmpty())
        output += TAB_IN + "// " + comment + "\n";

    if(inMemoryType.isString)
    {
        key = "_pg_prename + \":" + name + "\"";

        output += TAB_IN + "key = " + key + ";\n\n";

        output += TAB_IN + "if (_pg_map.contains(key))\n";
        output += TAB_IN + TAB_IN + "strncpy(" + access + ", _pg_map[key].toString().toLatin1().constData(), " + array + ");\n";
    }
    else
    {
        QString spacing = TAB_IN;
        bool closeforloop = false;
        bool closeforloop2 = false;

        if(isArray())
        {
            closeforloop = true;

            if(!variableArray.isEmpty() && isStructureMember)
                output += spacing + "for(_pg_i = 0; (_pg_i < " + array + ") && (_pg_i < (unsigned)_pg_user->" + variableArray + "); _pg_i++)\n";
            else
                output += spacing + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";

            output += spacing + "{\n";
            spacing += TAB_IN;
            access = access + "[_pg_i]";
        }

        if(is2dArray())
        {
            closeforloop2 = true;

            if(!variable2dArray.isEmpty() && isStructureMember)
            {
                output += spacing + "for(_pg_j = 0; (_pg_j < " + array2d + ") && (_pg_j < (unsigned)_pg_user->" + variable2dArray + "); _pg_j++)\n";
                output += spacing + "{\n";
                spacing += TAB_IN;
            }
            else
            {
                output += spacing + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";
                output += spacing + "{\n";
                spacing += TAB_IN;
            }

            access = access + "[_pg_j]";
        }

        if(inMemoryType.isStruct)
        {
            output += spacing + "mapDecode" + typeName + "(_pg_prename + \":" + name + "\"";

            if(isArray())
                output += " + \"[\" + QString::number(_pg_i) + \"]\"";
            if(is2dArray())
                output += " + \"[\" + QString::number(_pg_j) + \"]\"";

            output += ", _pg_map, &" + access + ");\n";

        }// data type is a struct
        else
        {
            key = "_pg_prename + \":" + name + "\"";

            if(isArray())
                key += " + \"[\" + QString::number(_pg_i) + \"]\"";
            if(is2dArray())
                key += " + \"[\" + QString::number(_pg_j) + \"]\"";

            output += spacing + "key = " + key + ";\n";

            decode = "_pg_map[key]";

            if(inMemoryType.isFloat || !printScalerString.isEmpty())
                decode += ".toDouble";
            else if (inMemoryType.isSigned)
            {
                if(inMemoryType.bits > 32)
                    decode += ".toLongLong";
                else
                    decode += ".toInt";
            }
            else
            {
                if(inMemoryType.bits > 32)
                    decode += ".toULongLong";
                else
                    decode += ".toUInt";
            }

            /* Generate code to load the data from the map:
             * 1. Check that the given key exists
             * 2. Check that the provided value decodes properly (for the given data type)
             * 3. Load out the value from the map if 1. and 2. pass
             */

            output += spacing + "if(_pg_map.contains(key))\n";
            output += spacing + "{\n";
            output += spacing + TAB_IN + decode + "(&ok);\n";
            output += spacing + TAB_IN + "if(ok)\n";
            output += spacing + TAB_IN + TAB_IN + access + " = (" + typeName + ")" + decode + "()";
            if(inMemoryType.isFloat && !printScalerString.isEmpty())
                output += readScalerString;
            output += ";\n";
            output += spacing + "}\n";

        }// else not a struct

        if(closeforloop2)
        {
            spacing.remove(spacing.lastIndexOf(TAB_IN), TAB_IN.count());
            output += spacing + "}\n";
        }

        if(closeforloop)
        {
            spacing.remove(spacing.lastIndexOf(TAB_IN), TAB_IN.count());
            output += spacing + "}\n";
        }

    }// else numeric output

    return output;

}// ProtocolField::getMapDecodeString


/*!
 * Return the string that sets this encodable to its default value in code
 * \param isStructureMember should be true if this field is accessed through a "user" structure pointer
 * \return the string to add to the source file, including line feed
 */
QString ProtocolField::getSetToDefaultsString(bool isStructureMember) const
{
    return getSetToValueString(isStructureMember, defaultString);
}


/*!
 * Return the string that sets this encodable to its initial value in code
 * \param isStructureMember should be true if this field is accessed through a "user" structure pointer
 * \return the string to add to the source file, including line feed
 */
QString ProtocolField::getSetInitialValueString(bool isStructureMember) const
{
    QString output;

    if(!hasInit())
        return output;

    if(inMemoryType.isNull)
        return output;

    if(inMemoryType.isStruct)
    {
        QString access;

        if(!comment.isEmpty())
            output += TAB_IN + "// " + comment + "\n";

        if(isArray())
        {
            output += TAB_IN + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";

            if(is2dArray())
            {
                output += TAB_IN + TAB_IN + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";

                if(isStructureMember)
                    access = "&_pg_user->" + name + "[_pg_i][_pg_j]";
                else
                    access = "&" + name + "[_pg_i][_pg_j]";

                output += TAB_IN + TAB_IN + TAB_IN + "init" + typeName + "(" + access + ");\n";

            }// if 2D array of structures
            else
            {
                if(isStructureMember)
                    access = "&_pg_user->" + name + "[_pg_i]";
                else
                    access = "&" + name + "[_pg_i]";

                output += TAB_IN + TAB_IN + "init" + typeName + "(" + access + ");\n";

            }// else if only 1-D array

        }// if array of structures
        else
        {
            if(isStructureMember)
                access = "&_pg_user->" + name;
            else
                access = name;  // in this case, name is already pointer, so we don't need "&"

            output += TAB_IN + "init" + typeName + "(" + access + ");\n";

        }// else if simple structure, no array

    }// if struct
    else if(!initialValueString.isEmpty())
    {
        if(!comment.isEmpty())
            output += TAB_IN + "// " + comment + "\n";

        output += getSetToValueString(isStructureMember, initialValueString);
    }

    return output;

}// ProtocolField::getSetInitialValueString


/*!
 * Return the string that sets this encodable to specific value in code, this cannot be used with a struct
 * \param isStructureMember should be true if this field is accessed through a "user" structure pointer
 * \param value is the string of the specific value.
 * \return the string to add to the source file, including line feed
 */
QString ProtocolField::getSetToValueString(bool isStructureMember, QString value) const
{
    QString output;
    QString access;

    if(inMemoryType.isStruct)
        return output;

    if(value.isEmpty())
        return QString();

    // Write out the defaults code
    if(inMemoryType.isString)
    {
        if(isStructureMember)
            access = "_pg_user->";

        if(value.isEmpty() || value.compare("null", Qt::CaseInsensitive) == 0)
            output += TAB_IN + access + name + "[0] = 0;\n";
        else
            output += TAB_IN + "strncpy((char*)" + access + name + ", \"" + value + "\", " + array + ");\n";
    }
    else
    {
        // Deal with casting doubles to floats
        if(inMemoryType.isFloat && (inMemoryType.bits < 64))
        {
            // If the value contains a decimal then we assume it is a floating
            // point constant, if it does not contain ".0f" then we assume it
            // is a double precision constant
            if(value.contains('.') && !value.endsWith('f'))
            {
                // Rather than presume to change the default constant we simply put a cast in front of it
                value = "(float)" + value;
            }
        }

        if(isArray())
        {
            if(isStructureMember)
                access = "_pg_user->";

            if(is2dArray())
            {
                output += TAB_IN + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";
                output += TAB_IN + TAB_IN + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";
                output += TAB_IN + TAB_IN + TAB_IN + access + name + "[_pg_i][_pg_j] = " + value + ";\n";
            }
            else
            {
                output += TAB_IN + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";
                output += TAB_IN + TAB_IN + access + name + "[_pg_i] = " + value + ";\n";
            }
        }
        else
        {
            if(isStructureMember)
                access = "_pg_user->";
            else
                access = "*";

            // Direct pointer access
            output += TAB_IN + access + name + " = " + value + ";\n";
        }

    }// else if not a struct

    return output;

}// ProtocolField::getSetToValueString


//! Return the strings that #define initial and variable values
QString ProtocolField::getInitialAndVerifyDefines(bool includeComment) const
{
    Q_UNUSED(includeComment);

    QString output;

    if(inMemoryType.isNull || inMemoryType.isStruct)
        return output;

    QString start = getHierarchicalName();

    start.replace(":", "_");

    if(!initialValueString.isEmpty())
        output += "#define " + start + "_InitValue " + initialValueString + "\n";

    if(!verifyMinString.isEmpty())
        output += "#define " + start + "_VerifyMin " + verifyMinString + "\n";

    if(!verifyMaxString.isEmpty())
        output += "#define " + start + "_VerifyMax " + verifyMaxString + "\n";

    return output;

}// ProtocolField::getInitialAndVerifyDefines


//! True if this encodable has a direct child that uses bitfields
bool ProtocolField::usesBitfields(void) const
{
    return (encodedType.isBitfield && !isNotEncoded());
}


//! True if this field has a smaller encoded size than in memory size, which requires a size check
bool ProtocolField::requiresSizeCheck(void) const
{
    // No size check needed if nothing is in memory or if not encoded
    if(inMemoryType.isNull || encodedType.isNull)
        return false;

    // If we are encoding a constant, then it's up to the user to make sure it fits
    if(!constantString.isEmpty())
        return false;

    // Different in-memory versus encoded bit size, requires size check
    if(inMemoryType.bits > encodedType.bits)
        return true;

    // If the in memory type is a float, and the encoded type is not then it
    // needs a size check even if the in memory bits are less than the encoded bits
    if(inMemoryType.isFloat && !encodedType.isFloat)
        return true;

    return false;

}

//! True if this bitfield crosses a byte boundary
bool ProtocolField::bitfieldCrossesByteBoundary(void) const
{
    if(!usesBitfields())
        return false;

    // No byte boundary crossing if only one bit
    if(encodedType.bits <= 1)
        return false;

    // Greater than 8 bits crosses byte boundary for sure
    if(encodedType.bits > 8)
        return true;

    // To check the in-between cases, we have to check the starting bit count
    if(((bitfieldData.startingBitCount % 8) + encodedType.bits) > 8)
        return true;

    return false;
}


//! True if this encodable needs a temporary buffer for its bitfield
bool ProtocolField::usesEncodeTempBitfield(void) const
{
    // We need a temporary bitfield for encode under the following circumstances:
    // 0) Bitfield length is less than 32 bits AND
    // 1) The in-memory type uses more bits than the encoded type, requiring a size check OR
    // 2) The encoded bitfield crosses a byte boundary
    if(usesBitfields() && (encodedType.bits <= 32))
    {
        // If we are encoding a constant zero we don't need temporary variables, no matter how big the field is
        if(getConstantString() == "0")
            return false;

        // Different in-memory versus encoded bit size, requires size check
        if(requiresSizeCheck())
            return true;

        return bitfieldCrossesByteBoundary();
    }

    // If we get here we do not need a temporary variable for the bitfield
    return false;
}


//! True if this encodable needs a temporary long buffer for its bitfield
bool ProtocolField::usesEncodeTempLongBitfield(void) const
{
    // We need a temporary long bitfield under the following circumstances:
    // 0) Bitfield length is more than 32 bits AND
    // 1) The in-memory type uses more bits than the encoded type, requiring a size check OR
    // 2) The encoded bitfield crosses a byte boundary
    if(usesBitfields() && (encodedType.bits > 32))
    {
        // If we are encoding a constant zero we don't need temporary variables, no matter how big the field is
        if(getConstantString() == "0")
            return false;

        // Different in-memory versus encoded bit size, requires size check
        if(requiresSizeCheck())
            return true;

        return bitfieldCrossesByteBoundary();
    }

    // If we get here we do not need a temporary variable for the bitfield
    return false;
}


//! True if this encodable needs a temporary buffer for its bitfield
bool ProtocolField::usesDecodeTempBitfield(void) const
{
    // We need a temporary bitfield for decode under the following circumstances:
    // 0) Bitfield length is less than 32 bits AND
    // 1) The in-memory type is null, and we need to do a constant check
    // 2) The encoded bitfield crosses a byte boundary
    if(usesBitfields() && (encodedType.bits <= 32))
    {
        // If we have no in-memory type there is no decoding to do
        if(inMemoryType.isNull)
        {
            // Unless we are checking a constant
            if(checkConstant)
                return true;
            else
                return false;
        }

        // If we are scaling we need a temporary
        if(isFloatScaling() || isIntegerScaling())
            return true;

        return bitfieldCrossesByteBoundary();
    }

    // If we get here we do not need a temporary variable for the bitfield
    return false;
}


//! True if this encodable needs a temporary long buffer for its bitfield
bool ProtocolField::usesDecodeTempLongBitfield(void) const
{
    // We need a temporary long bitfield under the following circumstances:
    // 0) Bitfield length is more than 32 bits AND
    // 1) The in-memory type is null, and we need to do a constant check
    // 2) The encoded bitfield crosses a byte boundary
    if(usesBitfields() && (encodedType.bits > 32))
    {
        // If we have no in-memory type there is no decoding to do
        if(inMemoryType.isNull)
        {
            // Unless we are checking a constant
            if(checkConstant)
                return true;
            else
                return false;
        }

        // If we are scaling we need a temporary
        if(isFloatScaling() || isIntegerScaling())
            return true;

        return bitfieldCrossesByteBoundary();
    }

    // If we get here we do not need a temporary variable for the bitfield
    return false;
}


/*!
 * Get the next lines(s) of source coded needed to encode this bitfield field
 * \param bitcount points to the running count of bits in this string of
 *        bitfields, and will be updated by this fields encoded bit count.
 * \param isStructureMember should be true if the left hand side is a
 *        member of a user structure, else the left hand side is a pointer
 *        to the inMemoryType
 * \return The string to add to the source file that encodes this field.
 */
QString ProtocolField::getEncodeStringForBitfield(int* bitcount, bool isStructureMember) const
{
    QString argument;
    QString output;
    QString constantstring = getConstantString();

    if(encodedType.isNull)
        return output;

    if(!comment.isEmpty())
        output += TAB_IN + "// " + comment + "\n";

    if(constantstring.isEmpty())
    {
        if(isStructureMember)
            argument = "_pg_user->" + name;
        else
            argument = name;
    }
    else
        argument = constantstring;

    // check to see if this is a scaled bitfield
    if(isFloatScaling())
    {
        // Additional commenting to describe the scaling
        output += TAB_IN + "// Range of " + name + " is " + getNumberString(encodedMin) + " to " + getNumberString(encodedMax) +  ".\n";

        if(encodedType.bits > 32)
        {
            if(support.longbitfield)
            {
                if(support.float64)
                    argument = "float64ScaledToLongBitfield((double)" + argument;
                else
                    argument = "float32ScaledToLongBitfield((float)" + argument;
            }
            else
            {
                if(support.float64)
                   argument = "float64ScaledToBitfield((double)" + argument;
                else
                   argument = "float32ScaledToBitfield((float)" + argument;

            }
        }
        else
            argument = "float32ScaledToBitfield((float)" + argument;

        argument += ", " + getNumberString(encodedMin, encodedType.bits);
        argument += ", " + getNumberString(scaler, encodedType.bits);
        argument += ")";

    }
    else if(isIntegerScaling())
    {
        // Additional commenting to describe the scaling
        output += TAB_IN + "// Range of " + name + " is " + getNumberString(encodedMin) + " to " + getNumberString(encodedMax) +  ".\n";

        if((encodedMin != 0) && !encodedType.isSigned)
            argument = "(" + argument + " - " + QString::number(encodedMin, 'f', 0) + ")";

        if(scaler != 1.0)
            argument += "*" + QString::number(scaler, 'f', 0);

        // Now put the cast in
        if((encodedType.bits > 32) && (support.longbitfield))
            argument = "(uint64_t)" + argument;
        else
            argument = "(unsigned int)" + argument;
    }

    // The size check inserts a line, replacing the original argument with a temporary field
    if(requiresSizeCheck())
    {
        QString maxvalue = QString().setNum(ProtocolBitfield::maxvalueoffield(encodedType.bits));
        QString tempname;

        if((encodedType.bits > 32) && (support.longbitfield))
        {
            maxvalue += "ULL";
            tempname = "_pg_templongbitfield";
        }
        else
        {
            tempname = "_pg_tempbitfield";
        }

        // This block makes sure the size does not overflow the bitfield
        output += TAB_IN + tempname + " = " + argument + ";\n";
        output += TAB_IN + "if(" + tempname + " > " + maxvalue + ")\n";
        output += TAB_IN + TAB_IN + tempname + " = " + maxvalue + ";\n\n";
        argument = tempname;
    }
    else if(usesEncodeTempBitfield())
    {
        output += TAB_IN + "_pg_tempbitfield = (unsigned int)" + argument + ";\n";
        argument = "_pg_tempbitfield";
    }
    else if(usesEncodeTempLongBitfield())
    {
        output += TAB_IN + "_pg_templongbitfield = (uint64_t)" + argument + ";\n";
        argument = "_pg_templongbitfield";
    }

    if(bitfieldData.groupMember)
        output += ProtocolBitfield::getEncodeString(TAB_IN, argument, "_pg_bitfieldbytes", "_pg_bitfieldindex", bitfieldData.startingBitCount, encodedType.bits);
    else
        output += ProtocolBitfield::getEncodeString(TAB_IN, argument, "_pg_data", "_pg_byteindex", bitfieldData.startingBitCount, encodedType.bits);

    // Keep track of the total bits
    *bitcount += encodedType.bits;

    if(bitfieldData.lastBitfield)
    {
        if((bitfieldData.groupMember) && (bitfieldData.groupBits > 0))
        {
            // Number of bytes needed for all the bits
            int num = ((bitfieldData.groupBits+7)/8);

            output += "\n";

            output += TAB_IN + "// Encode the entire group of bits in one shot\n";

            if(support.bigendian)
                output += TAB_IN + "bytesToBeBytes(_pg_bitfieldbytes, _pg_data, &_pg_byteindex, " + QString::number(num) + ");\n";
            else
                output += TAB_IN + "bytesToLeBytes(_pg_bitfieldbytes, _pg_data, &_pg_byteindex, " + QString::number(num) + ");\n";

            output += TAB_IN + "_pg_bitfieldindex = 0;\n\n";

        }// if terminating a group
        else if((*bitcount) != 0)
        {
            // Increment our byte counter, 1 to 8 bits should result in 1 byte, 9 to 16 bits in 2 bytes, etc.
            int bytes = ((*bitcount)+7)/8;

            output += TAB_IN + "_pg_byteindex += " + QString().setNum(bytes) + "; // close bit field\n\n";

        }// else if terminating a non-group

        // Reset bit counter
        *bitcount = 0;

    }// if this is a terminating bit field

    return output;

}// ProtocolField::getEncodeBitfieldString


/*!
 * Get the next lines(s) of source coded needed to decode this bitfield field
 * \param bitcount points to the running count of bits in this string of
 *        bitfields, and will be updated by this fields encoded bit count.
 * \param isStructureMember should be true if the left hand side is a
 *        member of a user structure, else the left hand side is a pointer
 *        to the inMemoryType
 * \param defaultEnabled should be true to enable defaults for this decode
 * \return The string to add to the source file that decodes this field.
 */
QString ProtocolField::getDecodeStringForBitfield(int* bitcount, bool isStructureMember, bool defaultEnabled) const
{
    QString output;

    if(encodedType.isNull)
        return output;

    // If this field has a default value, or overrides a previous value
    if(defaultEnabled && (!defaultString.isEmpty() || overridesPrevious))
    {
        int bytes;

        QString lengthString;

        // How many bytes do we need? From 1 to 8 bits we need 1 byte, from
        // 9 to 15 we need 2 bytes, etc. However, some bits may already have
        // gone by
        if(bitfieldData.groupStart)
            bytes = (bitfieldData.groupBits+7)/8;
        else
            bytes = ((*bitcount) + encodedType.bits + 7)/8;

        lengthString = QString::number(bytes);

        // Technically we only need to check the length if bitcount is zero
        // (i.e. a new byte is being decoded) or if this bitfield will cross
        // a byte boundary (again, requiring a new byte)
        if(((*bitcount) == 0) || (bytes > 1))
        {
            output += TAB_IN + "if(_pg_byteindex + " + lengthString + " > _pg_numbytes)\n";
            output += TAB_IN + "    return 1;\n";
            output += "\n";
        }
    }

    if(bitfieldData.groupStart)
    {
        int num = (bitfieldData.groupBits+7)/8;
        output += TAB_IN + "// Decode the entire group of bits in one shot\n";
        if(support.bigendian)
            output += TAB_IN + "bytesFromBeBytes(_pg_bitfieldbytes, _pg_data, &_pg_byteindex, " + QString::number(num) + ");\n";
        else
            output += TAB_IN + "bytesFromLeBytes(_pg_bitfieldbytes, _pg_data, &_pg_byteindex, " + QString::number(num) + ");\n";

        output += "\n";
    }

    if(!comment.isEmpty())
        output += TAB_IN + "// " + comment + "\n";

    // Handle the case where we just want to skip some bits
    if(inMemoryType.isNull && !checkConstant)
    {
        // Nothing to do in this case, it all gets handled when the bitfield terminates
    }
    else
    {        
        QString argument;
        QString cast;

        // How we are going to access the field
        if(usesDecodeTempBitfield())
            argument = "_pg_tempbitfield";
        else if(usesDecodeTempLongBitfield())
            argument = "_pg_templongbitfield";
        else
        {
            // If we are going to assign this bitfield directly to an enumeration,
            // and we do not have an intermediate temporary field, then we need a cast
            if(inMemoryType.isEnum)
                cast = "(" + typeName + ")";

            if(isStructureMember)
                argument = "_pg_user->" + name;     // Access via structure pointer
            else
                argument = "(*" + name + ")";   // Access via direct pointer
        }

        // Additional commenting to describe the scaling
        if(isFloatScaling() || isIntegerScaling())
            output += TAB_IN + "// Range of " + name + " is " + getNumberString(encodedMin) + " to " + getNumberString(encodedMax) +  ".\n";

        if(bitfieldData.groupMember)
            output += ProtocolBitfield::getDecodeString(TAB_IN, argument, cast, "_pg_bitfieldbytes", "_pg_bitfieldindex", *bitcount, encodedType.bits);
        else
            output += ProtocolBitfield::getDecodeString(TAB_IN, argument, cast, "_pg_data", "_pg_byteindex", *bitcount, encodedType.bits);

        // Handle scaled bitfield
        if(isFloatScaling() && !inMemoryType.isNull)
        {
            QString cast;

            if(isStructureMember)
                output += TAB_IN + "_pg_user->" + name + " = ";    // Access via structure pointer
            else
                output += TAB_IN + "(*" + name + ")" + " = ";  // Access via direct pointer

            if(!inMemoryType.isFloat)
                cast = "(" + typeName +")";

            if(encodedType.bits > 32)
            {
                if((inMemoryType.bits != 64) && support.float64)
                    cast = "(" + typeName +")";

                if(support.longbitfield)
                {
                    if(support.float64)
                        output += cast + "float64ScaledFromLongBitfield(" + argument;
                    else
                        output += cast + "float32ScaledFromLongBitfield(" + argument;
                }
                else
                {
                    if(support.float64)
                        output += cast + "float64ScaledFromBitfield(" + argument;
                    else
                        output += cast + "float32ScaledFromBitfield(" + argument;
                }
            }
            else
                output += cast + "float32ScaledFromBitfield(" + argument;

            output += ", " + getNumberString(encodedMin, encodedType.bits);
            output += ", " + getNumberString(1.0, encodedType.bits) + "/" + getNumberString(scaler, encodedType.bits);
            output += ");\n";

            if(isStructureMember)
                argument = "_pg_user->" + name;    // Access via structure pointer
            else
                argument = "(*" + name + ")";  // Access via direct pointer

        }// if scaled bitfield
        else if(!inMemoryType.isNull)
        {
            // Do the assignment from the temporary field
            if(usesDecodeTempBitfield() || usesDecodeTempLongBitfield())
            {
                if(isIntegerScaling())
                {
                    if(scaler != 1.0)
                        argument = "(" + argument + "/" + QString::number(scaler, 'f', 0) + ")";

                    if((encodedMin != 0) && !encodedType.isSigned)
                        argument = "(" + argument + " + " + QString::number(encodedMin, 'f', 0) + ")";
                }

                // If we are going to assign directly to an enumeration we need a cast
                if(inMemoryType.isEnum)
                    cast = "(" + typeName + ")";
                else
                    cast.clear();

                if(isStructureMember)
                {
                    // Access via structure pointer
                    output += TAB_IN + "_pg_user->" + name +   " = " + cast + argument + ";\n";
                    argument = "_pg_user->" + name;
                }
                else
                {
                    // Access via direct pointer
                    output += TAB_IN + "(*" + name + ")" + " = " + cast + argument + ";\n";
                    argument = "(*" + name + ")";
                }
            }

        }// else if not scaled, but goes in memory

        if(checkConstant)
        {
            QString constantstring = getConstantString();

            // Verify the constant value
            output += TAB_IN + "// Decoded value must be " + constantstring + "\n";
            output += TAB_IN + "if (" + argument + " != " + constantstring + ")\n";
            output += TAB_IN + "    return 0;\n";
        }

    }// else if we do something with the decoded value

    // Keep track of the number of bitfield bits that go by
    *bitcount += encodedType.bits;

    if(bitfieldData.lastBitfield)
    {
        if((bitfieldData.groupMember) && (bitfieldData.groupBits > 0))
        {
            output += TAB_IN + "_pg_bitfieldindex = 0;\n";

        }// if terminating a group
        else if((*bitcount) != 0)
        {
            // Increment our byte counter, 1 to 8 bits should result in 1 byte, 9 to 16 bits in 2 bytes, etc.
            int bytes = ((*bitcount)+7)/8;

            output += TAB_IN + "_pg_byteindex += " + QString().setNum(bytes) + "; // close bit field\n\n";

        }// else if terminating a non-group

        output += "\n";

        // Reset bit counter
        *bitcount = 0;

    }// if this is a terminating bit field

    return output;

}// ProtocolField::getDecodeStringForBitfield


/*!
 * Get the source needed to close out a string of bitfields.
 * \param bitcount points to the running count of bits in this string of
 *        bitfields, and will be updated by this function.
 * \return The string to add to the source file that closes the bitfield.
 */
QString ProtocolField::getCloseBitfieldString(int* bitcount) const
{
    QString output;
    QString spacing;

    if(*bitcount != 0)
    {
        // The number of bytes that the previous sequence of bit fields used up
        int length = *bitcount / 8;

        // Get the spacing right
        spacing += "    ";

        // If bitcount is not modulo 8, then the last byte was still in
        // progress, so increment past that
        if((*bitcount) % 8)
        {
            output += spacing + "bitcount = 0; _pg_byteindex++; // close bit field, go to next byte\n";
            length++;
        }
        else
        {
           output += spacing + "bitcount = 0; // close bit field, byte index already advanced\n";
        }

        output += "\n";

        // Reset bit counter
        *bitcount = 0;
    }

    return output;

}// ProtocolField::getCloseBitfieldString


/*!
 * Get the next lines of source needed to encode this string field.
 * \param isStructureMember should be true if the left hand side is a
 *        member of a user structure, else the left hand side is a pointer
 *        to the inMemoryType
 * \return The string to add to the source file to that encodes this field
 */
QString ProtocolField::getEncodeStringForString(bool isStructureMember) const
{
    QString output;
    QString lhs;

    QString constantstring = getConstantString();

    if(encodedType.isNull)
        return output;

    if(isStructureMember)
        lhs = "_pg_user->";
    else
        lhs = "";

    if(!comment.isEmpty())
        output += "    // " + comment + "\n";

    if(constantstring.isEmpty())
        output += "    stringToBytes(" + lhs + name + ", _pg_data, &_pg_byteindex, " + array;
    else
        output += "    stringToBytes(" + constantstring + ", _pg_data, &_pg_byteindex, " + array;

    if(inMemoryType.isFixedString)
        output += ", 1);\n";
    else
        output += ", 0);\n";

    return output;

}// ProtocolField::getEncodeStringForString


/*!
 * Get the next lines of source needed to decode this string field
 * \param isStructureMember should be true if the left hand side is a
 *        member of a user structure, else the left hand side is a pointer
 *        to the inMemoryType
 * \param defaultEnabled should be true to handle defaults
 * \return The string to add to the source file to that decodes this field
 */
QString ProtocolField::getDecodeStringForString(bool isStructureMember, bool defaultEnabled) const
{
    QString output;
    QString spacing = TAB_IN;

    if(encodedType.isNull)
        return output;

    QString lhs;

    if(isStructureMember)
        lhs = "_pg_user->";
    else
        lhs = "";

    if(!comment.isEmpty())
        output += spacing + "// " + comment + "\n";

    // If this field has a default value, or overrides a previous value
    if(defaultEnabled && (!defaultString.isEmpty() || overridesPrevious))
    {
        if(inMemoryType.isFixedString)
        {
            // If its a fixed string we must have the full monty
            output += spacing + "if(_pg_byteindex + " + array + " > _pg_numbytes)\n";
            output += spacing + TAB_IN + "return 1;\n";
            output += "\n";
            output += spacing + "stringFromBytes(" + lhs + name + ", _pg_data, &_pg_byteindex, " + array + ", 1);\n";
        }
        else
        {
            // Normal strings can be as short as a single character (empty strings with just a null)
            output += spacing + "if(_pg_byteindex + 1 > _pg_numbytes)\n";
            output += spacing + TAB_IN + "return 1;\n";
            output += "\n";

            // When pulling the bytes we have to control the maximum, it could be limited by the in memory space, or by the packet size
            output += spacing + "stringFromBytes(" + lhs + name + ", _pg_data, &_pg_byteindex, " + array + " < (_pg_numbytes - _pg_byteindex) ? " + array + " : (_pg_numbytes - _pg_byteindex), 0);\n";
        }
    }
    else
    {
        // When pulling the bytes we have to control the maximum, it could be limited by the in memory space, or by the packet size
        output += spacing + "stringFromBytes(" + lhs + name + ", _pg_data, &_pg_byteindex, " + array + " < (_pg_numbytes - _pg_byteindex) ? " + array + " : (_pg_numbytes - _pg_byteindex), 0);\n";
    }

    if(checkConstant)
    {
        QString constantstring = getConstantString();
        output += "\n";
        output += spacing + "// Decoded value must be " + constantstring + "\n";
        output += spacing + "if (strncmp(" + lhs + name + ", " + constantstring + ", " + array + ") != 0)\n";
        output += spacing + TAB_IN + "return 0;\n";
    }

    return output;

}// ProtocolField::getDecodeStringForString


/*!
 * Return the string that is used to encode this structure.
 * \param isStructureMember is true if this encodable is accessed by structure pointer
 * \return the string to add to the source to encode this structure
 */
QString ProtocolField::getEncodeStringForStructure(bool isStructureMember) const
{
    QString output;
    QString access;
    QString spacing = TAB_IN;

    if(encodedType.isNull)
        return output;

    if(!comment.isEmpty())
        output += spacing + "// " + comment + "\n";

    if(!dependsOn.isEmpty())
    {
        if(isStructureMember)
            output += spacing + "if(_pg_user->" + dependsOn + ")\n";
        else
            output += spacing + "if(" + dependsOn + ")\n";
        output += spacing + "{\n";
        spacing += "    ";
    }

    if(isArray())
    {
        if(variableArray.isEmpty())
        {
            output += spacing + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";
        }
        else
        {
            if(isStructureMember)
                output += spacing + "for(_pg_i = 0; _pg_i < (unsigned)_pg_user->" + variableArray + " && _pg_i < " + array + "; _pg_i++)\n";
            else
                output += spacing + "for(_pg_i = 0; _pg_i < (unsigned)" + variableArray + " && _pg_i < " + array + "; _pg_i++)\n";
        }


        if(is2dArray())
        {
            if(variable2dArray.isEmpty())
            {
                output += spacing + TAB_IN + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";
            }
            else
            {
                if(isStructureMember)
                    output += spacing + TAB_IN + "for(_pg_j = 0; _pg_j < (unsigned)_pg_user->" + variable2dArray + " && _pg_j < " + array2d + "; _pg_j++)\n";
                else
                    output += spacing + TAB_IN + "for(_pg_j = 0; _pg_j < (unsigned)" + variable2dArray + " && _pg_j < " + array2d + "; _pg_j++)\n";
            }

            if(isStructureMember)
                access = "&_pg_user->" + name + "[_pg_i][_pg_j]";
            else
                access = "&" + name + "[_pg_i][_pg_j]";

            output += spacing + "        encode" + typeName + "(_pg_data, &_pg_byteindex, " + access + ");\n";
        }
        else
        {
            if(isStructureMember)
                access = "&_pg_user->" + name + "[_pg_i]";
            else
                access = "&" + name + "[_pg_i]";

            output += spacing + "    encode" + typeName + "(_pg_data, &_pg_byteindex, " + access + ");\n";
        }
    }
    else
    {
        if(isStructureMember)
            access = "&_pg_user->" + name;
        else
            access = name;  // in this case, name is already pointer, so we don't need "&"

        output += spacing + "encode" + typeName + "(_pg_data, &_pg_byteindex, " + access + ");\n";
    }

    if(!dependsOn.isEmpty())
        output += "    }\n";

    return output;

}// ProtocolField::getEncodeStringForStructure


/*!
 * Get the next lines of source needed to decode this external structure field
 * \param isStructureMember should be true if the left hand side is a
 *        member of a user structure, else the left hand side is a pointer
 *        to the inMemoryType
 * \return The string to add to the source file to that decodes this field
 */
QString ProtocolField::getDecodeStringForStructure(bool isStructureMember) const
{
    QString output;
    QString access;
    QString spacing = "    ";

    if(encodedType.isNull)
        return output;

    if(!comment.isEmpty())
        output += "    // " + comment + "\n";

    if(!dependsOn.isEmpty())
    {
        if(isStructureMember)
            output += spacing + "if(_pg_user->" + dependsOn + ")\n";
        else
            output += spacing + "if(" + dependsOn + ")\n";
        output += spacing + "{\n";
        spacing += "    ";
    }

    if(isArray())
    {
        if(variableArray.isEmpty())
            output += spacing + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";
        else
        {
            if(isStructureMember)
                output += spacing + "for(_pg_i = 0; _pg_i < (unsigned)_pg_user->" + variableArray + " && _pg_i < " + array + "; _pg_i++)\n";
            else
                output += spacing + "for(_pg_i = 0; _pg_i < (unsigned)(*" + variableArray + ") && _pg_i < " + array + "; _pg_i++)\n";
        }

        output += spacing + "{\n";

        if(is2dArray())
        {
            if(variable2dArray.isEmpty())
                output += spacing + TAB_IN + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";
            else
            {
                if(isStructureMember)
                    output += spacing + TAB_IN + "for(_pg_j = 0; _pg_j < (unsigned)_pg_user->" + variable2dArray + " && _pg_j < " + array2d + "; _pg_j++)\n";
                else
                    output += spacing + TAB_IN + "for(_pg_j = 0; _pg_j < (unsigned)(*" + variable2dArray + ") && _pg_j < " + array2d + "; _pg_j++)\n";
            }

            output += spacing + "    {\n";

            if(isStructureMember)
                access = "&_pg_user->" + name + "[_pg_i][_pg_j]";
            else
                access = "&" + name + "[_pg_i][_pg_j]";

            output += spacing + "        if(decode" + typeName + "(_pg_data, &_pg_byteindex, " + access + ") == 0)\n";
            output += spacing + "            return 0;\n";
            output += spacing + "    }\n";
            output += spacing + "}\n";
        }
        else
        {
            if(isStructureMember)
                access = "&_pg_user->" + name + "[_pg_i]";
            else
                access = "&" + name + "[_pg_i]";

            output += spacing + "    if(decode" + typeName + "(_pg_data, &_pg_byteindex, " + access + ") == 0)\n";
            output += spacing + "        return 0;\n";
            output += spacing + "}\n";
        }

    }
    else
    {
        if(isStructureMember)
            access = "&_pg_user->" + name;
        else
            access = name;  // in this case, name is already pointer, so we don't need "&"

        output += spacing + "if(decode" + typeName + "(_pg_data, &_pg_byteindex, " + access + ") == 0)\n";
        output += spacing + "    return 0;\n";
    }

    if(!dependsOn.isEmpty())
        output += "    }\n";

    return output;

}// ProtocolField::getDecodeStringForStructure


/*!
 * Look for a constantValue, in order of preference:
 * 1. constantValue
 * 2. If in memory type is null use "0"
 * \return the constant value
 */
QString ProtocolField::getConstantString(void) const
{
    if (!constantString.isEmpty())
    {
        if(encodedType.isString)
        {
            // constantValue is a string literal, so include the quotes. Except for
            // a special case. If constantValue ends in "()" then we assume its a
            // function or macro call
            if(constantString.contains("(") && constantString.contains(")"))
                return constantString;
            else
                return "\"" + constantString + "\"";
        }
        else
            return constantString;
    }
    else if(inMemoryType.isNull)
    {
        if(encodedType.isString)
        {
            // A zero with quotes around it
            return "\"0\"";
        }
        else
            return "0";
    }
    else
        return QString();

}// ProtocolField::getConstantString


/*!
 * Get the next lines(s) of source coded needed to encode this field, which
 * is not a bitfield or a string
 * \param isBigEndian should be true for big endian encoding.
 * \param isStructureMember should be true if the left hand side is a
 *        member of a user structure, else the left hand side is a pointer
 *        to the inMemoryType
 * \return The string to add to the source file that encodes this field.
 */
QString ProtocolField::getEncodeStringForField(bool isBigEndian, bool isStructureMember) const
{
    QString output;
    QString endian;
    QString lengthString;
    QString lhs;
    QString constantstring = getConstantString();

    if(encodedType.isNull)
        return output;

    if(isStructureMember)
        lhs = "_pg_user->";
    else
        lhs = "";

    if(!comment.isEmpty())
        output += TAB_IN + "// " + comment + "\n";

    int length = encodedType.bits / 8;
    lengthString.setNum(length);

    // Remember that we could be encoding an array
    if(isArray())
        lengthString += "*" + array;

    if(is2dArray())
        lengthString += "*" + array2d;

    // The endian string, which is empty for 1 byte, since
    // endian only applies to multi-byte fields
    if(length > 1)
    {
        if(isBigEndian)
            endian += "Be";
        else
            endian += "Le";
    }
    else
        endian = "";

    QString spacing = TAB_IN;

    if(!dependsOn.isEmpty())
    {
        if(isStructureMember)
            output += spacing + "if(_pg_user->" + dependsOn + ")\n";
        else
            output += spacing + "if(" + dependsOn + ")\n";
        output += spacing + "{\n";
        spacing += TAB_IN;
    }

    if(encodedType.isFloat)
    {
        // In this case we are encoding as a floating point. Typically we
        // would not scale here, but there are cases where scaling is
        // interesting.
        QString scalestring;

        // Notice that encodedMax and encodedMin do not make sense since
        // the encoded type is float
        if(scaler != 1.0)
        {
            scalestring = "*" + getNumberString(scaler, inMemoryType.bits);

            // Additional commenting to describe the scaling
            output += spacing + "// " + name + " is scaled by " + getNumberString(scaler) +  ".\n";

        }

        // Notice that we have to cast to the
        // input parameter type, since the user might (for example) have
        // the in-memory type as a double, but the encoded as a float
        QString cast = "(" + encodedType.toTypeString() + ")";

        if(array.isEmpty())
        {
            if(constantstring.isEmpty())
                output += spacing + "float" + QString().setNum(encodedType.bits) + "To" + endian + "Bytes(" + cast + lhs + name + scalestring + ", _pg_data, &_pg_byteindex";
            else
                output += spacing + "float" + QString().setNum(encodedType.bits) + "To" + endian + "Bytes(" + cast + constantstring + ", _pg_data, &_pg_byteindex";

            if((encodedType.bits == 16) || (encodedType.bits == 24))
                output += ", " + QString().setNum(encodedType.sigbits);

            output += ");\n";
        }
        else
        {
            if(variableArray.isEmpty())
                output += spacing + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";
            else
                output += spacing + "for(_pg_i = 0; _pg_i < (unsigned)" + lhs + variableArray + " && _pg_i < " + array + "; _pg_i++)\n";

            if(is2dArray())
            {
                if(variable2dArray.isEmpty())
                    output += spacing + TAB_IN + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";
                else
                    output += spacing + TAB_IN + "for(_pg_j = 0; _pg_j < (unsigned)" + lhs + variable2dArray + " && _pg_j < " + array2d + "; _pg_j++)\n";

                if(constantstring.isEmpty())
                    output += spacing + TAB_IN + TAB_IN + "float" + QString().setNum(encodedType.bits) + "To" + endian + "Bytes(" + cast + lhs + name +  scalestring + "[_pg_i][_pg_j], _pg_data, &_pg_byteindex";
                else
                    output += spacing + TAB_IN + TAB_IN + "float" + QString().setNum(encodedType.bits) + "To" + endian + "Bytes(" + cast + constantstring + ", _pg_data, &_pg_byteindex";

                if((encodedType.bits == 16) || (encodedType.bits == 24))
                    output += ", " + QString().setNum(encodedType.sigbits);

                output += ");\n";
            }
            else
            {
                if(constantstring.isEmpty())
                    output += spacing + TAB_IN + "float" + QString().setNum(encodedType.bits) + "To" + endian + "Bytes(" + cast + lhs + name +  scalestring + "[_pg_i], _pg_data, &_pg_byteindex";
                else
                    output += spacing + TAB_IN + "float" + QString().setNum(encodedType.bits) + "To" + endian + "Bytes(" + cast + constantstring + ", _pg_data, &_pg_byteindex";

                if((encodedType.bits == 16) || (encodedType.bits == 24))
                    output += ", " + QString().setNum(encodedType.sigbits);

                output += ");\n";
            }
        }
    }
    else if(isFloatScaling())
    {
        // Additional commenting to describe the scaling
        output += spacing + "// Range of " + name + " is " + getNumberString(encodedMin) + " to " + getNumberString(encodedMax) +  ".\n";

        // Handle the array
        if(!array.isEmpty())
        {
            if(variableArray.isEmpty())
                output += spacing + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";
            else
                output += spacing + "for(_pg_i = 0; _pg_i < (unsigned)" + lhs + variableArray + " && _pg_i < " + array + "; _pg_i++)\n";

            if(is2dArray())
            {
                if(variable2dArray.isEmpty())
                    output += spacing + TAB_IN + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";
                else
                    output += spacing + TAB_IN + "for(_pg_j = 0; _pg_j < (unsigned)" + lhs + variable2dArray + " && _pg_j < " + array2d + "; _pg_j++)\n";

                // indent the next line
                output += TAB_IN;

            }

            // indent the next line
            output += TAB_IN;
        }

        output += spacing;

        // If we are scaling, then we are going to use a float-encoding
        // function, since even an integer encoding function would still
        // have to cast to float to apply the scaler.
        if((inMemoryType.bits > 32) && support.float64)
            output += "float64";
        else
            output += "float32";

        output += "ScaledTo";

        // Now the encoded part:

        // Number of bytes
        output += QString().setNum(length);

        // Signed or unsigned
        if(encodedType.isSigned)
            output += "Signed";
        else
            output += "Unsigned";

        // More of the encode call signature, including endian
        output += endian + "Bytes(";

        // Check to see if we need to cast to float
        if(!inMemoryType.isFloat)
        {
            if((inMemoryType.bits > 32) && support.float64)
                output += "(double)";
            else
                output += "(float)";
        }

        // Scaling a constant would be really weird...
        if(constantstring.isEmpty())
        {
            // The reference to the data
            output += lhs + name;

            if(is2dArray())
                output += "[_pg_i][_pg_j]";
            else if(isArray())
                output += "[_pg_i]";
        }
        else
        {
            // It may be weird, but we do support it. Notice the cast in case the constant string is not the correct type
            if(inMemoryType.isFloat && (inMemoryType.bits <= 32) && constantstring.contains('.') && !constantstring.endsWith('f'))
                output += "(" + inMemoryType.toTypeString() + ")(" + constantstring + ")";
            else
                output += constantstring;
        }

        output += ", _pg_data, &_pg_byteindex";

        // Signature changes for signed versus unsigned
        if(!encodedType.isSigned)
            output += ", " + getNumberString(encodedMin, inMemoryType.bits);

        output +=  ", " + getNumberString(scaler, inMemoryType.bits);

        output += ");\n";

    }// if scaled to integer
    else
    {
        QString function;

        if(encodedType.isSigned)
        {
            // "int32ToBeBytes(" for example
            function = "int" + QString().setNum(encodedType.bits) + "To" + endian + "Bytes(";
        }
        else
        {
            // "uint32ToBeBytes(" for example
            function = "uint" + QString().setNum(encodedType.bits) + "To" + endian + "Bytes(";
        }

        // Cast the constant string, just in case
        if(!constantstring.isEmpty())
        {
            // "uint32ToBeBytes((uint32_t)(CONSTANT)" for example
            function += "(" + encodedType.toTypeString() + ")(" + constantstring + ")";
        }
        else
        {
            // The value that is being encoded
            QString value = lhs + name;

            // the value may have array indices
            if(isArray())
            {
                value += "[_pg_i]";
                if(is2dArray())
                    value += "[_pg_j]";
            }

            // Add the scaling operation if there is one
            if(isIntegerScaling())
            {
                // Additional commenting to describe the scaling
                output += spacing + "// Range of " + name + " is " + getNumberString(encodedMin) + " to " + getNumberString(encodedMax) +  ".\n";

                // Subtract the minimum value
                if((encodedMin != 0) && !encodedType.isSigned)
                    value = "(" + value + " - " + QString::number(encodedMin, 'f', 0) + ")";

                // Scale it up
                if(scaler != 1.0)
                    value = "(" + value + "*" + QString::number(scaler, 'f', 0) + ")";
            }

            // Add a cast in case the encoded type is different from the in memory type
            if(inMemoryType.isFloat || (inMemoryType.bits != encodedType.bits) || isIntegerScaling())
            {
                // "int32ToBeBytes((int32_t)((_pg_user->value - min)*scale)" for example
                function += "(" + encodedType.toTypeString() + ")(" + value + ")";
            }
            else
                function += value;

        }// else if not constant

        // This is the termination of the function and the line
        function += ", _pg_data, &_pg_byteindex);\n";

        if(isArray())
        {
            if(variableArray.isEmpty())
                output += spacing + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";
            else
                output += spacing + "for(_pg_i = 0; _pg_i < (unsigned)" + lhs + variableArray + " && _pg_i < " + array + "; _pg_i++)\n";

            spacing += TAB_IN;

            if(is2dArray())
            {
                if(variable2dArray.isEmpty())
                    output += spacing + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";
                else
                    output += spacing + "for(_pg_j = 0; _pg_j < (unsigned)" + lhs + variable2dArray + " && _pg_j < " + array2d + "; _pg_j++)\n";

                spacing += TAB_IN;
            }
        }

        output += spacing + function;

    }// else not floating point scaled

    if(!dependsOn.isEmpty())
        output += TAB_IN + "}\n";

    return output;

}// ProtocolField::getEncodeStringForField


/*!
 * Check to see if we should be doing floating point scaling on this field
 * \return true if scaling should be based on floating point operations
 */
bool ProtocolField::isFloatScaling(void) const
{
    if(inMemoryType.isFloat)
    {
        // If the user input a scaler string, then they want float scaling
        if(!scalerString.isEmpty() || (scaler != 1.0))
           return true;

        // The min value only matters if we are signed
        if(!encodedType.isSigned && (encodedMin != 0.0))
            return true;
    }
    else
    {
        // In this case the inMemoryType is not a float, but we might still
        // want float scaling if the scaling terms are not integer
        if((scaler != 1.0) && (ceil(scaler) != floor(scaler)))
            return true;

        // The min value only matters if we are signed
        if(!encodedType.isSigned && (encodedMin != 0.0) && (ceil(encodedMin) != floor(encodedMin)))
            return true;
    }

    return false;

}// ProtocolField::isFloatScaling


/*!
 * Check to see if we should be doing integer scaling on this field
 * \return true if scaling should be based on integer operations
 */
bool ProtocolField::isIntegerScaling(void) const
{
    if(!inMemoryType.isFloat)
    {
        if((scaler != 1.0) && (ceil(scaler) == floor(scaler)))
            return true;

        // The min value only matters if we are signed
        if(!encodedType.isSigned && (encodedMin != 0.0) && (ceil(encodedMin) == floor(encodedMin)))
            return true;
    }

    return false;

}// ProtocolField::isIntegerScaling


/*!
 * Get the next lines(s) of source coded needed to decode this field, which
 * is not a bitfield or a string
 * \param isBigEndian should be true if the protocol uses big endian ordering.
 * \param isStructureMember should be true if the left hand side is a
 *        member of a user structure, else the left hand side is a pointer
 *        to the inMemoryType
 * \param defaultEnabled should be true to enable default handling
 * \return The string to add to the source file that decodes this field.
 */
QString ProtocolField::getDecodeStringForField(bool isBigEndian, bool isStructureMember, bool defaultEnabled) const
{
    QString output;
    QString endian;
    QString spacing = TAB_IN;
    QString lhs;

    QString constantstring = getConstantString();

    if(encodedType.isNull)
        return output;

    if(isStructureMember)
        lhs = "_pg_user->"; // member of a structure
    else if(array.isEmpty())
        lhs = "*";      // direct pointer
    else
        lhs = "";       // pointer with array de-referencing

    if(!comment.isEmpty())
        output += spacing + "// " + comment + "\n";

    int length = encodedType.bits / 8;

    // The endian string, which is empty for 1 byte, since
    // endian only applies to multi-byte fields
    if(length > 1)
    {
        if(isBigEndian)
            endian += "Be";
        else
            endian += "Le";
    }
    else
        endian = "";

    QString maxlengthString;

    // What is the length in bytes of this field, remember that we could be encoding an array
    maxlengthString.setNum(length);

    QString lengthString = maxlengthString;

    if(isArray())
    {
        maxlengthString += "*" + array;

        if(variableArray.isEmpty())
            lengthString += "*" + array;
        else
        {
            if(isStructureMember)
                lengthString += "*_pg_user->" + variableArray;
            else
                lengthString += "*(*" + variableArray + ")";
        }
    }

    if(is2dArray())
    {
        maxlengthString += "*" + array2d;

        if(variable2dArray.isEmpty())
            lengthString += "*" + array2d;
        else
        {
            if(isStructureMember)
                lengthString += "*_pg_user->" + variable2dArray;
            else
                lengthString += "*(*" + variable2dArray + ")";
        }
    }

    if(!dependsOn.isEmpty())
    {
        if(isStructureMember)
            output += spacing + "if(_pg_user->" + dependsOn + ")\n";
        else
            output += spacing + "if(*" + dependsOn + ")\n";
        output += spacing + "{\n";
        spacing += TAB_IN;
    }

    // If this field has a default value, or overrides a previous value
    if(defaultEnabled && (!defaultString.isEmpty() || overridesPrevious))
    {
        output += spacing + "if(_pg_byteindex + " + lengthString + " > _pg_numbytes)\n";
        output += spacing + TAB_IN + "return 1;\n";
        output += "\n";
    }

    if(inMemoryType.isNull)
    {
        if (checkConstant && array.isEmpty())
        {
            output += spacing + "// Decoded value must be " + constantstring + "\n";
            output += spacing + "if (";

            if (encodedType.isFloat)
            {
                if(encodedType.bits == 16)
                    output += "float16From" + endian + "Bytes(_pg_data, &_pg_byteindex, " + QString::number(encodedType.sigbits) + ")";
                else if(encodedType.bits == 24)
                    output += "float24From" + endian + "Bytes(_pg_data, &_pg_byteindex, " + QString::number(encodedType.sigbits) + ")";
                else if((inMemoryType.bits > 32) && support.float64)
                    output += "float64From" + endian + "Bytes(_pg_data, &_pg_byteindex)";
                else
                    output += "float32From" + endian + "Bytes(_pg_data, &_pg_byteindex)";
            }
            else
            {
                if (encodedType.isSigned)
                    output += "int";
                else
                    output += "uint";

                output += QString().setNum(encodedType.bits) + "From" + endian + "Bytes(_pg_data, &_pg_byteindex)";

            }

            output += " != (" + encodedType.toTypeString() + ") " + constantstring + ")\n";
            output += spacing + TAB_IN + "return 0;\n";
        }
        else
        {
            // Skip over reserved space
            if(comment.isEmpty())
                output += spacing + "// Skip over reserved space\n";

            // Note how it's not possible to skip a variable amount of space
            output += spacing + "_pg_byteindex += " + EncodedLength::collapseLengthString(maxlengthString, true) + ";\n";

            // TODO: Handle depends on variable array, and variable2darray

        }

    }
    else if(encodedType.isFloat)
    {
        // In this case we are encoding as a floating point. Typically we
        // would not scale here, but there are cases where scaling is
        // interesting.
        QString scalestring;

        // Notice that encodedMax and encodedMin do not make sense since
        // the encoded type is float
        if(scaler != 1.0)
        {
            scalestring = "(" + getNumberString(1.0, inMemoryType.bits) + "/" + getNumberString(scaler, inMemoryType.bits) + ")*" ;

            // Additional commenting to describe the scaling
            output += spacing + "// " + name + " is scaled by " + getNumberString(scaler) +  ".\n";
        }

        if(array.isEmpty())
        {
            output += spacing + lhs + name + " = " + scalestring + "float" + QString().setNum(encodedType.bits) + "From" + endian + "Bytes(_pg_data, &_pg_byteindex";

            if((encodedType.bits == 16) || (encodedType.bits == 24))
                output += ", " + QString::number(encodedType.sigbits);

            output += ");\n";
        }
        else
        {
            if(variableArray.isEmpty())
                output += spacing + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";
            else
            {
                if(isStructureMember)
                    output += spacing + "for(_pg_i = 0; _pg_i < (unsigned)_pg_user->" + variableArray + " && _pg_i < " + array + "; _pg_i++)\n";
                else
                    output += spacing + "for(_pg_i = 0; _pg_i < (unsigned)(*" + variableArray + ") && _pg_i < " + array + "; _pg_i++)\n";
            }

            if(is2dArray())
            {
                if(variable2dArray.isEmpty())
                    output += spacing + TAB_IN + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";
                else
                {
                    if(isStructureMember)
                        output += spacing + TAB_IN + "for(_pg_j = 0; _pg_j < (unsigned)_pg_user->" + variable2dArray + " && _pg_j < " + array2d + "; _pg_j++)\n";
                    else
                        output += spacing + TAB_IN + "for(_pg_j = 0; _pg_j < (unsigned)(*" + variable2dArray + ") && _pg_j < " + array2d + "; _pg_j++)\n";
                }

                output += spacing + TAB_IN + TAB_IN + lhs + name + "[_pg_i][_pg_j] = " + scalestring + "float" + QString().setNum(encodedType.bits) + "From" + endian + "Bytes(_pg_data, &_pg_byteindex";

                if((encodedType.bits == 16) || (encodedType.bits == 24))
                    output += ", " + QString::number(encodedType.sigbits);

                output += ");\n";
            }
            else
            {
                output += spacing + TAB_IN + lhs + name + "[_pg_i] = " + scalestring + "float" + QString().setNum(encodedType.bits) + "From" + endian + "Bytes(_pg_data, &_pg_byteindex";

                if((encodedType.bits == 16) || (encodedType.bits == 24))
                    output += ", " + QString::number(encodedType.sigbits);

                output += ");\n";
            }

        }// if array

    }// if float
    else if(isFloatScaling())
    {
        // Additional commenting to describe the scaling
        output += spacing + "// Range of " + name + " is " + getNumberString(encodedMin) + " to " + getNumberString(encodedMax) +  ".\n";

        // Handle the array
        if(isArray())
        {
            if(variableArray.isEmpty())
                output += spacing + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";
            else
                output += spacing + "for(_pg_i = 0; _pg_i < (unsigned)" + lhs + variableArray + " && _pg_i < " + array + "; _pg_i++)\n";

            if(is2dArray())
            {
                if(variable2dArray.isEmpty())
                    output += spacing + TAB_IN + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";
                else
                    output += spacing + TAB_IN + "for(_pg_j = 0; _pg_j < (unsigned)" + lhs + variable2dArray + " && _pg_j < " + array2d + "; _pg_j++)\n";

                // start the next line
                output += spacing + TAB_IN + TAB_IN + lhs + name + "[_pg_i][_pg_j] = ";
            }
            else
            {
                // start the next line
                output += spacing + TAB_IN + lhs + name + "[_pg_i] = ";
            }
        }
        else
            output += spacing + lhs + name + " = ";

        // The cast if the in memory type is not floating
        if(!inMemoryType.isFloat)
        {
            if(inMemoryType.isSigned)
                output += "(int" + QString().setNum(inMemoryType.bits) + "_t)";
            else
                output += "(uint" + QString().setNum(inMemoryType.bits) + "_t)";
        }

        if((inMemoryType.bits > 32) && support.float64)
            output += "float64";
        else
            output += "float32";

        output += "ScaledFrom";

        // Now the encoded part:

        // Number of bytes
        output += QString().setNum(length);

        // Signed or unsigned
        if(encodedType.isSigned)
            output += "Signed";
        else
            output += "Unsigned";

        output += endian + "Bytes(_pg_data, &_pg_byteindex";

        // Signature changes for signed versus unsigned
        if(!encodedType.isSigned)
            output += ", " + getNumberString(encodedMin, inMemoryType.bits);

        output +=  ", " + getNumberString(1.0, inMemoryType.bits) + "/" + getNumberString(scaler, inMemoryType.bits);

        output +=  ");\n";

    }// if scaled to integer
    else
    {
        QString function;

        if(encodedType.isSigned)
            function = "int";
        else
            function = "uint";

        // "int32FromBeBytes(data, &_pg_byteindex)" for example
        function += QString().setNum(encodedType.bits) + "From" + endian + "Bytes(_pg_data, &_pg_byteindex)";

        // The value that is being encoded
        QString value = lhs + name;

        // the value may have array indices
        if(isArray())
        {
            value += "[_pg_i]";
            if(is2dArray())
                value += "[_pg_j]";
        }

        // The value goes on the left hand side and gets set equal to the function output;
        value += " = ";

        // Add the scaling operation if there is one
        if(isIntegerScaling())
        {
            // Additional commenting to describe the scaling
            output += spacing + "// Range of " + name + " is " + getNumberString(encodedMin) + " to " + getNumberString(encodedMax) +  ".\n";

            // Scale it down
            // "(int32FromBeBytes(data, &_pg_byteindex)/scaler)" for example
            if(scaler != 1.0)
                function = "(" + function + "/" + QString::number(scaler, 'f', 0) + ")";

            // Add the minimum value
            // "((int32FromBeBytes(data, &_pg_byteindex)/scaler) + encodedMin)" for example
            if((encodedMin != 0) && !encodedType.isSigned)
                function = "(" + function + " + " + QString::number(encodedMin, 'f', 0) + ")";
        }

        // Add a cast in case the encoded type is different from the in memory type
        if(inMemoryType.isFloat || (inMemoryType.bits != encodedType.bits) || isIntegerScaling() || inMemoryType.isEnum)
        {
            // "int32ToBeBytes((int32_t)((_pg_user->value - min)*scale)" for example
            function = "(" + typeName + ")" + function;
        }

        if(isArray())
        {
            if(variableArray.isEmpty())
                output += spacing + "for(_pg_i = 0; _pg_i < " + array + "; _pg_i++)\n";
            else
            {
                if(isStructureMember)
                    output += spacing + "for(_pg_i = 0; _pg_i < (unsigned)_pg_user->" + variableArray + " && _pg_i < " + array + "; _pg_i++)\n";
                else
                    output += spacing + "for(_pg_i = 0; _pg_i < (unsigned)(*" + variableArray + ") && _pg_i < " + array + "; _pg_i++)\n";
            }

            value = "    " + value;

            if(is2dArray())
            {
                if(variable2dArray.isEmpty())
                    output += spacing + TAB_IN + "for(_pg_j = 0; _pg_j < " + array2d + "; _pg_j++)\n";
                else
                {
                    if(isStructureMember)
                        output += spacing + TAB_IN + "for(_pg_j = 0; _pg_j < (unsigned)_pg_user->" + variable2dArray + " && _pg_j < " + array2d + "; _pg_j++)\n";
                    else
                        output += spacing + TAB_IN + "for(_pg_j = 0; _pg_j < (unsigned)(*" + variable2dArray + ") && _pg_j < " + array2d + "; _pg_j++)\n";
                }

                value = "    " + value;

            }// if 2D array

        }// if 1D array

        output += "    " + value + function + ";\n";

    }// else not floating point scaled

    // Handle the check constant case, the null case was handled above
    if(!inMemoryType.isNull && checkConstant && array.isEmpty())
    {
        output += "\n";
        output += spacing + "// Decoded value must be " + constantstring + "\n";
        output += spacing + "if (" + lhs + name + " != " + constantstring + ")\n";
        output += spacing + spacing + "return 0;\n";
    }

    // Close the depends on block
    if(!dependsOn.isEmpty())
    {
        // Remove the last four spaces
        spacing.remove(spacing.size()-4, 4);
        output += spacing + "}\n";
    }

    return output;

}// ProtocolField::getDecodeStringForField


/*!
 * Get a properly formatted number string for a floating point number
 * \param number is the number to turn into a string
 * \param bits is the number of bits in memory for this string. 32 or less will prompt a 'f' suffix on the string
 * \return the string.
 */
QString ProtocolField::getNumberString(double number, int bits) const
{
    QString string;

    string.setNum(number, 'g', 16);

    // Make sure we have a decimal point
    if(!string.contains("."))
        string += ".0";

    // Float suffix
    if((bits <= 32) || !support.float64)
        string += "f";

    return string;
}


/*!
 * Get a properly formatted number string for a floating point number. If the
 * number is one of these multiples of pi (-2, -1, -.5, .5, 1, 2), then return
 * a string that includes the html pi token.
 * \param number is the number to turn into a string
 * \return the string.
 */
QString ProtocolField::getDisplayNumberString(double number)
{
    if(number == -2*3.14159265358979323846)
    {
        return "-2&pi;";
    }
    else if(number == -3.14159265358979323846)
    {
        return "-&pi;";
    }
    else if(number == -0.5*3.14159265358979323846)
    {
        return "-&pi;/2";
    }
    else if(number == 0.5*3.14159265358979323846)
    {
        return "&pi;/2";
    }
    else if(number == 3.14159265358979323846)
    {
        return "&pi;";
    }
    else if(number == 2*3.14159265358979323846)
    {
        return "2&pi;";
    }
    else
    {
        QString string;

        string.setNum(number, 'g', 16);

        // Make sure we have a decimal point
        if(!string.contains("."))
            string += ".0";

        return string;
    }
}


/*!
 * Compute the power of 2 raised to some bits
 * \param bits is the number of bits to raise to
 * \return the power of 2 raised to bits.
 */
uint64_t ProtocolField::pow2(uint8_t bits) const
{
    uint64_t output = 2;

    if(bits == 0)
        return 1;

    // If bits is 1, then 2 is output, if bits is 3 then 2 multiplies occur, yeilding 8
    for(int i = 1; i < bits; i++)
        output *= 2;

    return output;
}
