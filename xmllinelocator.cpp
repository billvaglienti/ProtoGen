#include "xmllinelocator.h"
#include <QStringList>


/*!
 * Set the contents of this XMLContent. This function is recursive
 * \param text is the entire input text from the xml file
 * \param startindex is the starting index to search in text, which will be
 *        updated as text is searched
 * \param linenumber is the current line number, which will be updated as text
 *        is searched
 */
void XMLContent::setXMLContents(const QString& text, int& startindex, int& linenumber)
{
    // Remember my starting line number
    mylinenumber = linenumber;

    while((startindex < (text.count()-1)) && (startindex >= 0))
    {
        // Search character by character
        QChar character = text.at(startindex++);

        // Look for xml tag delimeters
        if(character == '<')
        {
            if(text.at(startindex) == '/')
            {
                // "</" is the opening of a terminating tag, find the end of it.
                // We assume there are no line breaks between </ and >
                startindex = text.indexOf('>', startindex);

                if(startindex > 0)
                {
                    // Skip past the '>'
                    startindex++;

                    // Figure out our name from the contents
                    parseNameFromContents();
                }

                // Tag is closed (or failed), return to the next level
                return;
            }
            else if(text.at(startindex) == '?')
            {
                // This is the prolog, find the end of it, and just skip over
                // We assume there are no line breaks between "<?" and "?>"
                startindex = text.indexOf("?>", startindex);

                // Skip past the "?>"
                if(startindex > 0)
                    startindex+=2;
                else
                    return;
            }
            else
            {
                // '<' is the opening of a new tag, create a new sub for it, next level down
                subs.append(XMLContent(linenumber, outline + 1));

                // Set the contents of the sub, recursive call
                subs.last().setXMLContents(text, startindex, linenumber);
            }
        }
        else if(character == '/')
        {
            if(text.at(startindex) == '>')
            {
                // "/>", Tag is closed
                startindex++;

                // Figure out our name from the contents
                parseNameFromContents();

                // return to the next level
                return;
            }

        }
        else if(character == '>')
        {
            // Tag is not closing (not preceded by '/'), so we keep
            // pulling down data until we hit the next opener: '<'
        }
        else if(character == '\n')
        {
            // Every linefeed counts towards our line number
            linenumber++;

            // Keep the line feeds as part of our contents
            contents += character;
        }
        else
        {
            contents += character;
        }

    }// while bytes to process

    // Figure out our name from the contents
    parseNameFromContents();

}// XMLContent::setXMLContents


/*!
 * Determine the "name" attribute from the contents of this XMLContent
 */
void XMLContent::parseNameFromContents(void)
{
    name = parseAttribute("name", contents);
}


/*!
 * Determine an attribute value from the contents of an XMLContent
 * \param label is the label of the attribute
 * \param xmltext is the conents of an XMLContent
 * \return the attribute value, or an empty string if the attribute is not
 *         found. The attribute value will not include quotation marks.
 */
QString XMLContent::parseAttribute(const QString& label, const QString& xmltext)
{
    // It starts with the label
    int startingindex = xmltext.indexOf(label, 0, Qt::CaseInsensitive);
    if(startingindex < 0)
        return QString();

    // Then there must be an "="
    startingindex = xmltext.indexOf("=", startingindex, Qt::CaseInsensitive);
    if(startingindex < 0)
        return QString();

    // Then there must be an " \" "
    startingindex = xmltext.indexOf("\"", startingindex, Qt::CaseInsensitive);
    if(startingindex < 0)
        return QString();

    // And a closing quote
    int endingindex = xmltext.indexOf("\"", startingindex+1, Qt::CaseInsensitive);
    if(endingindex < 0)
        return QString();

    // Extract the attribute value
    return xmltext.mid(startingindex+1, endingindex - startingindex - 1);

}


/*!
 * Search a hierarchy of names to find the corresponding XML element and return
 * its line number. This function may be called recursively
 * \param names is the list of names to search for
 * \param level identifies which name in the names list to search for
 * \return the line number of the xml element which corresponds to the last
 *         name in the name list, or -1 of its not found
 */
int XMLContent::getMatchingLineNumber(const QStringList& names, int level) const
{
    if(names.at(level).compare(name) == 0)
    {
        if(names.count() <= (level+1))
            return mylinenumber;
        else
        {
            for(int i = 0; i < subs.length(); i++)
            {
                int line = subs.at(i).getMatchingLineNumber(names, level+1);
                if(line >= 0)
                    return line;
            }
        }
    }

    return -1;

}// XMLContent::getMatchingLineNumber


/*!
 * Set the raw xml contents for the line locator. This will cause the line
 * locator to parse the xml, building a tree of xml contents with corresponding
 * line numbers
 * \param text is the raw xml text
 */
void XMLLineLocator::setXMLContents(QString text)
{
    // The first character in a string is index 0
    int startindex = 0;

    // The first line of a file is line "1"
    int linenumber = 1;

    // Convert the line endings
    text.replace("\r\n", "\n");

    while(startindex < text.count())
    {
        QChar character = text.at(startindex++);

        if(character == '<')
        {
            if(text.at(startindex) == '?')
            {
                // This is the prolog, find the end of it, and just skip over
                // We assume there are no line breaks between <? and ?>
                startindex = text.indexOf("?>", startindex);
                if(startindex > 0)
                    startindex+=2;
                else
                    return;
            }
            else
            {
                // Opening tag, data starts here
                contents.setXMLContents(text, startindex, linenumber);

                // We're done, we just needed to find the opener
                return;
            }
        }
        else if(character == '\n')
        {
            linenumber++;
        }

    }// while looking for the opening tag

}// XMLLineLocator::setXMLContents


/*!
 * Find the line number given a hierarchical name (names separated by ':')
 * \param hierarchicalName is the name hierarchy to search for
 * \return the line number corresponding to the last name in the hierarchy,
 *         or -1 if not found
 */
int XMLLineLocator::getLineNumber(QString hierarchicalName) const
{
    return contents.getMatchingLineNumber(hierarchicalName.split(":"), 0);

}// XMLLineLocator::getLineNumber


