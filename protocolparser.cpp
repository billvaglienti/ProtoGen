#include "protocolparser.h"
#include "protocolstructuremodule.h"
#include "protocolpacket.h"
#include "enumcreator.h"
#include "protocolscaling.h"
#include "fieldcoding.h"
#include "protocolsupport.h"
#include "protocolbitfield.h"
#include "protocoldocumentation.h"
#include <QDomDocument>
#include <QFile>
#include <QDir>
#include <QFileDevice>
#include <QDateTime>
#include <QStringList>
#include <QProcess>
#include <iostream>

// The version of the protocol generator is set here
const QString ProtocolParser::genVersion = "2.14.b";

/*!
 * \brief ProtocolParser::ProtocolParser
 */
ProtocolParser::ProtocolParser() :
    latexHeader(1),
    latexEnabled(false),
    nomarkdown(false),
    nohelperfiles(false),
    nodoxygen(false),
    noAboutSection(false),
    showAllItems(false),
    nocss(false),
    tableOfContents(false)
{
}


/*!
 * Destroy the protocol parser object
 */
ProtocolParser::~ProtocolParser()
{
    // Delete all of our lists of new'ed objects
    qDeleteAll(documents.begin(), documents.end());
    documents.clear();

    qDeleteAll(structures.begin(), structures.end());
    structures.clear();

    qDeleteAll(enums.begin(), enums.end());
    enums.clear();

    qDeleteAll(globalEnums.begin(), globalEnums.end());
    globalEnums.clear();

    qDeleteAll(lines.begin(), lines.end());
    lines.clear();
}


/*!
 * Parse the DOM from the xml file. This kicks off the auto code generation for the protocol
 * \param filenames is the list of files to read the xml contents from. The
 *        first file is the master file that sets the protocol options
 * \param path is the output path for generated files
 * \return true if something was written to a file
 */
