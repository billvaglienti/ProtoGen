#ifndef ENUMCREATOR_H
#define ENUMCREATOR_H

#include <QString>
#include <QStringList>
#include <QDomElement>

class EnumCreator
{
public:
    //! Create an empty enumeration list
    EnumCreator(void){}

    //! Create an enumeration list and populate it from the DOM
    EnumCreator(const QDomElement& e);

    //! Empty the enumeration list
    void clear(void);

    //! Parse the DOM to fill out the enumeration list
    QString parse(const QDomElement& e);

    //! Get the markdown documentation for this enumeration
    QString getMarkdown(QString outline) const;

    //! Return the enumeration name
    QString getName(void) const {return name;}

    //! Return the enumeration comment
    QString getComment(void) const {return comment;}

    //! Return the header file output string
    QString getOutput(void) const {return output;}

    //! Return the minimum number of bits needed to encode the enumeration
    int getMinBitWidth(void) const {return minbitwidth;}

protected:
    //! Parse the enumeration values to build the number list
    void computeNumberList(void);

    QStringList nameList;
    QStringList commentList;
    QStringList valueList;
    QStringList numberList;

    QString name;       //!< The name of the enumeration
    QString comment;    //!< The comment of the enumeration
    QString output;     //!< The header file output string of the enumeration
    int minbitwidth;    //!< Minimum number of bits needed to encode the enumeration


};

//! Output a string with specific spacing
QString spacedString(QString text, int spacing, bool code = false);

#endif // ENUMCREATOR_H
