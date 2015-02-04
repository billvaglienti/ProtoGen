#include "encodedlength.h"

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
        EncodedLength::addToLengthString(maxEncodedLength, rightLength.maxEncodedLength);
        EncodedLength::addToLengthString(nonDefaultEncodedLength, rightLength.nonDefaultEncodedLength);

        // If not variable or dependent, then add to minimum length
        if(!isVariable && !isDependent)
            EncodedLength::addToLengthString(minEncodedLength, rightLength.minEncodedLength);
    }
    else
    {
        EncodedLength::addToLengthString(maxEncodedLength, array + "*(" + rightLength.maxEncodedLength + ")");
        EncodedLength::addToLengthString(nonDefaultEncodedLength, array + "*(" + rightLength.nonDefaultEncodedLength + ")");

        // If not variable or dependent, then add to minimum length
        if(!isVariable && !isDependent)
            EncodedLength::addToLengthString(minEncodedLength, array + "*(" + rightLength.minEncodedLength + ")");

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
void EncodedLength::addToLengthString(QString & totalLength, const QString & length)
{
    if(totalLength.isEmpty())
        totalLength = length;
    else if(!length.isEmpty())
        totalLength += " + " + length;
}
