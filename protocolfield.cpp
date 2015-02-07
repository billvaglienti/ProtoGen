#include "protocolfield.h"
#include "protocolparser.h"
#include "shuntingyard.h"
#include <QString>
#include <QDomElement>
#include <iostream>
#include <math.h>

TypeData::TypeData() :
    isStruct(false),
    isSigned(false),
    isBitfield(false),
    isFloat(false),
    isEnum(false),
    isString(false),
    isNull(false),
    bits(8),
    warning()
{
}


TypeData::TypeData(const TypeData& that) :
    isStruct(that.isStruct),
    isSigned(that.isSigned),
    isBitfield(that.isBitfield),
    isFloat(that.isFloat),
    isEnum(that.isEnum),
    isString(that.isString),
    isNull(that.isNull),
    bits(that.bits),
    warning(that.warning)
{
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


/*!
 * Extract the type information from the type string, for in memory types
 * \param type is the type string
 */
void TypeData::extractInMemoryType(const QString& typeString)
{
    QString type(typeString);

    // Reset to defaults first
    *this = TypeData();

    if(type.startsWith("n", Qt::CaseInsensitive))
        isNull = true;
    else if(type.startsWith("stru", Qt::CaseInsensitive))
    {
        isStruct = true;
    }
    else if(type.startsWith("b", Qt::CaseInsensitive))
    {
        // Bitfields cannot be floats and cannot be signed
        isBitfield = true;

        // Get the number of bits, between 1 and 32 inclusive
        bits = extractPositiveInt(type);

        // Bitfields must have at least one bit, and less than 33 bits
        if(bits < 1)
        {
            bits = 1;
            warning += "bitfields must have a bit width of at least one\n";
        }
        else if(bits > 32)
        {
            warning += "bitfields must have a bit width less than 33";
            bits = 32;
        }
    }
    else if(type.startsWith("e", Qt::CaseInsensitive))
    {
        // Enum type is only for in-memory
        isEnum = true;

        // bit width is actually not interesting in this case
        bits = 8;
    }
    else
    {
        bits = extractPositiveInt(type);

        if(type.startsWith("u", Qt::CaseInsensitive))
        {
            isSigned = false;
        }
        else
        {
            isSigned = true;

            if(type.startsWith("f", Qt::CaseInsensitive))
            {
                isFloat = true;

                // "float" is not a warning
                if(bits == 0)
                    bits = 32;
            }
            else if(type.startsWith("d", Qt::CaseInsensitive))
            {
                isFloat = true;

                // "double" is not a warning
                if(bits == 0)
                    bits = 64;
            }
            else if(!type.startsWith("s", Qt::CaseInsensitive) && !type.startsWith("i", Qt::CaseInsensitive))
            {
                warning += "in memory type name not understood, signed integer assumed\n";
            }
        }

        if(isFloat)
        {
            if((bits != 32) && (bits != 64))
            {
                warning += "in memory float types must be 32 or 64 bits\n";

                if(bits < 32)
                    bits = 32;
                else
                    bits = 64;
            }
        }
        else
        {
            if((bits != 8) && (bits != 16) && (bits != 32) && (bits != 64))
            {
                warning += "in memory integer types must be 8, 16, 32, or 64 bits\n";

                if(bits > 32)
                    bits = 64;
                else if(bits > 16)
                    bits = 32;
                else if(bits > 8)
                    bits = 16;
                else
                    bits = 8;
            }
        }

    }// if not bitfield

}// TypeData::extractInMemoryType


/*!
 * Extract the type information from the type string, for encoded types
 * \param type is the type string
 */
void TypeData::extractEncodedType(const QString& typeString)
{
    QString type(typeString);

    // Reset to defaults first
    *this = TypeData();

    if(type.startsWith("n", Qt::CaseInsensitive))
        isNull = true;
    else if(type.startsWith("b", Qt::CaseInsensitive))
    {
        // Bitfields cannot be floats and cannot be signed
        isBitfield = true;

        // Get the number of bits, between 1 and 32 inclusive
        bits = extractPositiveInt(type);

        // Bitfields must have at least one bit, and less than 33 bits
        if(bits < 1)
        {
            bits = 1;
            warning += "bitfields must have a bit width of at least one\n";
        }
        else if(bits > 32)
        {
            warning += "bitfields must have a bit width less than 33";
            bits = 32;
        }

    }    
    else if(type.startsWith("fixedstring", Qt::CaseInsensitive))
    {
        isString = true;
        isFixedString = true;

        bits = 8;
    }
    else if(type.startsWith("string", Qt::CaseInsensitive))
    {
        isString = true;
        isFixedString = false;

        bits = 8;
    }
    else
    {
        isBitfield = false;

        // Get the number of bits
        bits = extractPositiveInt(type);

        // For encoded types "enum" is the same as "unsigned"
        if(type.startsWith("u", Qt::CaseInsensitive) || type.startsWith("e", Qt::CaseInsensitive))
        {
            isSigned = false;
        }
        else
        {
            isSigned = true;

            if(type.startsWith("f", Qt::CaseInsensitive))
            {
                isFloat = true;

                // "float" is not a warning
                if(bits == 0)
                    bits = 32;
            }
            else if(type.startsWith("d"), Qt::CaseInsensitive)
            {
                isFloat = true;

                // "double" is not a warning
                if(bits == 0)
                    bits = 64;
            }
            else if(!type.startsWith("s", Qt::CaseInsensitive) && !type.startsWith("i", Qt::CaseInsensitive))
            {
                warning += "encoded type name not understood, signed integer assumed\n";
            }
        }

        // Encoded types have more flexibility in the bit width
        if(isFloat)
        {
            if((bits != 16) && (bits != 24) && (bits != 32) && (bits != 64))
            {
                warning += "encoded float types must be 16, 24, 32, or 64 bits\n";

                if(bits > 32)
                    bits = 64;
                else if(bits > 24)
                    bits = 32;
                else if(bits > 16)
                    bits = 24;
                else
                    bits = 16;
            }
        }
        else
        {
            if( (bits < 8) || (bits > 64) || ((bits%8) != 0) )
            {
                warning += "encoded integer types must be 8, 16, 24, 32, 40, 48, 56, or 64 bits\n";

                // Integer types, any byte boundary
                bits = (bits + 8)/8;
                if(bits < 8)
                    bits = 8;
                else if(bits > 64)
                    bits = 64;
            }
        }

    }// else if not string, null, or bitfield

}// TypeData::extractEncodedType


QString TypeData::toTypeString(QString enumName, QString structName)
{
    QString typeName;

    if(isNull)
        return typeName;
    else if(isString)
        typeName = "char";
    else if(isBitfield)
        typeName = "uint32_t";
    else if(isEnum)
    {
        // The enum name is the type
        if(enumName.isEmpty())
            warning += "enum name is missing\n";
        else
            typeName = enumName;
    }
    else if(isStruct)
    {
        // The structure name is the type
        if(structName.isEmpty())
            warning += "structure name is missing\n";
        else
            typeName = structName;

        // Make user it ends with "_t";
        if(!typeName.contains("_t"))
            typeName += "_t";
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

            // Add the bits
            typeName += QString().setNum(bits);
            typeName += "_t";

        }// else if integer type
    }

    return typeName;

}// toTypeString


