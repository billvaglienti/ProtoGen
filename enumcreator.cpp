#include "enumcreator.h"
#include "protocolparser.h"
#include <QStringList>
#include <math.h>

EnumCreator::EnumCreator(const QDomElement& e):
    minbitwidth(0),
    hidden(false)
{
    parse(e);
}


void EnumCreator::clear(void)
{
    minbitwidth = 0;
    hidden = false;
    name.clear();
    comment.clear();
    description.clear();
    output.clear();
    nameList.clear();
    commentList.clear();
    valueList.clear();
    numberList.clear();
}


/*!
 * Parse an Enum tag from the xml to create an enumeration.
 * \param e is the Enum tag DOM element
 * \return A string (including linefeeds) to declare the enumeration
 */
QString EnumCreator::parse(const QDomElement& e)
{
    clear();

    name = e.attribute("name");
    comment = e.attribute("comment");
    description = e.attribute("description");

    // If the enum struct has the attribute 'hidden="true"',
    // it won't be displayed in the documentation
    hidden = ProtocolParser::isFieldSet(e,"hidden");

    QDomNodeList list = e.elementsByTagName("Value");

    // If we have no entries there is nothing to do
    if(list.size() <= 0)
        return output;

    // Put the top level comment in
    if(!comment.isEmpty())
    {
        output += "/*!\n";
        output += ProtocolParser::outputLongComment(" *", comment) + "\n";
        output += " */\n";
    }

    QStringList declarationList;

    int maxLength = 0;
    for(int i = 0; i < list.size(); i++)
    {
        QDomElement field = list.at(i).toElement();

        if(field.isNull())
            continue;

        QString valueName = field.attribute("name");
        if(valueName.isEmpty())
            continue;

        // Add it to our list
        nameList.append(valueName);

        // The declared value, which may be empty
        QString value = field.attribute("value");
        valueList.append(value);

        // And don't forget the comment
        commentList.append(ProtocolParser::getComment(field));

        // Form the declaration string
        QString declaration = "    " + valueName;
        if(!value.isEmpty())
            declaration += " = " + value;

        declarationList.append(declaration);

        // Track the longest declaration
        int length = declaration.length();
        if(length > maxLength)
            maxLength = length;

    }// for all enum entries

    // Figure out the number list we will use for markdown
    computeNumberList();

    // Account for 1 character we will add below
    maxLength += 1;

    // We want maxLength to be a multiple of 4
    maxLength += 4 - (maxLength % 4);

    // Declare the enumeration
    output += "typedef enum\n";
    output += "{\n";

    for(int i = 0; i < declarationList.size(); i++)
    {
        // Output the enumerator name and separator
        output += declarationList.at(i);

        // Output a comma separator or space for the last item
        if(i < (declarationList.size() - 1))
            output += ",";
        else
            output += " ";

        // Pad to maxLength
        for(int j = declarationList.at(i).length(); j < maxLength; j++)
            output += " ";

        // Output the comment
        if(commentList.at(i).isEmpty())
            output += "\n";
        else
            output += "//!< " + commentList.at(i) + "\n";

    }// for all enumerators

    output += "}";
    output += name;
    output += ";\n";

    return output;

}// EnumCreator::parse


/*!
 * Go through the list of enum strings and attempt to determine the list of
 * actual numbers that will be output in markdown. This is also how we
 * determine the number of bits needed to encode. This is called by parse()
 */
void EnumCreator::computeNumberList(void)
{
    // Attempt to get a list of numbers that represents the value of each enumeration
    int maxValue = 1;
    int value = -1;
    QString baseString;
    for(int i = 0; i < valueList.length(); i++)
    {
        // The string from the XML, which may be empty
        QString stringValue = valueList.at(i);

        // Clear any whitespace from it just to be sure
        stringValue = stringValue.trimmed();

        if(stringValue.isEmpty())
        {
            // Increment enumeration value by one
            value++;

            // Is this incremented value absolute, or referenced to
            // some other string we could not resolve?
            if(baseString.isEmpty())
                stringValue.setNum(value);
            else
                stringValue = baseString + " + " + QString().setNum(value);

            // Append to the number list
            numberList.append(stringValue);

        }// if the xml was empty
        else
        {
            bool ok;

            if(stringValue.startsWith("0x", Qt::CaseInsensitive))
                value = stringValue.toUInt(&ok, 16);
            else if(stringValue.startsWith("0b", Qt::CaseInsensitive))
                value = stringValue.toUInt(&ok, 2);
            else
                value = stringValue.toUInt(&ok, 10);

            // If we didn't get a value there is one remainaing
            // possibility: this text refers to a previous enumeration
            // whose value we *do* know. We can check that case for any
            // enumeration which was defined before us
            if(!ok)
            {
                ProtocolParser::replaceEnumerationNameWithValue(stringValue);
                value = stringValue.toUInt(&ok, 10);
            }

            // If we didn't get a number, then this string has to be resolved
            // by the compiler, all we can do is track offsets from it
            if(!ok)
            {
                baseString = stringValue;
                value = 0;

                // No useful information is available
                numberList.append(QString());
            }
            else
            {
                baseString.clear();
                stringValue.setNum(value);

                // Append to the number list
                numberList.append(stringValue);
            }

        }// if we got a string from the xml

        // keep track of maximum value
        if(value > maxValue)
            maxValue = value;

    }// for the whole list of value strings

    // Its possible we have no idea, so go with 8 bits in that case
    if(maxValue > 0)
    {
        // Figure out the number of bits needed to encode the maximum value
        minbitwidth = (int)ceil(log2(maxValue + 1));
    }
    else
        minbitwidth = 8;

}// EnumCreator::computeNumberList


