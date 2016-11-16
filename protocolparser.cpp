#include "protocolparser.h"
#include "protocolpacket.h"
#include "enumcreator.h"
#include "protocolscaling.h"
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
const QString ProtocolParser::genVersion = "1.6.4.c";

/*!
 * \brief ProtocolParser::ProtocolParser
 */
ProtocolParser::ProtocolParser() :
    latexHeader(1),
    latexEnabled(false),
    nomarkdown(false),
    nohelperfiles(false),
    nodoxygen(false),
    showAllItems(false)
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
}


/*!
 * Parse the DOM from the xml file. This kicks off the auto code generation for the protocol
 * \param file is the already opened xml file to read the xml contents from
 * \param path is the output path for generated files
 * \return true if something was written to a file
 */
bool ProtocolParser::parse(QString filename, QString path)
{
    QFile file(filename);
    QFileInfo fileinfo(filename);

    // Remember the input path, in case there are files referenced by the main file
    inputpath = ProtocolFile::sanitizePath(fileinfo.absolutePath());

    // Also remember the name of the file, which we use for warning outputs
    inputfile = fileinfo.fileName();

    if (!file.open(QIODevice::ReadOnly))
    {
        emitWarning(" error: Failed to open protocol file");
        return false;
    }

    // The entire contents of the file, which we use twice
    QString contents(file.readAll());

    // Done with the file
    file.close();

    // Qt's XML parsing
    if(!doc.setContent(contents))
    {
        emitWarning(" error: Failed to validate xml from file");
        return false;
    }

    // My XML parsing, I can find no way to recover line numbers from Qt's parsing...
    line.setXMLContents(contents);

    // The outer most element
    QDomElement docElem = doc.documentElement();

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

    // This element must have the "Protocol" tag
    if(docElem.tagName() != "Protocol")
    {
        emitWarning(" error: Protocol tag not found in XML");
        return false;
    }

    name = docElem.attribute("name").trimmed();
    if(name.isEmpty())
    {
        emitWarning(" error: Protocol name not found in Protocol tag");
        return false;
    }

    // Protocol support options specified in the xml
    support.parse(docElem);

    // All of the top level Structures, which stand alone in their own modules
    QList<QDomNode> structlist = childElementsByTagName(docElem, "Structure");

    // All of the top level packets and top level documentation. Packets can only be at the top level
    QList<QDomNode> doclist = childElementsByTagName(docElem, "Packet", "Enum", "Documentation");

    // We want to create the objects first, then parse them later. That way we
    // can retain the order of their definition, but get enumerations and
    // structures to parse first
    for(int i = 0; i < structlist.size(); i++)
    {
        // Create the module object
        ProtocolStructureModule* module = new ProtocolStructureModule(this, name, support, api, version);

        // Remember its xml
        module->setElement(structlist.at(i).toElement());

        // Keep track of the structure
        structures.append(module);
    }
    structlist.clear();

    // Create the packets, enums, and documents
    for(int i = 0; i < doclist.size(); i++)
    {
        if(doclist.at(i).nodeName().contains("Packet", Qt::CaseInsensitive))
        {
            // Create the module object
            ProtocolPacket* packet = new ProtocolPacket(this, name, support, api, version);

            // Remember its xml
            packet->setElement(doclist.at(i).toElement());

            // Keep it around
            packets.append(packet);

            // Keep it in the master document list as well
            alldocumentsinorder.append(packet);
        }
        else if(doclist.at(i).nodeName().contains("Enum", Qt::CaseInsensitive))
        {
            EnumCreator* Enum = new EnumCreator(this, name, support);

            // Enums can be parsed now
            Enum->setElement(doclist.at(i).toElement());
            Enum->parse();

            // Keep it around in the global list
            if(!Enum->getOutput().isEmpty())
                globalEnums.append(Enum);

            // Keep it in the master document list as well
            alldocumentsinorder.append(Enum);
        }
        else
        {
            // Create the module object
            ProtocolDocumentation* document = new ProtocolDocumentation(this, name);

            // Documents can be parsed now
            document->setElement(doclist.at(i).toElement());
            document->parse();

            // Keep it around
            documents.append(document);

            // Keep it in the master document list as well
            alldocumentsinorder.append(document);
        }

    }// for all packets, enums, and documents
    doclist.clear();

    // The list of our output files
    QStringList fileNameList;

    // Now parse the global structures
    for(int i = 0; i < structures.size(); i++)
    {
        ProtocolStructureModule* module = structures[i];

        // Parse its XML
        module->parse();

        // Keep a list of all the file names
        fileNameList.append(module->getHeaderFileName());
        fileNameList.append(module->getSourceFileName());

    }// for all top level structures

    // And the global packets. We want to sort the packets into two batches:
    // those packets which can be used by other packets; and those which cannot.
    // This way we can parse the first batch ahead of the second
    for(int i = 0; i < packets.size(); i++)
    {
        ProtocolPacket* packet = packets[i];

        if(!isFieldSet(packet->getElement(), "useInOtherPackets"))
            continue;

        // Parse its XML
        packet->parse();

        // The structures have been parsed, adding this packet to the list
        // makes it available for other packets to find as structure reference
        structures.append(packet);

        // Keep a list of all the file names
        fileNameList.append(packet->getHeaderFileName());
        fileNameList.append(packet->getSourceFileName());
    }

    // And the packets which are not available for other packets
    for(int i = 0; i < packets.size(); i++)
    {
        ProtocolPacket* packet = packets[i];

        if(isFieldSet(packet->getElement(), "useInOtherPackets"))
            continue;

        // Parse its XML
        packet->parse();

        // Keep a list of all the file names
        fileNameList.append(packet->getHeaderFileName());
        fileNameList.append(packet->getSourceFileName());
    }

    // Build the top level module
    createProtocolFiles(docElem);

    fileNameList.append(header.fileName());

    if(!nohelperfiles)
    {
        // Auto-generated files for coding
        ProtocolScaling(support).generate();
        FieldCoding(support).generate();

        fileNameList.append("scaledencode.h");
        fileNameList.append("scaledencode.c");
        fileNameList.append("scaleddecode.h");
        fileNameList.append("scaleddecode.c");
        fileNameList.append("fieldencode.h");
        fileNameList.append("fieldencode.c");
        fileNameList.append("fielddecode.h");
        fileNameList.append("fielddecode.c");

        if(support.bitfield)
        {
            fileNameList.append("bitfieldspecial.h");
            fileNameList.append("bitfieldspecial.c");
            ProtocolBitfield(support).generate();
        }

        // Copy the resource files
        // This is where the files are stored in the resources
        QString sourcePath = ":/files/prebuiltSources/";

        QStringList fileNames;
        if(support.specialFloat)
        {
            fileNameList.append("floatspecial.h");
            fileNameList.append("floatspecial.c");
            fileNames << "floatspecial.c" << "floatspecial.h";
        }

        for(int i = 0; i < fileNames.length(); i++)
        {
            // ProtocolFile::deleteFile(fileNames[i]);
            QFile::copy(sourcePath + fileNames[i], support.outputpath + ProtocolFile::tempprefix + fileNames[i]);
        }
    }

    if(!nomarkdown)
        outputMarkdown(support.bigendian, inlinecss);

    #ifndef _DEBUG
    if(!nodoxygen)
        outputDoxygen();
    #endif

    header.flush();

    // This is fun...replace all the temporary files with real ones if needed
    fileNameList.removeDuplicates();
    for(int i = 0; i < fileNameList.count(); i++)
        ProtocolFile::copyTemporaryFile(support.outputpath, fileNameList.at(i));

    // If we are putting the files in our local directory then we don't just want an empty string in our printout
    if(path.isEmpty())
        path = "./";

    std::cout << "Generated protocol files in " << QDir::toNativeSeparators(path).toStdString() << std::endl;

    return true;

}// ProtocolParser::parse


