#include "enumcreator.h"
#include "protocolparser.h"
#include <QStringList>


EnumCreator::EnumCreator(const QDomElement& e)
{
    parse(e);
}


void EnumCreator::clear(void)
{
    name.clear();
    comment.clear();
    output.clear();
    nameList.clear();
    commentList.clear();
    valueList.clear();
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
    comment = ProtocolParser::getComment(e);

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
        output += "//!< " + commentList.at(i) + "\n";

    }// for all enumerators

    output += "}";
    output += name;
    output += ";\n";

    return output;

}// EnumCreator::parse


/*!
 * Get the markdown output that documents this enumeration
 * \param indent is the indent level for the markdown output
 * \return the markdown output string
 */
QString EnumCreator::getMarkdown(QString indent) const
{
    QString output;
    QStringList numberList;

    // Attempt to get a list of numbers that represents the value of each enumeration
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

            // Is this incremented value aboslute, or referenced to
            // some other string we could not resolve?
            if(baseString.isEmpty())
                stringValue.setNum(value);
            else
                stringValue = baseString + " + " + QString().setNum(value);

        }// if the xml was empty
        else
        {
            bool ok;

            if(stringValue.startsWith("0x"))
                value = stringValue.toUInt(&ok, 16);
            else if(stringValue.startsWith("0b"))
                value = stringValue.toUInt(&ok, 2);
            else
                value = stringValue.toUInt(&ok, 10);

            // If we didn't get a number, then this is string has to be resolved
            // by the compiler, all we can do is track offsets from it
            if(!ok)
            {
                baseString = stringValue;
                value = 0;
            }
            else
            {
                baseString.clear();
                stringValue.setNum(value);
            }

        }// if we got a string from the xml

        // Append to the number list
        numberList.append(stringValue);

    }// for the whole list of value strings

    // Add one more space for visibility
    output += indent + "* `" + name + "`";
    if(!comment.isEmpty())
        output += " :  " + comment + ".";
    output += "\n";
    output += "\n";

    indent += "    ";
    for(int i = 0; i < nameList.length(); i++)
    {
        output += indent;

        // bulleted list with name and value in code
        output += "* `" + nameList.at(i) + " = " + numberList.at(i) + "`";
        if(!commentList.at(i).isEmpty())
            output += " :  " +  commentList.at(i) + ".";
        output += "\n";
        output += "\n";
    }

    return output;

}// EnumCreator::getMarkdown