/*!
 * Construct a blank protocol field
 * \param protocolName is the name of the protocol
 * \param prtoocolPrefix is the naming prefix
 * \param supported indicates what the protocol can support
 */
ProtocolField::ProtocolField(const QString& protocolName, const QString& protocolPrefix, ProtocolSupport supported):
    Encodable(protocolName, protocolPrefix, supported),
    encodedMin(0),
    encodedMax(0),
    scaler(1)
{

}


/*!
 * Construct a protocol field by parsing the DOM
 * \param protocolName is the name of the protocol
 * \param prtoocolPrefix is the naming prefix
 * \param supported indicates what the protocol can support
 * \param field is the DOM element
 */
ProtocolField::ProtocolField(const QString& protocolName, const QString& protocolPrefix, ProtocolSupport supported, const QDomElement& field):
    Encodable(protocolName, protocolPrefix, supported),
    encodedMin(0),
    encodedMax(0),
    scaler(1)
{
    parse(field);
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
    defaultValue.clear();
    encodedType = inMemoryType = TypeData();

}// ProtocolField::clear


/*!
 * Parse the DOM to determine the details of this ProtocolField
 * \param field is the DOM element
 */
void ProtocolField::parse(const QDomElement& field)
{
    name = field.attribute("name").trimmed();
    QString memoryTypeString = field.attribute("inMemoryType").trimmed();

    if(name.isEmpty())
        std::cout << "data tag without a name: " << field.text().toStdString() << std::endl;

    // maybe its an enum or a external struct?
    if(memoryTypeString.isEmpty())
    {
        // Maybe its an enum?
        if(!field.attribute("enum").isEmpty())
            memoryTypeString = "enum";
        else if(!field.attribute("struct").isEmpty())
            memoryTypeString = "struct";
        else
        {
            std::cout << "failed to find inMemoryType attribute for: " << name.toStdString() << std::endl;
            return;
        }
    }


    if(!name.isEmpty() && !memoryTypeString.isEmpty())
    {
        // Pull all the attribute data
        QString encodedTypeString = field.attribute("encodedType").trimmed();
        QString maxString = field.attribute("max").trimmed();
        QString minString = field.attribute("min").trimmed();
        QString scalerString = field.attribute("scaler").trimmed();
        QString structName = field.attribute("struct").trimmed();
        array = field.attribute("array").trimmed();
        variableArray = field.attribute("variableArray").trimmed();
        dependsOn = field.attribute("dependsOn").trimmed();
        enumName = field.attribute("enum").trimmed();
        defaultValue = field.attribute("default").trimmed();
        comment = ProtocolParser::getComment(field);

        // Start out by assuming no encoding
        encodedMin = encodedMax = 0;
        scaler = 1;

        // Extract the in memory type
        inMemoryType.extractInMemoryType(memoryTypeString.trimmed());

        if(inMemoryType.isBitfield && (support.bitfield == false))
        {
            std::cout << name.toStdString() << ": " << "bitfield support is disabled in this protocol" << std::endl;

            if(inMemoryType.bits <= 8)
                inMemoryType.bits = 8;
            else if(inMemoryType.bits <= 16)
                inMemoryType.bits = 16;
            else if(inMemoryType.bits <= 32)
                inMemoryType.bits = 32;
            else if(inMemoryType.bits <= 64)
                inMemoryType.bits = 64;

            inMemoryType.isBitfield=false;
        }

        if((inMemoryType.isFloat == true) && (inMemoryType.bits > 32) && (support.float64 == false))
        {
            std::cout << name.toStdString() << ": " << "64 bit float support is disabled in this protocol" << std::endl;
            inMemoryType.bits=32;
        }

        if((inMemoryType.isFloat == false) && (inMemoryType.bits > 32) && (support.int64 == false))
        {
            std::cout << name.toStdString() << ": " << "Integer types greater than 64 bit are disabled in this protocol" << std::endl;
            inMemoryType.bits=32;
        }

        if(!inMemoryType.warning.isEmpty())
            std::cout << name.toStdString() << ": " << inMemoryType.warning.toStdString() << std::endl;

        if(array.isEmpty() && !variableArray.isEmpty())
        {
            std::cout << name.toStdString() << ": Must specify array length to specify variable array length" << std::endl;
            variableArray.clear();
        }

        if(!dependsOn.isEmpty() && !variableArray.isEmpty())
        {
            std::cout << name.toStdString() << ": variable length arrays cannot also use dependsOn" << std::endl;
            dependsOn.clear();
        }

        if(!scalerString.isEmpty() && !maxString.isEmpty())
        {
            std::cout << name.toStdString() << ": scaler ignored because max is provided" << std::endl;
            scalerString.clear();
        }

        if(inMemoryType.isNull)
        {
            if(!encodedTypeString.isEmpty())
                encodedType.extractEncodedType(encodedTypeString);

            inMemoryType.bits = encodedType.bits;

            reserved = true;
            variableArray.clear();
            defaultValue.clear();
        }
        else if(inMemoryType.isBitfield)
        {
            // Bitfields follow the rule that the encoded type is exactly the same as the in-memory type
            encodedType = inMemoryType;

            if(!encodedTypeString.isEmpty())
                std::cout << name.toStdString() << ": encoded type ignored because in memory type is bitfield" << std::endl;

            if(!array.isEmpty())
            {
                array.clear();
                std::cout << name.toStdString() << ": array ignored because in memory type is bitfield" << std::endl;
            }

            variableArray.clear();

            if(!dependsOn.isEmpty())
            {
                std::cout << name.toStdString() << ": bitfields cannot use dependsOn" << std::endl;
                dependsOn.clear();
            }

            if(!enumName.isEmpty())
            {
                enumName.clear();
                std::cout << name.toStdString() << ": enumeration name ignored because in memory type is bitfield" << std::endl;
            }

            if(!maxString.isEmpty() || !minString.isEmpty() || !scalerString.isEmpty())
            {
                std::cout << name.toStdString() << ": min, max, and scaler are ignored because in memory type is bitfield" << std::endl;
                maxString.clear();
                minString.clear();
                scalerString.clear();
            }

        }
        else
        {
            // Now identify the encoded layout. Start by assuming it is the same as in memory
            encodedType = inMemoryType;

            if(!encodedTypeString.isEmpty())
            {
                encodedType.extractEncodedType(encodedTypeString);

                if(!encodedType.warning.isEmpty())
                    std::cout << name.toStdString() << ": " << encodedType.warning.toStdString() << std::endl;

                if((encodedType.isFloat == true) && (encodedType.bits > 32) && (support.float64 == false))
                {
                    std::cout << name.toStdString() << ": " << "64 bit float support is disabled in this protocol" << std::endl;
                    encodedType.bits=32;
                }

                if((encodedType.isFloat == true) && (encodedType.bits < 32) && (support.specialFloat == false))
                {
                    std::cout << name.toStdString() << ": " << "Support for special floating types less than 32 bits is disabled in this protocol" << std::endl;
                    encodedType.bits=32;
                }

                if((encodedType.isFloat == false) && (encodedType.bits > 32) && (support.int64 == false))
                {
                    std::cout << name.toStdString() << ": " << "Integer types greater than 64 bit are disabled in this protocol" << std::endl;
                    encodedType.bits=32;
                }

                if(inMemoryType.isStruct)
                {
                    std::cout << name.toStdString() << ": encoded type information ignored for external struct" << std::endl;
                }
                else if(inMemoryType.isEnum)
                {
                    if(enumName.isEmpty())
                    {
                        std::cout << name.toStdString() << ": enumeration is missing an enum attribute to give an enumeration type name; name \"missing\" provided" << std::endl;
                        enumName = "missing";
                    }

                    // For enumerations the encoded type is always unsigned,
                    // and the only thing that the type data specifies is the
                    // size of the field to use
                    if(encodedType.isEnum || encodedType.isSigned || encodedType.isFloat || encodedType.isBitfield)
                        std::cout << name.toStdString() << ": encoded type for enumeration overridden to unsigned integer" << std::endl;

                    encodedType.isBitfield = encodedType.isFloat = encodedType.isSigned = encodedType.isEnum = false;

                    if(inMemoryType.bits != encodedType.bits)
                        std::cout << name.toStdString() << ": in memory bit width for enumeration set to match encoded bit width" << std::endl;

                    inMemoryType.bits = encodedType.bits;

                    if(!maxString.isEmpty() || !minString.isEmpty() || !scalerString.isEmpty())
                    {
                        std::cout << name.toStdString() << ": min, max, and scaler are ignored because in memory type is enum" << std::endl;
                        maxString.clear();
                        minString.clear();
                        scalerString.clear();
                    }

                }
                else if(encodedType.isString || inMemoryType.isString)
                {
                    if(!encodedType.isNull)
                        inMemoryType = encodedType;

                    // Strings have to be arrays, default to 64 characters
                    if(array.isEmpty())
                    {
                        std::cout << name.toStdString() << ": string length not provided, assuming 64" << std::endl;
                        array = "64";
                    }

                    // Strings are always variable length, through null termination
                    if(!variableArray.isEmpty())
                    {
                        std::cout << name.toStdString() << ": strings cannot use variableAray attribute, they are always variable length through null termination (unless fixedstring)" << std::endl;
                        variableArray.clear();
                    }

                    if(!dependsOn.isEmpty())
                    {
                        std::cout << name.toStdString() << ": strings cannot use dependsOn" << std::endl;
                        dependsOn.clear();
                    }

                    if(!maxString.isEmpty() || !minString.isEmpty() || !scalerString.isEmpty())
                    {
                        std::cout << name.toStdString() << ": min, max, and scaler are ignored because type is string" << std::endl;
                        maxString.clear();
                        minString.clear();
                        scalerString.clear();
                    }

                }
                else if(encodedType.isFloat)
                {
                    if(!maxString.isEmpty() || !minString.isEmpty() || !scalerString.isEmpty())
                    {
                        std::cout << name.toStdString() << ": min, max, and scaler are ignored because encoded type is float, which are never scaled" << std::endl;
                        maxString.clear();
                        minString.clear();
                        scalerString.clear();
                    }

                }
                else if(encodedType.isNull)
                {
                    if(!maxString.isEmpty() || !minString.isEmpty() || !scalerString.isEmpty())
                    {
                        std::cout << name.toStdString() << ": min, max, and scaler are ignored because encoded type is null" << std::endl;
                        maxString.clear();
                        minString.clear();
                        scalerString.clear();
                    }

                    if(!variableArray.isEmpty())
                    {
                        std::cout << name.toStdString() << ": variable length arrays do not make sense for types that are not encoded (null)" << std::endl;
                        variableArray.clear();
                    }

                    if(!dependsOn.isEmpty())
                    {
                        std::cout << name.toStdString() << ": dependsOn does not make sense for types that are not encoded (null)" << std::endl;
                        dependsOn.clear();
                    }
                }
                else
                {
                    bool ok;

                    if(!minString.isEmpty())
                    {
                        if(encodedType.isSigned)
                            std::cout << name.toStdString() << ": min value ignored because encoded type is signed" << std::endl;
                        else
                        {
                            encodedMin = ShuntingYard::computeInfix(minString, &ok);

                            if(!ok)
                                std::cout << name.toStdString() + ": min is not a number, 0.0 assumed" << std::endl;
                        }
                    }

                    if(!maxString.isEmpty())
                    {
                        encodedMax = ShuntingYard::computeInfix(maxString, &ok);
                        if(!ok)
                        {
                            std::cout << name.toStdString() + ": max is not a number, 1.0 assumed" << std::endl;
                            encodedMax = 1.0;
                        }

                        if(encodedType.isSigned)
                        {
                            scaler = (powf(2.0, encodedType.bits-1) - 1.0)/encodedMax;

                            // This is not exactly true, there is one more bit that could be used,
                            // but this makes conciser commenting, and is clearer to the user
                            encodedMin = -encodedMax;
                        }
                        else
                        {
                            scaler = (powf(2.0, encodedType.bits) - 1.0)/(encodedMax - encodedMin);
                        }

                    }
                    else if(!scalerString.isEmpty())
                    {
                        scaler = ShuntingYard::computeInfix(scalerString, &ok);

                        if(!ok)
                        {
                            std::cout << name.toStdString() + ": scaler is not a number, 1.0 assumed" << std::endl;
                            scaler = 1.0;
                        }
                        else if(scaler <= 0.0)
                        {
                            std::cout << name.toStdString() + ": scaler must be greater than zero, 1.0 used" << std::endl;
                            scaler = 1.0;
                        }

                        if(encodedType.isSigned)
                        {
                            encodedMax = (powf(2.0, encodedType.bits-1) - 1.0)/scaler;

                            // This is not exactly true, there is one more bit that could be used,
                            // but this makes conciser commenting, and is clearer to the user
                            encodedMin = -encodedMax;
                        }
                        else
                        {
                            encodedMax = encodedMin + (powf(2.0, encodedType.bits) - 1.0)/scaler;
                        }

                    }


                    // Max must be larger than minimum
                    if(encodedMin > encodedMax)
                    {
                        encodedMin = encodedMax = 0.0;
                        scaler = 1.0;
                        std::cout << name.toStdString() << ": max is not more than min, encoding not scaled" << std::endl;
                    }

                }// if not an enumeration

            }// if we have encoded type

        }// if not a bitfield member

        // Just the type data
        typeName = inMemoryType.toTypeString(enumName, prefix + structName);

        // Check for data not encoded
        if(encodedType.isNull)
            null = true;

        // Check for reserved data in packet
        if(inMemoryType.isNull)
            reserved = true;

    }// If valid DOM element

}// ProtocolField::parse


