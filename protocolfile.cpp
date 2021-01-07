#include "protocolfile.h"
#include "protocolparser.h"
#include <fstream>
#include <filesystem>

std::string ProtocolFile::tempprefix = "temporarydeleteme_";

/*!
 * Create the file object
 * \param moduleName is the name of the file, not counting any extension
 * \param supported are the Protocol-wide options.
 * \param temp should be true for this file to be a temp file
 */
ProtocolFile::ProtocolFile(const std::string& moduleName, ProtocolSupport supported, bool temp) :
    support(supported),
    module(moduleName),
    dirty(false),
    appending(false),
    temporary(temp)
{
}


/*!
 * Create the file object. After this constructor you must call setModuleNameAndPath()
 * or a file will not be created
 * \param supported are the Protocol-wide options.
 */
ProtocolFile::ProtocolFile(ProtocolSupport supported) :
    support(supported),
    dirty(false),
    appending(false),
    temporary(true)
{
}


void ProtocolFile::setModuleNameAndPath(std::string name, std::string filepath)
{
    setModuleNameAndPath(std::string(), name, filepath, support.language);
}

void ProtocolFile::setModuleNameAndPath(std::string name, std::string filepath, ProtocolSupport::LanguageType languageoverride)
{
    setModuleNameAndPath(std::string(), name, filepath, languageoverride);
}

void ProtocolFile::setModuleNameAndPath(std::string prefix, std::string name, std::string filepath)
{
    setModuleNameAndPath(prefix, name, filepath, support.language);
}

void ProtocolFile::setModuleNameAndPath(std::string prefix, std::string name, std::string filepath, ProtocolSupport::LanguageType languageoverride)
{
    // Remove any contents we currently have
    clear();

    support.language = languageoverride;

    // Clean it all up
    separateModuleNameAndPath(name, filepath);

    // Extract the extension
    extractExtension(name);

    // Remember the module name
    module = prefix + name;

    // And the path
    path = filepath;

    // This will see if the file already exists and will setup the initial output
    prepareToAppend();
}


/*!
 * Get the extension information for this name, and remove it from the name.
 * \param name has its extension (if any) recorded and removed
 */
void ProtocolFile::extractExtension(std::string& name)
{
    extractExtension(name, extension);
}


/*!
 * Get the extension information for this name, and remove it from the name.
 * \param name has its extension (if any) removed
 * \param extension receives the extension
 */
void ProtocolFile::extractExtension(std::string& name, std::string& extension)
{
    // Note that "." as the first character is not an extension, it indicates a hidden file
    std::size_t index = name.rfind(".");
    if((index >= 1) && (index < name.size()))
    {
        // The extension, including the "."
        extension = name.substr(index);

        // The name without the extension
        name = name.erase(index);
    }
    else
        extension.clear();

}// ProtocolFile::extractExtension


/*!
 * Given a module name and path adjust the name and path so that all the path
 * information is in the path, the base name is in the name
 * \param name contains the name with may include some path information. The
 *        path and extension will be removed
 * \param filepath contains the path information, which will be augmented
 *        with any path information from the name, unless the name contains
 *        absolute path information, in which case the filepath will be
 *        replaced with the name path
 */
void ProtocolFile::separateModuleNameAndPath(std::string& name, std::string& filepath)
{
    // Handle the case where the file includes "./" to reference the current working
    // directory. We just remove this as its not needed.
    if((name.find("./") == 0) || (name.find(".\\") == 0))
        name.erase(0, 2);

    // We use this to get any path data from the name
    std::filesystem::path namepath(name);

    // Remove the path from the name and add it to the file path
    if(namepath.has_parent_path())
    {
        filepath += std::filesystem::path::preferred_separator;
        filepath += namepath.parent_path().string();
        name = namepath.filename().string();
    }

    // Make sure the path uses native separators and ends with a separator (unless its empty)
    filepath = sanitizePath(filepath);

}// ProtocolFile::separateModuleNameAndPath


/*!
 * Clear the contents of the file. This will also mark the file as clean
 */
void ProtocolFile::clear()
{
    contents.clear();
    dirty = false;
    appending = false;
}


/*!
 * Append to the contents of the file, not including any prologue/epilogue. This will
 * mark the file as dirty, which will cause it to be flushed to disk on destruction
 * \param text is the information to append
 */
void ProtocolFile::write(const std::string& text)
{
    contents += text;
    dirty = true;
}


