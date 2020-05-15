#include "encodedlength.h"
#include "shuntingyard.h"
#include "protocolsupport.h"

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
    return maxEncodedLength.empty();
}


/*!
 * Determine if the length is zero
 * \return true if the maximum length is empty or is "0"
 */
bool EncodedLength::isZeroLength(void) const
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
void EncodedLength::addToLength(const std::string & length, bool isString, bool isVariable, bool isDependent, bool isDefault)
{
    if(length.empty())
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
void EncodedLength::addToLength(const EncodedLength& rightLength, const std::string& array, bool isVariable, bool isDependent, const std::string& array2d)
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
void EncodedLength::add(EncodedLength* leftLength, const EncodedLength& rightLength, const std::string& array, bool isVariable, bool isDependent, const std::string& array2d)
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
void EncodedLength::addToLengthString(std::string & totalLength, std::string length, std::string array, std::string array2d)
{
    bool ok;
    double number;
    int inumber;

    if(length.empty())
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

        length = std::to_string(inumber);
    }


    if(!array.empty() && array != "1")
    {
        // How we handle the array multiplier depends on the contents of length
        // We only expect length to have +, -, or * operators. If there are no
        // operators, or the only operators are multipliers, then we don't need
        // the parenthesis. Otherwise we do need them.

        if(array2d.empty() || (array2d == "1"))
        {
            if(length.find_first_of("+-/") != std::string::npos)
                length = array + "*(" + length + ")";
            else
                length = array + "*" + length;
        }
        else
        {
            if(length.find_first_of("+-/") != std::string::npos)
                length = array + "*" + array2d + "*(" + length + ")";
            else
                length = array + "*" + array2d + "*" + length;
        }
    }


    if(totalLength.empty())
        totalLength = length;
    else
    {
        // Add them up
        totalLength = collapseLengthString(totalLength + "+" + length);

    }// if totalLength previously had data

}// EncodedLength::addToLengthString


/*!
 * Collapse a length string as best we can by summing terms
 * \param totalLength is the existing length string.
 * \param keepZero should be true keep "0" in the output.
 * \param minusOne should be true to subtract 1 from the output.
 * \return an equivalent collapsed string
 */
std::string EncodedLength::collapseLengthString(std::string totalLength, bool keepZero, bool minusOne)
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

        return std::to_string(number);
    }

    // We don't properly support collapsing strings with parenthesis
    /// TODO: support parenthesis using recursion
    if(totalLength.find_first_of("()") != std::string::npos)
    {
        if(minusOne)
            return totalLength + "-1";
        else
            return totalLength;
    }

    // Split according to the pluses
    std::vector<std::string> list = split(totalLength, "+");
    std::vector<std::string> others;

    // Some of these groups are simple numbers, some of them are not. We are going to re-order to put the numbers together
    number = 0;
    for(std::size_t i = 0; i < list.size(); i++)
    {
        if(list.at(i).empty())
            continue;

        if(isNumber(list.at(i)))
        {
            number += std::stoi(list.at(i));
        }
        else
        {
            others.push_back(list.at(i));
        }

    }// for all the fragments

    // Handle the minus one here
    if(minusOne)
        number--;

    std::vector<std::vector<std::string>> pairlist;
    for(std::size_t i = 0; i < others.size(); i++)
    {
        // Separate a string like "1*N3D" into "1" and "N3D"
        pairlist.push_back(split(others.at(i), "*"));
    }

    others.clear();
    std::string output;
    for(std::size_t i = 0; i < pairlist.size(); i++)
    {
        // Each entry should have two elements and the first should be a number,
        // if not then something weird happened, just put it back the way it was
        if((pairlist.at(i).size() != 2) || !isNumber(pairlist.at(i).at(0)))
        {
            if(pairlist.at(i).size() != 0)
            {
                if(!output.empty())
                    output += "+";

                output += join(pairlist.at(i), "*");
            }
            continue;
        }

        int counter = std::stoi(pairlist.at(i).at(0));

        // Now go through all entries after this one to find anyone that is common
        for(std::size_t j = i + 1; j < pairlist.size(); j++)
        {
            // if its not a number, then we can't do anything with it,
            // we'll catch it in the outer loop when we get to it
            if((pairlist.at(j).size() != 2) || !isNumber(pairlist.at(j).at(0)))
                continue;

            // If the comparison is exactly the same, then we can add this one up
            if(pairlist.at(i).at(1) == pairlist.at(j).at(1))
            {
                // These two are multiples of the same thing
                counter += std::stoi(pairlist.at(j).at(0));

                // Clear this element, no contribution in the future.
                pairlist[j].clear();
            }
        }

        // finally include this in the output
        if(counter != 0)
        {
            if(!output.empty())
                output += "+";

            output += std::to_string(counter) + "*" + pairlist.at(i).at(1);
        }
    }

    // A negative number outputs the "-" by default
    if(number < 0)
        output += std::to_string(number);
    else
    {
        if(number != 0)
        {
            if(output.empty())
                output = std::to_string(number);
            else
                output += "+" + std::to_string(number);
        }
    }

    if((keepZero) && output.empty())
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

        output = std::to_string(number);
    }

    return output;

}// EncodedLength::collapseLengthString


/*!
* Subtract one from a length string
* \param totalLength is the existing length string.
* \param keepZero should be true keep "0" in the output.
* \return the string with one less
*/
std::string EncodedLength::subtractOneFromLengthString(std::string totalLength, bool keepZero)
{
    return EncodedLength::collapseLengthString(totalLength, keepZero, true);
}


/*!
 * Determine if a segment of text contains only decimal digits
 * \param text is the text to check
 * \return true if only decimal digits, else false
 */
bool EncodedLength::isNumber(const std::string& text)
{
    /// TODO not sure if I need this size check
    if(text.size() <= 0)
        return false;

    // Only decimal digits and minus signs are OK
    return (text.find_first_not_of("-0123456789") == std::string::npos);
}