/*!
 * Get the declaration for this field as a member of a structure
 * \return the declaration string
 */
QString ProtocolField::getDeclaration(void) const
{
    QString output;

    if(reserved)
        return output;

    output = "    " + typeName + " " + name;

    if(usesBitfields())
        output += " : " + QString().setNum(inMemoryType.bits);
    else if(isArray())
        output += "[" + array + "]";

    output += ";";

    if(!comment.isEmpty())
        output += " //!< " + comment;

    output += "\n";

    return output;

}// ProtocolField::getDeclaration


/*!
 * Return the inclue directive needed for this encodable. Mostly this is empty,
 * but for external structures we need to bring in the include file
 */
QString ProtocolField::getIncludeDirective(void)
{
    if(inMemoryType.isStruct)
    {
        QString output = ProtocolParser::lookUpIncludeName(typeName);

        if(output.isEmpty())
        {
            if(!null)
            {
                // In this case, we guess at the include name
                output = typeName;
                output.remove("_t");
                output += ".h";
                std::cout << name.toStdString() + ": unknown include for " << typeName.toStdString() << "; guess supplied" << std::endl;
            }
            else
            {
                // If there is no encoding for this field, then we assume its
                // some externally supplied struct definition, and its up to the
                // user to specify the correct include file, since we don't need
                // the encoding functions anyway.
                return QString();
            }
        }

        return output;
    }
    else
        return QString();
}


