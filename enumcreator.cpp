#include "enumcreator.h"
#include "protocolparser.h"
#include <QStringList>
#include <QRegularExpression>
#include <QChar>
#include <math.h>
#include <iostream>

EnumElement::EnumElement(ProtocolParser *parse, EnumCreator *creator, QString Parent, ProtocolSupport supported) :
    ProtocolDocumentation(parse, Parent, supported),
    parentEnum(creator)
{
}

//! Create an empty enumeration list
EnumCreator::EnumCreator(ProtocolParser* parse, QString Parent, ProtocolSupport supported) :
    ProtocolDocumentation(parse, Parent, supported),
    minbitwidth(0),
    hidden(false),
    lookup(false),
    isglobal(false)
{
}

void EnumElement::parse()
{
    auto map = e.attributes();

    testAndWarnAttributes(map, QStringList()
                          << "name"
                          << "lookupName"
                          << "value"
                          << "comment"
                          << "hidden"
                          << "ignorePrefix"
                          << "ignoreLookup");

    Name = ProtocolParser::getAttribute("name", map);
    LookupName = ProtocolParser::getAttribute("lookupName", map);
    Value = ProtocolParser::getAttribute("value", map);
    Comment = ProtocolParser::getAttribute("comment", map);
    IsHidden = ProtocolParser::isFieldSet("hidden", map);
    IgnoresPrefix = ProtocolParser::isFieldSet("ignorePrefix", map);
    IgnoresLookup = ProtocolParser::isFieldSet("ignoreLookup", map);
}

EnumCreator::~EnumCreator(void)
{
    // Delete all the objects in the list
    for(int i = 0; i < documentList.count(); i++)
    {
        if(documentList[i] != NULL)
        {
            delete documentList[i];
            documentList[i] = NULL;
        }
    }
}


void EnumCreator::clear(void)
{
    file.clear();
    sourceOutput.clear();
    minbitwidth = 0;
    hidden = false;
    name.clear();
    comment.clear();
    description.clear();
    output.clear();
    nameList.clear();
    prefix.clear();
    commentList.clear();
    valueList.clear();
    numberList.clear();
    hiddenList.clear();

    // Delete all the objects in the list
    for(int i = 0; i < documentList.count(); i++)
    {
        if(documentList[i] != NULL)
        {
            delete documentList[i];
            documentList[i] = NULL;
        }
    }

    // clear the list
    documentList.clear();
}


/*!
 * Parse the DOM to fill out the enumeration list for a global enum. This sill
 * also set the header reference file name that others need to include to use
 * this enum.
 * \param filename is the reference file if none is specified in the xml
 */
void EnumCreator::parseGlobal(QString filename)
{
    isglobal = true;

    parse();

    // Global enums must have a reference file
    if(file.isEmpty())
        file = filename.left(filename.indexOf("."));    // remove any extension

    isglobal = false;
}


/*!
 * Parse an Enum tag from the xml to create an enumeration.
 */
void EnumCreator::parse(void)
{
    clear();

    // Get any documentation for this enumeration
    ProtocolDocumentation::getChildDocuments(parser, getHierarchicalName(), support, e, documentList);

    QDomNamedNodeMap map = e.attributes();

    // We use name as part of our debug outputs, so its good to have it first.
    name = ProtocolParser::getAttribute("name", map);

    // Tell the user of any problems in the attributes
    testAndWarnAttributes(map, QStringList() <<  "name" << "comment" << "description" << "hidden" << "lookup" << "prefix" << "file");

    // Go get the rest of the attributes
    description = ProtocolParser::getAttribute("description", map);
    prefix = ProtocolParser::getAttribute("prefix", map);
    comment = ProtocolParser::reflowComment(ProtocolParser::getAttribute("comment", map));
    hidden = ProtocolParser::isFieldSet("hidden", map);
    lookup = ProtocolParser::isFieldSet("lookup", map);

    if(isglobal)
    {
        // Remove the extension if any
        file = ProtocolParser::getAttribute("file", map);
        file = file.left(file.indexOf("."));
    }

    QDomNodeList list = e.elementsByTagName("Value");

    // If we have no entries there is nothing to do
    if(list.size() <= 0)
        return;

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

        QDomNamedNodeMap map = field.attributes();

        // We use name as part of our debug outputs, so its good to have it first.
        QString valueName = ProtocolParser::getAttribute("name", map);

        // Tell the user of any problems in the attributes
        testAndWarnAttributes(map, QStringList() <<  "name" << "value" << "comment" << "hidden" << "ignorePrefix", valueName);

        // Now get the attributes
        QString value = ProtocolParser::getAttribute("value", map);
        QString comment = ProtocolParser::reflowComment(ProtocolParser::getAttribute("comment", map));
        bool hiddenvalue = ProtocolParser::isFieldSet("hidden", map);
        bool ignorePrefix = ProtocolParser::isFieldSet("ignorePrefix", map);

        // Add the enum prefix if applicable
        if ( !prefix.isEmpty() && !ignorePrefix )
            valueName = prefix + valueName;

        // Add it to our list
        nameList.append(valueName);

        // The declared value, which may be empty
        valueList.append(value);

        // And don't forget the comment
        commentList.append(comment);

        // Specific enum values can be hidden
        hiddenList.append(hiddenvalue);

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

    // Check for keywords that will cause compilation problems
    checkAgainstKeywords();

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

    if (lookup)
    {
        output += "\n";
        output += "//! \\return the label of a '" + name + "' enum entry, based on its value\n";

        QString func = "const char* " + name + "_Label(int value)";

        output += func + ";\n";

        // Add reverse-lookup code to the source file
        sourceOutput += "\n/*!\n";
        sourceOutput += " * \\brief Lookup label for '" + name + "' enum entry\n";
        sourceOutput += " * \n";
        sourceOutput += " * \\param value is the integer value of the enum entry\n";
        sourceOutput += " * \\return string label of the given entry (emptry string if not found)\n";
        sourceOutput += " */\n";

        sourceOutput += func + "\n";
        sourceOutput += "{\n";

        sourceOutput += "    switch (value)\n";
        sourceOutput += "    {\n";
        sourceOutput += "    default:\n";
        sourceOutput += "        return \"\";\n";

        for (int i=0; i < nameList.size(); i++ )
        {
            sourceOutput += "    case " + nameList.at(i) + ":\n";
            sourceOutput += "        return \"" + nameList.at(i) + "\";\n";
        }

        sourceOutput += "    }\n";
        sourceOutput += "}\n";
    }

}// EnumCreator::parse


