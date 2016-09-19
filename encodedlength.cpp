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
 * Determine if the length is zero
 * \return true if the maximum length is empty or is "0"
 */
bool EncodedLength::isZeroLength(void)
{
    if(collapseLengthString(maxEncodedLength, true).compare("0") == 0)
        return true;
    else
        return false;
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
 * \param array2d is the 2nd dimension array length, which can be empty.
 */
void EncodedLength::addToLength(const EncodedLength& rightLength, const QString& array, bool isVariable, bool isDependent, const QString& array2d)
{
    addToLengthString(maxEncodedLength, rightLength.maxEncodedLength, array, array2d);
    addToLengthString(nonDefaultEncodedLength, rightLength.nonDefaultEncodedLength, array, array2d);

    // If not variable or dependent, then add to minimum length
    if(!isVariable && !isDependent)
        addToLengthString(minEncodedLength, rightLength.minEncodedLength, array, array2d);
}


/*!
 * Add a grouping of length strings
 * \param leftLength is the group that is incremented, can be NULL in which case this function does nothing.
 * \param rightLength is the length strings to add.
 * \param array is the array length, which can be empty.
 * \param isVariable is true if this length is for a variable length array.
 * \param isDependent is true if this length is for a field whose presence depends on another field.
 * \param array2d is the 2nd dimension array length, which can be empty.
 */
void EncodedLength::add(EncodedLength* leftLength, const EncodedLength& rightLength, const QString& array, bool isVariable, bool isDependent, const QString& array2d)
{
    if(leftLength != NULL)
        leftLength->addToLength(rightLength, array, isVariable, isDependent, array2d);
}


/*!
 * Create a length string like "4 + 3 + N3D*2" by adding successive length strings
 * \param totalLength is the total length string
 * \param length is the new length to add
 * \param array is the number of times to add it
 */
void EncodedLength::addToLengthString(QString & totalLength, QString length, QString array, QString array2d)
{
    bool ok;
    double number;
    int inumber;

    if(length.isEmpty())
        return;

    // Its possible that length represents something like 24*(6),
    // which we can resolve easily, so lets try
    number = ShuntingYard::computeInfix(length, &ok);
    if(ok)
    {
        // round to nearest integer
        if(number >= 0)
            inumber = (int)(number + 0.5);
        else
            inumber = (int)(number - 0.5);

        if(inumber == 0)
            return;

        length = QString().setNum(inumber);
    }


    if(!array.isEmpty() && array != "1")
    {
        // How we handle the array multiplier depends on the contents of length
        // We only expect length to have +, -, or * operators. If there are no
        // operators, or the only operators are multipliers, then we don't need
        // the parenthesis. Otherwise we do need them.

        if(array2d.isEmpty() || (array2d == "1"))
        {
            if(length.contains("+") || length.contains("-") || length.contains("/"))
                length = array + "*(" + length + ")";
            else
                length = array + "*" + length;
        }
        else
        {
            if(length.contains("+") || length.contains("-") || length.contains("/"))
                length = array + "*" + array2d + "*(" + length + ")";
            else
                length = array + "*" + array2d + "*" + length;
        }
    }


    if(totalLength.isEmpty())
        totalLength = length;
    else
    {
        // Add them up
        totalLength = collapseLengthString(totalLength + "+" + length);

        /*
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
        */

    }// if totalLength previously had data

}// EncodedLength::addToLengthString


/*!
 * Collapse a length string as best we can by summing terms
 * \param totalLength is the existing length string.
 * \param keepZero should be true keep "0" in the output.
 * \param minusOne should be true to subtract 1 from the output.
 * \return an equivalent collapsed string
 */
QString EncodedLength::collapseLengthString(QString totalLength, bool keepZero, bool minusOne)
{
    int number = 0;

    // It might be that we can compute a value directly, give it a
    // try and see if we can save all the later effort
    bool ok;
    double dnumber = ShuntingYard::computeInfix(totalLength, &ok);
    if(ok)
    {
        // round to nearest integer
        if(dnumber >= 0)
            number = (int)(dnumber + 0.5);
        else
            number = (int)(dnumber - 0.5);

        // Handle the minus one here
        if(minusOne)
            number--;

        return QString().setNum(number);
    }

    // We don't properly support collapsing strings with parenthesis
    /// TODO: support parenthesis using recursion
    if(totalLength.contains("(") || totalLength.contains(")"))
    {
        if(minusOne)
            return totalLength + "-1";
        else
            return totalLength;
    }

    // Split according to the pluses
    QStringList list = totalLength.split("+");
    QStringList others;

    // Some of these groups are simple numbers, some of them are not. We are going to re-order to put the numbers together
    number = 0;
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

    // Handle the minus one here
    if(minusOne)
        number--;

    QList<QStringList> pairlist;
    for(int i = 0; i < others.size(); i++)
    {
        // Separate a string like "1*N3D" into "1" and "N3D"
        pairlist.append(others.at(i).split("*"));
    }

    others.clear();
    QString output;
    for(int i = 0; i < pairlist.size(); i++)
    {
        // Each entry should have two elements and the first should be a number,
        // if not then something weird happened, just put it back the way it was
        if((pairlist.at(i).size() != 2) || !isNumber(pairlist.at(i).at(0)))
        {
            if(pairlist.at(i).size() != 0)
            {
                if(!output.isEmpty())
                    output += "+";

                output += pairlist.at(i).join("*");
            }
            continue;
        }

        int counter = pairlist.at(i).at(0).toInt();

        // Now go through all entries after this one to find anyone that is common
        for(int j = i + 1; j < pairlist.size(); j++)
        {
            // if its not a number, then we can't do anything with it,
            // we'll catch it in the outer loop when we get to it
            if((pairlist.at(j).size() != 2) || !isNumber(pairlist.at(j).at(0)))
                continue;

            // If the comparison is exactly the same, then we can add this one up
            if(pairlist.at(i).at(1).compare(pairlist.at(j).at(1)) == 0)
            {
                // These two are multiples of the same thing
                counter += pairlist.at(j).at(0).toInt();

                // Clear this element, no contribution in the future.
                pairlist[j].clear();
            }
        }

        // finally include this in the output
        if(counter != 0)
        {
            if(!output.isEmpty())
                output += "+";

            output += QString().setNum(counter) + "*" + pairlist.at(i).at(1);
        }
    }

    // A negative number outputs the "-" by default
    if(number < 0)
        output += QString().setNum(number);
    else
    {
        if(number != 0)
        {
            if(output.isEmpty())
                output = QString().setNum(number);
            else
                output += "+" + QString().setNum(number);
        }
    }

    if((keepZero) && output.isEmpty())
        output = "0";

    // It might be that we can compute a value now, give it a try
    dnumber = ShuntingYard::computeInfix(output, &ok);
    if(ok)
    {
        // round to nearest integer
        if(dnumber >= 0)
            number = (int)(dnumber + 0.5);
        else
            number = (int)(dnumber - 0.5);

        output = QString().setNum(number);
    }

    return output;

}// EncodedLength::collapseLengthString


/*!
* Subtract one from a length string
* \param totalLength is the existing length string.
* \param keepZero should be true keep "0" in the output.
* \return the string with one less
*/
QString EncodedLength::subtractOneFromLengthString(QString totalLength, bool keepZero)
{
    return EncodedLength::collapseLengthString(totalLength, keepZero, true);
}


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