/*!
 * Return the signature of this field in an encode function signature
 * \return The encode signature of this field
 */
QString ProtocolField::getEncodeSignature(void) const
{
    if(null || reserved)
        return "";
    else if(isArray())
        return ", const " + typeName + " " + name + "[" + array + "]";
    else if(!inMemoryType.isStruct)
        return ", " + typeName + " " + name;
    else
        return ", const " + typeName + "* " + name;
}


//! Return markdown formatted information about this encodables fields
QString ProtocolField::getMarkdown(QString indent) const
{
    QString output = indent;

    if(encodedType.isNull)
        return QString();
    else if(inMemoryType.isNull)
    {
        output += "* reserved space in the packet. ";
        if(isArray())
            output += "[`" + array + "`]*";

        output += QString().setNum(encodedType.bits) + " bits.";
    }
    else if(inMemoryType.isStruct)
    {
        // Backtick quotes to make the name be "code"
        output += "* `" + name + "` :  ";

        if(!comment.isEmpty())
            output += comment + ".";

        if(isArray())
        {
            if(variableArray.isEmpty())
                output += "  Repeat `" + array + "` times.";
            else
                output += "  Repeat `" + variableArray + "` times, up to `" + array + "` times.";
        }

        if(!dependsOn.isEmpty())
            output += "  Only included if `" + dependsOn + "` is non-zero.";

        output += "\n";
        output += ProtocolParser::getStructureSubMarkdown(typeName, indent + "    ");
        return output;
    }
    else
    {
        // Backtick quotes to make the name be "code"
        output += "* `" + name + "` :  ";

        if(!comment.isEmpty())
            output += comment + ".  ";

        if(inMemoryType.isString)
        {
            if(inMemoryType.isFixedString)
                output += "Zero terminated fixed length string of characters with length of " + array + ".";
            else
                output += "Zero terminated variable length string of characters with a maximum length of " + array + ".";
        }
        else if(encodedType.isBitfield)
            output += QString().setNum(encodedType.bits) + " bit bitfield.";
        else
        {
            if(isArray())
            {
                if(variableArray.isEmpty())
                    output += "[`" + array + "`]*";
                else
                    output += "variable length array: [`min(" + variableArray + ", " + array + ")`]*";
            }

            if(encodedType.isFloat)
                output += QString().setNum(encodedType.bits) + " bit floating point.";
            else if(encodedType.isEnum)
                output += QString().setNum(encodedType.bits) + " bit enumeration.";
            else if(encodedType.isSigned)
            {
                output += QString().setNum(encodedType.bits) + " bit signed integer.";

                if(encodedMax != 0.0)
                    output += "  Scaled by " + getNumberString(scaler) + " from " + getNumberString(encodedMin) + " to " + getNumberString(encodedMax) + ".";
            }
            else
            {
                output += QString().setNum(encodedType.bits) + " bit unsigned integer.";

                if(encodedMax != 0.0)
                    output += "  Scaled by " + getNumberString(scaler) + " from " + getNumberString(encodedMin) + " to " + getNumberString(encodedMax) + ".";
            }
        }

        if(!dependsOn.isEmpty())
            output += "  Only included if `" + dependsOn + "` is non-zero.";
    }
    return output + "\n";
}


