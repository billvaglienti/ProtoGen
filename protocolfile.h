#ifndef PROTOCOLFILE_H
#define PROTOCOLFILE_H

#include "protocolsupport.h"
#include <vector>
#include <string>
#include <cstdio>

class ProtocolFile
{
public:
    //! Construct the protocol file
    ProtocolFile(const std::string& moduleName, ProtocolSupport supported, bool temporary = true);

    //! Construct the protocol file
    ProtocolFile(ProtocolSupport supported);

    //! Destructor that performs the actual file write
    virtual ~ProtocolFile();

    //! Write the file to disc, including any prologue/epilogue
    virtual bool flush(void);

    //! Set the contents of the file, not including any prologue/epilogue
    void clear(void);

    //! Append to the current contents of the file
    void write(const std::string& text);

    //! Append to the current contents of the file, if text is not already in the file
    void writeOnce(const std::string& text);

    //! Output a list of include directives
    void writeIncludeDirectives(const std::vector<std::string>& list);

    //! Output an include directive
    void writeIncludeDirective(const std::string& include, const std::string& comment = std::string(), bool global = false, bool autoextension = true);

    //! Set the name of the module
    void setModuleNameAndPath(std::string name, std::string filepath);

    //! Set the name of the module
    void setModuleNameAndPath(std::string name, std::string filepath, ProtocolSupport::LanguageType languageoverride);

    //! Set the name of the module
    void setModuleNameAndPath(std::string prefix, std::string name, std::string filepath);

    //! Set the name of the module
    void setModuleNameAndPath(std::string prefix, std::string name, std::string filepath, ProtocolSupport::LanguageType languageoverride);

    //! Return the path
    virtual std::string filePath(void) const {return path;}

    //! Return the filename
    virtual std::string fileName(void) const {return module+extension;}

    //! Return the module name
    std::string moduleName(void) const {return module;}

    //! \return true if an append operation is in progress
    bool isAppending(void) const {return appending;}

    //! Make sure one blank line at end
    void makeLineSeparator(void);

    //! Adjust the name and path so that all the path information is in the path
    static void separateModuleNameAndPath(std::string& name, std::string& filepath);

    //! Extract the extension from a name
    static void extractExtension(std::string& name, std::string& extension);

    //! Make a nice, native, relative path
    static std::string sanitizePath(const std::string& path);

    //! Make a specific file writable.
    static void makeFileWritable(const std::string& fileName);

    //! delete a specific file
    static void deleteFile(const std::string& fileName);

    //! delete both the .c and .h file
    static void deleteModule(const std::string& moduleName);

    //! Rename a file from oldName to newName
    static void renameFile(const std::string& oldName, const std::string& newName);

    //! Copy a temporary file to the real file and delete the temporary file
    static void copyTemporaryFile(const std::string& path, const std::string& fileName);

    //! Make sure one blank line at end
    static void makeLineSeparator(std::string& contents);

    //! The prefix used to indicate a temporary name
    static std::string tempprefix;

protected:

    //! Prepare to do an append operation
    virtual void prepareToAppend(void) {}

    //! Get the extension information for this name
    virtual void extractExtension(std::string& name);

    //! Return the correct on disk name
    std::string fileNameAndPathOnDisk(void) const;

    ProtocolSupport support;//!< Protocol wide support details
    std::string extension;      //!< The file extension
    std::string path;           //!< Output path for the file
    std::string module;         //!< The module name, not including the file extension
    std::string contents;       //!< The contents, not including the prologue or epilogue

    bool dirty;             //!< Flag set to indicate that the file contents are dirty and need to be flushed
    bool appending;         //!< Flag set if an append operation is in progress
    bool temporary;         //!< Flag to indicate this is a temporary file with "temporarydeleteme_" preceding the name
};


class ProtocolHeaderFile : public ProtocolFile
{
public:

    //! Construct the protocol header file
    ProtocolHeaderFile(ProtocolSupport supported) : ProtocolFile(supported){}

    //! Write the file to disc, including any prologue/epilogue
    bool flush(void) override;

    //! Write a comment for the entire file in the \file block
    void setFileComment(const std::string& comment);

protected:

    //! Prepare to do an append operation
    void prepareToAppend(void) override;

    //! Get the extension information for this name
    void extractExtension(std::string& name) override;

    //! \return the text that is appended to close a header file
    std::string getClosingStatement(void);
};


class ProtocolSourceFile : public ProtocolFile
{
public:

    //! Construct the protocol header file
    ProtocolSourceFile(ProtocolSupport supported) : ProtocolFile(supported){}

    //! Write the file to disc, including any prologue/epilogue
    bool flush(void) override;

protected:

    //! Prepare to do an append operation
    void prepareToAppend(void) override;

    //! Get the extension information for this name
    void extractExtension(std::string& name) override;

    //! \return the text that is appended to close a source file
    std::string getClosingStatement(void);
};

#endif // PROTOCOLFILE_H