bool ProtocolParser::parse(QString filename, QString path, QStringList otherfiles)
{
    QDomDocument doc;
    QFile file(filename);
    QFileInfo fileinfo(filename);

    // Top level printout of the version information
    std::cout << "ProtoGen version " << genVersion.toStdString() << std::endl;

    // Remember the input path, in case there are files referenced by the main file
    inputpath = ProtocolFile::sanitizePath(fileinfo.absolutePath());

    // Also remember the name of the file, which we use for warning outputs
    inputfile = fileinfo.fileName();

    if (!file.open(QIODevice::ReadOnly))
    {
        emitWarning(" error: Failed to open protocol file");
        return false;
    }

    // Qt's XML parsing
    QString errorMsg;
    int errorLine;
    int errorCol;

    if(!doc.setContent(file.readAll(), false, &errorMsg, &errorLine, &errorCol))
    {
        file.close();
        QString warning = filename + ":" + QString::number(errorLine) + ":" + QString::number(errorCol) + " error: " + errorMsg;
        std::cerr << warning.toStdString() << std::endl;
        return false;
    }

    // Done with the file
    file.close();

    // Set our output directory
    // Make the path as short as possible
    path = ProtocolFile::sanitizePath(path);

    // There is an interesting case here: the user's output path is not
    // empty, but after sanitizing it is empty, which implies the output
    // path is the same as the current working directory. If this happens
    // then mkpath() will fail because the input string cannot be empty,
    // so we must test path.isEmpty() again. Note that in this case the
    // path definitely exists (since its our working directory)

    // Make sure the path exists
    if(path.isEmpty() || QDir::current().mkpath(path))
    {
        // Remember the output path for all users
        support.outputpath = path;
    }
    else
    {
        std::cerr << "error: Failed to create output path: " << QDir::toNativeSeparators(path).toStdString() << std::endl;
        return false;
    }

    // The outer most element
    QDomElement docElem = doc.documentElement();

    // This element must have the "Protocol" tag
    if(docElem.tagName().toLower() != "protocol")
    {
        emitWarning(" error: Protocol tag not found in XML");
        return false;
    }

    // Protocol options options specified in the xml
    QDomNamedNodeMap map = docElem.attributes();

    support.protoName = name = getAttribute("name", map);
    if(support.protoName.isEmpty())
    {
        emitWarning(" error: Protocol name not found in Protocol tag");
        return false;
    }

    title = getAttribute("title", map);
    api = getAttribute("api", map);
    version = getAttribute("version", map);
    comment = getAttribute("comment", map);
    support.parse(map);

    if(support.disableunrecognized == false)
    {
        // All the attributes understood by the protocol support
        QStringList attriblist = support.getAttriblist();

        // and the ones we understand
        attriblist << "name" << "title" << "api" << "version" << "comment";

        for(int i = 0; i < map.count(); i++)
        {
            QDomAttr attr = map.item(i).toAttr();
            if(attr.isNull())
                continue;

            if((attriblist.contains(attr.name(), Qt::CaseInsensitive) == false))
                emitWarning(support.protoName, support.protoName + ": Unrecognized attribute \"" + attr.name() + "\"");

        }// for all the attributes

    }// if we need to warn for unrecognized attributes

    // The list of our output files
    QStringList fileNameList;
    QStringList filePathList;

    // Build the top level module
    createProtocolHeader(docElem);

    // And record its file name
    fileNameList.append(header.fileName());
    filePathList.append(header.filePath());

    // Now parse the contents of all the files. We do other files first since
    // we expect them to be helpers that the main file may depend on.
    for(int i = 0; i < otherfiles.count(); i++)
        parseFile(otherfiles.at(i));

    // Finally the main file
    parseFile(filename);

    // This is a resource file for bitfield testing
    if(support.bitfieldtest && support.bitfield)
        parseFile(":/files/prebuiltSources/bitfieldtest.xml");

    // Output the global enumerations first, they will go in the main
    // header file by default, unless the enum specifies otherwise
    ProtocolHeaderFile enumfile;
    ProtocolSourceFile enumSourceFile;

    for(int i = 0; i < globalEnums.size(); i++)
    {
        EnumCreator* module = globalEnums.at(i);

        module->parseGlobal();
        enumfile.setLicenseText(support.licenseText);
        enumfile.setModuleNameAndPath(module->getHeaderFileName(), module->getHeaderFilePath());
        enumfile.write(module->getOutput());
        enumfile.makeLineSeparator();
        enumfile.flush();

        // If there is source-code available
        QString source = module->getSourceOutput();

        if(!source.isEmpty())
        {
            enumSourceFile.setLicenseText(support.licenseText);
            enumSourceFile.setModuleNameAndPath(module->getHeaderFileName(), support.outputpath);

            enumSourceFile.write(source);
            enumSourceFile.makeLineSeparator();
            enumSourceFile.flush();

            fileNameList.append(enumSourceFile.fileName());
            filePathList.append(enumSourceFile.filePath());
        }

        // Keep a list of all the file names we used
        fileNameList.append(enumfile.fileName());
        filePathList.append(enumfile.filePath());
    }

    // Now parse the global structures
    for(int i = 0; i < structures.size(); i++)
    {
        ProtocolStructureModule* module = structures[i];

        // Parse its XML and generate the output
        module->parse();

        // Keep a list of all the file names
        fileNameList.append(module->getDefinitionFileName());
        filePathList.append(module->getDefinitionFilePath());
        fileNameList.append(module->getHeaderFileName());
        filePathList.append(module->getHeaderFilePath());
        fileNameList.append(module->getSourceFileName());
        filePathList.append(module->getSourceFilePath());
        fileNameList.append(module->getVerifySourceFileName());
        filePathList.append(module->getVerifySourceFilePath());
        fileNameList.append(module->getVerifyHeaderFileName());
        filePathList.append(module->getVerifyHeaderFilePath());
        fileNameList.append(module->getCompareSourceFileName());
        filePathList.append(module->getCompareSourceFilePath());
        fileNameList.append(module->getCompareHeaderFileName());
        filePathList.append(module->getCompareHeaderFilePath());
        fileNameList.append(module->getPrintSourceFileName());
        filePathList.append(module->getPrintSourceFilePath());
        fileNameList.append(module->getPrintHeaderFileName());
        filePathList.append(module->getPrintHeaderFilePath());

    }// for all top level structures

    // And the global packets. We want to sort the packets into two batches:
    // those packets which can be used by other packets; and those which cannot.
    // This way we can parse the first batch ahead of the second
    for(int i = 0; i < packets.size(); i++)
    {
        ProtocolPacket* packet = packets.at(i);

        if(!isFieldSet(packet->getElement(), "useInOtherPackets"))
            continue;

        // Parse its XML
        packet->parse();

        // The structures have been parsed, adding this packet to the list
        // makes it available for other packets to find as structure reference
        structures.append(packet);

        // Keep a list of all the file names
        fileNameList.append(packet->getDefinitionFileName());
        filePathList.append(packet->getDefinitionFilePath());
        fileNameList.append(packet->getHeaderFileName());
        filePathList.append(packet->getHeaderFilePath());
        fileNameList.append(packet->getSourceFileName());
        filePathList.append(packet->getSourceFilePath());
        fileNameList.append(packet->getVerifySourceFileName());
        filePathList.append(packet->getVerifySourceFilePath());
        fileNameList.append(packet->getVerifyHeaderFileName());
        filePathList.append(packet->getVerifyHeaderFilePath());
        fileNameList.append(packet->getCompareSourceFileName());
        filePathList.append(packet->getCompareSourceFilePath());
        fileNameList.append(packet->getCompareHeaderFileName());
        filePathList.append(packet->getCompareHeaderFilePath());
        fileNameList.append(packet->getPrintSourceFileName());
        filePathList.append(packet->getPrintSourceFilePath());
        fileNameList.append(packet->getPrintHeaderFileName());
        filePathList.append(packet->getPrintHeaderFilePath());

    }

    // And the packets which are not available for other packets
    for(int i = 0; i < packets.size(); i++)
    {
        ProtocolPacket* packet = packets.at(i);

        if(isFieldSet(packet->getElement(), "useInOtherPackets"))
            continue;

        // Parse its XML
        packet->parse();

        // Keep a list of all the file names
        fileNameList.append(packet->getDefinitionFileName());
        filePathList.append(packet->getDefinitionFilePath());
        fileNameList.append(packet->getHeaderFileName());
        filePathList.append(packet->getHeaderFilePath());
        fileNameList.append(packet->getSourceFileName());
        filePathList.append(packet->getSourceFilePath());
        fileNameList.append(packet->getVerifySourceFileName());
        filePathList.append(packet->getVerifySourceFilePath());
        fileNameList.append(packet->getVerifyHeaderFileName());
        filePathList.append(packet->getVerifyHeaderFilePath());
        fileNameList.append(packet->getCompareSourceFileName());
        filePathList.append(packet->getCompareSourceFilePath());
        fileNameList.append(packet->getCompareHeaderFileName());
        filePathList.append(packet->getCompareHeaderFilePath());
        fileNameList.append(packet->getPrintSourceFileName());
        filePathList.append(packet->getPrintSourceFilePath());
        fileNameList.append(packet->getPrintHeaderFileName());
        filePathList.append(packet->getPrintHeaderFilePath());

    }

    // Parse all of the documentation
    for(int i=0; i<documents.size(); i++)
    {
        ProtocolDocumentation* doc = documents.at(i);

        doc->parse();
    }

    if(!nohelperfiles)
    {
        // Auto-generated files for coding
        ProtocolScaling(support).generate();
        FieldCoding(support).generate();

        fileNameList.append("scaledencode.h");
        filePathList.append(support.outputpath);
        fileNameList.append("scaledencode.c");
        filePathList.append(support.outputpath);
        fileNameList.append("scaleddecode.h");
        filePathList.append(support.outputpath);
        fileNameList.append("scaleddecode.c");
        filePathList.append(support.outputpath);
        fileNameList.append("fieldencode.h");
        filePathList.append(support.outputpath);
        fileNameList.append("fieldencode.c");
        filePathList.append(support.outputpath);
        fileNameList.append("fielddecode.h");
        filePathList.append(support.outputpath);
        fileNameList.append("fielddecode.c");
        filePathList.append(support.outputpath);

        // Copy the resource files
        // This is where the files are stored in the resources
        QString sourcePath = ":/files/prebuiltSources/";

        if(support.specialFloat)
        {
            fileNameList.append("floatspecial.h");
            filePathList.append(support.outputpath);
            fileNameList.append("floatspecial.c");
            filePathList.append(support.outputpath);

            QFile::copy(sourcePath + "floatspecial.c", support.outputpath + ProtocolFile::tempprefix + "floatspecial.c");
            QFile::copy(sourcePath + "floatspecial.h", support.outputpath + ProtocolFile::tempprefix + "floatspecial.h");
        }

    }

    // Code for testing bitfields
    if(support.bitfieldtest && support.bitfield)
        ProtocolBitfield::generatetest(support);

    if(!nomarkdown)
        outputMarkdown(support.bigendian, inlinecss);

    #ifndef _DEBUG
    if(!nodoxygen)
        outputDoxygen();
    #endif

    // The last bit of the protocol header
    finishProtocolHeader();

    // This is fun...replace all the temporary files with real ones if needed
    for(int i = 0; i < fileNameList.count(); i++)
        ProtocolFile::copyTemporaryFile(filePathList.at(i), fileNameList.at(i));

    // If we are putting the files in our local directory then we don't just want an empty string in our printout
    if(path.isEmpty())
        path = "./";

    std::cout << "Generated protocol files in " << QDir::toNativeSeparators(path).toStdString() << std::endl;

    return true;

}// ProtocolParser::parse


