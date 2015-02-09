#ifndef PROTOCOLFILE_H
#define PROTOCOLFILE_H

#include <QString>

class ProtocolFile
{
public:
    ProtocolFile(const QString& moduleName, bool onlyversion = false);

    ProtocolFile();

    //! Set the contents of the file, not including any prologue/epilogue
    void clear(void);

    //! Append to the current contents of the file
    void write(const QString& text);

    //! Output an include directive
    void writeIncludeDirective(const QString& include, const QString& comment = QString());

    //! Set the name of the module
    void setModuleName(const QString& name);

    //! Set the versionOnly flag, which controls if version and date or just version are output
    void setVersionOnly(bool onlyversion) {versionOnly = onlyversion;}

    //! Return the filename
    virtual QString fileName(void) const = 0;

    //! \return true if an append operation is in progress
    bool isAppending(void){return appending;}

    //! Make sure one blank line at end
    void makeLineSeparator(void);

    //! delete a specific file
    static void deleteFile(const QString& fileName);

    //! delte both the .c and .h file
    static void deleteModule(const QString& moduleName);

    //! Make sure one blank line at end
    static void makeLineSeparator(QString& contents);

protected:
    QString module;     //!< The module name, not including the file extension
    QString contents;   //!< The contents, not including the prologue or epilogue
    QString prototypeContents;  //!< Contents that go before the main body of the file

    bool dirty;         //!< Flag set to indicate that the file contents are dirty and need to be flushed
    bool appending;     //!< Flag set if an append operation is in progress
    bool versionOnly;   //!< Flag to limit protogen notice to use only the version, not the date and time
};


class ProtocolHeaderFile : public ProtocolFile
{
public:

    //! Destroy the protocol file,
    ~ProtocolHeaderFile();

    //! Write the file to disc, including any prologue/epilogue
    bool flush(void);

    //! Return the filename
    virtual QString fileName(void) const;

    //! Prepare to do an append operation
    void prepareToAppend(void);

protected:
    //! \return the text that is appended to close a header file
    QString getClosingStatement(void);
};


class ProtocolSourceFile : public ProtocolFile
{
public:

    //! Destroy the protocol file,
    ~ProtocolSourceFile();

    //! Append to the prototype section of the file
    void writePrototype(const QString& text);

    //! Write the file to disc, including any prologue/epilogue
    bool flush(void);

    //! Return the filename
    virtual QString fileName(void) const;

    //! Prepare to do an append operation
    void prepareToAppend(void);

protected:

    //! \return the text that is appended to close a source file
    QString getClosingStatement(void);
};


#endif // PROTOCOLFILE_H
