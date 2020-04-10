#include "enumcreator.h"
#include "protocolparser.h"
#include "shuntingyard.h"
#include "encodedlength.h"
#include <QStringList>
#include <QRegularExpression>
#include <QChar>
#include <math.h>
#include <iostream>

EnumElement::EnumElement(ProtocolParser *parse, EnumCreator *creator, QString Parent, ProtocolSupport supported) :
    ProtocolDocumentation(parse, Parent, supported),
    hidden(false),
    ignoresPrefix(false),
    ignoresLookup(false),
    parentEnum(creator)
{
}

void EnumElement::checkAgainstKeywords()
{
    if (keywords.contains(getName()))
    {
        emitWarning("enum value name matches C keyword, changed to name_");
        name += "_";
    }

    if (keywords.contains(value))
    {
        emitWarning("enum value matches C keyword, changed to value_");
        value += "_";
    }
}

void EnumElement::parse()
{
    auto map = e.attributes();

    testAndWarnAttributes(map, QStringList()
                          << "name"
                          << "title"
                          << "lookupName"
                          << "value"
                          << "comment"
                          << "hidden"
                          << "ignorePrefix"
                          << "ignoreLookup");

    name = ProtocolParser::getAttribute("name", map);
    title = ProtocolParser::getAttribute("title", map);
    lookupName = ProtocolParser::getAttribute("lookupName", map);
    value = ProtocolParser::getAttribute("value", map);
    comment = ProtocolParser::getAttribute("comment", map);
    hidden = ProtocolParser::isFieldSet("hidden", map);
    ignoresPrefix = ProtocolParser::isFieldSet("ignorePrefix", map);
    ignoresLookup = ProtocolParser::isFieldSet("ignoreLookup", map);

    checkAgainstKeywords();
}

QString EnumElement::getName() const
{
    if(ignoresPrefix)
        return name;
    else
        return parentEnum->getPrefix() + name;
}

QString EnumElement::getLookupName() const
{
    if (lookupName.isEmpty())
    {
        return getName();
    }
    else
    {
        return lookupName;
    }
}

QString EnumElement::getDeclaration() const
{
    QString decl = getName();

    if (!value.isEmpty())
    {
        decl += " = " + value;
    }

    return decl;
}

