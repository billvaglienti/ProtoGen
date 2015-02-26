#include "encodedlength.h"
#include "shuntingyard.h"
#include <QStringList>

EncodedLength::EncodedLength() :
    minEncodedLength(),
    maxEncodedLength(),
    nonDefaultEncodedLength()
{
}


/*!
 * Clear the encoded length
 */
void EncodedLength::clear(void)
{
    minEncodedLength.clear();
    maxEncodedLength.clear();
    nonDefaultEncodedLength.clear();
}


/*!
 * Determine if there is any data here
 * \return true if an length is recorded, else false
 */
bool EncodedLength::isEmpty(void)
{
    return maxEncodedLength.isEmpty();

}


/*!
 * Add successive length strings
 * \param length is the new length string to add
 * \param isString is true if this length is for a string
 * \param isVariable is true if this length is for a variable length array
 * \param isDependent is true if this length is for a field whose presence depends on another field
 * \parma isDefault is true if this lenght is for a default field.
 */
void EncodedLength::addToLength(const QString & length, bool isString, bool isVariable, bool isDependent, bool isDefault)
{
    if(length.isEmpty())
        return;

    addToLengthString(maxEncodedLength, length);

    // Default fields do not add to the length of anything else
    if(isDefault)
        return;

    // Length of everthing except default, strings are 1 byte
    if(isString)
        addToLengthString(nonDefaultEncodedLength, "1");
    else
        addToLengthString(nonDefaultEncodedLength, length);

    // If not variable or dependent, then add to minimum length
    if(!isVariable && !isDependent)
    {
        // Strings add a minimum length of 1 byte
        if(isString)
            addToLengthString(minEncodedLength, "1");
        else
            addToLengthString(minEncodedLength, length);
    }

}// EncodedLength::addToLength


/*!
 * Add a grouping of length strings to this length
 * \param rightLength is the length strings to add.
 * \param array is the array length, which can be empty.
 * \param isVariable is true if this length is for a variable length array.
 * \param isDependent is true if this length is for a field whose presence depends on another field.
 */
void EncodedLength::addToLength(const EncodedLength& rightLength, const QString& array, bool isVariable, bool isDependent)
{
    if(array.isEmpty())
    {
        addToLengthString(maxEncodedLength, rightLength.maxEncodedLength);
        addToLengthString(nonDefaultEncodedLength, rightLength.nonDefaultEncodedLength);

        // If not variable or dependent, then add to minimum length
        if(!isVariable && !isDependent)
            addToLengthString(minEncodedLength, rightLength.minEncodedLength);
    }
    else
    {
        addToLengthString(maxEncodedLength, array + "*(" + rightLength.maxEncodedLength + ")");
        addToLengthString(nonDefaultEncodedLength, array + "*(" + rightLength.nonDefaultEncodedLength + ")");

        // If not variable or dependent, then add to minimum length
        if(!isVariable && !isDependent)
            addToLengthString(minEncodedLength, array + "*(" + rightLength.minEncodedLength + ")");

    }
}


/*!
 * Add a grouping of length strings
 * \param leftLength is the group that is incremented, can be NULL in which case this function does nothing.
 * \param rightLength is the length strings to add.
 * \param array is the array length, which can be empty.
 * \param isVariable is true if this length is for a variable length array.
 * \param isDependent is true if this length is for a field whose presence depends on another field.
 */
void EncodedLength::add(EncodedLength* leftLength, const EncodedLength& rightLength, const QString& array, bool isVariable, bool isDependent)
{
    if(leftLength != NULL)
        leftLength->addToLength(rightLength, array, isVariable, isDependent);
}


/*!
 * Create a length string like "4 + 3 + N3D*2" by adding successive length strings
 * \param totalLength is the total length string
 * \param length is the new length to add
 */
void EncodedLength::addToLengthString(QString & totalLength, QString length)
{
    bool ok;
    double number;
    int inumber;

    if(length.isEmpty())
        return;

    // Its possible that length represents something like 24*(6), which we can resolve easily, so lets try
    number = ShuntingYard::computeInfix(length, &ok);
    if(ok)
    {
        // round to nearest integer
        int inumber;
        if(number >= 0)
            inumber = (int)(number + 0.5);
        else
            inumber = (int)(number - 0.5);

        length = QString().setNum(inumber);
    }

    if(length == "0")
        return;

    if(totalLength.isEmpty())
        totalLength = length;
    else
    {
        if(totalLength.endsWith(")") || !ok)
        {
            totalLength += "+" + length;
        }
        else
        {
            QString trailingArgument;

            // find the last addition operator
            int index = totalLength.lastIndexOf("+");

            // Get the trailing argument
            if(index <= 0)
                trailingArgument = totalLength;
            else
                trailingArgument = totalLength.right(totalLength.size() - index - 1);

            if(isNumber(trailingArgument))
            {
                number = ShuntingYard::computeInfix(trailingArgument + "+" + length, &ok);
                if(ok)
                {
                    // round to nearest integer
                    if(number >= 0)
                        inumber = (int)(number + 0.5);
                    else
                        inumber = (int)(number - 0.5);

                    if(index >= 0)
                    {
                        // Remove the previous trailing argument
                        totalLength.remove(index+1, totalLength.size());

                        // Replace with our new one
                        totalLength += QString().setNum(inumber);
                    }
                    else
                        totalLength = QString().setNum(inumber);
                }
                else
                    totalLength += "+" + length;
            }
            else
                totalLength += "+" + length;

        }// if length is something we can compute

    }// if totalLength previously had data

}// EncodedLength::addToLengthString


/*!
 * Collapse a length string as best we can by summing terms
 * \param totalLength is the existing length string.
 * \param keepZero should be true keep "0" in the output
 * \return an equivalent collapsed string
 */
QString EncodedLength::collapseLengthString(QString totalLength, bool keepZero)
{
    if(totalLength.contains("(") || totalLength.contains(")"))
        return totalLength;

    // Split according to the pluses
    QStringList list = totalLength.split("+");
    QStringList others;

    // Some of these groups are simple numbers, some of them are not. We are going to re-order to put the numbers together
    int number = 0;
    for(int i = 0; i < list.size(); i++)
    {
        if(list.at(i).isEmpty())
            continue;

        if(isNumber(list.at(i)))
        {
            number += list.at(i).toInt();
        }
        else
        {
            others.append(list.at(i));
        }

    }// for all the fragments

    totalLength = "";

    // Add the others into the string first
    for(int i = 0; i < others.size(); i++)
    {
        if(totalLength.isEmpty())
            totalLength = others.at(i);
        else
            totalLength += "+" + others.at(i);
    }

    // These are the numbers we could directly add
    if((number != 0) || keepZero)
    {
        if(totalLength.isEmpty())
            totalLength = QString().setNum(number);
        else
            totalLength += "+" + QString().setNum(number);
    }

    return totalLength;

}// EncodedLength::collapseLengthString


/*!
 * Determine if a segment of text contains only decimal digits
 * \param text is the text to check
 * \return true if only decimal digits, else false
 */
bool EncodedLength::isNumber(const QString& text)
{
    if(text.size() <= 0)
        return false;

    for(int i = 0; i < text.size(); i++)
    {
        // Minus signs are OK
        if(!text.at(i).isDigit() && (text.at(i) != '-'))
            return false;
    }

    return true;
}
