#ifndef PROTOCOLFILE_H
#define PROTOCOLFILE_H

#include <QString>
#include <QFile>

class ProtocolFile
{
public:
    ProtocolFile(const QString& moduleName, bool temporary = true);

    ProtocolFile();

    //! Destructor that performs the actual file write
    virtual ~ProtocolFile();

    //! Write the file to disc, including any prologue/epilogue
    virtual bool flush(void);

    //! Set the contents of the file, not including any prologue/epilogue
    void clear(void);

    //! Append to the current contents of the file
    void write(const QString& text);

    //! Output a list of include directives
    void writeIncludeDirectives(const QStringList& list);

    //! Output an include directive
    void writeIncludeDirective(const QString& include, const QString& comment = QString(), bool Global = false);

    //! Set the name of the module
    void setModuleNameAndPath(QString name, QString filepath);

    //! Return the filename
    virtual QString fileName(void) const {return module;}

    //! \return true if an append operation is in progress
    bool isAppending(void) const {return appending;}

    //! Make sure one blank line at end
    void makeLineSeparator(void);

    //! Make a nice, native, relative path
    static QString sanitizePath(const QString& path);

    //! Make a specific file writable.
    static void makeFileWritable(const QString& fileName);

    //! delete a specific file
    static void deleteFile(const QString& fileName);

    //! delete both the .c and .h file
    static void deleteModule(const QString& moduleName);

    //! Rename a file from oldName to newName
    static void renameFile(const QString& oldName, const QString& newName);

    //! Copy a temporary file to the real file and delete the temporary file
    static void copyTemporaryFile(const QString& path, const QString& fileName);

    //! Make sure one blank line at end
    static void makeLineSeparator(QString& contents);

    //! The prefix used to indicate a temporary name
    static QString tempprefix;

protected:

    //! Prepare to do an append operation
    virtual void prepareToAppend(void){}

    //! Return the correct on disk name
    QString fileNameOnDisk(void) const;

    QString path;       //!< Output path for the file
    QString module;     //!< The module name, not including the file extension
    QString contents;   //!< The contents, not including the prologue or epilogue

    bool dirty;         //!< Flag set to indicate that the file contents are dirty and need to be flushed
    bool appending;     //!< Flag set if an append operation is in progress
    bool temporary;     //!< Flag to indicate this is a temporary file with "temporarydeleteme_" preceding the name
};


class ProtocolHeaderFile : public ProtocolFile
{
public:

    //! Write the file to disc, including any prologue/epilogue
    virtual bool flush(void);

    //! Return the filename
    virtual QString fileName(void) const;

protected:

    //! Prepare to do an append operation
    virtual void prepareToAppend(void);

    //! \return the text that is appended to close a header file
    QString getClosingStatement(void);
};


class ProtocolSourceFile : public ProtocolFile
{
public:

    //! Append to the prototype section of the file
    void writePrototype(const QString& text);

    //! Write the file to disc, including any prologue/epilogue
    bool flush(void);

    //! Return the filename
    virtual QString fileName(void) const;

protected:

    //! Prepare to do an append operation
    virtual void prepareToAppend(void);

    //! \return the text that is appended to close a source file
    QString getClosingStatement(void);
};


#endif // PROTOCOLFILE_H