/*!
 * Parses a single XML file handling any require tags to flatten a file
 * heirarchy into a single flat structure
 * \param xmlFilename is the file to parse
 */
bool ProtocolParser::parseFile(QString xmlFilename)
{
    QFile xmlFile( xmlFilename );
    QFileInfo fileinfo(xmlFilename);

    // Don't parse the same file twice
    if(filesparsed.contains(fileinfo.absoluteFilePath()))
        return false;

    // Keep a record of what we have already parsed, so we don't parse the same file twice
    filesparsed.append(fileinfo.absoluteFilePath());

    std::cout << "Parsing file " << ProtocolFile::sanitizePath(fileinfo.absolutePath()).toStdString() << fileinfo.fileName().toStdString() << std::endl;

    if( !xmlFile.open( QIODevice::ReadOnly ) )
    {
        QString warning = "error: Failed to open xml protocol file " + xmlFilename;
        std::cerr << warning.toStdString() << std::endl;
        return false;
    }

    QString contents = xmlFile.readAll();

    xmlFile.close();

    // Error parsing
    QString error;
    int errorLine, errorCol;

    QDomDocument xmlDoc;

    // Extract XML data
    if( !xmlDoc.setContent( contents, false, &error, &errorLine, &errorCol ) )
    {
        QString warning = xmlFilename + ":" + QString::number(errorLine) + ":" + QString::number(errorCol) + " error: " + error;
        std::cerr << warning.toStdString() << std::endl;
        return false;
    }

    // Extract the outer-most tag from the document
    QDomElement top = xmlDoc.documentElement();

    // Ensure that outer tag is correct
    if( top.tagName().toLower() != "protocol" )
    {
        QString warning = xmlFilename + ":0:0: error: 'Protocol' tag not found in file";
        std::cerr << warning.toStdString() << std::endl;
        return false;
    }

    // My XML parsing, I can find no way to recover line numbers from Qt's parsing...
    lines.append(new XMLLineLocator());
    lines.last()->setXMLContents(contents, ProtocolFile::sanitizePath(fileinfo.absolutePath()), fileinfo.fileName(), name);

    // Iterate over each top level node in the file
    QDomNodeList nodes = top.childNodes();

    for(int i = 0; i < nodes.size(); i++)
    {
        QDomNode node = nodes.item(i);

        QString nodename = node.nodeName().toLower();

        // Import another file recursively
        // File recursion is handled first so that ordering is preserved
        // This effectively creates a single flattened XML structure
        if( nodename == "require" )
        {
            QString subfile = getAttribute( "file", node.attributes() );

            if( subfile.isEmpty() )
            {
                QString warning = xmlFilename + ": warning: file attribute missing from \"Require\" tag";
                std::cerr << warning.toStdString() << std::endl;
            }
            else
            {
                if(!subfile.endsWith(".xml", Qt::CaseInsensitive))
                    subfile += ".xml";

                // The new file is relative to this file
                QString newFile  = fileinfo.absoluteDir().absolutePath() + "/" + subfile;

                parseFile(newFile);
            }

        }
        else if( nodename == "struct" || nodename == "structure" )
        {
            ProtocolStructureModule* module = new ProtocolStructureModule( this, support, api, version );

            // Remember the XML
            module->setElement( node.toElement() );

            structures.append( module );
        }
        else if( nodename == "enum" || nodename == "enumeration" )
        {
            EnumCreator* Enum = new EnumCreator( this, nodename, support );

            Enum->setElement( node.toElement() );

            globalEnums.append( Enum );
            alldocumentsinorder.append( Enum );
        }
        // Define a packet
        else if( nodename == "packet" || nodename == "pkt" )
        {
            ProtocolPacket* packet = new ProtocolPacket( this, support, api, version );

            packet->setElement( node.toElement() );

            packets.append( packet );
            alldocumentsinorder.append( packet );
        }
        else if ( nodename == "doc" || nodename == "documentation" )
        {
            ProtocolDocumentation* document = new ProtocolDocumentation( this, nodename, support );

            document->setElement( node.toElement() );

            documents.append( document );
            alldocumentsinorder.append( document );
        }
        else
        {
            //TODO
        }

    }

    return true;

}// ProtocolParser::parseFile



/*!
 * Send a warning string out standard error, referencing the main input file
 * \param warning is the string to warn with
 */
void ProtocolParser::emitWarning(QString warning) const
{
    QString output = inputpath + inputfile + ":" + warning;
    std::cerr << output.toStdString() << std::endl;
}


/*!
 * Send a warning string out standard error, referencing a file by object name
 * \param hierarchicalName is the object name to reference
 * \param warning is the string to warn with
 */
void ProtocolParser::emitWarning(QString hierarchicalName, QString warning) const
{
    for(int i = 0; i < lines.count(); i++)
    {
        if(lines.at(i)->emitWarning(hierarchicalName, warning))
            return;
    }

    // If we get here then we should emit some warning
    QString output = "unknown file:" + hierarchicalName + ":" + warning;
    std::cerr << output.toStdString() << std::endl;

}


/*!
 * Create the header file for the top level module of the protocol
 * \param docElem is the "protocol" element from the DOM
 */
