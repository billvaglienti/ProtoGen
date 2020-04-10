#ifndef ENUMCREATOR_H
#define ENUMCREATOR_H

#include <QString>
#include <QStringList>
#include <QDomElement>
#include "protocolsupport.h"
#include "protocoldocumentation.h"

// Forward declaration of EnumCreator so EnumElement can see it
class EnumCreator;

class EnumElement : public ProtocolDocumentation
{
    // So EnumCreator can access my protected members
    friend class EnumCreator;

public:

    //! Cronstruct an enumeration element
    EnumElement(ProtocolParser* parse, EnumCreator* creator, QString Parent, ProtocolSupport supported);

    //! Parse an enumeration element
    virtual void parse() Q_DECL_OVERRIDE;

    //! Check the enumeration element against C keywords
    virtual void checkAgainstKeywords() Q_DECL_OVERRIDE;

    //! Return true if this element is hidden from the documentation
    virtual bool isHidden (void) const Q_DECL_OVERRIDE {return hidden;}

    //! The enumeration element name, with or without prefix
    QString getName() const;

    //! The enumeration element lookup name
    QString getLookupName() const;

    //! The enumeration element declaration string
    QString getDeclaration() const;

protected:

    //! The name returned
    QString lookupName;

    //! The numeration value string
    QString value;

    //! The numeration number (if it can be computed)
    QString number;

    //! Indicates if this enumeration element is hidden from documentation
    bool hidden;

    //! Indicates if this enumeration element does not use the enumeration prefix
    bool ignoresPrefix;

    //! Indicates if this enumeration element does not produce lookup code
    bool ignoresLookup;

    //! Pointer to the parent enumeration that owns this element
    EnumCreator* parentEnum;

};

class EnumCreator : public ProtocolDocumentation
{
public:
    //! Construct the enumeration object
    EnumCreator(ProtocolParser* parse, QString Parent, ProtocolSupport supported);

    ~EnumCreator(void);

    //! Empty the enumeration list
    void clear(void);

    //! Parse the DOM to fill out the enumeration list
    virtual void parse() Q_DECL_OVERRIDE;

    //! Parse the DOM to fill out the enumeration list for a global enum
    void parseGlobal(void);

    //! Check names against the list of C keywords
    virtual void checkAgainstKeywords(void) Q_DECL_OVERRIDE;

    //! Get the markdown documentation for this enumeration
    virtual QString getTopLevelMarkdown(bool global = false, const QStringList& ids = QStringList()) const Q_DECL_OVERRIDE;

    //! Return the enumeration name
    QString getName(void) const {return name;}

    //! Return the enumeration comment
    QString getComment(void) const {return comment;}

    //! Return the header file output string
    QString getOutput(void) const {return output;}

    QString getPrefix() const { return prefix; }

    //! Return the source file output string
    QString getSourceOutput(void) const { return sourceOutput; }

    //! Replace any text that matches an enumeration name with the value of that enumeration
    QString& replaceEnumerationNameWithValue(QString& text) const;

    //! Find the enumeration value with this name and return its comment, or an empty string
    QString getEnumerationValueComment(const QString& name) const;

    //! Determine if text is an enumeration name
    bool isEnumerationValue(const QString& text) const;

    //! Return the minimum number of bits needed to encode the enumeration
    int getMinBitWidth(void) const {return minbitwidth;}

    //! Return true if this enumeration is hidden from the documentation
    virtual bool isHidden (void) const Q_DECL_OVERRIDE {return hidden;}

    //! The hierarchical name of this object
    virtual QString getHierarchicalName(void) const Q_DECL_OVERRIDE {return parent + ":" + name;}

    //! Get the name of the header file (if any) holding this enumeration
    QString getHeaderFileName(void) const {return file;}

    //! Get the path of the header file (if any) holding this enumeration
    QString getHeaderFilePath(void) const {return filepath;}

protected:

    //! Parse the enumeration values to build the number list
    void computeNumberList(void);

    //! Split string around math operators
    QStringList splitAroundMathOperators(QString text) const;

    //! Determine if a character is a math operator
    bool isMathOperator(QChar op) const;

    //! Output file for global enumerations
    QString file;

    //! Output file path for global enumerations
    QString filepath;

    //! Output file for source code file (may not be used)
    QString sourceOutput;

    //! List of all the enumerated values
    QList<EnumElement> elements;

    //! A longer description is possible for enums (will be displayed in the documentation)
    QString description;

    //! A prefix can be specified for each element in the enum
    QString prefix;

    //! The header file output string of the enumeration
    QString output;

    //! Minimum number of bits needed to encode the enumeration
    int minbitwidth;

    //! Determines if this enum will be hidden from the documentation
    bool hidden;

    //! Determines if this enumeration will support revese-lookup of label (based on value)
    bool lookup;

    //! Determines if this enumeration will support revese-lookup of title (based on value)
    bool lookupTitle;

    //! Determines if this enumeration will support revese-lookup of comment (based on value)
    bool lookupComment;

    //! Flag set true if parseGlobal() is called
    bool isglobal;

    //! List of document objects
    QList<ProtocolDocumentation*> documentList;
};

//! Output a string with specific spacing
QString spacedString(QString text, int spacing);

#endif // ENUMCREATOR_H