void ProtocolParser::emitWarning(QString warning) const
{
    QString output = inputpath + inputfile + ":" + warning;
    std::cerr << output.toStdString() << std::endl;
}


/*!
 * Create the header file for the top level module of the protocol
 * \param docElem is the "protocol" element from the DOM
 */
void ProtocolParser::createProtocolFiles(const QDomElement& docElem)
{
    // If the name is "coollink" then make everything "coollinkProtocol"
    QString nameex = name + "Protocol";

    // The file names
    header.setModuleName(nameex);
    header.setPath(support.outputpath);

    comment = docElem.attribute("comment");

    // Comment block at the top of the header file
    header.write("/*!\n");
    header.write(" * \\file\n");
    header.write(" * \\mainpage " + name + " protocol stack\n");
    header.write(" *\n");

    // A long comment that should be wrapped at 80 characters
    outputLongComment(header, " *", comment);

    // The protocol enumeration API, which can be empty
    api = docElem.attribute("api");
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
    version = docElem.attribute("version");
    if(!version.isEmpty())
    {
        header.write("\n *\n");
        outputLongComment(header, " *", "The protocol version is " + version);
        header.write("\n");
    }

    header.write(" */\n");
    header.write("\n");

    // Includes
    header.write("#include <stdint.h>\n");

    // Add other includes
    outputIncludes(name, header, docElem);

    // Output the global enumerations to my header
    for(int i = 0; i < globalEnums.size(); i++)
    {
        header.makeLineSeparator();
        header.write(globalEnums.at(i)->getOutput());
    }

    // API functions
    if(!api.isEmpty())
    {
        header.write("\n");
        header.write("//! \\return the protocol API enumeration\n");
        header.write("#define get" + name + "Api() " + api + "\n");
    }

    // Version functions
    if(!version.isEmpty())
    {
        header.write("\n");
        header.write("//! \\return the protocol version string\n");
        header.write("#define get" + name + "Version() \""  + version + "\"\n");
    }

    header.write("\n");
    header.write("// The prototypes below provide an interface to the packets.\n");
    header.write("// They are not auto-generated functions, but must be hand-written\n");
    header.write("\n");
    header.write("//! \\return the packet data pointer from the packet\n");
    header.write("uint8_t* get" + name + "PacketData(void* pkt);\n");
    header.write("\n");
    header.write("//! \\return the packet data pointer from the packet, const\n");
    header.write("const uint8_t* get" + name + "PacketDataConst(const void* pkt);\n");
    header.write("\n");
    header.write("//! Complete a packet after the data have been encoded\n");
    header.write("void finish" + name + "Packet(void* pkt, int size, uint32_t packetID);\n");
    header.write("\n");
    header.write("//! \\return the size of a packet from the packet header\n");
    header.write("int get" + name + "PacketSize(const void* pkt);\n");
    header.write("\n");
    header.write("//! \\return the ID of a packet from the packet header\n");
    header.write("uint32_t get" + name + "PacketID(const void* pkt);\n");
    header.write("\n");

}// ProtocolParser::createProtocolFiles


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
    // We use name as part of our debug outputs, so its good to have it first.
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

    if(!Enum->getOutput().isEmpty())
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
                emitWarning(parent + ":" + include + ": Unrecognized attribute " + attrname);

        }// for all attributes

        if(!include.isEmpty())
            file.writeIncludeDirective(include, comment, global);

    }// for all include tags

}// ProtocolParser::outputIncludes