void ProtocolParser::createProtocolHeader(const QDomElement& docElem)
{
    // If the name is "coollink" then make everything "coollinkProtocol"
    QString nameex = name + "Protocol";

    // The file names
    header.setLicenseText(support.licenseText);
    header.setModuleNameAndPath(nameex, support.outputpath);

    // Comment block at the top of the header file
    header.write("/*!\n");
    header.write(" * \\file\n");
    header.write(" * \\mainpage " + name + " protocol stack\n");
    header.write(" *\n");

    // A long comment that should be wrapped at 80 characters
    outputLongComment(header, " *", comment);

    // The protocol enumeration API, which can be empty
    if(!api.isEmpty())
    {
        // Make sure this is only a number
        bool ok = false;
        int number = api.toInt(&ok);
        if(ok && number > 0)
        {
            // Turn it back into a string
            api = QString(api);

            header.write("\n *\n");
            outputLongComment(header, " *", "The protocol API enumeration is incremented anytime the protocol is changed in a way that affects compatibility with earlier versions of the protocol. The protocol enumeration for this version is: " + api);
        }
        else
            api.clear();

    }// if we have API enumeration

    // The protocol version string, which can be empty
    if(!version.isEmpty())
    {
        header.write("\n *\n");
        outputLongComment(header, " *", "The protocol version is " + version);
        header.write("\n");
    }

    if(version.isEmpty() && api.isEmpty())
        header.write("\n");

    header.write(" */\n");

    header.makeLineSeparator();

    // Includes
    header.writeIncludeDirective("stdint.h", QString(), true);

    // Add other includes
    outputIncludes(name, header, docElem);

    header.makeLineSeparator();

    // API functions
    if(!api.isEmpty())
    {
        header.makeLineSeparator();
        header.write("//! \\return the protocol API enumeration\n");
        header.write("#define get" + name + "Api() " + api + "\n");
    }

    // Version functions
    if(!version.isEmpty())
    {
        header.makeLineSeparator();
        header.write("//! \\return the protocol version string\n");
        header.write("#define get" + name + "Version() \""  + version + "\"\n");
    }

    header.makeLineSeparator();

    header.flush();

}// ProtocolParser::createProtocolFiles


void ProtocolParser::finishProtocolHeader(void)
{
    // If the name is "coollink" then make everything "coollinkProtocol"
    QString nameex = name + "Protocol";

    // The file name, this will result in an append to the previously created file
    header.setLicenseText(support.licenseText);
    header.setModuleNameAndPath(nameex, support.outputpath);

    header.makeLineSeparator();

    // We want these prototypes to be the last things written to the file, because support.pointerType may be defined above
    header.write("\n");
    header.write("// The prototypes below provide an interface to the packets.\n");
    header.write("// They are not auto-generated functions, but must be hand-written\n");
    header.write("\n");
    header.write("//! \\return the packet data pointer from the packet\n");
    header.write("uint8_t* get" + name + "PacketData(" + support.pointerType + " _pg_pkt);\n");
    header.write("\n");
    header.write("//! \\return the packet data pointer from the packet, const\n");
    header.write("const uint8_t* get" + name + "PacketDataConst(const " + support.pointerType + " _pg_pkt);\n");
    header.write("\n");
    header.write("//! Complete a packet after the data have been encoded\n");
    header.write("void finish" + name + "Packet(" + support.pointerType + " _pg_pkt, int size, uint32_t packetID);\n");
    header.write("\n");
    header.write("//! \\return the size of a packet from the packet header\n");
    header.write("int get" + name + "PacketSize(const " + support.pointerType + " _pg_pkt);\n");
    header.write("\n");
    header.write("//! \\return the ID of a packet from the packet header\n");
    header.write("uint32_t get" + name + "PacketID(const " + support.pointerType + " _pg_pkt);\n");
    header.write("\n");

    header.flush();

}


/*!
 * Output a long string of text which should be wrapped at 80 characters.
 * \param file receives the output
 * \param prefix precedes each line (for example "//" or " *"
 * \param text is the long text string to output. If text is empty
 *        nothing is output
 */
void ProtocolParser::outputLongComment(ProtocolFile& file, const QString& prefix, const QString& text)
{
    file.write(outputLongComment(prefix, text));

}// ProtocolParser::outputLongComment


/*!
 * Output a long string of text which should be wrapped at 80 characters.
 * \param prefix precedes each line (for example "//" or " *"
 * \param text is the long text string to output. If text is empty
 *        nothing is output.
 * \return The formatted text string.
 */
QString ProtocolParser::outputLongComment(const QString& prefix, const QString& text)
{
    return reflowComment(text, prefix, 80);

}// ProtocolParser::outputLongComment


/*!
 * Get a correctly reflowed comment from a DOM
 * \param e is the DOM to get the comment from
 * \return the correctly reflowed comment, which could be empty
 */
QString ProtocolParser::getComment(const QDomElement& e)
{
    return reflowComment(e.attribute("comment"));
}

/*!
 * Take a comment line which may have some unusual spaces and line feeds that
 * came from the xml formatting and reflow it for our needs.
 * \param text is the raw comment from the xml.
 * \param prefix precedes each line (for example "//" or " *"
 * \return the reflowed comment.
 */
QString ProtocolParser::reflowComment(QString text, QString prefix, int charlimit)
{
    // Remove leading and trailing white space
    QString output = text.trimmed();

    // Convert to unix style line endings, just in case
    output.replace("\r\n", "\n");

    // Separate by blocks that have \verbatim surrounding them
    QStringList blocks = output.split("\\verbatim", QString::SkipEmptyParts);

    // Empty the output string so we can build the output up
    output.clear();

    for(int b = 0; b < blocks.size(); b++)
    {
        // odd blocks are "verbatim", even blocks are not
        if((b & 0x01) == 1)
        {
            // Separate the paragraphs, as given by single line feeds
            QStringList paragraphs = blocks[b].split("\n", QString::KeepEmptyParts);

            if(prefix.isEmpty())
            {
                for(int i = 0; i < paragraphs.size(); i++)
                    output += paragraphs[i] + "\n";
            }
            else
            {
                // Output with the prefix
                for(int i = 0; i < paragraphs.size(); i++)
                    output += prefix + " " + paragraphs[i] + "\n";
            }
        }
        else
        {
            // Separate the paragraphs, as given by dual line feeds
            QStringList paragraphs = blocks[b].split("\n\n", QString::SkipEmptyParts);

            for(int i = 0; i < paragraphs.size(); i++)
            {
                // Replace line feeds with spaces
                paragraphs[i].replace("\n", " ");

                // Replace tabs with spaces
                paragraphs[i].replace("\t", " ");

                int length = 0;

                // Break it apart into words
                QStringList list = paragraphs[i].split(' ', QString::SkipEmptyParts);

                // Now write the words one at a time, wrapping at 80 character length
                for (int j = 0; j < list.size(); j++)
                {
                    // Length of the word plus the preceding space
                    int wordLength = list.at(j).length() + 1;

                    if(charlimit > 0)
                    {
                        if((length != 0) && (length + wordLength > charlimit))
                        {
                            // Terminate the previous line
                            output += "\n";
                            length = 0;
                        }
                    }

                    // All lines in the header comment block start with this spacing
                    if(length == 0)
                    {
                        output += prefix;
                        length += prefix.length();
                    }

                    // prefix could be zero length
                    if(length != 0)
                        output += " ";

                    output += list.at(j);
                    length += wordLength;

                }// for all words in the comment

                // Paragraph break, except for the last paragraph
                if(i < paragraphs.size() - 1)
                    output += "\n" + prefix + "\n";

            }// for all paragraphs

        }// if block is not verbatim

    }// for all blocks

    return output;
}