//! Check names against the list of C keywords, this includes the global enumeration name as well as all the value names
void EnumCreator::checkAgainstKeywords(void)
{
    if(keywords.contains(name))
    {
        emitWarning("name matches C keyword, changed to _name");
        name = "_" + name;
    }

    for(int i = 0; i < nameList.size(); i++)
    {
        if(keywords.contains(nameList.at(i)))
        {
            emitWarning("enum value name matches C keyword, changed to _name");
            nameList[i] = "_" + nameList.at(i);
        }

        if(keywords.contains(valueList.at(i)))
        {
            emitWarning("enum value matches C keyword, changed to _value");
            valueList[i] = "_" + valueList.at(i);
        }

    }

}// EnumCreator::checkAgainstKeywords


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
                parser->replaceEnumerationNameWithValue(stringValue);
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
 * \param global should be true to include a paragraph number for this heading
 * \param packetids is the list of packet identifiers, used to determine if a link should be added
 * \return the markdown output string
 */
QString EnumCreator::getTopLevelMarkdown(bool global, const QStringList& packetids) const
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

            // Mark name as code, with or without a link to an anchor elsewhere
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
        if(global)
            output += "## " + name + " enumeration\n\n";

        // commenting for this field
        if(!comment.isEmpty())
            output += comment + "\n\n";

        for(int i = 0; i < documentList.count(); i++)
            output += documentList.at(i)->getTopLevelMarkdown();

        // If a longer description exists for this enum, display it in the documentation
        if (!description.isEmpty())
        {
            output += "**Description:**\n";
            output += description;
            output += "\n\n";
        }

        // Table caption, with an anchor for the enumeration name
        output += "[<a name=\""+name+"\"></a>" + name + " enumeration]\n";

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
            // Skip hidden values
            if(hiddenList.at(i) == true)
                continue;

            output += "| ";
            output += spacedString(codeNameList.at(i), firstColumnSpacing);
            output += " | ";
            output += spacedString(numberList.at(i), secondColumnSpacing);
            output += " | ";
            output += spacedString(commentList.at(i), thirdColumnSpacing);
            output += " |\n";

        }

        output += "\n";
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
    // split words around mathematical operators
    QStringList tokens = splitAroundMathOperators(text);

    for(int j = 0; j < tokens.size(); j++)
    {
        // Don't look to replace the mathematical operators
        if(isMathOperator(tokens.at(j).at(0)))
            continue;

        for(int i = 0; i < nameList.length(); i++)
        {
            // If we don't have a name or number there is no point
            if(nameList.at(i).isEmpty() || numberList.at(i).isEmpty())
                continue;

            // The entire token must match before we will replace it
            if(tokens.at(j).compare(nameList.at(i)) == 0)
                tokens[j] = numberList.at(i);

        }// for all names

    }// for all tokens

    // Rejoin the strings
    text = tokens.join(QString());
    return text;

}// EnumCreator::replaceEnumerationNameWithValue


/*!
 * Split a string around the math operators. This is just like QString::split(), except we keep the operators as tokens
 * \param text is the input text to split
 * \return is the list of split strings
 */
QStringList EnumCreator::splitAroundMathOperators(QString text) const
{
    QStringList output;
    QString token;

    for(int i = 0; i < text.count(); i++)
    {
        if(isMathOperator(text.at(i)))
        {
            // If we got a math operator, then append the preceding token to the list
            if(!token.isEmpty())
            {
                output.append(token);

                // Clear the token, its finished
                token.clear();
            }

            // Also append the operator as a token, we want to keep this
            output.append(text.at(i));
        }
        else
        {
            // If not a math operator, then just add to the current token
            token.append(text.at(i));

        }// switch on character

    }// for all characters in the input

    // Get the last token (might be the only one)
    if(!token.isEmpty())
        output.append(token);

    return output;

}// EnumCreator::splitAroundMathOperators


/*!
 * Determine if a character is a math operator
 * \param op is the character to check
 * \return true if op is a math operator
 */
bool EnumCreator::isMathOperator(QChar op) const
{
    if((op == '^') || (op == '*') || (op == '/') || (op == '+') || (op == '-') || (op == '(') || (op == ')'))
        return true;
    else
        return false;

}// EnumCreator::isMathOperator


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