/*!
 * Find the include name for a specific global structure type
 * \param typeName is the type to lookup
 * \return the file name to be included to reference this global structure type
 */
QString ProtocolParser::lookUpIncludeName(const QString& typeName) const
{
    for(int i = 0; i < structures.size(); i++)
    {
        if(structures.at(i)->typeName == typeName)
        {
            return structures.at(i)->getHeaderFileName();
        }
    }

    for(int i = 0; i < packets.size(); i++)
    {
        if(packets.at(i)->typeName == typeName)
        {
            return packets.at(i)->getHeaderFileName();
        }
    }

    return "";
}


/*!
 * Find the global structure pointer for a specific type
 * \param typeName is the type to lookup
 * \return a pointer to the structure encodable, or NULL if it does not exist
 */
const ProtocolStructure* ProtocolParser::lookUpStructure(const QString& typeName) const
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

    ProtocolFile file(filename, false);

    file.write("Base Header Level: 1 \n");  // Base header level refers to the HTML output format
    file.write("LaTeX Header Level: " + QString::number(latexHeader) + " \n"); // LaTeX header level can be set by user

    file.write("\n");

    // Open the style tag
    file.write("<style>\n");

    if(inlinecss.isEmpty())
        file.write(getDefaultInlinCSS());
    else
        file.write(inlinecss);

    // Close the style tag
    file.write("</style>\n");

    file.write("\n");
    file.write("# " + name + " Protocol\n");
    file.write("\n");

    if(!comment.isEmpty())
    {
        file.write(outputLongComment("", comment) + "\n");
        file.write("\n");
    }

    if(!version.isEmpty())
    {
        file.write(name + " Protocol version is " + version + ".\n");
        file.write("\n");
    }

    if(!api.isEmpty())
    {
        file.write(name + " Protocol API is " + api + ".\n");
        file.write("\n");
    }

    file.write("----------------------------\n\n");

    QStringList packetids;
    for(int i = 0; i < packets.size(); i++)
        packetids.append(packets.at(i)->getId());

    for(int i = 0; i < alldocumentsinorder.size(); i++)
    {
        if(alldocumentsinorder.at(i) == NULL)
            continue;

        // If the particular documention is to be hidden
        if(alldocumentsinorder.at(i)->isHidden() && !showAllItems)
            continue;

        file.write(alldocumentsinorder.at(i)->getTopLevelMarkdown(true, packetids));
        file.write("\n");
    }

    file.write("----------------------------\n\n");
    file.write("# About this ICD\n");
    file.write("\n");

    file.write("This is the interface control document for data *on the wire*, \
not data in memory. This document was automatically generated by the [ProtoGen software](https://github.com/billvaglienti/ProtoGen), \
version " + ProtocolParser::genVersion + ". ProtoGen also generates C source code for doing \
most of the work of encoding data from memory to the wire, and vice versa.\n");
    file.write("\n");

    file.write("## Encodings\n");
    file.write("\n");

    if(isBigEndian)
        file.write("Data for this protocol are sent in BIG endian format. Any field larger than one byte is sent with the most signficant byte first, and the least significant byte last.\n\n");
    else
        file.write("Data for this protocol are sent in LITTLE endian format. Any field larger than one byte is sent with the least signficant byte first, and the most significant byte last. However bitfields are always sent most significant bits first.\n\n");

    file.write("Data can be encoded as unsigned integers, signed integers (two's complement), bitfields, and floating point.\n");
    file.write("\n");

    file.write("\
| <a name=\"Enc\"></a>Encoding | Interpretation                        | Notes                                                                       |\n\
| :--------------------------: | ------------------------------------- | --------------------------------------------------------------------------- |\n\
| UX                           | Unsigned integer X bits long          | X must be: 8, 16, 24, 32, 40, 48, 56, or 64                                 |\n\
| IX                           | Signed integer X bits long            | X must be: 8, 16, 24, 32, 40, 48, 56, or 64                                 |\n\
| BX                           | Unsigned integer bitfield X bits long | X must be greater than 0 and less than 32                                   |\n\
| F16                          | 16 bit floating point                 | 1 sign bit : 6 exponent bits : 9 significant bits with implied leading 1    |\n\
| F24                          | 24 bit floating point                 | 1 sign bit : 8 exponent bits : 15 significant bits with implied leading 1   |\n\
| F32                          | 32 bit floating point (IEEE-754)      | 1 sign bit : 8 exponent bits : 23 significant bits with implied leading 1   |\n\
| F64                          | 64 bit floating point (IEEE-754)      | 1 sign bit : 11 exponent bits : 52 significant bits with implied leading 1  |\n");
    file.write("\n");

    file.write("## Size of fields");
    file.write("\n");

    file.write("The encoding tables give the bytes for each field as X...Y; \
where X is the first byte (counting from 0) and Y is the last byte. For example \
a 4 byte field at the beginning of a packet will give 0...3. If the field is 1 \
byte long then only the starting byte is given. Bitfields are more complex, they \
are displayed as Byte:Bit. A 3-bit bitfield at the beginning of a packet \
will give 0:7...0:5, indicating that the bitfield uses bits 7, 6, and 5 of byte \
0. Note that the most signficant bit of a byte is 7, and the least signficant \
bit is 0. If the bitfield is 1 bit long then only the starting bit is given.\n");
    file.write("\n");

    file.write("The byte count in the encoding tables are based on the maximum \
length of the field(s). If a field is variable length then the actual byte \
location of the data may be different depending on the value of the variable \
field. If the field is a variable length character string this will be indicated \
in the encoding column of the table. If the field is a variable length array \
this will be indicated in the repeat column of the encoding table. If the field \
depends on the non-zero value of another field this will be indicated in the \
description column of the table.\n");
    file.write("\n");

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
    QString inlinecss("\
    body {\n\
        text-align:justify;\n\
        width: 1000px;\n\
        background-color:#eee;\n\
        margin-left: auto;\n\
        margin-right: auto;\n\
        font-family:Verdana;\n\
        counter-reset: h1counter;\
    }\n\
\n\
    table {\n\
       border: 2px solid darkred;\n\
       border-collapse: collapse;\n\
       margin-bottom: 25px;\n\
    }\n\
\n\
    th, td {\n\
       border: 1px solid green;\n\
       font-family: Courier New, monospace;\n\
    }\n\
\n\
    td{ padding: 2px; }\n\
    h1, h2, h3, h4, h5 { font-family: Arial; }\n\
    h1 { font-size:120%; margin-bottom: 25px;}\n\
    h2 { font-size:110%; margin-bottom: 15px; }\n\
    h3 { font-size:100%; margin-bottom: 10px;}\n\
    h4, li { font-size:100%; }\n\
    caption{ font-family:Verdana; }\n\
\n\
    hr{\n\
        color: black;\n\
        background-color: dimgray;\n\
        height: 1px;\n\
        margin-bottom:25px;\n\
        margin-top:25px;\n\
    }\n\
\n\
    code, pre, .codelike {\n\
        font-family: Courier New, monospace;\n\
        font-size: 100%;\n\
        color: darkblue;\n\
    }\n\
    h1:before {\n\
      content: counter(h1counter) \"\\00a0 \";\n\
      counter-increment: h1counter;\n\
      counter-reset: h2counter;\n\
    }\n\
    h1 {\n\
      counter-reset: h2counter;\n\
    }\n\
    h2:before {\n\
      content: counter(h1counter) \".\" counter(h2counter) \"\\00a0 \";\n\
      counter-increment: h2counter;\n\
      counter-reset: h3counter;\n\
    }\n\
    h3:before {\n\
      content: counter(h1counter) \".\" counter(h2counter) \".\" counter(h3counter) \"\\00a0 \";\n\
      counter-increment: h3counter;\n\
    }");

    return inlinecss;
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