/*!
 * Return a list of QDomNodes that are direct children and have a specific tag
 * name. This is different then elementsByTagName() because that gets all
 * descendants, including grand-children. We just want children.
 * \param node is the parent node.
 * \param tag is the tag name to look for.
 * \param tag2 is a second optional tag name to look for.
 * \param tag3 is a third optional tag name to look for.
 * \return a list of QDomNodes.
 */
QList<QDomNode> ProtocolParser::childElementsByTagName(const QDomNode& node, QString tag, QString tag2, QString tag3)
{
    QList<QDomNode> list;

    // All the direct children
    QDomNodeList children = node.childNodes();

    // Now just the ones with the tag(s) we want
    for (int i = 0; i < children.size(); ++i)
    {
        if(!tag.isEmpty() && children.item(i).nodeName().contains(tag, Qt::CaseInsensitive))
            list.append(children.item(i));
        else if(!tag2.isEmpty() && children.item(i).nodeName().contains(tag2, Qt::CaseInsensitive))
            list.append(children.item(i));
        else if(!tag3.isEmpty() && children.item(i).nodeName().contains(tag3, Qt::CaseInsensitive))
            list.append(children.item(i));
    }

    return list;

}// ProtocolParser::childElementsByTagName


/*!
 * Return the value of an attribute from a node map
 * \param name is the name of the attribute to return. name is not case sensitive
 * \param map is the attribute node map from a DOM element.
 * \param defaultIfNone is returned if no name attribute is found.
 * \return the value of the name attribute, or defaultIfNone if none found
 */
QString ProtocolParser::getAttribute(QString name, const QDomNamedNodeMap& map, QString defaultIfNone)
{
    for(int i = 0; i < map.count(); i++)
    {
        QDomAttr attr = map.item(i).toAttr();
        if(attr.isNull())
            continue;

        // This is the only way I know to get a case insensitive attribute tag
        if(attr.name().compare(name, Qt::CaseInsensitive) == 0)
            return attr.value().trimmed();
    }

    return defaultIfNone;

}// ProtocolParser::getAttribute


/*!
 * Parse all enumerations which are direct children of a DomNode. The
 * enumerations will be stored in the global list
 * \param parent is the hierarchical name of the object which owns the new enumeration
 * \param node is parent node
 */
void ProtocolParser::parseEnumerations(QString parent, const QDomNode& node)
{
    // Build the top level enumerations
    QList<QDomNode>list = childElementsByTagName(node, "Enum");

    for(int i = 0; i < list.size(); i++)
    {
        parseEnumeration(parent, list.at(i).toElement());

    }// for all my enum tags

}// ProtocolParser::parseEnumerations


/*!
 * Parse a single enumeration given by a DOM element. This will also
 * add the enumeration to the global list which can be searched with
 * lookUpEnumeration().
 * \param parent is the hierarchical name of the object which owns the new enumeration
 * \param element is the QDomElement that represents this enumeration
 * \return a pointer to the newly created enumeration object.
 */
const EnumCreator* ProtocolParser::parseEnumeration(QString parent, const QDomElement& element)
{
    EnumCreator* Enum = new EnumCreator(this, parent, support);

    Enum->setElement(element);
    Enum->parse();
    enums.append(Enum);

    return Enum;

}// ProtocolParser::parseEnumeration


/*!
 * Output all include directions which are direct children of a DomNode
 * \param parent is the hierarchical name of the owning object
 * \param file receives the output
 * \param node is parent node
 */
void ProtocolParser::outputIncludes(QString parent, ProtocolFile& file, const QDomNode& node) const
{
    // Build the top level enumerations
    QList<QDomNode>list = childElementsByTagName(node, "Include");
    for(int i = 0; i < list.size(); i++)
    {
        QDomElement e = list.at(i).toElement();
        QDomNamedNodeMap map = e.attributes();

        QString include = ProtocolParser::getAttribute("name", map);
        QString comment;
        bool global = false;

        for(int i = 0; i < map.count(); i++)
        {
            QDomAttr attr = map.item(i).toAttr();
            if(attr.isNull())
                continue;

            QString attrname = attr.name();

            if(attrname.compare("name", Qt::CaseInsensitive) == 0)
                include = attr.value().trimmed();
            else if(attrname.compare("comment", Qt::CaseInsensitive) == 0)
                comment = ProtocolParser::reflowComment(attr.value().trimmed());
            else if(attrname.compare("global", Qt::CaseInsensitive) == 0)
                global = ProtocolParser::isFieldSet(attr.value().trimmed());
            else if(support.disableunrecognized == false)
                emitWarning(parent + ":" + include + ": Unrecognized attribute \"" + attrname + "\"");

        }// for all attributes

        if(!include.isEmpty())
            file.writeIncludeDirective(include, comment, global);

    }// for all include tags

}// ProtocolParser::outputIncludes


/*!
 * Find the include name for a specific global structure type or enumeration type
 * \param typeName is the type to lookup
 * \return the file name to be included to reference this global structure type
 */
QString ProtocolParser::lookUpIncludeName(const QString& typeName) const
{
    for(int i = 0; i < structures.size(); i++)
    {
        if(structures.at(i)->typeName == typeName)
        {
            return structures.at(i)->getDefinitionFileName();
        }
    }

    for(int i = 0; i < packets.size(); i++)
    {
        if(packets.at(i)->typeName == typeName)
        {
            return packets.at(i)->getDefinitionFileName();
        }
    }

    for(int i = 0; i < globalEnums.size(); i++)
    {
        if((globalEnums.at(i)->getName() == typeName) || globalEnums.at(i)->isEnumerationValue(typeName))
        {
            return globalEnums.at(i)->getHeaderFileName();
        }
    }

    return "";
}