/*!
 * Append to the contents of the file, not including any prologue/epilogue.
 * This will mark the file as dirty, which will cause it to be flushed to disk
 * on destruction. The append will only take place if `text` does not already
 * appear in the file contents.
 * \param text is the information to append
 */
void ProtocolFile::writeOnce(const std::string& text)
{
    if(contents.find(text) >= contents.size())
    {
        write(text);
    }
}


/*!
 * Output multiple include directives. The include directives are all done
 * using quotes, not global brackes
 * \param list is the list of module names that need to be included
 */
void ProtocolFile::writeIncludeDirectives(const std::vector<std::string>& list)
{
    for(std::size_t i = 0; i < list.size(); i++)
        writeIncludeDirective(list.at(i));
}


/*!
 * Output an include directive, which looks like "#include filename.h\n". You
 * can pass the entire include directive or just the module name. The include
 * directive will not be output if this file already contains this directive.
 * \param include is the module name to include
 * \param comment is a trailing comment for the include directive, can be empty
 * \param global should be true to use brackets ("<>") instead of quotes.
 * \param autoextensions should be true to automatically append ".h" or ".hpp" to the
 *        include name if it is not already included
 */
void ProtocolFile::writeIncludeDirective(const std::string& include, const std::string& comment, bool global, bool autoextension)
{
    if(include.empty())
        return;

    std::string directive = trimm(include);

    // Technically things other than .h* could be included, but not by ProtoGen
    if(!contains(directive, ".h") && autoextension)
    {
        if(support.language == ProtocolSupport::cpp_language)
            directive += ".hpp";
        else
            directive += ".h";
    }

    // Don't include ourselves
    if(directive == fileName())
        return;

    // Build the include directive with quotes or brackets based on the global status
    if (global == false)
        directive = "#include \"" + directive + "\"";
    else
        directive = "#include <" + directive + ">";

    // See if this include directive is already present, in which case we don't need to add it again
    if(contains(contents, directive, true))
        return;

    // Add the comment if there is one
    if(comment.empty())
        directive += "\n";
    else
        directive += "\t// " + comment + "\n";

    // We try to group all the #includes together
    std::size_t index = contents.rfind("#include");
    if(index < contents.size())
    {
        // Find the end of the line
        index = contents.find("\n", index);
        if(index >= 0)
        {
            contents.insert(index+1, directive);
            dirty = true;
            return;
        }
    }

    // If we get here there were no #includes in the file, this is the first
    // one; which we put at the current end of the file
    makeLineSeparator();
    write(directive);

}// ProtocolFile::writeIncludeDirective


/*!
 * Make sure the file data end such that there is exactly one blank line
 * between the current contents and anything that is added after this function
 */
void ProtocolFile::makeLineSeparator(void)
{
    makeLineSeparator(contents);
    dirty = true;
}


/*!
 * Make sure the string data end such that there is exactly one blank line
 * between the current contents and anything that is added after this function
 * \param contents is the string whose ending is adjusted
 */
void ProtocolFile::makeLineSeparator(std::string& contents)
{
    // Make sure that there are exactly two line terminators at the end of the string
    int last = ((int)contents.size()) - 1;
    int lastChar = last;

    if(last < 0)
        return;

    // Scroll backwards until we have cleared the linefeeds and lastChar is pointing at the last non-linefeed character
    while(lastChar >= 0)
    {
        if(contents.at(lastChar) == '\n')
            lastChar--;
        else
            break;
    }

    // Three cases:
    // 1) Too many linefeeds, chop some off
    // 2) Just the right amount of line feeds, change nothing
    // 3) Too few linefeeds, add some
    if(last > lastChar + 2)
        contents.erase(lastChar + 3);
    else
    {
        while(last < lastChar + 2)
        {
            contents += "\n";
            last++;
        }
    }
}


/*!
 * Make a nice, native, relative path with a trailing directory separator
 * \param path is the path to sanitize
 * \return the sanitized path
 */