/*!
 * Get the markdown output that documents this enumeration
 * \param outline gives the outline number for this heading
 * \param packetids is the list of packet identifiers, used to determine if a link should be added
 * \return the markdown output string
 */
QString EnumCreator::getMarkdown(QString outline, const QStringList& packetids) const
{
    QString output;

    if(nameList.length() > 0)
    {
        QStringList codeNameList;

        // figure out the column spacing in the tables
        int firstColumnSpacing = QString("Name").length();
        int secondColumnSpacing = QString("Value").length();
        int thirdColumnSpacing = QString("Description").length();
        for(int i = 0; i < nameList.length(); i++)
        {
            bool link = false;

            // Check to see if this enumeration is a packet identifier
            for(int j = 0; j < packetids.size(); j++)
            {
                if(packetids.at(j) == nameList.at(i))
                {
                    link = true;
                    break;
                }
            }

            // Make name as code, with or without a link to an anchor elsewhere
            if(link)
                codeNameList.append("[`" + nameList.at(i) + "`](#" + nameList.at(i) + ")");
            else
                codeNameList.append("`" + nameList.at(i) + "`");

            if(firstColumnSpacing < codeNameList.at(i).length())
                firstColumnSpacing = codeNameList.at(i).length();
            if(secondColumnSpacing < numberList.at(i).length())
                secondColumnSpacing = numberList.at(i).length();
            if(thirdColumnSpacing < commentList.at(i).length())
                thirdColumnSpacing = commentList.at(i).length();
        }


        // The outline paragraph
        if(!outline.isEmpty())
            output += "## " + name + "\n\n";

        // commenting for this field
        if(!comment.isEmpty())
            output += comment + "\n\n";

        // If a longer description exists for this enum, display it in the documentation
        if (!description.isEmpty())
        {
            output += "**Description:**\n";
            output += description;
            output += "\n\n";
        }

        // Table caption, with an anchor for the enumeration name
        output += "[<a name=\""+name+"\"></a>" + name + "]\n";

        // Table header
        output += "| ";
        output += spacedString("Name", firstColumnSpacing);
        output += " | ";
        output += spacedString("Value", secondColumnSpacing);
        output += " | ";
        output += spacedString("Description", thirdColumnSpacing);
        output += " |\n";

        // Underscore the header
        output += "| ";
        for(int i = 0; i < firstColumnSpacing; i++)
            output += "-";
        output += " | :";
        for(int i = 1; i < secondColumnSpacing-1; i++)
            output += "-";
        output += ": | ";
        for(int i = 0; i < thirdColumnSpacing; i++)
            output += "-";
        output += " |\n";

        // Now write out the outputs
        for(int i = 0; i < codeNameList.length(); i++)
        {
            output += "| ";
            output += spacedString(codeNameList.at(i), firstColumnSpacing);
            output += " | ";
            output += spacedString(numberList.at(i), secondColumnSpacing);
            output += " | ";
            output += spacedString(commentList.at(i), thirdColumnSpacing);
            output += " |\n";

        }

        output += "\n";

    }

    return output;

}// EnumCreator::getMarkdown


/*!
 * Replace any text that matches an enumeration name with the value of that enumeration
 * \param text is modified to replace names with numbers
 * \return a reference to text
 */
QString& EnumCreator::replaceEnumerationNameWithValue(QString& text) const
{
    for(int i = 0; i < nameList.length(); i++)
    {
        // If we don't have a name or number there is no point
        if(nameList.at(i).isEmpty() || numberList.at(i).isEmpty())
            continue;

        // It may be possible that text contains multiple different enum
        // values, so don't just exit after the first replace
        text.replace(nameList.at(i), numberList.at(i));
    }

    return text;

}// EnumCreator::replaceEnumerationNameWithValue


/*!
 * Determine if text is part of an enumeration. This will compare against all
 * elements in this enumeration and return true if any of them match.
 * \param text is the enumeration value string to compare.
 * \return true if text is a value in this enumeration.
 */
bool EnumCreator::isEnumerationValue(const QString& text) const
{
    for(int i = 0; i < nameList.length(); i++)
    {
        // If we don't have a name there is no point
        if(nameList.at(i).isEmpty())
            continue;

        if(text.compare(nameList.at(i)) == 0)
            return true;
    }

    return false;

}// EnumCreator::isEnumerationValue


/*!
 * Output a spaced string
 * \param text is the first part of the string
 * \param spacing is the total length of the string. The output is padded with
 *        spaces to reach this length.
 * \return The spaced string.
 */
QString spacedString(QString text, int spacing)
{
    QString output = text;

    while(output.length() < spacing)
        output += " ";

    return output;
}