/*!
 * Find the global structure pointer for a specific type
 * \param typeName is the type to lookup
 * \return a pointer to the structure encodable, or NULL if it does not exist
 */
const ProtocolStructureModule* ProtocolParser::lookUpStructure(const QString& typeName) const
{
    for(int i = 0; i < structures.size(); i++)
    {
        if(structures.at(i)->typeName == typeName)
        {
            return structures.at(i);
        }
    }


    for(int i = 0; i < packets.size(); i++)
    {
        if(packets.at(i)->typeName == typeName)
        {
            return packets.at(i);
        }
    }

    return NULL;
}


/*!
 * Find the enumeration creator and return a constant pointer to it
 * \param enumName is the name of the enumeration.
 * \return A pointer to the enumeration creator, or NULL if it cannot be found
 */
const EnumCreator* ProtocolParser::lookUpEnumeration(const QString& enumName) const
{
    for(int i = 0; i < globalEnums.size(); i++)
    {
        if(globalEnums.at(i)->getName() == enumName)
        {
            return globalEnums.at(i);
        }
    }

    for(int i = 0; i < enums.size(); i++)
    {
        if(enums.at(i)->getName() == enumName)
        {
            return enums.at(i);
        }
    }

    return 0;
}


/*!
 * Replace any text that matches an enumeration name with the value of that enumeration
 * \param text is modified to replace names with numbers
 * \return a reference to text
 */
QString& ProtocolParser::replaceEnumerationNameWithValue(QString& text) const
{
    for(int i = 0; i < globalEnums.size(); i++)
    {
        globalEnums.at(i)->replaceEnumerationNameWithValue(text);
    }

    for(int i = 0; i < enums.size(); i++)
    {
        enums.at(i)->replaceEnumerationNameWithValue(text);
    }

    return text;
}


/*!
 * Determine if text is part of an enumeration. This will compare against all
 * elements in all enumerations and return the enumeration name if a match is found.
 * \param text is the enumeration value string to compare.
 * \return The enumeration name if a match is found, or an empty string for no match.
 */
QString ProtocolParser::getEnumerationNameForEnumValue(const QString& text) const
{
    for(int i = 0; i < globalEnums.size(); i++)
    {
        if(globalEnums.at(i)->isEnumerationValue(text))
            return globalEnums.at(i)->getName();
    }

    for(int i = 0; i < enums.size(); i++)
    {
        if(enums.at(i)->isEnumerationValue(text))
            return enums.at(i)->getName();
    }

    return QString();

}


/*!
 * Find the enumeration value with this name and return its comment, or an empty
 * string. This will search all the enumerations that the parser knows about to
 * find the enumeration value.
 * \param name is the name of the enumeration value to find
 * \return the comment string of the name enumeration element, or an empty
 *         string if name is not found
 */
QString ProtocolParser::getEnumerationValueComment(const QString& name) const
{
    QString comment;

    for(int i = 0; i < globalEnums.size(); i++)
    {
        comment = globalEnums.at(i)->getEnumerationValueComment(name);
        if(!comment.isEmpty())
            return comment;
    }

    for(int i = 0; i < enums.size(); i++)
    {
        comment = enums.at(i)->getEnumerationValueComment(name);
        if(!comment.isEmpty())
            return comment;
    }

    return comment;

}


/*!
 * Get details needed to produce documentation for a global encodable. The top level details are ommitted.
 * \param typeName identifies the type of the global encodable.
 * \param parentName is the name of the parent which will be pre-pended to the name of this encodable
 * \param startByte is the starting byte location of this encodable, which will be updated for the following encodable.
 * \param bytes is appended for the byte range of this encodable.
 * \param names is appended for the name of this encodable.
 * \param encodings is appended for the encoding of this encodable.
 * \param repeats is appended for the array information of this encodable.
 * \param comments is appended for the description of this encodable.
 */
void ProtocolParser::getStructureSubDocumentationDetails(QString typeName, QList<int>& outline, QString& startByte, QStringList& bytes, QStringList& names, QStringList& encodings, QStringList& repeats, QStringList& comments) const
{
    for(int i = 0; i < structures.size(); i++)
    {
        if(structures.at(i)->typeName == typeName)
        {
            return structures.at(i)->getSubDocumentationDetails(outline, startByte, bytes, names, encodings, repeats, comments);
        }
    }


    for(int i = 0; i < packets.size(); i++)
    {
        if(packets.at(i)->typeName == typeName)
        {
            return packets.at(i)->getSubDocumentationDetails(outline, startByte, bytes, names, encodings, repeats, comments);
        }
    }

}


/*!
 * Ouptut documentation for the protocol as a markdown file
 * \param isBigEndian should be true for big endian documentation, else the documentation is little endian.
 * \param inlinecss is the css to use for the markdown output, if blank use default.
 */