/*!
 * Get the next lines(s) of source coded needed to encode this field
 * \param isBigEndian should be true for big endian encoding.
 * \param encLength is appended for length information of this field.
 * \param bitcount points to the running count of bits in a bitfields and should persist between calls
 * \param isStructureMember should be true if the left hand side is a
 *        member of a user structure, else the left hand side is a pointer
 *        to the inMemoryType
 * \return The string to add to the source file that encodes this field.
 */
QString ProtocolField::getEncodeString(bool isBigEndian, EncodedLength& encLength, int* bitcount, bool isStructureMember) const
{
    if(inMemoryType.isBitfield)
    {
        return getEncodeStringForBitfield(bitcount, isStructureMember);
    }
    else
    {
        QString output;

        // If previous output was a bit field, then we need to close it out
        if(*bitcount != 0)
            output = getCloseBitfieldString(bitcount, &encLength);

        if(inMemoryType.isString)
            output += getEncodeStringForString(encLength, isStructureMember);
        else if(inMemoryType.isStruct)
            output += getEncodeStringForStructure(encLength, isStructureMember);
        else
            output += getEncodeStringForField(isBigEndian, encLength, isStructureMember);

        return output;
    }
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
    if(inMemoryType.isBitfield)
    {
        return getDecodeStringForBitfield(bitcount, isStructureMember);
    }
    else
    {
        QString output;

        // If previous output was a bit field, then we need to close it out
        if(*bitcount != 0)
            output = getCloseBitfieldString(bitcount);

        if(inMemoryType.isString)
            output += getDecodeStringForString(isStructureMember);
        else if(inMemoryType.isStruct)
            output += getDecodeStringForStructure(isStructureMember);
        else
            output += getDecodeStringForField(isBigEndian, isStructureMember, defaultEnabled);

        return output;
    }
}


/*!
 * Return the string that sets this encodable to its default value in code
 * \param isStructureMember should be true if this field is accessed through a "user" structure pointer
 * \return the string to add to the source file, including line feed
 */
QString ProtocolField::getSetToDefaultsString(bool isStructureMember) const
{
    QString output;
    QString access;

    if(defaultValue.isEmpty())
        return output;

    // Write out the defaults code
    if(isArray())
    {
        if(isStructureMember)
            access = "user->";

        output += "    for(i = 0; i < " + array + "; i++)\n";
        output += "        " + access + name + "[i] = " + defaultValue + ";\n";
    }
    else
    {
        if(isStructureMember)
            access = "user->";
        else
            access = "*";

        // Direct pointer access
        output += "    " + access + name + " = " + defaultValue + ";\n";
    }

    return output;

}// ProtocolField::getSetToDefaultsString


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
    QString output;

    if(encodedType.isNull)
        return output;

    if(!comment.isEmpty())
        output += "    // " + comment + "\n";

    if(isStructureMember)
        output += "    encodeBitfield(user->";
    else
        output += "    encodeBitfield(";

    output += name + ", data, &byteindex, &bitcount, " + QString().setNum(inMemoryType.bits) + ");\n";
    *bitcount += inMemoryType.bits;

    return output;

}// ProtocolField::getEncodeBitfieldString


/*!
 * Get the next lines(s) of source coded needed to decode this bitfield field
 * \param bitcount points to the running count of bits in this string of
 *        bitfields, and will be updated by this fields encoded bit count.
 * \param isStructureMember should be true if the left hand side is a
 *        member of a user structure, else the left hand side is a pointer
 *        to the inMemoryType
 * \return The string to add to the source file that decodes this field.
 */
QString ProtocolField::getDecodeStringForBitfield(int* bitcount, bool isStructureMember) const
{
    QString output;

    if(encodedType.isNull)
        return output;

    if(!comment.isEmpty())
        output += "    // " + comment + "\n";

    if(isStructureMember)
        output += "    user->"; // Access via structure pointer
    else
        output += "    *";      // Access via direct pointer

    output += name + " = decodeBitfield(data, &byteindex, &bitcount, " + QString().setNum(inMemoryType.bits) + ");\n";

    *bitcount += inMemoryType.bits;

    return output;

}// ProtocolField::getDecodeStringForBitfield


