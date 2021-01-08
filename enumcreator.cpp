#include "enumcreator.h"
#include "protocolparser.h"
#include "shuntingyard.h"
#include "encodedlength.h"
#include <math.h>
#include <iostream>
#include <algorithm>

EnumElement::EnumElement(ProtocolParser *parse, EnumCreator *creator, const std::string& parent, ProtocolSupport supported) :
    ProtocolDocumentation(parse, parent, supported),
    hidden(false),
    ignoresPrefix(false),
    ignoresLookup(false),
    parentEnum(creator)
{
    attriblist = {"name", "title", "lookupName", "value", "comment", "hidden", "ignorePrefix", "ignoreLookup"};
}

void EnumElement::checkAgainstKeywords()
{
    if (contains(keywords, getName(), true))
    {
        emitWarning("enum value name matches C keyword, changed to name_");
        name += "_";
    }

    if (contains(keywords, value, true))
    {
        emitWarning("enum value matches C keyword, changed to value_");
        value += "_";
    }
}

void EnumElement::parse()
{
    if(e == nullptr)
        return;

    const XMLAttribute* map = e->FirstAttribute();

    testAndWarnAttributes(map);

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

std::string EnumElement::getName() const
{
    if(ignoresPrefix)
        return name;
    else
        return parentEnum->getPrefix() + name;
}

std::string EnumElement::getLookupName() const
{
    if (lookupName.empty())
    {
        return getName();
    }
    else
    {
        return lookupName;
    }
}

std::string EnumElement::getDeclaration() const
{
    std::string decl = getName();

    if (!value.empty())
    {
        decl += " = " + value;
    }

    return decl;
}

//! Create an empty enumeration list
EnumCreator::EnumCreator(ProtocolParser* parse, const std::string& parent, ProtocolSupport supported) :
    ProtocolDocumentation(parse, parent, supported),
    minbitwidth(0),
    maxvalue(0),
    hidden(false),
    neverOmit(false),
    lookup(false),
    lookupTitle(false),
    lookupComment(false),
    isglobal(false)
{
    attriblist = {"name", "title", "comment", "description", "hidden", "neverOmit", "lookup", "lookupTitle", "lookupComment", "prefix", "file", "translate"};
}

EnumCreator::~EnumCreator(void)
{
    // Delete all the objects in the list
    for(std::size_t i = 0; i < documentList.size(); i++)
    {
        if(documentList.at(i) != nullptr)
        {
            delete documentList.at(i);
            documentList.at(i) = nullptr;
        }
    }
}


void EnumCreator::clear(void)
{
    file.clear();
    filepath.clear();
    sourceOutput.clear();
    minbitwidth = 0;
    maxvalue = 0;
    hidden = false;
    neverOmit = false;
    lookup = false;
    lookupTitle = false;
    lookupComment = false;
    name.clear();
    comment.clear();
    description.clear();
    output.clear();
    prefix.clear();
    translate.clear();

    // Delete all the objects in the list
    for(std::size_t i = 0; i < documentList.size(); i++)
    {
        if(documentList.at(i) != nullptr)
        {
            delete documentList.at(i);
            documentList.at(i) = nullptr;
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

    const XMLAttribute* map = e->FirstAttribute();

    // We use name as part of our debug outputs, so its good to have it first.
    name = ProtocolParser::getAttribute("name", map);

    // Get any documentation for this enumeration
    ProtocolDocumentation::getChildDocuments(parser, getHierarchicalName(), support, e, documentList);

    // Tell the user of any problems in the attributes
    testAndWarnAttributes(map);

    // Go get the rest of the attributes
    title = ProtocolParser::getAttribute("title", map);
    description = ProtocolParser::getAttribute("description", map);
    prefix = ProtocolParser::getAttribute("prefix", map);
    comment = ProtocolParser::reflowComment(ProtocolParser::getAttribute("comment", map));
    hidden = ProtocolParser::isFieldSet("hidden", map);
    neverOmit = ProtocolParser::isFieldSet("neverOmit", map);
    lookup = ProtocolParser::isFieldSet("lookup", map);
    lookupTitle = ProtocolParser::isFieldSet("lookupTitle", map);
    lookupComment = ProtocolParser::isFieldSet("lookupComment", map);
    file = ProtocolParser::getAttribute("file", map);
    translate = ProtocolParser::getAttribute("translate", map, support.globaltranslate);

    // The file attribute is only supported on global enumerations
    if(isglobal)
    {
        filepath = support.outputpath;

        // If no file information is provided we use the global header name
        if(file.empty())
            file = support.protoName + "Protocol";

        // This will separate all the path information
        ProtocolFile::separateModuleNameAndPath(file, filepath);
    }
    else if(!file.empty())
    {
        file.clear();
        emitWarning("Enumeration must be global to support file attribute");
    }


    // Put the top level comment in
    if(!comment.empty())
    {
        output += "/*!\n";
        output += ProtocolParser::outputLongComment(" * ", comment) + "\n";
        output += " */\n";
    }

    elements.clear();

    std::size_t maxLength = 0;

    for(const XMLElement* child = e->FirstChildElement(); child != nullptr; child = child->NextSiblingElement())
    {
        if(!contains(trimm(child->Name()), "value"))
            continue;

        EnumElement elem(parser, this, parent, support);

        elem.setElement(child);

        elem.parse();

        elements.push_back(elem);

        // Track the longest declaration
        std::size_t length = elem.getDeclaration().length();
        if(length > maxLength)
            maxLength = length;

    }// for all enum entries

    // If we have no entries there is nothing to do
    if(elements.size() <= 0)
    {
        output.clear();
        return;
    }

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

    for(std::size_t i = 0; i < elements.size(); i++)
    {
        const auto& element = elements.at(i);

        std::string declaration = TAB_IN + element.getDeclaration();

        // Output the enumerator name and separator
        output += declaration;

        // Output a comma separator or space for the last item
        if(i < (elements.size() - 1))
            output += ",";
        else
            output += " ";

        // Pad to maxLength
        for(std::size_t j = declaration.length(); j < maxLength; j++)
            output += " ";

        // Output the comment
        if(element.comment.empty())
            output += "\n";
        else
            output += "//!< " + element.comment + "\n";

    }// for all enumerators

    output += "} ";
    output += name;
    output += ";\n";


    // If we have a translate macro then we add the special case to keep code compiling if the macro is not included
    if(!translate.empty() && (lookup || lookupComment || lookupTitle))
    {
        // Translation macro in the source file
        sourceOutput += "\n";
        sourceOutput += "// Translation provided externally as a preprocesso macro. The \n";
        sourceOutput += "// macro should take a `const char *` and return a `const char *.`\n";
        sourceOutput += "// This macro is to keep the code compiling if you don't provide the real one.\n";
        sourceOutput += "#ifndef " + translate + "\n";
        sourceOutput += TAB_IN + "#define " + translate + "(x) x\n";
        sourceOutput += "#endif";
        sourceOutput += "\n";
    }

    if (lookup)
    {
        output += "\n";
        output += "//! \\return the label of a '" + name + "' enum entry, based on its value\n";

        std::string func = "const char* " + name + "_EnumLabel(int value)";

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
        for(std::size_t i = 0; i < elements.size(); i++)
        {
            auto element = elements.at(i);

            if (element.ignoresLookup)
                continue;

            sourceOutput += TAB_IN + "case " + element.getName() + ":\n";
            if(translate.empty())
                sourceOutput += TAB_IN + TAB_IN + "return \"" + element.getName() + "\";\n";
            else
                sourceOutput += TAB_IN + TAB_IN + "return " + translate + "(\"" + element.getName() + "\");\n";
        }

        sourceOutput += TAB_IN + "}\n";
        sourceOutput += "}\n";

    }// if name lookup

    if (lookupTitle)
    {
        output += "\n";
        output += "//! \\return the title of a '" + name + "' enum entry, based on its value\n";

        std::string func = "const char* " + name + "_EnumTitle(int value)";

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
        for(std::size_t i = 0; i < elements.size(); i++)
        {
            auto element = elements.at(i);

            if (element.ignoresLookup)
                continue;

            sourceOutput += TAB_IN + "case " + element.getName() + ":\n";

            // Title takes first preference, if supplied
            std::string _title = element.title;

            // Comment takes second preference, if supplied
            if (_title.empty())
                _title = element.comment;

            if (_title.empty())
                _title = element.getLookupName();

            if(translate.empty())
                sourceOutput += TAB_IN + TAB_IN + "return \"" + _title + "\";\n";
            else
                sourceOutput += TAB_IN + TAB_IN + "return " + translate + "(\"" + _title + "\");\n";
        }

        sourceOutput += TAB_IN + "}\n";
        sourceOutput += "}\n";

    }// if title lookup

    if (lookupComment)
    {
        output += "\n";
        output += "//! \\return the comment of a '" + name + "' enum entry, based on its value\n";

        std::string func = "const char* " + name + "_EnumComment(int value)";

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
        for(std::size_t i = 0; i < elements.size(); i++)
        {
            auto element = elements.at(i);

            if (element.ignoresLookup)
                continue;

            sourceOutput += TAB_IN + "case " + element.getName() + ":\n";

            // Title takes first preference, if supplied
            std::string _comment = element.comment;

            // Title takes second preference, if supplied
            if (_comment.empty())
                _comment = element.title;

            if (_comment.empty())
                _comment = element.getLookupName();

            if(translate.empty())
                sourceOutput += TAB_IN + TAB_IN + "return \"" + _comment + "\";\n";
            else
                sourceOutput += TAB_IN + TAB_IN + "return " + translate + "(\"" + _comment + "\");\n";
        }

        sourceOutput += TAB_IN + "}\n";
        sourceOutput += "}\n";

    }// if comment lookup

}// EnumCreator::parse


//! Check names against the list of C keywords, this includes the global enumeration name as well as all the value names
void EnumCreator::checkAgainstKeywords(void)
{
    if(contains(keywords, name))
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
    maxvalue = 1;
    int64_t value = -1;
    std::string baseString;

    for(std::size_t i = 0; i < elements.size(); i++)
    {
        auto& element = elements[i];

        // The string from the XML, which may be empty, clear any whitespace from it just to be sure
        std::string stringValue = trimm(element.value);

        if(stringValue.empty())
        {
            // Increment enumeration value by one
            value++;

            // Is this incremented value absolute, or referenced to
            // some other string we could not resolve?
            if(baseString.empty())
            {
                stringValue = std::to_string(value);
            }
            else
            {
                stringValue = baseString + " + " + std::to_string(value);
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
                stringValue = parser->replaceEnumerationNameWithValue(stringValue);

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
                stringValue = std::to_string(value);
            }

        }// if we got a string from the xml

        // keep track of maximum value
        if(value > maxvalue)
            maxvalue = (int)value;

        // Remember the value
        element.number = stringValue;

    }// for the whole list of value strings

    // Its possible we have no idea, so go with 8 bits in that case
    if(maxvalue > 0)
    {
        // Figure out the number of bits needed to encode the maximum value
        minbitwidth = (int)ceil(log2(maxvalue + 1));
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
std::string EnumCreator::getTopLevelMarkdown(bool global, const std::vector<std::string>& packetids) const
{
    std::string output;

    if(elements.size() > 0)
    {
        std::vector<std::string> codeNameList;

        // figure out the column spacing in the tables
        std::size_t firstColumnSpacing = std::string("Name").length();
        std::size_t secondColumnSpacing = std::string("Value").length();
        std::size_t thirdColumnSpacing = std::string("Description").length();

        for(std::size_t i = 0; i < elements.size(); i++)
        {
            auto element = elements.at(i);

            bool link = false;

            // Check to see if this enumeration is a packet identifier
            for(std::size_t j = 0; j < packetids.size(); j++)
            {
                if(packetids.at(j) == element.getName())
                {
                    link = true;
                    break;
                }
            }

            std::string linkText;

            // Mark name as code, with or without a link to an anchor elsewhere
            if(element.title.empty())
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

            codeNameList.push_back(linkText);

            firstColumnSpacing = std::max(firstColumnSpacing, linkText.length());
            secondColumnSpacing = std::max(secondColumnSpacing, element.number.length());
            thirdColumnSpacing = std::max(thirdColumnSpacing, element.comment.length());
        }

        // The outline paragraph
        if(global)
        {
            if(title.empty())
                output += "## " + name + " enumeration\n\n";
            else
                output += "## " + title + "\n\n";
        }

        // commenting for this field
        if(!comment.empty())
            output += comment + "\n\n";

        for(std::size_t i = 0; i < documentList.size(); i++)
            output += documentList.at(i)->getTopLevelMarkdown();

        // If a longer description exists for this enum, display it in the documentation
        if (!description.empty())
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

        // Underscore the header. The first column is left aligned, and the second column is centered
        output += "| :";
        for(std::size_t i = 1; i < firstColumnSpacing; i++)
            output += "-";
        output += " | :";
        for(std::size_t i = 1; i < secondColumnSpacing-1; i++)
            output += "-";
        output += ": | ";
        for(std::size_t i = 0; i < thirdColumnSpacing; i++)
            output += "-";
        output += " |\n";

        // Now write out the outputs
        for(std::size_t i = 0; i < codeNameList.size(); i++)
        {
            const auto& element = elements.at(i);

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
        if(title.empty())
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
std::string EnumCreator::replaceEnumerationNameWithValue(const std::string& text) const
{
    // split words around mathematical operators
    std::vector<std::string> tokens = splitAroundMathOperators(text);

    for(std::size_t j = 0; j < tokens.size(); j++)
    {
        std::string token = trimm(tokens.at(j));

        // Don't look to replace the mathematical operators
        if(isMathOperator(token.at(0)))
            continue;

        // Don't look to replace elements that are already numeric
        if (ShuntingYard::isInt(token))
            continue;

        for (const auto& element : elements )
        {
            // The entire token must match before we will replace it
            if(token == trimm(element.getName()))
            {
                if (!element.number.empty())
                {
                    tokens[j] = element.number;
                }
                else if (!element.value.empty())
                {
                    tokens[j] = element.value;
                }
                break;
            }
        }// for all names

    }// for all tokens

    // Rejoin the strings
    return join(tokens);

}// EnumCreator::replaceEnumerationNameWithValue


/*!
 * Split a string around the math operators, keeping the operators as strings in the output.
 * \param text is the input text to split
 * \return is the list of split strings
 */
std::vector<std::string> EnumCreator::splitAroundMathOperators(std::string text) const
{
    std::vector<std::string> output;
    std::string token;

    for(std::size_t i = 0; i < text.size(); i++)
    {
        if(isMathOperator(text.at(i)))
        {
            // If we got a math operator, then append the preceding token to the list
            if(!token.empty())
            {
                output.push_back(token);

                // Clear the token, its finished
                token.clear();
            }

            // Also append the operator as a token, we want to keep this
            output.push_back(text.substr(i, 1));
        }
        else
        {
            // If not a math operator, then just add to the current token
            token.push_back(text.at(i));

        }// switch on character

    }// for all characters in the input

    // Get the last token (might be the only one)
    if(!token.empty())
        output.push_back(token);

    return output;

}// EnumCreator::splitAroundMathOperators


/*!
 * Determine if a character is a math operator
 * \param op is the character to check
 * \return true if op is a math operator
 */
bool EnumCreator::isMathOperator(char op) const
{
    if(ShuntingYard::isOperator(op) || ShuntingYard::isParen(op))
        return true;
    else
        return false;

}// EnumCreator::isMathOperator


//! Return the name of the first enumeration in the list (used for initial value)
std::string EnumCreator::getFirstEnumerationName(void) const
{
    if(elements.size() <= 0)
    {
        // In this case just return zero, cast to the enumeration name
        return "(" + name + ")0";
    }
    else
        return elements.front().getName();
}


/*!
 * Find the enumeration value with this name and return its comment, or an empty string
 * \param name is the name of the enumeration value to find
 * \return the comment string of the name enumeration element, or an empty string if name is not found
 */
std::string EnumCreator::getEnumerationValueComment(const std::string& name) const
{
    for(const auto& element : elements )
    {
        if(trimm(name) == trimm(element.getName()))
            return element.comment;
    }

    return std::string();
}


/*!
 * Determine if text is part of an enumeration. This will compare against all
 * elements in this enumeration and return true if any of them match.
 * \param text is the enumeration value string to compare.
 * \return true if text is a value in this enumeration.
 */
bool EnumCreator::isEnumerationValue(const std::string& text) const
{
    for(const auto& element : elements )
    {
        if(trimm(text) == trimm(element.getName()))
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
std::string spacedString(const std::string& text, std::size_t spacing)
{
    std::string output = text;

    while(output.length() < spacing)
        output += " ";

    return output;
}