void ProtocolParser::outputMarkdown(bool isBigEndian, QString inlinecss)
{
    QString basepath = support.outputpath;

    if (!docsDir.isEmpty())
        basepath = docsDir;

    QString filename = basepath + name + ".markdown";
    QString filecontents = "\n\n";
    ProtocolFile file(filename, false);

    QStringList packetids;
    for(int i = 0; i < packets.size(); i++)
        packets.at(i)->appendIds(packetids);
    packetids.removeDuplicates();

    /* Write protocol introductory information */
    if (hasAboutSection())
    {
        if(title.isEmpty())
            filecontents += "# " + name + " Protocol\n\n";
        else
            filecontents += "# " + title + "\n\n";

        if(!comment.isEmpty())
            filecontents += outputLongComment("", comment) + "\n\n";

        if(!version.isEmpty())
            filecontents += title + " Protocol version is " + version + ".\n\n";

        if(!api.isEmpty())
            filecontents += title + " Protocol API is " + api + ".\n\n";

    }

    for(int i = 0; i < alldocumentsinorder.size(); i++)
    {
        if(alldocumentsinorder.at(i) == NULL)
            continue;

        // If the particular documention is to be hidden
        if(alldocumentsinorder.at(i)->isHidden() && !showAllItems)
            continue;

        filecontents += alldocumentsinorder.at(i)->getTopLevelMarkdown(true, packetids);
        filecontents += "\n";
    }

    if (hasAboutSection())
        filecontents += getAboutSection(isBigEndian);

    // The title attribute, remove any emphasis characters. We only put this
    // out if we have a title page, this preserves the behavior before 2.14,
    // which did not have a title attribute
    if(!titlePage.isEmpty())
        file.write("Title:" + title.remove("*") + "\n\n");

    // Specific header-level definitions are required for LaTeX compatibility
    if (latexEnabled)
    {
        file.write("Base Header Level: 1 \n");  // Base header level refers to the HTML output format
        file.write("LaTeX Header Level: " + QString::number(latexHeader) + " \n"); // LaTeX header level can be set by user
        file.write("\n");
    }

    // Add stylesheet information (unless it is disabled entirely)
    if (!nocss)
    {
        // Open the style tag
        file.write("<style>\n");

        if(inlinecss.isEmpty())
            file.write(getDefaultInlinCSS());
        else
            file.write(inlinecss);

        // Close the style tag
        file.write("</style>\n");

        file.write("\n");
    }

    if(tableOfContents)
    {
        QString temp = getTableOfContents(filecontents);
        temp += "----------------------------\n\n";
        temp += filecontents;
        filecontents = temp;
    }

    if(!titlePage.isEmpty())
    {
        QString temp = titlePage;
        temp += "\n----------------------------\n\n";
        temp += filecontents;
        filecontents = temp;
    }

    // Add html page breaks at each ---
    filecontents.replace("\n---", "<div class=\"page-break\"></div>\n\n\n---");

    // Deal with the degrees symbol, which doesn't render in html
    filecontents.replace("°", "&deg;");

    file.write(filecontents);

    file.flush();

    QProcess process;
    QStringList arguments;

    // Write html documentation
    QString htmlfile = basepath + name + ".html";

    // Tell the QProcess to send stdout to a file, since that's how the script outputs its data
    process.setStandardOutputFile(QString(htmlfile));

    std::cout << "Writing HTML documentation to " << QDir::toNativeSeparators(htmlfile).toStdString() << std::endl;

    arguments << filename;   // The name of the source file
    #if defined(__APPLE__) && defined(__MACH__)
    process.start(QString("/usr/local/bin/MultiMarkdown"), arguments);
    #else
    process.start(QString("multimarkdown"), arguments);
    #endif
    process.waitForFinished();

    if (latexEnabled)
    {
        // Write LaTeX documentation
        QString latexFile = basepath + name + ".tex";

        std::cout << "Writing LaTeX documentation to " << latexFile.toStdString() << "\n";

        QProcess latexProcess;

        latexProcess.setStandardOutputFile(latexFile);

        arguments.clear();
        arguments << filename;
        arguments << "--to=latex";

        #if defined(__APPLE__) && defined(__MACH__)
        latexProcess.start(QString("/usr/local/bin/MultiMarkdown"), arguments);
        #else
        latexProcess.start(QString("multimarkdown"), arguments);
        #endif
        latexProcess.waitForFinished();
    }
}


/*!
 * Get the table of contents, based on the file contents
 * \param filecontents are the file contents (without a TOC)
 * \return the table of contents data
 */
QString ProtocolParser::getTableOfContents(const QString& filecontents)
{
    QString output;

    // The table of contents identifies all the heding references, which start
    // with #. Each heading reference is also prefaced by two line feeds. The
    // level of the heading (and toc line) is defined by the number of #s.
    QStringList headings = filecontents.split("\n\n#", QString::SkipEmptyParts);

    if(headings.count() > 0)
        output = "<toctitle id=\"tableofcontents\">Table of contents</toctitle>\n";

    for(int i = 0; i < headings.count(); i++)
    {
        QString refname;
        QString text;
        int level = 1;

        // Pull the first line out of the heading group
        QString line = headings.at(i);
        line = line.left(line.indexOf("\n"));

        int prolog;
        for(prolog = 0; prolog < line.count(); prolog++)
        {
            QChar symbol = line.at(prolog);

            if(symbol == '#')       // Count the level
                level++;
            else if(symbol == ' ')  // Remove leading spaces
                continue;
            else
            {
                // Remove the prolog and get out
                line = line.mid(prolog);
                break;
            }

        }// for all characters

        // Special check in case we got a weird one
        if(line.isEmpty())
            continue;

        // Now figure out the reference. It is either going to be given by some
        // embedded html, or by the id that markdown will put into the output html
        if(line.contains("<a name=") && line.contains("\""))
        {
            // The text is after the reference closure
            text = line.mid(line.indexOf("</a>")+4);

            // In this case the reference name is contained within quotes
            refname = line.mid(line.indexOf("\"")+1);
            refname = refname.left(refname.indexOf("\""));
        }
        else
        {
            // The text for the line is the line
            text = line;

            // In this case the reference name is the whole line, lower case, no spaces and other special characters
            refname = line.toLower();
            refname.remove(" ");
            refname.remove("(");
            refname.remove(")");
            refname.remove("{");
            refname.remove("}");
            refname.remove("[");
            refname.remove("]");
            refname.remove("`");
            refname.remove("\"");
            refname.remove("*");
        }

        // Finally the actual line
        output += "<toc"+QString::number(level)+"><a href=\"#" + refname + "\">" + text + "</a></toc"+QString::number(level)+">\n";

    }// for all headings

    return output + "\n";

}// ProtocolParser::getTableOfContents


/*!
 * Return the string that describes the about section
 * \param isBigEndian should be true to describe a big endian protocol
 * \return the about section contents
 */
