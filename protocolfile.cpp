#include "protocolfile.h"
#include "protocolparser.h"
#include <QDateTime>
#include <QFile>
#include <QFileDevice>
#include <iostream>

#ifdef WIN32
// For access to NTFS permissions
extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
#endif

/*!
 * Create the file object
 * \param moduleName is the name of the file, not counting any extension
 */
ProtocolFile::ProtocolFile(const QString& moduleName, bool onlyversion) :
    module(moduleName),
    dirty(false),
    appending(false),
    versionOnly(onlyversion)
{
}


/*!
 * Create the file object. After this constructor you must call setModuleName()
 * or a file will not be created
 */
ProtocolFile::ProtocolFile() :
    dirty(false),
    appending(false)
{
}


void ProtocolFile::setModuleName(const QString& name)
{
    module = name;
}


/*!
 * Clear the contents of the file. This will also clear the warning indicator and mark the file as clean
 */
void ProtocolFile::clear()
{
    prototypeContents.clear();
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
 * Output an include directive, which looks like "#include filename.h\n". You
 * can pass the entire include directive or just the module name. The include
 * directive will not be output if this file already contains this directive.
 * \param include is the module name to include
 * \param comment is a trailing comment for the include directive, can be empty
 */
void ProtocolFile::writeIncludeDirective(const QString& include, const QString& comment)
{
    if(include.isEmpty())
        return;

    QString directive = include.trimmed();

    // Technically things other than .h could be included, but not by ProtoGen
    if(!directive.contains(".h"))
        directive += ".h";

    // Don't include ourselves
    if(directive == fileName())
        return;

    // Build the include directive with quotes
    directive = "#include \"" + directive + "\"";

    // See if this include directive is already present, in which case we don't need to add it again
    if(contents.contains(directive))
        return;

    // Add the comment if there is one
    if(comment.isEmpty())
        directive += "\n";
    else
        directive += "\t// " + comment + "\n";

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
 * delete both the .c and .h file. The files will be deleted even if they are read-only.
 * \param moduleName gives the file name without extension.
 */
void ProtocolFile::deleteModule(const QString& moduleName)
{
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
 * Write the file to disc, including any prologue/epilogue
 * \return true if the file is written, else there is a problem opening\overwriting the file
 */
bool ProtocolFile::flush(void)
{
    if(!dirty)
        return false;

    // Got to have a name
    if(module.isEmpty())
    {
        std::cout << "Empty module name when writing protocol file" << std::endl;
        return false;
    }

    QFile file(fileName());

    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        std::cout << "Failed to open " << fileName().toStdString() << std::endl;
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
 * Return the filename, which is the module name plus ".h"
 * \return the filename
 */
QString ProtocolHeaderFile::fileName(void) const
{
    return module + ".h";
}


/*!
 * Write the file to disc, including any prologue/epilogue
 * \return true if the file is written, else there is a problem opening\overwriting the file
 */
bool ProtocolHeaderFile::flush(void)
{
    if(!dirty)
        return false;

    // Got to have a name
    if(module.isEmpty())
    {
        std::cout << "Empty module name when writing protocol header file" << std::endl;
        return false;
    }

    QFile file(fileName());

    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        std::cout << "Failed to open " << fileName().toStdString() << std::endl;
        return false;
    }

    if(!appending)
    {
        // Tag for when the file was generated
        if(versionOnly)
            file.write(qPrintable("// " + file.fileName() + " was generated by ProtoGen version " + ProtocolParser::genVersion + "\n\n"));
        else
            file.write(qPrintable("// " + file.fileName() + " was generated by ProtoGen version " + ProtocolParser::genVersion + " on " + QDateTime::currentDateTime().toString() + "\n\n"));

        // The opening #ifdef
        QString define = "_" + module.toUpper() + "_H";
        file.write(qPrintable("#ifndef " + define + "\n"));
        file.write(qPrintable("#define " + define + "\n"));

        file.write("\n// C++ compilers: don't mangle us\n");
        file.write("#ifdef __cplusplus\n");
        file.write("extern \"C\" {\n");
        file.write("#endif\n\n");
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

    // close the __cplusplus
    close += "#ifdef __cplusplus\n";
    close += "}\n";
    close += "#endif\n";

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
    QFile file(fileName());

    if(file.exists())
    {
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            std::cout << "Failed to open " << fileName().toStdString() << " for append" << std::endl;
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

}// ProtocolHeaderFile::prepareToAppend


/*!
 * Return the filename, which is the module name plus ".h"
 * \return the filename
 */
QString ProtocolSourceFile::fileName(void) const
{
    return module + ".c";
}


/*!
 * Write the file to disc, including any prologue/epilogue
 * \return true if the file is written, else there is a problem opening\overwriting the file
 */
bool ProtocolSourceFile::flush(void)
{
    if(!dirty)
        return false;

    // Got to have a name
    if(module.isEmpty())
    {
        std::cout << "Empty module name when writing protocol source file" << std::endl;
        return false;
    }

    QFile file(fileName());

    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        std::cout << "Failed to open " << fileName().toStdString() << std::endl;
        return false;
    }

    if(!appending)
    {
        // Tag for when the file was generated
        if(versionOnly)
            file.write(qPrintable("// " + file.fileName() + " was generated by ProtoGen version " + ProtocolParser::genVersion + "\n\n"));
        else
            file.write(qPrintable("// " + file.fileName() + " was generated by ProtoGen version " + ProtocolParser::genVersion + " on " + QDateTime::currentDateTime().toString() + "\n\n"));

        // The source file includes the header
        file.write(qPrintable("#include \"" + module + ".h\"\n"));
    }

    // The part of the file that goes before the main body
    file.write(qPrintable(prototypeContents));

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
    QFile file(fileName());

    if(file.exists())
    {
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            std::cout << "Failed to open " << fileName().toStdString() << " for append" << std::endl;
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

}// ProtocolSourceFile::prepareToAppend
