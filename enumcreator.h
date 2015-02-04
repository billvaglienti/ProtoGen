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
    QString getMarkdown(QString indent) const;

    QString name;       //!< The name of the enumeration
    QString comment;    //!< The comment of the enumeration
    QString output;     //!< The header file output string of the enumeration

protected:
    QStringList nameList;
    QStringList commentList;
    QStringList valueList;


};

#endif // ENUMCREATOR_H
