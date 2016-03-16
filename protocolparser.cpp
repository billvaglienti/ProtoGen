#include "protocolparser.h"
#include "protocolpacket.h"
#include "enumcreator.h"
#include "protocolscaling.h"
#include "fieldcoding.h"
#include "protocolsupport.h"
#include <QDomDocument>
#include <QFile>
#include <QDir>
#include <QFileDevice>
#include <QDateTime>
#include <QStringList>
#include <QProcess>
#include <iostream>

// The version of the protocol generator is set here
const QString ProtocolParser::genVersion = "1.3.0.a";

// A static list of parsed structures
QList<ProtocolStructureModule*> ProtocolParser::structures;

// A static list of parsed packets
QList<ProtocolPacket*> ProtocolParser::packets;

// A static list of parsed enumerations
QList<EnumCreator*> ProtocolParser::enums;

// A static list of parsed global enumerations
QList<EnumCreator*> ProtocolParser::globalEnums;

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

    latexEnabled = false; //Needs the -latex switch to enable

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

    // enums and globalEnums contain the same pointers
    enums.clear();
    globalEnums.clear();
}


/*!
 * Parse the DOM from the xml file. This kicks off the auto code generation for the protocol
 * \param doc is the DOM from the xml file.
 * \param nodoxygen should be true to skip the doxygen generation.
 * \param nomarkdown should be true to skip the markdown generation.
 * \param nohelperfiles should be true to skip generating helper source files.
 * \param inlinecss is the css to use for the markdown output, if blank use default.
 * \return true if something was written to a file
 */
bool ProtocolParser::parse(const QDomDocument& doc, bool nodoxygen, bool nomarkdown, bool nohelperfiles, QString inlinecss)
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

    if(!nomarkdown)
        outputMarkdown(bigendian, inlinecss);

    #ifndef _DEBUG
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
    header.setVersionOnly(true);

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
    outputIncludes(header, docElem);

    // Output enumerations
    parseEnumerations(docElem);
    outputEnumerations(header);

    // At this point the list of enums are the globals, we want to track those
    // separately from the next set of enums that come from packets and structures
    for(int i = 0; i < enums.size(); i++)
        globalEnums.append(enums.at(i));

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

        }// if block is not verbatim

    }// for all blocks

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

    }// for all paragraphs in a block

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
 * Parse all enumerations which are direct children of a DomNode. The
 * enumerations will be stored in the global list
 * \param node is parent node
 */
void ProtocolParser::parseEnumerations(const QDomNode& node)
{
    // Build the top level enumerations
    QList<QDomNode>list = childElementsByTagName(node, "Enum");

    for(int i = 0; i < list.size(); i++)
    {
        parseEnumeration(list.at(i).toElement());

    }// for all my enum tags

}// ProtocolParser::parseEnumerations


/*!
 * Parse a single enumeration given by a DOM element. This will also
 * add the enumeration to the global list which can be searched with
 * lookUpEnumeration().
 * \param element is the QDomElement that represents this enumeration
 * \return a pointer to the newly created enumeration object.
 */
const EnumCreator* ProtocolParser::parseEnumeration(const QDomElement& element)
{
    EnumCreator* Enum = new EnumCreator(element);

    if(!Enum->getOutput().isEmpty())
        enums.append(Enum);

    return Enum;

}// ProtocolParser::parseEnumeration


/*!
 * Output all enumerations in the global list
 * \param file receives the output
 */