/*!
 * Get the next lines of source needed to encode this string field
 * \param encLength is appended for length information of this field.
 * \param isStructureMember should be true if the left hand side is a
 *        member of a user structure, else the left hand side is a pointer
 *        to the inMemoryType
 * \return The string to add to the source file to that encodes this field
 */
QString ProtocolField::getEncodeStringForString(EncodedLength& encLength, bool isStructureMember) const
{
    QString output;
    QString lhs;

    if(encodedType.isNull)
        return output;

    if(isStructureMember)
        lhs = "user->";
    else
        lhs = "";

    if(!comment.isEmpty())
        output += "    // " + comment + "\n";

    output += "    // Reserve the last byte for null termination\n";
    output += "    for(i = 0; i < " + array + " - 1; i++)\n";
    output += "    {\n";
    if(inMemoryType.isFixedString)
        output += "        data[byteindex++] = " + lhs + name + "[i];\n";
    else
    {
        output += "        if(" + lhs + name + "[i] == 0)\n";
        output += "            break;\n";
        output += "        else\n";
        output += "            data[byteindex++] = " + lhs + name + "[i];\n";
    }
    output += "    }\n";
    output += "    data[byteindex++] = 0; // Add null termination\n";

    // Add to the length as a string. Note that a fixed string is not a string at all as far as the length is concerned
    encLength.addToLength(array, !inMemoryType.isFixedString, false, !dependsOn.isEmpty(), !defaultValue.isEmpty());

    return output;

}// ProtocolField::getEncodeStringForString


/*!
 * Get the next lines of source needed to decode this string field
 * \param isStructureMember should be true if the left hand side is a
 *        member of a user structure, else the left hand side is a pointer
 *        to the inMemoryType
 * \return The string to add to the source file to that decodes this field
 */
QString ProtocolField::getDecodeStringForString(bool isStructureMember) const
{
    QString output;

    if(encodedType.isNull)
        return output;

    QString lhs;

    if(isStructureMember)
        lhs = "user->";
    else
        lhs = "";

    if(!comment.isEmpty())
        output += "    // " + comment + "\n";

    output += "    for(i = 0; i < " + array + "; i++)\n";
    output += "    {\n";
    output += "        " + lhs + name + "[i] = data[byteindex++];\n";
    if(!inMemoryType.isFixedString)
    {
        output += "        if(" + lhs + name + "[i] == 0)\n";
        output += "            break;\n";
    }
    output += "    }\n";
    output += "\n";
    output += "    // Make sure the result is null terminated\n";
    output += "    " + lhs + name + "[" + array + " - 1] = 0;\n";

    return output;

}// ProtocolField::getDecodeStringForString


/*!
 * Return the string that is used to encode this structure
 * \param encLength is appended for length information of this field.
 * \param isStructureMember is true if this encodable is accessed by structure pointer
 * \return the string to add to the source to encode this structure
 */
QString ProtocolField::getEncodeStringForStructure(EncodedLength& encLength, bool isStructureMember) const
{
    QString output;
    QString access;
    QString spacing = "    ";

    if(encodedType.isNull)
        return output;

    if(!comment.isEmpty())
        output += spacing + "// " + comment + "\n";

    if(!dependsOn.isEmpty())
    {
        if(isStructureMember)
            output += spacing + "if(user->" + dependsOn + ")\n";
        else
            output += spacing + "if(" + dependsOn + ")\n";
        output += spacing + "{\n";
        spacing += "    ";
    }

    if(isArray())
    {
        encLength.addToLength("getMinLengthOf" + typeName + "()*" + array, false, !variableArray.isEmpty(), !dependsOn.isEmpty(), !defaultValue.isEmpty());

        if(variableArray.isEmpty())
        {
            output += spacing + "for(i = 0; i < " + array + "; i++)\n";
        }
        else
        {
            if(isStructureMember)
                output += spacing + "for(i = 0; i < (int)user->" + variableArray + " && i < " + array + "; i++)\n";
            else
                output += spacing + "for(i = 0; i < (int)" + variableArray + " && i < " + array + "; i++)\n";
        }

        if(isStructureMember)
            access = "&user->" + name + "[i]";
        else
            access = "&" + name + "[i]";

        output += spacing + "    byteindex = encode" + typeName + "(data, byteindex, " + access + ");\n";
    }
    else
    {
        encLength.addToLength("getMinLengthOf" + typeName + "()", false, false, !dependsOn.isEmpty(), !defaultValue.isEmpty());

        if(isStructureMember)
            access = "&user->" + name;
        else
            access = name;  // in this case, name is already pointer, so we don't need "&"

        output += spacing + "byteindex = encode" + typeName + "(data, byteindex, " + access + ");\n";
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
            output += spacing + "if(user->" + dependsOn + ")\n";
        else
            output += spacing + "if(" + dependsOn + ")\n";
        output += spacing + "{\n";
        spacing += "    ";
    }

    if(isArray())
    {
        if(variableArray.isEmpty())
            output += spacing + "for(i = 0; i < " + array + "; i++)\n";
        else
        {
            if(isStructureMember)
                output += spacing + "for(i = 0; i < (int)user->" + variableArray + " && i < " + array + "; i++)\n";
            else
                output += spacing + "for(i = 0; i < (int)(*" + variableArray + ") && i < " + array + "; i++)\n";
        }

        if(isStructureMember)
            access = "&user->" + name + "[i]";
        else
            access = "&" + name + "[i]";

        output += spacing + "    byteindex = decode" + typeName + "(data, byteindex, " + access + ");\n";
    }
    else
    {
        if(isStructureMember)
            access = "&user->" + name;
        else
            access = name;  // in this case, name is already pointer, so we don't need "&"

        output += spacing + "byteindex = decode" + typeName + "(data, byteindex, " + access + ");\n";
    }

    if(!dependsOn.isEmpty())
        output += "    }\n";

    return output;

}// ProtocolField::getDecodeStringForStructure


/*!
 * Get the next lines(s) of source coded needed to encode this field, which
 * is not a bitfield or a string
 * \param isBigEndian should be true for big endian encoding.
 * \param encLength is appended for length information of this field.
 * \param isStructureMember should be true if the left hand side is a
 *        member of a user structure, else the left hand side is a pointer
 *        to the inMemoryType
 * \return The string to add to the source file that encodes this field.
 */
