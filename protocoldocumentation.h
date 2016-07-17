#ifndef PROTOCOLDOCUMENTATION_H
#define PROTOCOLDOCUMENTATION_H

#include <QDomElement>
#include <QString>
#include <QStringList>

class ProtocolDocumentation
{
public:

    //! Construct the document object
    ProtocolDocumentation(void);

    //! Virtual destructor
    virtual ~ProtocolDocumentation(void) {;}

    //! Set the element from the DOM
    virtual void setElement(QDomElement element) {e = element;}

    //! Parse the document from the DOM
    virtual void parse(void);

    //! Return top level markdown documentation for this packet
    virtual QString getTopLevelMarkdown(bool global = false, const QStringList& ids = QStringList()) const;

    //! Documentation is by definition never hidden from the documentation
    virtual bool isHidden(void) const {return false;}

    //! Helper function to create a list of ProtocolDocumentation objects
    static void getChildDocuments(const QDomElement& e, QList<ProtocolDocumentation*>& list);

public:
    QString name;           //!< The name of this encodable
    QString comment;        //!< The comment that goes with this encodable

protected:
    QDomElement e;          //!< The DOM element which is the source of this object's data

private:
    int outlineLevel;       //!< The paragraph outline level
    QString docfile;        //!< File for external documentation
};

#endif // PROTOCOLDOCUMENTATION_H
