#ifndef ENUMCREATOR_H
#define ENUMCREATOR_H

#include <QString>
#include <QStringList>
#include <QDomElement>
#include "protocolsupport.h"
#include "protocoldocumentation.h"

class EnumCreator : public ProtocolDocumentation
{
public:
    //! Create an empty enumeration list
    EnumCreator(QString Parent, ProtocolSupport supported) :
        ProtocolDocumentation(Parent),
        minbitwidth(0),
        hidden(false),
        support(supported)
    {}

    ~EnumCreator(void);

    //! Empty the enumeration list
    void clear(void);

    //! Parse the DOM to fill out the enumeration list
    virtual void parse(void);

    //! Get the markdown documentation for this enumeration
    virtual QString getTopLevelMarkdown(bool global = false, const QStringList& ids = QStringList()) const;

    //! Return the enumeration name
    QString getName(void) const {return name;}

    //! Return the enumeration comment
    QString getComment(void) const {return comment;}

    //! Return the header file output string
    QString getOutput(void) const {return output;}

    //! Replace any text that matches an enumeration name with the value of that enumeration
    QString& replaceEnumerationNameWithValue(QString& text) const;

    //! Determine if text is an enumeration name
    bool isEnumerationValue(const QString& text) const;

    //! Return the minimum number of bits needed to encode the enumeration
    int getMinBitWidth(void) const {return minbitwidth;}

    //! Return true if this enumeration is hidden from the documentation
    virtual bool isHidden(void) const {return hidden;}

    //! The hierarchical name of this object
    virtual QString getHierarchicalName(void) const {return parent + ":" + name;}

protected:
    //! Parse the enumeration values to build the number list
    void computeNumberList(void);

    //! Split string around math operators
    QStringList splitAroundMathOperators(QString text) const;

    //! Determine if a character is a math operator
    bool isMathOperator(QChar op) const;

    QStringList nameList;
    QStringList commentList;
    QStringList valueList;
    QStringList numberList;
    QList<bool> hiddenList;

    QString description;//!< A longer description is possible for enums (will be displayed in the documentation)
    QString output;     //!< The header file output string of the enumeration
    int minbitwidth;    //!< Minimum number of bits needed to encode the enumeration

    bool hidden;        //!< Determines if this enum will be hidden from the documentation

    ProtocolSupport support;

    //! List of document objects
    QList<ProtocolDocumentation*> documentList;
};

//! Output a string with specific spacing
QString spacedString(QString text, int spacing);

#endif // ENUMCREATOR_H
