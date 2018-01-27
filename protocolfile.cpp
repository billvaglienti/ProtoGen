#include "protocolfile.h"
#include "protocolparser.h"
#include <QDateTime>
#include <QFile>
#include <QFileDevice>
#include <QDir>
#include <iostream>

#ifdef WIN32
// For access to NTFS permissions
extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
#endif

QString ProtocolFile::tempprefix = "temporarydeleteme_";

/*!
 * Create the file object
 * \param moduleName is the name of the file, not counting any extension
 * \param temp should be true for this file to be a temp file
 */
ProtocolFile::ProtocolFile(const QString& moduleName, bool temp) :
    module(moduleName),
    dirty(false),
    appending(false),
    temporary(temp),
    iscpp(false)
{
}


/*!
 * Create the file object. After this constructor you must call setModuleNameAndPath()
 * or a file will not be created
 */
ProtocolFile::ProtocolFile() :
    dirty(false),
    appending(false),
    temporary(true),
    iscpp(false)
{
}

void ProtocolFile::setModuleNameAndPath(QString name, QString filepath)
{
    setModuleNameAndPath(QString(), name, filepath);
}


void ProtocolFile::setModuleNameAndPath(QString prefix, QString name, QString filepath)
{
    // Remove any contents we currently have
    clear();

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
void ProtocolFile::extractExtension(QString& name)
{
    extractExtension(name, extension);
}


/*!
 * Get the extension information for this name, and remove it from the name.
 * \param name has its extension (if any) removed
 * \param extension receives the extension
 */
void ProtocolFile::extractExtension(QString& name, QString& extension)
{
    // Note that "." as the first character is not an extension, it indicates a hidden file
    int index = name.lastIndexOf(".");
    if(index >= 1)
    {
        // The extension, including the "."
        extension = name.right(name.size() - index);

        // The name without the extension
        name = name.left(index);
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
void ProtocolFile::separateModuleNameAndPath(QString& name, QString& filepath)
{
    // Handle the case where the file includes "./" to reference the current working
    // directory. We just remove this as its not needed.
    if(name.startsWith("./"))
        name.remove("./");
    else if(name.startsWith(".\\"))
        name.remove(".\\");

    // Does the name include some path information?
    int index = name.lastIndexOf("/");
    if(index < 0)
        index = name.lastIndexOf("\\");

    if(index >= 0)
    {
        // The path information in the name (including the path separator)
        QString namepath = name.left(index+1);

        // In case the user gave path information in the name which
        // references a global location, ignore the filepath (i.e. user
        // can override the working directory)
        if(namepath.contains(":") || namepath.startsWith('\\') || namepath.startsWith('/'))
            filepath = namepath;
        else
        {
            if(filepath.isEmpty())
                filepath += ".";

            filepath += "/" + namepath;
        }

        // The name without the path data
        name = name.right(name.size() - index - 1);

    }// If name contains a path separator

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
void ProtocolFile::write(const QString& text)
{
    contents += text;
    dirty = true;
}


/*!
 * Output multiple include directives. The include directives are all done
 * using quotes, not global brackes
 * \param list is the list of module names that need to be included
 */
void ProtocolFile::writeIncludeDirectives(const QStringList& list)
{
    for(int i = 0; i < list.count(); i++)
        writeIncludeDirective(list.at(i));
}


/*!
 * Output an include directive, which looks like "#include filename.h\n". You
 * can pass the entire include directive or just the module name. The include
 * directive will not be output if this file already contains this directive.
 * \param include is the module name to include
 * \param comment is a trailing comment for the include directive, can be empty
 * \param global should be true to use brackets ("<>") instead of quotes.
 * \param autoextensions should be true to automatically append ".h" to the
 *        include name if it is not already included
 */
void ProtocolFile::writeIncludeDirective(const QString& include, const QString& comment, bool global, bool autoextension)
{
    if(include.isEmpty())
        return;

    QString directive = include.trimmed();

    // Technically things other than .h* could be included, but not by ProtoGen
    if(!directive.contains(".h") && autoextension)
        directive += ".h";

    // Don't include ourselves
    if(directive == fileName())
        return;

    // Build the include directive with quotes or brackets based on the global status
    if (global == false)
        directive = "#include \"" + directive + "\"";
    else
        directive = "#include <" + directive + ">";

    // See if this include directive is already present, in which case we don't need to add it again
    if(contents.contains(directive))
        return;

    // Add the comment if there is one
    if(comment.isEmpty())
        directive += "\n";
    else
        directive += "\t// " + comment + "\n";

    // We try to group all the #includes together
    int index = contents.lastIndexOf("#include");
    if(index >= 0)
    {
        // Find the end of the line
        index = contents.indexOf("\n", index);
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
void ProtocolFile::makeLineSeparator(QString& contents)
{
    // Make sure that there are exactly two line terminators at the end of the string
    int last = contents.size() - 1;
    int lastChar = last;

    if(last < 0)
        return;

    // Scroll backwards until we have cleared the linefeeds and last is pointing at the last non-linefeed character
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
        contents.chop(last - (lastChar + 2));
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
QString ProtocolFile::sanitizePath(const QString& path)
{
    QString relative;
    QString absolute;

    // Empty paths are the simplest
    if(path.isEmpty())
        return relative;

    // Absolute path including the mount point
    absolute = QDir(path).absolutePath();

    // Make sure it has a trailing separator
    if(!absolute.endsWith('/') && !absolute.endsWith('\\'))
        absolute += '/';

    // Make the result relative to the current working directory
    relative = QDir::current().relativeFilePath(absolute + "randomfilename.txt").remove("randomfilename.txt");

    // Return the shorter of the two paths
    if(relative.count() > absolute.count())
        return absolute;
    else
        return relative;
}


/*!
 * delete a specific file. The file will be deleted even if it is read-only.
 * \param fileName identifies the file relative to the current working directory
 */
void ProtocolFile::deleteFile(const QString& fileName)
{
    makeFileWritable(fileName);
    if(QFile::exists(fileName))
        QFile::remove(fileName);
}


/*!
 * Rename a file from oldName to newName. If the file newName already exists
 * it will be deleted.
 * \param oldName is the old name of the file
 * \param newName is the new name of the file
 */
void ProtocolFile::renameFile(const QString& oldName, const QString& newName)
{
    // Make sure the new file does not exist
    deleteFile(newName);

    // Now make the old name become the new name
    makeFileWritable(oldName);
    QFile::rename(oldName, newName);

}// ProtocolFile::renameFile


/*!
 * Make a specific file writable.
 * \param fileName identifies the file relative to the current working directory
 */
void ProtocolFile::makeFileWritable(const QString& fileName)
{
    #ifdef WIN32
    // We want to work with permissions...
    qt_ntfs_permission_lookup++;
    #endif

    // Trying to delete the file directly can fail if we don't have permission
    QFileDevice::Permissions per;
    if(QFile::exists(fileName))
    {
        per = QFile::permissions(fileName);
        per |= QFileDevice::ReadUser;
        per |= QFileDevice::WriteUser;
        QFile::setPermissions(fileName, per);
    }

    #ifdef WIN32
    // Done working with permissions
    qt_ntfs_permission_lookup--;
    #endif
}


/*!
 * Copy a temporary file to the real file and delete the temporary file
 * \param path is the path to the files
 * \param fileName is the real file name, which does not include the temporary prefix
 */
void ProtocolFile::copyTemporaryFile(const QString& path, const QString& fileName)
{
    bool equal = false;
    QString tempFileName = path + tempprefix + fileName;
    QString permFileName = path + fileName;

    QFile tempfile(tempFileName);
    QFile permfile(permFileName);

    // Its possible we already copied and deleted the file, so this isn't an error
    if(!tempfile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    // Check if the files are the same
    if(permfile.open(QIODevice::ReadOnly | QIODevice::Text))
        equal = tempfile.readAll() == permfile.readAll();

    // Done with the file contents
    tempfile.close();
    permfile.close();

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
void ProtocolFile::deleteModule(const QString& moduleName)
{
    deleteFile(moduleName + ".cpp");
    deleteFile(moduleName + ".c");
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
QString ProtocolFile::fileNameAndPathOnDisk(void) const
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
    if(!dirty || contents.isEmpty())
        return false;

    // Got to have a name
    if(module.isEmpty())
    {
        std::cerr << "Empty module name when writing protocol file" << std::endl;
        return false;
    }

    QFile file(fileNameAndPathOnDisk());

    // Make sure the path exists
    if(!path.isEmpty())
        QDir::current().mkpath(path);

    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        std::cerr << "error: failed to open " << fileName().toStdString() << std::endl;
        return false;
    }

    // The actual interesting contents
    file.write(qPrintable(contents));

    // And the file
    file.close();

    // Empty our data
    clear();

    return true;

}// ProtocolFile::flush


/*!
 * Get the extension information for this name, and remove it from the name.
 * \param name has its extension (if any) logged and removed
 */
void ProtocolHeaderFile::extractExtension(QString& name)
{
    ProtocolFile::extractExtension(name);

    // A head file extension must start with ".h" (.h, .hpp, .hxx, etc.)
    if(!extension.contains(".h", Qt::CaseInsensitive))
        extension = ".h";

}// ProtocolHeaderFile::extractExtension


/*!
 * Write the file to disc, including any prologue/epilogue
 * \return true if the file is written, else there is a problem opening\overwriting the file
 */
bool ProtocolHeaderFile::flush(void)
{
    // Nothing to write
    if(!dirty || contents.isEmpty())
        return false;

    // Got to have a name
    if(module.isEmpty())
    {
        std::cerr << "Empty module name when writing protocol header file" << std::endl;
        return false;
    }

    QFile file(fileNameAndPathOnDisk());

    // Make sure the path exists
    if(!path.isEmpty())
        QDir::current().mkpath(path);

    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        std::cerr << "Failed to open " << fileName().toStdString() << std::endl;
        return false;
    }

    // The actual interesting contents
    file.write(qPrintable(contents));

    // close the file out
    file.write(qPrintable(getClosingStatement()));

    // And the file
    file.close();

    // Empty our data
    clear();

    return true;

}// ProtocolHeaderFile::flush


/*!
 * \return the text that is appended to close a header file
 */
QString ProtocolHeaderFile::getClosingStatement(void)
{
    QString close;

    if(!iscpp)
    {
        // close the __cplusplus
        close += "#ifdef __cplusplus\n";
        close += "}\n";
        close += "#endif\n";
    }

    // close the opening #ifdef
    close += "#endif\n";

    return close;
}


/*!
 * Setup a file for a possible append. The append will happen if the file
 * already exists, in which case it is read out, and the closing statement
 * removed so append can be performed
 */
void ProtocolHeaderFile::prepareToAppend(void)
{    
    QFile file(fileNameAndPathOnDisk());

    if(file.exists())
    {
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            std::cerr << "Failed to open " << fileName().toStdString() << " for append" << std::endl;
            return ;
        }

        // Read the entire file, and store as existing text string data
        contents = file.readAll();

        // Close the file, we don't need it anymore
        file.close();

        // Remove the trailing closing statement from the file so we can append further stuff
        contents.remove(getClosingStatement());

        // we are appending
        appending = true;

    }// If the file already exists
    else
    {
        // If this file does not yet exist, then put the stuff on top that is always included

        // Tag for what generated the file
        write(qPrintable("// " + fileName() + " was generated by ProtoGen version " + ProtocolParser::genVersion + "\n\n"));

        if (!license.isEmpty())
        {
            write(license);
            makeLineSeparator();
        }

        // The opening #ifdef
        QString define = "_" + module.toUpper() + "_H";
        write(qPrintable("#ifndef " + define + "\n"));
        write(qPrintable("#define " + define + "\n"));

        if(!iscpp)
        {
            write("\n// C++ compilers: don't mangle us\n");
            write("#ifdef __cplusplus\n");
            write("extern \"C\" {\n");
            write("#endif\n\n");
        }
    }

}// ProtocolHeaderFile::prepareToAppend


/*!
 * Get the extension information for this name, and remove it from the name.
 * \param name has its extension (if any) logged and removed
 */
void ProtocolSourceFile::extractExtension(QString& name)
{
    ProtocolFile::extractExtension(name);

    if(iscpp)
        extension = ".cpp";
    else
    {
        // A source file extension must start with ".c" (.c, .cpp, .cxx, etc.)
        if(!extension.contains(".c", Qt::CaseInsensitive))
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
    if(!dirty || contents.isEmpty())
        return false;

    // Got to have a name
    if(module.isEmpty())
    {
        std::cerr << "Empty module name when writing protocol source file" << std::endl;
        return false;
    }

    QFile file(fileNameAndPathOnDisk());

    // Make sure the path exists
    if(!path.isEmpty())
        QDir::current().mkpath(path);

    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        std::cerr << "Failed to open " << fileName().toStdString() << std::endl;
        return false;
    }

    // The actual interesting contents
    file.write(qPrintable(contents));

    // Close it out
    file.write(qPrintable(getClosingStatement()));

    // And the file
    file.close();

    // Empty our data
    clear();

    return true;

}// ProtocolSourceFile::flush


/*!
 * \return the text that is appended to close a source file
 */
QString ProtocolSourceFile::getClosingStatement(void)
{
    QString close;

    // close the __cplusplus
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
    QFile file(fileNameAndPathOnDisk());

    if(file.exists())
    {
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            std::cerr << "Failed to open " << file.fileName().toStdString() << " for append" << std::endl;
            return ;
        }

        // Read the entire file, and store as existing text string data
        contents = file.readAll();

        // Close the file, we don't need it anymore
        file.close();

        // Remove the trailing closing statement from the file so we can append further stuff
        contents.remove(getClosingStatement());

        // we are appending
        appending = true;

    }// If the file already exists
    else
    {
        // If this file does not yet exist, then put the stuff on top that is always included

        // Tag for what generated the file
        write(qPrintable("// " + fileName() + " was generated by ProtoGen version " + ProtocolParser::genVersion + "\n\n"));

        if (!license.isEmpty())
        {
            write(license);
            makeLineSeparator();
        }

        // The source file includes the header
        write(qPrintable("#include \"" + module + ".h\"\n"));
    }

}// ProtocolSourceFile::prepareToAppend