QString ProtocolParser::getAboutSection(bool isBigEndian)
{
    QString output;

    output += "----------------------------\n\n";
    output += "# About this ICD\n";
    output += "\n";

    output += "This is the interface control document for data *on the wire*, \
not data in memory. This document was automatically generated by the [ProtoGen software](https://github.com/billvaglienti/ProtoGen), \
version " + ProtocolParser::genVersion + ". ProtoGen also generates C source code for doing \
most of the work of encoding data from memory to the wire, and vice versa.\n";
    output += "\n";

    output += "## Encodings\n";
    output += "\n";

    if(isBigEndian)
        output += "Data for this protocol are sent in BIG endian format. Any field larger than one byte is sent with the most signficant byte first, and the least significant byte last.\n\n";
    else
        output += "Data for this protocol are sent in LITTLE endian format. Any field larger than one byte is sent with the least signficant byte first, and the most significant byte last. However bitfields are always sent most significant bits first.\n\n";

    output += "Data can be encoded as unsigned integers, signed integers (two's complement), bitfields, and floating point.\n";
    output += "\n";

    output += "\
| <a name=\"Enc\"></a>Encoding | Interpretation                        | Notes                                                                       |\n\
| :--------------------------: | ------------------------------------- | --------------------------------------------------------------------------- |\n\
| UX                           | Unsigned integer X bits long          | X must be: 8, 16, 24, 32, 40, 48, 56, or 64                                 |\n\
| IX                           | Signed integer X bits long            | X must be: 8, 16, 24, 32, 40, 48, 56, or 64                                 |\n\
| BX                           | Unsigned integer bitfield X bits long | X must be greater than 0 and less than 32                                   |\n\
| F16:X                        | 16 bit float with X significand bits  | 1 sign bit : 15-X exponent bits : X significant bits with implied leading 1 |\n\
| F24:X                        | 24 bit float with X significand bits  | 1 sign bit : 23-X exponent bits : X significant bits with implied leading 1 |\n\
| F32                          | 32 bit float (IEEE-754)               | 1 sign bit : 8 exponent bits : 23 significant bits with implied leading 1   |\n\
| F64                          | 64 bit float (IEEE-754)               | 1 sign bit : 11 exponent bits : 52 significant bits with implied leading 1  |\n";
    output += "\n";

    output += "## Size of fields";
    output += "\n";

    output += "The encoding tables give the bytes for each field as X...Y; \
where X is the first byte (counting from 0) and Y is the last byte. For example \
a 4 byte field at the beginning of a packet will give 0...3. If the field is 1 \
byte long then only the starting byte is given. Bitfields are more complex, they \
are displayed as Byte:Bit. A 3-bit bitfield at the beginning of a packet \
will give 0:7...0:5, indicating that the bitfield uses bits 7, 6, and 5 of byte \
0. Note that the most signficant bit of a byte is 7, and the least signficant \
bit is 0. If the bitfield is 1 bit long then only the starting bit is given.\n";
    output += "\n";

    output += "The byte count in the encoding tables are based on the maximum \
length of the field(s). If a field is variable length then the actual byte \
location of the data may be different depending on the value of the variable \
field. If the field is a variable length character string this will be indicated \
in the encoding column of the table. If the field is a variable length array \
this will be indicated in the repeat column of the encoding table. If the field \
depends on the non-zero value of another field this will be indicated in the \
description column of the table.\n";
    output += "\n";

    return output;
}


/*!
 * Determine if the field contains a given label, and the value is either {'true','yes','1'}
 * \param e is the element from the DOM to test
 * \param label is the name of the attribute form the element to test
 * \return true if the attribute value is "true", "yes", or "1"
 */
bool ProtocolParser::isFieldSet(const QDomElement &e, QString label)
{
    return isFieldSet(e.attribute(label).trimmed().toLower());
}


/*!
 * Determine if the value of an attribute is either {'true','yes','1'}
 * \param attribname is the name of the attribute to test
 * \param map is the list of all attributes to search
 * \return true if the attribute value is "true", "yes", or "1"
 */
bool ProtocolParser::isFieldSet(QString attribname, QDomNamedNodeMap map)
{
    return isFieldSet(ProtocolParser::getAttribute(attribname, map));
}


/*!
 * Determine if the value of an attribute is either {'true','yes','1'}
 * \param value is the attribute value to test
 * \return true if the attribute value is "true", "yes", or "1"
 */
bool ProtocolParser::isFieldSet(QString value)
{
    bool result = false;

    if (value.compare("true",Qt::CaseInsensitive) == 0)
        result = true;
    else if (value.compare("yes",Qt::CaseInsensitive) == 0)
        result = true;
    else if (value.compare("1",Qt::CaseInsensitive) == 0)
        result = true;

    return result;

}// ProtocolParser::isFieldSet


/*!
 * Determine if the field contains a given label, and the value is either {'false','no','0'}
 * \param e is the element from the DOM to test
 * \param label is the name of the attribute form the element to test
 * \return true if the attribute value is "false", "no", or "0"
 */
bool ProtocolParser::isFieldClear(const QDomElement &e, QString label)
{
    return isFieldClear(e.attribute(label).trimmed().toLower());
}


/*!
 * Determine if the value of an attribute is either {'false','no','0'}
 * \param value is the attribute value to test
 * \return true if the attribute value is "false", "no", or "0"
 */
bool ProtocolParser::isFieldClear(QString value)
{
    bool result = false;

    if (value.compare("false",Qt::CaseInsensitive) == 0)
        result = true;
    else if (value.compare("no",Qt::CaseInsensitive) == 0)
        result = true;
    else if (value.compare("0",Qt::CaseInsensitive) == 0)
        result = true;

    return result;
}


/*!
 * Get the string used for inline css. This must be bracketed in <style> tags in the html
 * \return the inline csss string
 */
QString ProtocolParser::getDefaultInlinCSS(void)
{
    QFile inlinecss(":/files/defaultcss.txt");

    inlinecss.open(QIODevice::ReadOnly | QIODevice::Text);
    QString css(inlinecss.readAll());

    inlinecss.close();

    return css;
}


/*!
 * \brief ProtocolParser::setDocsPath
 * Set the target path for writing output markdown documentation files
 * If no output path is set, then QDir::Current() is used
 * \param path
 */
void ProtocolParser::setDocsPath(QString path)
{
    if (QDir(path).exists())
        docsDir = path;
    else
        docsDir = "";
}


/*!
 * Output the doxygen HTML documentation
 */
void ProtocolParser::outputDoxygen(void)
{
    QString fileName = "ProtocolDoxyfile";

    QFile file(fileName);

    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        std::cerr << "Failed to open " << fileName.toStdString() << std::endl;
        return;
    }

    // This file allows us to have project name specific documentation in the
    // doxygen configuration file via the @INCLUDE directive
    file.write(qPrintable("PROJECT_NAME = \"" + name + " Protocol API\"\n"));
    file.close();

    // This is where the files are stored in the resources
    QString sourcePath = ":/files/prebuiltSources/";

    // Copy it to our working directory
    QFile::copy(sourcePath + "Doxyfile", "Doxyfile");

    // Launch the process
    QProcess process;

    // On the mac doxygen is a utility inside the Doxygen.app bundle.
    #if defined(__APPLE__) && defined(__MACH__)
    process.start(QString("/Applications/Doxygen.app/Contents/Resources/doxygen"), QStringList("Doxyfile"));
    #else
    process.start(QString("doxygen"), QStringList("Doxyfile"));
    #endif
    process.waitForFinished();

    // Delete our temporary files
    ProtocolFile::deleteFile("Doxyfile");
    ProtocolFile::deleteFile(fileName);
}