std::string ProtocolFile::sanitizePath(const std::string& path)
{
    std::error_code ec;

    // Empty paths are the simplest
    if(path.empty())
        return path;

    // Absolute and relative versions of path
    std::string absolute = std::filesystem::absolute(path, ec).string();
    std::string relative = std::filesystem::relative(path, ec).string();

    // Error checking, the path creation could fail, especially for relative paths that don't exist
    if(absolute.empty())
        absolute = path;

    if(relative.empty())
        relative = path;

    // Make sure it has a trailing separator
    if(!absolute.empty())
        absolute += std::filesystem::path::preferred_separator;

    // "." is the current working directory, same as an empty path
    if(relative == ".")
        relative.clear();
    else if(!relative.empty())
        relative += std::filesystem::path::preferred_separator;

    // Return the shorter of the two paths
    if(relative.size() > absolute.size())
        return absolute;
    else
        return relative;
}


/*!
 * delete a specific file. The file will be deleted even if it is read-only.
 * \param fileName identifies the file relative to the current working directory
 */
void ProtocolFile::deleteFile(const std::string& fileName)
{
    std::error_code ec;

    makeFileWritable(fileName);
    std::filesystem::remove(fileName, ec);
}


/*!
 * Rename a file from oldName to newName. If the file newName already exists
 * it will be deleted.
 * \param oldName is the old name of the file
 * \param newName is the new name of the file
 */
void ProtocolFile::renameFile(const std::string& oldName, const std::string& newName)
{
    std::error_code ec;

    // Make sure the new file does not exist
    deleteFile(newName);

    // Now make the old name become the new name
    makeFileWritable(oldName);
    std::filesystem::rename(oldName, newName, ec);

}// ProtocolFile::renameFile


/*!
 * Make a specific file writable.
 * \param fileName identifies the file relative to the current working directory
 */
void ProtocolFile::makeFileWritable(const std::string& fileName)
{
    std::error_code ec;

    // Make sure the file has owner read and write permissiosn
    std::filesystem::permissions(fileName, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write, std::filesystem::perm_options::add, ec);
}


/*!
 * Copy a temporary file to the real file and delete the temporary file
 * \param path is the path to the files
 * \param fileName is the real file name, which does not include the temporary prefix
 */
void ProtocolFile::copyTemporaryFile(const std::string& path, const std::string& fileName)
{
    bool equal = false;
    std::string tempFileName = path + tempprefix + fileName;
    std::string permFileName = path + fileName;

    // Open the temporary file
    std::fstream tempfile(tempFileName, std::ios_base::in);

    // Its possible we already copied and deleted the file, so this isn't an error
    if(!tempfile.is_open())
        return;

    // Open the permanent file
    std::fstream permfile(permFileName, std::ios_base::in);

    // Check if the files are the same
    if(permfile.is_open())
    {
        // Real all file bytes into a string, notice parentheses to deal with "most vexing parse problem"
        std::string tempdata((std::istreambuf_iterator<char>(tempfile)), std::istreambuf_iterator<char>());
        std::string permdata((std::istreambuf_iterator<char>(permfile)), std::istreambuf_iterator<char>());

        if(tempdata == permdata)
            equal = true;

        permfile.close();
    }

    // Done with the file contents
    tempfile.close();

    if(equal)
    {
        // If the two file contents are the same, delete the temporary
        // file, leave the original file unchanged
        deleteFile(tempFileName);
    }
    else
    {
        // else if the file contents are different, delete the original
        // file and rename the temp file to be the original file
        renameFile(tempFileName, permFileName);
    }

}// ProtocolFile::copyTemporaryFile


/*!
 * delete both the .c and .h file. The files will be deleted even if they are read-only.
 * \param moduleName gives the file name without extension.
 */
void ProtocolFile::deleteModule(const std::string& moduleName)
{
    deleteFile(moduleName + ".cpp");
    deleteFile(moduleName + ".c");
    deleteFile(moduleName + ".hpp");
    deleteFile(moduleName + ".h");
}


/*!
 * Destroy the protocol file making sure to dump the contents to disk if needed
 */
ProtocolFile::~ProtocolFile()
{
    flush();
}


/*!
 * \return the the correct on disk name. This name will include the temporary
 *         prefix if needed
 */
std::string ProtocolFile::fileNameAndPathOnDisk(void) const
{
    if(temporary)
        return path + tempprefix+fileName();
    else
        return path + fileName();
}


/*!
 * Write the file to disc, including any prologue/epilogue
 * \return true if the file is written, else there is a problem opening\overwriting the file
 */
