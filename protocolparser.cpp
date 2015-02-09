#include "protocolparser.h"
#include "protocolpacket.h"
#include "enumcreator.h"
#include "protocolscaling.h"
#include "fieldcoding.h"
#include "protocolsupport.h"
#include <QDomDocument>
#include <QFile>
#include <QFileDevice>
#include <QDateTime>
#include <QStringList>
#include <QProcess>
#include <iostream>

// The version of the protocol generator is set here
const QString ProtocolParser::genVersion = "1.1.0.b testing";

// A static list of parsed structures
QList<ProtocolStructureModule*> ProtocolParser::structures;

// A static list of parsed packets
QList<ProtocolPacket*> ProtocolParser::packets;

// A static list of parsed enumerations
QList<EnumCreator*> ProtocolParser::enums;

/*!
 * \brief ProtocolParser::ProtocolParser
 */
ProtocolParser::ProtocolParser()
{
    // wipe any previously exisiting static data
    clear();
}


/*!
 * Destroy the protocol parser object
 */
ProtocolParser::~ProtocolParser()
{
    // Write anything out that might be pending
    header.flush();

    clear();
}


/*!
 * Clear any data from the protocol parser, including static data
 */
void ProtocolParser::clear(void)
{
    header.clear();
    name.clear();
    prefix.clear();
    version.clear();
    api.clear();

    // Empty the static list
    for(int i = 0; i < structures.size(); i++)
    {
        if(structures[i] != NULL)
        {
            delete structures[i];
            structures[i] = NULL;
        }
    }

    structures.clear();

    // Empty the static list
    for(int i = 0; i < packets.size(); i++)
    {
        if(packets[i] != NULL)
        {
            delete packets[i];
            packets[i] = NULL;
        }
    }

    packets.clear();

    // Empty the static list
    for(int i = 0; i < enums.size(); i++)
    {
        if(enums[i] != NULL)
        {
            delete enums[i];
            enums[i] = NULL;
        }
    }

    enums.clear();

}


/*!
 * Parse the DOM from the xml file. This kicks off the auto code generation for the protocol
 * \param doc is the DOM from the xml file.
 * \param nodoxygen should be true to skip the doxygen generation.
 * \param nomarkdown should be true to skip the markdown generation.
 * \param nohelperfiles should be true to skip generating helper source files.
 * \return true if something was written to a file
 */
