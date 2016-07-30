#ifndef PROTOCOLDOCUMENTATION_H
#define PROTOCOLDOCUMENTATION_H

#include <QDomElement>
#include <QString>
#include <QStringList>

class ProtocolParser;

class ProtocolDocumentation
{
public:

    //! Construct the document object
    ProtocolDocumentation(ProtocolParser* parse, QString Parent);

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

    //! The hierarchical name of this object
    virtual QString getHierarchicalName(void) const {return parent + ":" + name;}

    //! Output a warning
    void emitWarning(QString warning) const;

    //! Helper function to create a list of ProtocolDocumentation objects
    static void getChildDocuments(ProtocolParser* parse, QString Parent, const QDomElement& e, QList<ProtocolDocumentation*>& list);

public:
    QString name;           //!< The name of this encodable
    QString comment;        //!< The comment that goes with this encodable

protected:
    ProtocolParser* parser; //!< The parser object
    QString parent;         //!< The parent name of this encodable
    QDomElement e;          //!< The DOM element which is the source of this object's data

private:
    int outlineLevel;       //!< The paragraph outline level
    QString docfile;        //!< File for external documentation
};

#endif // PROTOCOLDOCUMENTATION_H