bool ProtocolFile::flush(void)
{
    // Nothing to write
    if(!dirty || contents.empty())
        return false;

    // Got to have a name
    if(module.empty())
    {
        std::cerr << "Empty module name when writing protocol file" << std::endl;
        return false;
    }

    std::error_code ec;

    // Make sure the path exists
    if(!path.empty())
        std::filesystem::create_directories(path, ec);

    // Open the file for write
    std::fstream file(fileNameAndPathOnDisk(), std::ios_base::out);

    if(!file.is_open())
    {
        std::cerr << "error: failed to open " << fileName() << std::endl;
        return false;
    }

    // The actual interesting contents
    file << contents;

    // And the file
    file.close();

    // Empty our data
    clear();

    return true;

}// ProtocolFile::flush


/*!
 * Write a comment for the entire file in the \file block. This will do nothing
 * if the file comment has already been set.
 * \param comment is the file comment, which will be reflowed to 80 characters.
 */
void ProtocolHeaderFile::setFileComment(const std::string& comment)
{
    std::string match;
    std::string filecomment;

    // This is the comment block as it is without a file comment
    match += "/*!\n";
    match += " * \\file\n";
    match += " */\n";

    // Construct the file comment
    filecomment += "/*!\n";
    filecomment += " * \\file\n";
    filecomment += ProtocolParser::reflowComment(comment, " * ", 80) + "\n";
    filecomment += " */\n";

    replaceinplace(contents, match, filecomment);

}// ProtocolHeaderFile::setFileComment


/*!
 * Get the extension information for this name, and remove it from the name.
 * \param name has its extension (if any) logged and removed
 */
void ProtocolHeaderFile::extractExtension(std::string& name)
{
    ProtocolFile::extractExtension(name);

    // A header file extension must start with ".h" (.h, .hpp, .hxx, etc.)
    if(!contains(extension, ".h"))
    {
        if(support.language == ProtocolSupport::cpp_language)
            extension = ".hpp";
        else
            extension = ".h";
    }

}// ProtocolHeaderFile::extractExtension


/*!
 * Write the file to disc, including any prologue/epilogue
 * \return true if the file is written, else there is a problem opening\overwriting the file
 */
bool ProtocolHeaderFile::flush(void)
{
    // Nothing to write
    if(!dirty || contents.empty())
        return false;

    // Got to have a name
    if(module.empty())
    {
        std::cerr << "Empty module name when writing protocol header file" << std::endl;
        return false;
    }

    std::error_code ec;

    // Make sure the path exists
    if(!path.empty())
        std::filesystem::create_directories(path, ec);

    // Open the file for write
    std::fstream file(fileNameAndPathOnDisk(), std::ios_base::out);

    if(!file.is_open())
    {
        std::cerr << "Failed to open " << fileName() << std::endl;
        return false;
    }

    // The actual interesting contents
    file << contents;

    // close the file out
    file << getClosingStatement();

    // And the file
    file.close();

    // Empty our data
    clear();

    return true;

}// ProtocolHeaderFile::flush


/*!
 * \return the text that is appended to close a header file
 */
std::string ProtocolHeaderFile::getClosingStatement(void)
{
    std::string close;

    if(support.language == ProtocolSupport::c_language)
    {
        // close the __cplusplus
        close += "#ifdef __cplusplus\n";
        close += "}\n";
        close += "#endif\n";
    }

    // close the opening #ifdef
    close += "#endif // _" + toUpper(module) + replace(toUpper(extension), ".", "_") + "\n";

    return close;
}


/*!
 * Setup a file for a possible append. The append will happen if the file
 * already exists, in which case it is read out, and the closing statement
 * removed so append can be performed
 */