bool ProtocolParser::parse(const QDomDocument& doc, bool nodoxygen, bool nomarkdown, bool nohelperfiles)
{
    ProtocolSupport support;

    // The outer most element
    QDomElement docElem = doc.documentElement();

    // This element must have the "Protocol" tag
    if(docElem.tagName() != "Protocol")
    {
        std::cout << "Protocol tag not found in XML" << std::endl;
        return false;
    }

    name = docElem.attribute("name").trimmed();
    if(name.isEmpty())
    {
        std::cout << "Protocol name not found in Protocol tag" << std::endl;
        return false;
    }

    // 64-bit support can be turned off
    if(docElem.attribute("supportInt64").contains("false", Qt::CaseInsensitive))
        support.int64 = false;

    // double support can be turned off
    if(docElem.attribute("supportFloat64").contains("false", Qt::CaseInsensitive))
        support.float64 = false;

    // special float support can be turned off
    if(docElem.attribute("supportSpecialFloat").contains("false", Qt::CaseInsensitive))
        support.specialFloat = false;

    // bitfield support can be turned off
    if(docElem.attribute("supportBitfield").contains("false", Qt::CaseInsensitive))
        support.bitfield = false;

    // Prefix is not required
    prefix = docElem.attribute("prefix").trimmed();

    bool bigendian = true;
    if(docElem.attribute("endian").contains("little", Qt::CaseInsensitive))
        bigendian = false;

    // Build the top level module
    if(!createProtocolFiles(docElem))
        return false;

    // All of the top level Structures, which stand alone in their own modules
    QList<QDomNode> structlist = childElementsByTagName(docElem, "Structure");

    // All of the top level packets. Packets can only be at the top level
    QList<QDomNode> packetlist = childElementsByTagName(docElem, "Packet");

    // Delete the files we are going to create so we don't have to worry about appending when we shouldn't
    for(int i = 0; i < structlist.size(); i++)
    {
        QDomElement e = structlist.at(i).toElement();

        QString moduleName = e.attribute("file");
        if(moduleName.isEmpty())
            moduleName = e.attribute("name");

        ProtocolFile::deleteModule(moduleName);

    }// for all structures

    for(int i = 0; i < packetlist.size(); i++)
    {
        QDomElement e = packetlist.at(i).toElement();
        QString moduleName = e.attribute("file");
        if(moduleName.isEmpty())
            moduleName = e.attribute("name") + "Packet";

        ProtocolFile::deleteModule(moduleName);

    }// for all packets

    for(int i = 0; i < structlist.size(); i++)
    {
        // Create the module object
        ProtocolStructureModule* module = new ProtocolStructureModule(name, prefix, support, api, version, bigendian);

        // Parse its XML
        module->parse(structlist.at(i).toElement());

        // Keep it around, but only if we got something for it
        if(module->encodedLength.isEmpty())
            delete module;
        else
            structures.push_back(module);

    }// for all top level structures
    structlist.clear();

    // Create the packet files
    for(int i = 0; i < packetlist.size(); i++)
    {
        // Create the module object
        ProtocolPacket* packet = new ProtocolPacket(name, prefix, support, api, version, bigendian);

        // Parse its XML
        packet->parse(packetlist.at(i).toElement());

        // Keep it around
        packets.push_back(packet);

    }// for all packets
    packetlist.clear();

    if(!nohelperfiles)
    {
        // Auto-generated files for coding
        ProtocolScaling(support).generate();
        FieldCoding(support).generate();

        // Copy the resource files
        // This is where the files are stored in the resources
        QString sourcePath = ":/files/prebuiltSources/";

        QStringList fileNames;
        if(support.specialFloat)
            fileNames << "floatspecial.c" << "floatspecial.h";

        if(support.bitfield)
            fileNames << "bitfieldspecial.c" << "bitfieldspecial.h";

        for(int i = 0; i < fileNames.length(); i++)
        {
            ProtocolFile::deleteFile(fileNames[i]);
            QFile::copy(sourcePath + fileNames[i], fileNames[i]);
        }
    }

    #ifndef _DEBUG
    if(!nomarkdown)
        outputMarkdown(bigendian);

    if(!nodoxygen)
        outputDoxygen();
    #endif

    return true;

}// ProtocolParser::parse


/*!
 * Create the header file for the top level module of the protocol
 * \param docElem is the "protocol" element from the DOM
 * \return true if the generation happened, false if the DOM does not conform
 */