QString ProtocolField::getEncodeStringForField(bool isBigEndian, EncodedLength& encLength, bool isStructureMember) const
{
    QString output;
    QString endian;
    QString lengthString;
    QString lhs;

    if(encodedType.isNull)
        return output;

    if(isStructureMember)
        lhs = "user->";
    else
        lhs = "";

    if(!comment.isEmpty())
        output += "    // " + comment + "\n";

    int length = encodedType.bits / 8;
    lengthString.setNum(length);

    // Remember that we could be encoding an array
    if(!array.isEmpty())
        lengthString += "*" + array;

    encLength.addToLength(lengthString, false, !variableArray.isEmpty(), !dependsOn.isEmpty(), !defaultValue.isEmpty());

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

    if(inMemoryType.isNull)
    {
        // Put reserved space in the packet, initialized to zero
        if(array.isEmpty())
        {
            for(int i = 0; i < length; i++)
                output += "    data[byteindex++] = 0;\n";
        }
        else
        {
            output += "    for(i = 0; i < " + array + "; i++)\n";
            output += "    {\n";
            for(int i = 0; i < length; i++)
                output += "        data[byteindex++] = 0;\n";
            output += "    }\n";
        }

    }
    else
    {
        QString spacing = "    ";

        if(!dependsOn.isEmpty())
        {
            if(isStructureMember)
                output += spacing + "if(user->" + dependsOn + ")\n";
            else
                output += spacing + "if(" + dependsOn + ")\n";
            output += spacing + "{\n";
            spacing += "    ";
        }

        if(encodedMax > encodedMin)
        {
            // The scaled outputs. Note that scaled outputs never encode
            // in floating point, since floats carry their scaling with them

            // Additional commenting to describe the scaling
            output += spacing + "// Range of " + name + " is " + getNumberString(encodedMin) + " to " + getNumberString(encodedMax) +  ".\n";

            // Handle the array
            if(!array.isEmpty())
            {
                if(variableArray.isEmpty())
                    output += spacing + "for(i = 0; i < " + array + "; i++)\n";
                else
                    output += spacing + "for(i = 0; i < (int)" + lhs + variableArray + " && i < " + array + "; i++)\n";

                // indent the next line
                output += "    ";
            }

            output += spacing;

            // Handle the in-memory part
            if(inMemoryType.isFloat)
            {
                if(inMemoryType.bits > 32)
                    output += "float64";
                else
                    output += "float32";
            }
            else if(inMemoryType.isSigned)
                output += "int" + QString().setNum(inMemoryType.bits);
            else
                output += "uint" + QString().setNum(inMemoryType.bits);

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
            output += endian + "Bytes(" + lhs + name;

            if(!array.isEmpty())
                output += "[i]";

            output += ", data, &byteindex";

            // Signature changes for signed versus unsigned
            if(!encodedType.isSigned)
                output += ", " + getNumberString(encodedMin, inMemoryType.bits);

            output +=  ", " + getNumberString(scaler, inMemoryType.bits);

            output += ");\n";

        }// if scaled
        else if(encodedType.isFloat)
        {
            // In this case we are encoding as a floating point. No scaling
            // here, because floats carry their scaling with them, but we can
            // choose different bit widths. Notice that we have to cast to the
            // input parameter type, since the user might (for example) have
            // the in-memory type as a double, but the encoded as a float
            QString cast;

            if(encodedType.bits > 32)
                cast = "(double)";
            else
                cast = "(float)";

            if(array.isEmpty())
            {
                output += spacing + "float" + QString().setNum(encodedType.bits) + "To" + endian + "Bytes(" + cast + lhs + name + ", data, &byteindex);\n";
            }
            else
            {
                if(variableArray.isEmpty())
                    output += spacing + "for(i = 0; i < " + array + "; i++)\n";
                else
                    output += spacing + "for(i = 0; i < (int)" + lhs + variableArray + " && i < " + array + "; i++)\n";

                output += spacing + "    float" + QString().setNum(encodedType.bits) + "To" + endian + "Bytes(" + cast + lhs + name + "[i], data, &byteindex);\n";
            }
        }
        else
        {
            // Here we are not scaling, and we are not encoding a float. It may
            // be that the encoded type is the same as the in-memory, but in
            // case it is not we add a cast.
            QString cast;
            QString opener;

            output += spacing;

            if(encodedType.isSigned)
            {
                opener = "int";
                cast = "(int";
            }
            else
            {
                opener = "uint";
                cast = "(uint";
            }

            if(encodedType.bits > 32)
                cast += "64_t)";
            else if(encodedType.bits > 16)
                cast += "32_t)";
            else if(encodedType.bits > 8)
                cast += "16_t)";
            else
                cast += "8_t)";

            if(array.isEmpty())
                output += opener + QString().setNum(encodedType.bits) + "To" + endian + "Bytes(" + cast + lhs + name + ", data, &byteindex);\n";
            else
            {
                if(variableArray.isEmpty())
                    output += "for(i = 0; i < " + array + "; i++)\n";
                else
                    output += "for(i = 0; i < (int)" + lhs + variableArray + " && i < " + array + "; i++)\n";

                output += spacing + "    " + opener + QString().setNum(encodedType.bits) + "To" + endian + "Bytes(" + cast + lhs + name + "[i], data, &byteindex);\n";
            }

        }// else not scaled and not float

        if(!dependsOn.isEmpty())
            output += "    }\n";

    }// else not null

    return output;

}// ProtocolField::getEncodeStringForField



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
    QString spacing = "    ";
    QString lhs;

    if(encodedType.isNull)
        return output;

    if(isStructureMember)
        lhs = "user->"; // member of a structure
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

    QString lengthString;

    // What is the length in bytes of this field, remember that we could be encoding an array
    lengthString.setNum(length);
    if(!array.isEmpty())
        lengthString += "*" + array;

    if(!dependsOn.isEmpty())
    {
        if(isStructureMember)
            output += spacing + "if(user->" + dependsOn + ")\n";
        else
            output += spacing + "if(" + dependsOn + ")\n";
        output += spacing + "{\n";
        spacing += "    ";
    }

    // If this field has a default value
    if(defaultEnabled && !defaultValue.isEmpty())
    {
        output += spacing + "if(byteindex + " + lengthString + " >= numBytes)\n";
        output += spacing + "    return 1;\n";
        output += spacing + "else\n";
        output += spacing + "{\n";
        spacing += "    ";
    }

    if(inMemoryType.isNull)
    {
        // Skip over reserved space
        output += spacing + "byteindex += " + lengthString + ";\n";

    }
    else if(encodedMax > encodedMin)
    {
        // Additional commenting to describe the scaling
        output += spacing + "// Range of " + name + " is " + getNumberString(encodedMin) + " to " + getNumberString(encodedMax) +  ".\n";

        // Handle the array
        if(!array.isEmpty())
        {
            if(variableArray.isEmpty())
                output += spacing + "for(i = 0; i < " + array + "; i++)\n";
            else
                output += spacing + "for(i = 0; i < (int)" + lhs + variableArray + " && i < " + array + "; i++)\n";

            // start the next line
            output += spacing + "    " + lhs + name + "[i] = ";
        }
        else
            output += spacing + lhs + name + " = ";

        // Handle the in-memory part
        if(inMemoryType.isFloat)
        {
            if(inMemoryType.bits > 32)
                output += "float64";
            else
                output += "float32";
        }
        else if(inMemoryType.isSigned)
            output += "int" + QString().setNum(inMemoryType.bits);
        else
            output += "uint" + QString().setNum(inMemoryType.bits);

        output += "ScaledFrom";

        // Now the encoded part:

        // Number of bytes
        output += QString().setNum(length);

        // Signed or unsigned
        if(encodedType.isSigned)
            output += "Signed";
        else
            output += "Unsigned";

        output += endian + "Bytes(data, &byteindex";

        // Signature changes for signed versus unsigned
        if(!encodedType.isSigned)
            output += ", " + getNumberString(encodedMin, inMemoryType.bits);

        output +=  ", " + getNumberString(1.0, inMemoryType.bits) + "/" + getNumberString(scaler, inMemoryType.bits);

        output +=  ");\n";

    }// if scaled
    else if(encodedType.isFloat)
    {
        // In this case we are encoding as a floating point. No scaling
        // here, because floats carry their scaling with them, but we can
        // choose different bit widths. Notice that we have to cast to the
        // input parameter type, since the user might (for example) have
        // the in-memory type as a double, but the encoded as a float

        if(array.isEmpty())
            output += spacing + lhs + name + " = float" + QString().setNum(encodedType.bits) + "From" + endian + "Bytes(data, &byteindex);\n";
        else
        {
            if(variableArray.isEmpty())
                output += spacing + "for(i = 0; i < " + array + "; i++)\n";
            else
            {
                if(isStructureMember)
                    output += spacing + "for(i = 0; i < (int)user->" + variableArray + " && i < " + array + "; i++)\n";
                else
                    output += spacing + "for(i = 0; i < (int)(*" + variableArray + ") && i < " + array + "; i++)\n";
            }

            output += spacing + "    " + lhs + name + "[i] = float" + QString().setNum(encodedType.bits) + "From" + endian + "Bytes(data, &byteindex);\n";
        }
    }
    else
    {
        // Here we are not scaling, and we are not encoding a float. It may
        // be that the encoded type is the same as the in-memory, but in
        // case it is not we add a cast.
        QString cast;
        QString opener;

        if(encodedType.isSigned)
            opener = "int";
        else
            opener = "uint";

        if(inMemoryType.isEnum)
        {
            cast = "(" + enumName + ")";
        }
        else
        {
            if(inMemoryType.isSigned)
                cast = "(int";
            else
                cast = "(uint";

            if(inMemoryType.bits > 32)
                cast += "64_t)";
            else if(inMemoryType.bits > 16)
                cast += "32_t)";
            else if(inMemoryType.bits > 8)
                cast += "16_t)";
            else
                cast += "8_t)";
        }

        if(array.isEmpty())
            output += spacing + lhs + name + " = " + cast + opener + QString().setNum(encodedType.bits) + "From" + endian + "Bytes(data, &byteindex);\n";
        else
        {
            if(variableArray.isEmpty())
                output += spacing + "for(i = 0; i < " + array + "; i++)\n";
            else
            {
                if(isStructureMember)
                    output += spacing + "for(i = 0; i < (int)user->" + variableArray + " && i < " + array + "; i++)\n";
                else
                    output += spacing + "for(i = 0; i < (int)(*" + variableArray + ") && i < " + array + "; i++)\n";
            }

            output += spacing + "    " + lhs + name + "[i] = " + cast + opener + QString().setNum(encodedType.bits) + "From" + endian + "Bytes(data, &byteindex);\n";
        }

    }// else not scaled and not float

    // Close the default block
    if(defaultEnabled && !defaultValue.isEmpty())
    {
        // Remove the last four spaces
        spacing.remove(spacing.size()-4, 4);
        output += spacing + "}\n";
    }

    if(!dependsOn.isEmpty())
    {
        // Remove the last four spaces
        spacing.remove(spacing.size()-4, 4);
        output += spacing + "}\n";
    }

    return output;

}// ProtocolField::getDecodeStringForField


/*!
 * Get a properly formatted number string for a double precision number
 * \param number is the number to turn into a string
 * \param bits is the number of bits in memory for this string. 32 or less will prompt a 'f' suffix on the string
 * \return the string.
 */
QString ProtocolField::getNumberString(double number, int bits)
{
    QString string;

    string.setNum(number, 'g', 16);

    // Make sure we have a decimal point
    if(!string.contains("."))
        string += ".0";

    // Float suffix
    if(bits <= 32)
        string += "f";

    return string;
}