void ProtocolParser::outputEnumerations(ProtocolFile& file)
{
    for(int i = 0; i < enums.size(); i++)
    {
        // Output the enumerations to my header
        file.makeLineSeparator();
        file.write(enums.at(i)->getOutput());

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
            file.writeIncludeDirective(include, comment, e.attribute("global") == "true");
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
const ProtocolStructure* ProtocolParser::lookUpStructure(const QString& typeName)
{
    for(int i = 0; i < structures.size(); i++)
    {
        if(structures[i]->typeName == typeName)
        {
            return structures[i];
        }
    }


    for(int i = 0; i < packets.size(); i++)
    {
        if(packets[i]->typeName == typeName)
        {
            return packets[i];
        }
    }

    return NULL;
}


/*!
 * Find the enumeration creator and return a constant pointer to it
 * \param enumName is the name of the enumeration.
 * \return A pointer to the enumeration creator, or NULL if it cannot be found
 */
const EnumCreator* ProtocolParser::lookUpEnumeration(const QString& enumName)
{
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
QString& ProtocolParser::replaceEnumerationNameWithValue(QString& text)
{
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
QString ProtocolParser::getEnumerationNameForEnumValue(const QString& text)
{
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
void ProtocolParser::getStructureSubDocumentationDetails(QString typeName, QList<int>& outline, QString& startByte, QStringList& bytes, QStringList& names, QStringList& encodings, QStringList& repeats, QStringList& comments)
{
    for(int i = 0; i < structures.size(); i++)
    {
        if(structures[i]->typeName == typeName)
        {
            return structures[i]->getSubDocumentationDetails(outline, startByte, bytes, names, encodings, repeats, comments);
        }
    }


    for(int i = 0; i < packets.size(); i++)
    {
        if(packets[i]->typeName == typeName)
        {
            return packets[i]->getSubDocumentationDetails(outline, startByte, bytes, names, encodings, repeats, comments);
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
    int paragraph1 = 1, paragraph2 = 1;

    QString basepath;

    if (!docsDir.isEmpty())
        basepath = docsDir + QDir::separator();

    QString filename = basepath + name + ".markdown";

    ProtocolFile file(filename);

    //Adding this metadata improves LaTeX support
    file.write("latex input: mmd-article-header \n");

    // Metadata must appear at the top
    file.write("Title: " + name + " Protocol  \n");

    //Adding this metadata improves LaTeX support
    file.write("Base Header Level: 2 \n");
    file.write("latex input: mmd-article-begin-doc\n");

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
    file.write("# " + QString().setNum(paragraph1) + ") " + name + " Protocol\n");
    file.write("\n");

    if(!comment.isEmpty())
    {
        file.write(outputLongComment("", comment) + "\n");
        file.write("\n");
    }

    if(isBigEndian)
        file.write("Data *on the wire* are sent in BIG endian format. Any field larger than one byte is sent with the most signficant byte first, and the least significant byte last\n");
    else
        file.write("Data *on the wire* are sent in LITTLE endian format. Any field larger than one byte is sent with the least signficant byte first, and the most significant byte last\n");
    file.write("\n");

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

    paragraph1++;
    paragraph2 = 1;
    file.write("----------------------------\n\n");

    file.write("# " + QString().setNum(paragraph1) + ") About this ICD\n");
    file.write("\n");

    file.write("This is the interface control document for data *on the wire*, \
not data in memory. This document was automatically generated by the [ProtoGen software](https://github.com/billvaglienti/ProtoGen), \
version " + ProtocolParser::genVersion + ". ProtoGen also generates C source code for doing \
most of the work of encoding data from memory to the wire, and vice versa. \
Documentation for software developers (i.e. data *in memory*) is separately produced as a \
doxygen product, parsing comments embedded in the automatically generated code.\n");
    file.write("\n");

    file.write("# " + QString().setNum(paragraph1) + "." + QString().setNum(paragraph2++) + ") Encodings\n");
    file.write("\n");

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

    file.write("# " + QString().setNum(paragraph1) + "." + QString().setNum(paragraph2++) + ") Size of fields");
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
depends on the non-zero value of another field then will be indicated in the \
description column of the table.\n");
    file.write("\n");

    paragraph1++;
    paragraph2 = 1;
    file.write("----------------------------\n\n");
    if(globalEnums.size() > 0)
    {
        QStringList packetids;
        for(int i = 0; i < packets.size(); i++)
        {
            if(packets[i] == NULL)
                continue;

            packetids.append(packets.at(i)->getId());
        }

        file.write("# " + QString().setNum(paragraph1) + ") Enumerations\n");
        file.write("\n");
        file.write(name + " protocol defines these global enumerations.\n");
        file.write("\n");
        for(int i = 0; i < globalEnums.size(); i++)
        {
            if(globalEnums[i] == NULL)
                continue;

            file.write(globalEnums[i]->getMarkdown(QString().setNum(paragraph1) + "." + QString().setNum(paragraph2++), packetids));
            file.write("\n");
        }
    }

    paragraph1++;
    paragraph2 = 1;
    file.write("----------------------------\n\n");
    if(packets.size() > 0)
    {
        file.write("# " + QString().setNum(paragraph1) + ") Packets\n");
        file.write("\n");
        file.write("This section describes the data payloads of the packets; and how those data are represented in the bytes of the packets.\n");
        file.write("\n");

        for(int i = 0; i < packets.size(); i++)
        {
            if(packets[i] == NULL)
                continue;

            file.write(packets[i]->getTopLevelMarkdown(QString().setNum(paragraph1) + "." + QString().setNum(paragraph2++)));
            file.write("\n");
        }

        paragraph1++;
        paragraph2 = 1;
    }

    /*
    paragraph1++;
    paragraph2 = 1;
    file.write("----------------------------\n\n");
    if(structures.size() > 0)
    {
        paragraph2 = 1;
        file.write("# " + QString().setNum(paragraph1) + ") Structures");
        file.write("\n");
        file.write(name + " Protocol defines global structures. \
Structures are similar to packets but are intended to be reusable encodings \
that are contained in one or more packets. The information in this section \
may be repeating information already presented in the packets section\n"));
        file.write("\n");

        for(int i = 0; i < structures.size(); i++)
        {
            if(structures[i] == NULL)
                continue;

            file.write(structures[i]->getTopLevelMarkdown(QString().setNum(paragraph1) + "." + QString.setNum(paragraph2++)));
            file.write("\n");
        }

        paragraph1++;
        paragraph2 = 1;
    }
    */

    file.flush();

    QProcess process;
    QStringList arguments;

    //Write html documentation
    QString htmlfile = basepath + name + ".html";

    // Tell the QProcess to send stdout to a file, since that's how the script outputs its data
    process.setStandardOutputFile(QString(htmlfile));

    std::cout << "Writing HTML documentation to " << htmlfile.toStdString() << std::endl;

    arguments << filename;   // The name of the source file
    #if defined(__APPLE__) && defined(__MACH__)
    process.start(QString("/usr/local/bin/MultiMarkdown"), arguments);
    #else
    process.start(QString("multimarkdown"), arguments);
    #endif
    process.waitForFinished();

    if (latexEnabled) {
        //Write LaTeX documentation
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
    }\n\
\n\
    table {\n\
       border: 3px solid darkred;\n\
       border-collapse: collapse;\n\
    }\n\
\n\
    th, td {\n\
       border: 1px solid green;\n\
       font-family: Courier New, monospace;\n\
    }\n\
\n\
    td{ padding: 2px; }\n\
    h1, h2, h3, h4, h5 { font-family: Arial; }\n\
    h1 { font-size:150%; }\n\
    h2 { font-size:135%; }\n\
    h3 { font-size:120%; }\n\
    h4 { font-size:110%; }\n\
    h5, li { font-size:100%; }\n\
    caption{ font-family:Verdana; }\n\
\n\
    code, pre, .codelike {\n\
        font-family: Courier New, monospace;\n\
        font-size: 100%;\n\
        color: darkblue;\n\
    }\n");

    return inlinecss;
}

/*!
 * \brief ProtocolParser::setDocsPath
 * Set the target path for writing output markdown documentation files
 * If no output path is set, then QDir::Current() is used
 * \param path
 */
void ProtocolParser::setDocsPath(QString path) {
    if (QDir(path).exists()) {
        docsDir = path;
    } else {
        docsDir = "";
    }
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