bool ProtocolParser::createProtocolFiles(const QDomElement& docElem)
{
    // If the name is "coollink" then make everything "coollinkProtocol"
    QString nameex = name + "Protocol";

    // The file names
    header.setModuleName(nameex);

    comment = ProtocolParser::getComment(docElem);

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
    outputIncludes(header, docElem);

    // Output enumerations
    outputEnumerations(header, docElem);

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

    return true;

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
    // Remove leading and trailing white space
    QString output = text.trimmed();

    // Convert to unix style line endings, just in case
    output.replace("\r\n", "\n");

    // Separate the paragraphs, as given by dual line feeds
    QStringList paragraphs = output.split("\n\n", QString::SkipEmptyParts);

    // Empty the output string so we can build the output up
    output.clear();

    for(int i = 0; i < paragraphs.size(); i++)
    {
        // Replace line feeds with spaces
        paragraphs[i].replace("\n", " ");

        int length = 0;

        // Break it apart into words
        QStringList list = paragraphs[i].split(' ', QString::SkipEmptyParts);

        // Now write the words one at a time, wrapping at 80 character length
        for (int j = 0; j < list.size(); j++)
        {
            // Length of the word plus the preceding space
            int wordLength = list.at(j).length() + 1;

            if((length != 0) && (length + wordLength > 80))
            {
                // Terminate the previous line
                output += "\n";
                length = 0;
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

    return output;

}// ProtocolParser::outputLongComment


/*!
 * Get a correctly reflowed comment from a DOM
 * \param e is the DOM to get the comment from
 * \return the correctly reflowed comment, which could be empty
 */
QString ProtocolParser::getComment(const QDomElement& e)
{
    QString comment = e.attribute("comment");
    if(comment.isEmpty())
        return comment;
    else
        return ProtocolParser::reflowComment(comment);
}

/*!
 * Take a comment line which may have some unusual spaces and line feeds that
 * came from the xml formatting and reflow it for our needs.
 * \param text is the raw comment from the xml.
 * \return the reflowed comment.
 */
QString ProtocolParser::reflowComment(const QString& text)
{
    // Remove leading and trailing white space
    QString comment = text.trimmed();
    QString output;

    // Convert to unix style line endings, just in case
    comment.replace("\r\n", "\n");

    // Split into paragraphs, which are separated by dual line endings
    QStringList paragraphs = comment.split("\n\n", QString::SkipEmptyParts);

    for(int i = 0; i < paragraphs.size(); i++)
    {
        // Replace line feeds with spaces, this is the first part of the reflow
        paragraphs[i].replace("\n", " ");

        // Split based on spaces. Multiple spaces will be discarded, this is the second part of the reflow
        QStringList list = paragraphs[i].split(" ", QString::SkipEmptyParts);

        // Put it back together into one string
        int j;
        for(j = 0; j < list.size() - 1; j++)
            output += list.at(j) + " ";
        output += list.at(j);

        // If we have multiple paragraphs, put the paragraph separator back in
        if(i < paragraphs.size() - 1)
            output += "\n\n";
    }

    return output;
}


/*!
 * Return a list of QDomNodes that are direct children and have a specific tag
 * name. This is different then elementsByTagName() because that gets all
 * descendants, including grand-children. We just want children.
 * \param node is the parent node
 * \param tag is the tag name to look for
 * \return a list of QDomNodes.
 */
QList<QDomNode> ProtocolParser::childElementsByTagName(const QDomNode& node, QString tag)
{
    QList<QDomNode> list;

    // All the direct children
    QDomNodeList children = node.childNodes();

    // Now remove any whose tag is wrong
    for (int i = 0; i < children.size(); ++i)
    {
        if(children.item(i).nodeName().contains(tag, Qt::CaseInsensitive))
        {
            list.append(children.item(i));
        }
    }

    return list;

}// ProtocolParser::childElementsByTagName


/*!
 * Parse and output all enumerations which are direct children of a DomNode
 * \param file receives the output
 * \param node is parent node
 */
void ProtocolParser::outputEnumerations(ProtocolFile& file, const QDomNode& node)
{
    // Build the top level enumerations
    QList<QDomNode>list = childElementsByTagName(node, "Enum");

    for(int i = 0; i < list.size(); i++)
    {
        EnumCreator* Enum = new EnumCreator(list.at(i).toElement());

        if(Enum->output.isEmpty())
            delete Enum;
        else
        {
            // Output the enumerations to my header
            file.makeLineSeparator();
            file.write(Enum->output);

            // Keep track of my enumeration list
            enums.append(Enum);
        }

    }// for all my enum tags

}// ProtocolParser::outputEnumerations


/*!
 * Output all include directions which are direct children of a DomNode
 * \param file receives the output
 * \param node is parent node
 */
void ProtocolParser::outputIncludes(ProtocolFile& file, const QDomNode& node)
{
    // Build the top level enumerations
    QList<QDomNode>list = childElementsByTagName(node, "Include");
    for(int i = 0; i < list.size(); i++)
    {
        QDomElement e = list.at(i).toElement();

        QString include = e.attribute("name");
        if(!include.isEmpty())
        {
            QString comment = ProtocolParser::getComment(e);
            file.writeIncludeDirective(include, comment);
        }
    }

}// ProtocolParser::outputIncludes


/*!
 * Find the include name for a specific global structure type
 * \param typeName is the type to lookup
 * \return the file name to be included to reference this global structure type
 */
QString ProtocolParser::lookUpIncludeName(const QString& typeName)
{
    for(int i = 0; i < structures.size(); i++)
    {
        if(structures[i]->typeName == typeName)
        {
            return structures[i]->getHeaderFileName();
        }
    }

    return "";
}


/*!
 * Get the markdown documentation for a specific global structure type.
 * Only return the sub fields documentation.
 * \param typeName is the type to lookup
 * \param indent is the indentation string to use for this list.
 * \return the markdown documentation
 */
QString ProtocolParser::getStructureSubMarkdown(const QString& typeName, QString indent)
{
    for(int i = 0; i < structures.size(); i++)
    {
        if(structures[i]->typeName == typeName)
        {
            return structures[i]->getMarkdown(indent);
        }
    }


    for(int i = 0; i < packets.size(); i++)
    {
        if(packets[i]->typeName == typeName)
        {
            return packets[i]->getMarkdown(indent);
        }
    }

    return QString();
}


/*!
 * Ouptut documentation for the protocol as a markdown file
 */
void ProtocolParser::outputMarkdown(bool isBigEndian)
{
    QString fileName = name + ".markdown";
    QFile file(fileName);

    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        std::cout << "Failed to open markdown file: " << fileName.toStdString() << std::endl;
        return;
    }

    // Some css to make the html a output a little prettier
    file.write("\
<head>\n\
<style>\n\
body {\n\
    text-align:justify;\n\
    width: 800px;\n\
    background-color:#F0F0F0;\n\
    margin-left: auto;\n\
    margin-right: auto;\n\
}\n\
section#text {\n\
    background-color: #DDD;\n\
    padding: 5px 20px;\n\
}\n\
</style>\n\
</head>\n\
<section id=\"text\">\n\n");

    file.write(qPrintable("# " + name + " Protocol\n"));
    file.write("\n");

    if(!comment.isEmpty())
    {
        file.write(qPrintable(outputLongComment("", comment) + "\n"));
        file.write("\n");
    }

    file.write(qPrintable("This is the interface control document for data *on the wire*, \
not data in memory. This document was automatically generated by the ProtoGen software, \
version " + ProtocolParser::genVersion + ". ProtoGen also generates C source code for doing \
most of the hard work of encoding and decoding data from memory to the wire, and vice versa. \
Documentation for software developers (i.e. data *in memory*) is separately produced as a \
doxygen product, parsing comments embedded in the automatically generated code.\n"));
    file.write("\n");

    if(isBigEndian)
        file.write("Data *on the wire* are sent in BIG ENDIAN format. Any field composed of multiple bytes (e.g. `uint32_t`) is sent with the most signficant byte first, and the least significant byte last\n");
    else
        file.write("Data *on the wire* are sent in LITTLE ENDIAN format. Any field composed of multiple bytes (e.g. `uint32_t`) is sent with the least signficant byte first, and the most significant byte last\n");
    file.write("\n");

    if(!version.isEmpty())
    {
        file.write(qPrintable(name + " Protocol version is " + version + "\n"));
        file.write("\n");
    }

    if(!api.isEmpty())
    {
        file.write(qPrintable(name + " Protocol API is " + api + "\n"));
        file.write("\n");
    }

    if(enums.size() > 0)
    {
        file.write("# Enumerations\n");
        file.write("\n");
        file.write(qPrintable(name + " Protocol defines global enumerations. \
Enumerations are lists of identifiers commonly used as packet \
identifiers or array indices. Enumerations can also be defined by individual \
packets or strctures.\n"));
        file.write("\n");
        for(int i = 0; i < enums.size(); i++)
        {
            if(enums[i] == NULL)
                continue;

            file.write(qPrintable(enums[i]->getMarkdown(QString())));
            file.write("\n");
        }
    }

    if(packets.size() > 0)
    {
        file.write("# Packets\n");
        file.write("\n");
        file.write(qPrintable(name + " Protocol defines packets. A packet is a \
hunk of data and the rules for encoding that data for transmission on the wire.\n"));
        file.write("\n");

        for(int i = 0; i < packets.size(); i++)
        {
            if(packets[i] == NULL)
                continue;

            file.write(qPrintable(packets[i]->getTopLevelMarkdown(QString())));
            file.write("\n");
        }
    }

    if(structures.size() > 0)
    {
        file.write("# Structures");
        file.write("\n");
        file.write(qPrintable(name + " Protocol defines global structures. \
Structures are similar to packets but are intended to be reusable encodings \
that are contained in one or more packets. The information in this section \
may be repeating information already presented in the packets section\n"));
        file.write("\n");

        for(int i = 0; i < structures.size(); i++)
        {
            if(structures[i] == NULL)
                continue;

            file.write(qPrintable(structures[i]->getTopLevelMarkdown(QString())));
            file.write("\n");
        }
    }


    // Close out the css section
    file.write("</section>\n");

    file.close();

    // The markdown perl script
    QFile::copy(":/files/Markdown.pl", "Markdown.pl");

    QProcess process;
    QStringList arguments;
    arguments << "Markdown.pl"; // Markdown perl script
    arguments << fileName;      // The name of the source file

    // Tell the QProcess to send stdout to a file, since that's how the script outputs its data
    process.setStandardOutputFile(QString(name + ".html"));

    process.start(QString("perl"), arguments);
    process.waitForFinished();

    // Don't need to leave this one hanging around
    ProtocolFile::deleteFile("Markdown.pl");

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
        std::cout << "Failed to open " << fileName.toStdString() << std::endl;
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
