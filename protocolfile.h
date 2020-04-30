#ifndef PROTOCOLFILE_H
#define PROTOCOLFILE_H

#include "protocolsupport.h"
#include <QString>
#include <QFile>

class ProtocolFile
{
public:
    //! Construct the protocol file
    ProtocolFile(const QString& moduleName, bool temporary = true);

    //! Construct the protocol file
    ProtocolFile(void);

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
    void writeIncludeDirective(const QString& include, const QString& comment = QString(), bool global = false, bool autoextension = true);

    //! Set the license text for the file
    void setLicenseText(const QString text) { license = text; }

    //! Set the name of the module
    void setModuleNameAndPath(QString name, QString filepath, ProtocolSupport::LanguageType lang);

    //! Set the name of the module
    void setModuleNameAndPath(QString prefix, QString name, QString filepath, ProtocolSupport::LanguageType lang);

    //! Set the language of this file
    void setLanguage(ProtocolSupport::LanguageType lang) {language = lang;}

    //! Return the path
    virtual QString filePath(void) const {return path;}

    //! Return the filename
    virtual QString fileName(void) const {return module+extension;}

    //! Return the module name
    QString moduleName(void) const {return module;}

    //! \return true if an append operation is in progress
    bool isAppending(void) const {return appending;}

    //! Make sure one blank line at end
    void makeLineSeparator(void);

    //! Adjust the name and path so that all the path information is in the path
    static void separateModuleNameAndPath(QString& name, QString& filepath);

    //! Extract the extension from a name
    static void extractExtension(QString& name, QString& extension);

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
    virtual void prepareToAppend(void) {}

    //! Get the extension information for this name
    virtual void extractExtension(QString& name);

    //! Return the correct on disk name
    QString fileNameAndPathOnDisk(void) const;

    QString extension;  //!< The file extension
    QString path;       //!< Output path for the file
    QString module;     //!< The module name, not including the file extension
    QString contents;   //!< The contents, not including the prologue or epilogue

    QString license;    //!< License text

    bool dirty;         //!< Flag set to indicate that the file contents are dirty and need to be flushed
    bool appending;     //!< Flag set if an append operation is in progress
    bool temporary;     //!< Flag to indicate this is a temporary file with "temporarydeleteme_" preceding the name

    //! The language this file is intended for
    ProtocolSupport::LanguageType language;
};


class ProtocolHeaderFile : public ProtocolFile
{
public:

    //! Write the file to disc, including any prologue/epilogue
    virtual bool flush(void) Q_DECL_OVERRIDE;

protected:

    //! Prepare to do an append operation
    virtual void prepareToAppend(void) Q_DECL_OVERRIDE;

    //! Get the extension information for this name
    virtual void extractExtension(QString& name) Q_DECL_OVERRIDE;

    //! \return the text that is appended to close a header file
    QString getClosingStatement(void);
};


class ProtocolSourceFile : public ProtocolFile
{
public:

    //! Write the file to disc, including any prologue/epilogue
    bool flush(void) Q_DECL_OVERRIDE;

protected:

    //! Prepare to do an append operation
    virtual void prepareToAppend(void) Q_DECL_OVERRIDE;

    //! Get the extension information for this name
    virtual void extractExtension(QString& name) Q_DECL_OVERRIDE;

    //! \return the text that is appended to close a source file
    QString getClosingStatement(void);
};

#endif // PROTOCOLFILE_H