void ProtocolHeaderFile::prepareToAppend(void)
{    
    std::error_code ec;

    if(std::filesystem::exists(fileNameAndPathOnDisk(), ec))
    {
        std::fstream file(fileNameAndPathOnDisk(), std::ios_base::in);

        if(!file.is_open())
        {
            std::cerr << "Failed to open " << fileName() << " for append" << std::endl;
            return;
        }

        // Read the entire file, and store as existing text string data
        contents = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        // Close the file, we don't need it anymore
        file.close();

        // Remove the trailing closing statement from the file so we can append further stuff
        size_t index = contents.rfind(getClosingStatement());
        if(index < contents.size())
            contents.erase(index);

        // we are appending
        appending = true;

    }// If the file already exists
    else
    {
        // If this file does not yet exist, then put the stuff on top that is always included

        // Tag for what generated the file
        write("// " + fileName() + " was generated by ProtoGen version " + ProtocolParser::genVersion + "\n\n");

        if (!support.licenseText.empty())
        {
            write(support.licenseText);
            makeLineSeparator();
        }

        // The opening #ifdef
        std::string define = "_" + toUpper(module) + replace(toUpper(extension), ".", "_");
        write("#ifndef " + define + "\n");
        write("#define " + define + "\n");

        if(support.language == ProtocolSupport::c_language)
        {
            write("\n// Language target is C, C++ compilers: don't mangle us\n");
            write("#ifdef __cplusplus\n");
            write("extern \"C\" {\n");
            write("#endif\n\n");
        }
        else if(support.language == ProtocolSupport::cpp_language)
        {
            write("\n// Language target is C++\n\n");
        }

        // Comment block at the top of the header file needed so doxygen will document the file
        write("/*!\n");
        write(" * \\file\n");
        write(" */\n");
        write("\n");

        if((support.language == ProtocolSupport::c_language) || (support.language == ProtocolSupport::cpp_language))
            writeIncludeDirective("stdint.h", "", true);

        if(support.supportbool && (support.language == ProtocolSupport::c_language))
            writeIncludeDirective("stdbool.h", "", true);
    }

}// ProtocolHeaderFile::prepareToAppend


/*!
 * Get the extension information for this name, and remove it from the name.
 * \param name has its extension (if any) logged and removed
 */
void ProtocolSourceFile::extractExtension(std::string& name)
{
    ProtocolFile::extractExtension(name);

    if(support.language == ProtocolSupport::cpp_language)
    {
        // We cannot allow the .c extension for c++
        if(extension.empty() || endsWith(extension, ".c"))
            extension = ".cpp";
    }
    else
    {
        // A source file extension must start with ".c" (.c, .cpp, .cxx, etc.)
        if(!contains(extension, ".c"))
            extension = ".c";
    }

}// ProtocolSourceFile::extractExtension


/*!
 * Write the file to disc, including any prologue/epilogue
 * \return true if the file is written, else there is a problem opening\overwriting the file
 */
bool ProtocolSourceFile::flush(void)
{
    // Nothing to write
    if(!dirty || contents.empty())
        return false;

    // Got to have a name
    if(module.empty())
    {
        std::cerr << "Empty module name when writing protocol source file" << std::endl;
        return false;
    }

    std::error_code ec;

    // Make sure the path exists
    if(!path.empty())
        std::filesystem::create_directories(path, ec);

    // Open the file for write
    std::fstream file(fileNameAndPathOnDisk(), std::ios_base::out);

    if(!file.is_open())
    {
        std::cerr << "Failed to open " << fileName() << std::endl;
        return false;
    }

    // The actual interesting contents
    file << contents;

    // close the file out
    file << getClosingStatement();

    // And the file
    file.close();

    // Empty our data
    clear();

    return true;

}// ProtocolSourceFile::flush


/*!
 * \return the text that is appended to close a source file
 */
std::string ProtocolSourceFile::getClosingStatement(void)
{
    std::string close;

    // Mark the end of the file (so we can find it later if we append)
    close += "// end of " + fileName() + "\n";

    return close;
}


/*!
 * Setup a file for a possible append. The append will happen if the file
 * already exists, in which case it is read out, and the closing statement
 * removed so append can be performed
 */
void ProtocolSourceFile::prepareToAppend(void)
{
    std::error_code ec;

    if(std::filesystem::exists(fileNameAndPathOnDisk(), ec))
    {
        std::fstream file(fileNameAndPathOnDisk(), std::ios_base::in);

        if(!file.is_open())
        {
            std::cerr << "Failed to open " << fileName() << " for append" << std::endl;
            return;
        }

        // Read the entire file, and store as existing text string data 
        contents = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        // Close the file, we don't need it anymore
        file.close();

        // Remove the trailing closing statement from the file so we can append further stuff
        size_t index = contents.rfind(getClosingStatement());
        if(index < contents.size())
            contents.erase(index);

        // we are appending
        appending = true;

    }// If the file already exists
    else
    {
        // If this file does not yet exist, then put the stuff on top that is always included

        // Tag for what generated the file
        write("// " + fileName() + " was generated by ProtoGen version " + ProtocolParser::genVersion + "\n\n");

        if (!support.licenseText.empty())
        {
            write(support.licenseText);
            makeLineSeparator();
        }

        // The source file includes the header
        writeIncludeDirective(module);
    }

}// ProtocolSourceFile::prepareToAppend