//! Create an empty enumeration list
EnumCreator::EnumCreator(ProtocolParser* parse, QString Parent, ProtocolSupport supported) :
    ProtocolDocumentation(parse, Parent, supported),
    minbitwidth(0),
    hidden(false),
    lookup(false),
    lookupTitle(false),
    lookupComment(false),
    isglobal(false)
{
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
    filepath.clear();
    sourceOutput.clear();
    minbitwidth = 0;
    hidden = false;
    lookup = false;
    lookupTitle = false;
    lookupComment = false;
    name.clear();
    comment.clear();
    description.clear();
    output.clear();
    prefix.clear();

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
 */
void EnumCreator::parseGlobal(void)
{
    isglobal = true;

    parse();

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
    testAndWarnAttributes(map, QStringList()
                          << "name"
                          << "title"
                          << "comment"
                          << "description"
                          << "hidden"
                          << "lookup"
                          << "lookupTitle"
                          << "lookupComment"
                          << "prefix"
                          << "file");

    // Go get the rest of the attributes
    title = ProtocolParser::getAttribute("title", map);
    description = ProtocolParser::getAttribute("description", map);
    prefix = ProtocolParser::getAttribute("prefix", map);
    comment = ProtocolParser::reflowComment(ProtocolParser::getAttribute("comment", map));
    hidden = ProtocolParser::isFieldSet("hidden", map);
    lookup = ProtocolParser::isFieldSet("lookup", map);
    lookupTitle = ProtocolParser::isFieldSet("lookupTitle", map);
    lookupComment = ProtocolParser::isFieldSet("lookupComment", map);
    file = ProtocolParser::getAttribute("file", map);

    // The file attribute is only supported on global enumerations
    if(isglobal)
    {
        QString extension;
        filepath = support.outputpath;

        // If no file information is provided we use the global header name
        if(file.isEmpty())
            file = support.protoName + "Protocol";

        // This will separate all the path information
        ProtocolFile::separateModuleNameAndPath(file, filepath);

        // Make sure the extension is correct (.h, .hpp, .hxx, etc)
        ProtocolFile::extractExtension(file, extension);
        if(!extension.contains(".h"))
            extension = ".h";

        // Now put the (corrected) extension back
        file += extension;

    }
    else if(!file.isEmpty())
    {
        file.clear();
        emitWarning("Enumeration must be global to support file attribute");
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

    elements.clear();

    int maxLength = 0;

    for (int i=0; i<list.size(); i++)
    {
        EnumElement elem(parser, this, parent, support);

        elem.setElement(list.at(i).toElement());

        elem.parse();

        elements.append(elem);

        // Track the longest declaration
        int length = elem.getDeclaration().length();
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

    for(int i = 0; i < elements.size(); i++)
    {
        auto element = elements.at(i);

        auto declaration = TAB_IN + element.getDeclaration();

        // Output the enumerator name and separator
        output += declaration;

        // Output a comma separator or space for the last item
        if(i < (elements.size() - 1))
            output += ",";
        else
            output += " ";

        // Pad to maxLength
        for(int j = declaration.length(); j < maxLength; j++)
            output += " ";

        // Output the comment
        if(element.comment.isEmpty())
            output += "\n";
        else
            output += "//!< " + element.comment + "\n";

    }// for all enumerators

    output += "} ";
    output += name;
    output += ";\n";

    if (lookup)
    {
        output += "\n";
        output += "//! \\return the label of a '" + name + "' enum entry, based on its value\n";

        QString func = "const char* " + name + "_EnumLabel(int value)";

        output += func + ";\n";

        // Add reverse-lookup code to the source file
        sourceOutput += "\n/*!\n";
        sourceOutput += " * \\brief Lookup label for '" + name + "' enum entry\n";
        sourceOutput += " * \n";
        sourceOutput += " * \\param value is the integer value of the enum entry\n";
        sourceOutput += " * \\return string label of the given entry\n";
        sourceOutput += " */\n";

        sourceOutput += func + "\n";
        sourceOutput += "{\n";

        sourceOutput += TAB_IN + "switch (value)\n";
        sourceOutput += TAB_IN + "{\n";
        sourceOutput += TAB_IN + "default:\n";
        sourceOutput += TAB_IN + TAB_IN + "return \"\";\n";

        // Add the reverse-lookup text for each entry in the enumeration
        for (int i=0; i<elements.size(); i++)
        {
            auto element = elements.at(i);

            if (element.ignoresLookup)
                continue;

            sourceOutput += TAB_IN + "case " + element.getName() + ":\n";
            sourceOutput += TAB_IN + TAB_IN + "return translate" + support.protoName + "(\"" + element.getName() + "\");\n";
        }

        sourceOutput += TAB_IN + "}\n";
        sourceOutput += "}\n";

    }// if name lookup

    if (lookupTitle)
    {
        output += "\n";
        output += "//! \\return the title of a '" + name + "' enum entry, based on its value\n";

        QString func = "const char* " + name + "_EnumTitle(int value)";

        output += func + ";\n";

        // Add reverse-lookup code to the source file
        sourceOutput += "\n/*!\n";
        sourceOutput += " * \\brief Lookup title for '" + name + "' enum entry\n";
        sourceOutput += " * \n";
        sourceOutput += " * \\param value is the integer value of the enum entry\n";
        sourceOutput += " * \\return string title of the given entry (comment if no title given)\n";
        sourceOutput += " */\n";

        sourceOutput += func + "\n";
        sourceOutput += "{\n";

        sourceOutput += TAB_IN + "switch (value)\n";
        sourceOutput += TAB_IN + "{\n";
        sourceOutput += TAB_IN + "default:\n";
        sourceOutput += TAB_IN + TAB_IN + "return \"\";\n";

        // Add the reverse-lookup text for each entry in the enumeration
        for (int i=0; i<elements.size(); i++)
        {
            auto element = elements.at(i);

            if (element.ignoresLookup)
                continue;

            sourceOutput += TAB_IN + "case " + element.getName() + ":\n";

            // Title takes first preference, if supplied
            QString title = element.title;

            // Comment takes second preference, if supplied
            if (title.isEmpty())
                title = element.comment;

            if (title.isEmpty())
                title = element.getLookupName();

            sourceOutput += TAB_IN + TAB_IN + "return translate" + support.protoName + "(\"" + title + "\");\n";
        }

        sourceOutput += TAB_IN + "}\n";
        sourceOutput += "}\n";

    }// if title lookup

    if (lookupComment)
    {
        output += "\n";
        output += "//! \\return the comment of a '" + name + "' enum entry, based on its value\n";

        QString func = "const char* " + name + "_EnumComment(int value)";

        output += func + ";\n";

        // Add reverse-lookup code to the source file
        sourceOutput += "\n/*!\n";
        sourceOutput += " * \\brief Lookup comment for '" + name + "' enum entry\n";
        sourceOutput += " * \n";
        sourceOutput += " * \\param value is the integer value of the enum entry\n";
        sourceOutput += " * \\return string comment of the given entry (title if no comment given)\n";
        sourceOutput += " */\n";

        sourceOutput += func + "\n";
        sourceOutput += "{\n";

        sourceOutput += TAB_IN + "switch (value)\n";
        sourceOutput += TAB_IN + "{\n";
        sourceOutput += TAB_IN + "default:\n";
        sourceOutput += TAB_IN + TAB_IN + "return \"\";\n";

        // Add the reverse-lookup text for each entry in the enumeration
        for (int i=0; i<elements.size(); i++)
        {
            auto element = elements.at(i);

            if (element.ignoresLookup)
                continue;

            sourceOutput += TAB_IN + "case " + element.getName() + ":\n";

            // Title takes first preference, if supplied
            QString _comment = element.comment;

            // Title takes second preference, if supplied
            if (_comment.isEmpty())
                _comment = element.title;

            if (_comment.isEmpty())
                _comment = element.getLookupName();

            sourceOutput += TAB_IN + TAB_IN + "return translate" + support.protoName + "(\"" + _comment + "\");\n";
        }

        sourceOutput += TAB_IN + "}\n";
        sourceOutput += "}\n";

    }// if comment lookup

}// EnumCreator::parse


//! Check names against the list of C keywords, this includes the global enumeration name as well as all the value names
void EnumCreator::checkAgainstKeywords(void)
{
    if(keywords.contains(name))
    {
        emitWarning("name matches C keyword, changed to _name");
        name = "_" + name;
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

    for (int i=0; i<elements.size(); i++)
    {
        auto& element = elements[i];

        // The string from the XML, which may be empty
        QString stringValue = element.value;

        // Clear any whitespace from it just to be sure
        stringValue = stringValue.trimmed();

        if(stringValue.isEmpty())
        {
            // Increment enumeration value by one
            value++;

            // Is this incremented value absolute, or referenced to
            // some other string we could not resolve?
            if(baseString.isEmpty())
            {
                stringValue.setNum(value);
            }
            else
            {
                stringValue = baseString + " + " + QString().setNum(value);
            }

        }// if the xml was empty
        else
        {
            bool ok = false;

            // First check that the value provided is numeric
            value = ShuntingYard::toInt(stringValue, &ok);

            // Next, check if the value was defined in *this* enumeration or other enumerations
            if (!ok)
            {
                replaceEnumerationNameWithValue(stringValue);
                parser->replaceEnumerationNameWithValue(stringValue);

                // If this string is a composite of numbers, add them together if we can
                stringValue = EncodedLength::collapseLengthString(stringValue, true);

                // Finally convert to integer
                value = ShuntingYard::toInt(stringValue, &ok);
            }

            // If we didn't get a number, then this string has to be resolved
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

        // keep track of maximum value
        if(value > maxValue)
            maxValue = value;

        // Remember the value
        element.number = stringValue;

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

    if(elements.size() > 0)
    {
        QStringList codeNameList;

        // figure out the column spacing in the tables
        int firstColumnSpacing = QString("Name").length();
        int secondColumnSpacing = QString("Value").length();
        int thirdColumnSpacing = QString("Description").length();

        for(int i = 0; i < elements.size(); i++)
        {
            auto element = elements.at(i);

            bool link = false;

            // Check to see if this enumeration is a packet identifier
            for(int j = 0; j < packetids.size(); j++)
            {
                if(packetids.at(j) == element.getName())
                {
                    link = true;
                    break;
                }
            }

            QString linkText;

            // Mark name as code, with or without a link to an anchor elsewhere
            if(element.title.isEmpty())
            {
                if(link)
                {
                    linkText = "[`" + element.getName() + "`](#" + element.getName() + ")";
                }
                else
                {
                    linkText = "`" + element.getName() + "`";
                }
            }
            else
            {
                if(link)
                {
                    linkText = "[" + element.title + "](#" + element.getName() + ")";
                }
                else
                {
                    linkText = element.title;
                }
            }

            codeNameList.append(linkText);

            firstColumnSpacing = qMax(firstColumnSpacing, linkText.length());
            secondColumnSpacing = qMax(secondColumnSpacing, element.number.length());
            thirdColumnSpacing = qMax(thirdColumnSpacing, element.comment.length());
        }

        // The outline paragraph
        if(global)
        {
            if(title.isEmpty())
                output += "## " + name + " enumeration\n\n";
            else
                output += "## " + title + "\n\n";
        }

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
            auto element = elements.at(i);

            // Skip hidden values
            if(element.isHidden())
                continue;

            output += "| ";
            output += spacedString(codeNameList.at(i), firstColumnSpacing);
            output += " | ";
            output += spacedString(element.number, secondColumnSpacing);
            output += " | ";
            output += spacedString(element.comment, thirdColumnSpacing);
            output += " |\n";
        }

        // Table caption, with an anchor for the enumeration name
        if(title.isEmpty())
            output += "[<a name=\""+name+"\"></a>" + name + " enumeration]\n";
        else
            output += "[<a name=\""+name+"\"></a>" + title + " enumeration]\n";

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
        QString token = tokens.at(j).trimmed();

        // Don't look to replace the mathematical operators
        if(isMathOperator(token.at(0)))
            continue;

        // Don't look to replace elements that are already numeric
        if (ShuntingYard::isInt(token))
            continue;

        for (auto element : elements )
        {
            // The entire token must match before we will replace it
            if(token.compare(element.getName().trimmed()) == 0)
            {
                if (!element.number.isEmpty())
                {
                    tokens[j] = element.number;
                }
                else if (!element.value.isEmpty())
                {
                    tokens[j] = element.value;
                }
                break;
            }
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
    if(ShuntingYard::isOperator(op) || ShuntingYard::isParen(op))
        return true;
    else
        return false;

}// EnumCreator::isMathOperator


/*!
 * Find the enumeration value with this name and return its comment, or an empty string
 * \param name is the name of the enumeration value to find
 * \return the comment string of the name enumeration element, or an empty string if name is not found
 */
QString EnumCreator::getEnumerationValueComment(const QString& name) const
{
    for (auto element : elements )
    {
        if (name.compare(element.getName()) == 0 )
            return element.comment;
    }

    return QString();
}


/*!
 * Determine if text is part of an enumeration. This will compare against all
 * elements in this enumeration and return true if any of them match.
 * \param text is the enumeration value string to compare.
 * \return true if text is a value in this enumeration.
 */
bool EnumCreator::isEnumerationValue(const QString& text) const
{
    for (auto element : elements )
    {
        if (text.trimmed().compare(element.getName().trimmed()) == 0 )
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
