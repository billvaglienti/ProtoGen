#include "protocolparser.h"
#include "protocolstructuremodule.h"
#include "protocolpacket.h"
#include "enumcreator.h"
#include "protocolscaling.h"
#include "fieldcoding.h"
#include "protocolfloatspecial.h"
#include "protocolsupport.h"
#include "protocolbitfield.h"
#include "protocoldocumentation.h"
#include "shuntingyard.h"
#include <string>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <fstream>

// The version of the protocol generator is set here
const std::string ProtocolParser::genVersion = "3.1.c";

/*!
 * \brief ProtocolParser::ProtocolParser
 */
ProtocolParser::ProtocolParser() :
    currentxml(nullptr),
    header(nullptr),
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
    for(auto doc : documents)
        delete doc;
    documents.clear();

    for(auto struc : structures)
        delete struc;
    structures.clear();

    for(auto enu : enums)
        delete enu;
    enums.clear();

    for(auto globalenu : globalEnums)
        delete globalenu;
    globalEnums.clear();

    for(auto xmldoc : xmldocs)
        delete xmldoc;
    xmldocs.clear();

    if(header != nullptr)
        delete header;
}


/*!
 * Parse the DOM from the xml file. This kicks off the auto code generation for the protocol
 * \param filenames is the list of files to read the xml contents from. The
 *        first file is the master file that sets the protocol options
 * \param path is the output path for generated files
 * \return true if something was written to a file
 */
bool ProtocolParser::parse(std::string filename, std::string path, std::vector<std::string> otherfiles)
{
    // Top level printout of the version information
    std::cout << "ProtoGen version " << genVersion << std::endl;

    std::filesystem::path filepath(filename);

    // Remember the input path, in case there are files referenced by the main file
    if(filepath.has_parent_path())
        inputpath = ProtocolFile::sanitizePath(filepath.parent_path().string());
    else
        inputpath.clear();

    // Also remember the name of the file, which we use for warning outputs
    inputfile = filepath.filename().string();
    std::fstream file(filename, std::ios_base::in);

    if(!file.is_open())
    {
        std::cerr << filename << " : error: Failed to open protocol file" << std::endl;
        return false;
    }

    currentxml = new XMLDocument();
    xmldocs.push_back(currentxml);
    if(currentxml->Parse(std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()).c_str()) != XML_SUCCESS)
    {
        file.close();
        std::cerr << currentxml->ErrorStr() << std::endl;
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
    // so we must test path.empty() again. Note that in this case the
    // path definitely exists (since its our working directory)

    std::error_code ec;

    // Make sure the path exists
    if(!path.empty())
        std::filesystem::create_directories(path, ec);

    // Make sure the path exists
    if(!ec)
    {
        // Remember the output path for all users
        support.outputpath = path;
    }
    else
    {
        std::cerr << "error: Failed to create output path: " << path << std::endl;
        return false;
    }

    // The outer most element
    XMLElement* docElem = currentxml->RootElement();

    // This element must have the "Protocol" tag
    if((docElem == nullptr) || (XMLUtil::StringEqual(docElem->Name(), "protocol") == false))
    {
        std::cerr << filename << " : error: Protocol tag not found in XML" << std::endl;
        return false;
    }

    // Protocol options specified in the xml
    support.sourcefile = inputpath + inputfile;
    support.protoName = name = getAttribute("name", docElem->FirstAttribute());
    if(support.protoName.empty())
    {
        std::cerr << filename << " : error: Protocol name not found in XML" << std::endl;
        return false;
    }

    title = getAttribute("title", docElem->FirstAttribute());
    api = getAttribute("api", docElem->FirstAttribute());
    version = getAttribute("version", docElem->FirstAttribute());
    comment = getAttribute("comment", docElem->FirstAttribute());
    support.parse(docElem->FirstAttribute());

    if(support.disableunrecognized == false)
    {
        // All the attributes we understand
        std::vector<std::string> attriblist = {"name", "title", "api", "version", "comment"};

        // and the ones understood by the protocol support
        std::vector<std::string> supportlist = support.getAttriblist();

        for(const XMLAttribute* a = docElem->FirstAttribute(); a != nullptr; a = a->Next())
        {
            std::string test = trimm(a->Name());
            if(!contains(attriblist, test) && !contains(supportlist, test))
                std::cerr << support.sourcefile << ":" << a->GetLineNum() << ":0: warning: Unrecognized attribute \"" + test + "\"" << std::endl;

        }// for all the attributes

    }// if we need to warn for unrecognized attributes

    // The list of our output files
    std::vector<std::string> fileNameList;
    std::vector<std::string> filePathList;

    // Build the top level module
    createProtocolHeader(docElem);

    // And record its file name
    fileNameList.push_back(header->fileName());
    filePathList.push_back(header->filePath());

    // Now parse the contents of all the files. We do other files first since
    // we expect them to be helpers that the main file may depend on.
    for(std::size_t i = 0; i < otherfiles.size(); i++)
        parseFile(otherfiles.at(i));

    // Finally the main file
    parseFile(filename);

    // This is a resource file for bitfield testing
    if(support.bitfieldtest && support.bitfield)
    {
        std::fstream btestfile("bitfieldtester.xml", std::ios_base::out);

        // Raw string literal trick, I love this!
        std::string filedata = (
            #include "bitfieldtest.xml"
            );

        if(btestfile.is_open())
        {
            btestfile << filedata;
            btestfile.close();
            parseFile("bitfieldtester.xml");
            ProtocolFile::deleteFile("bitfieldtester.xml");
        }
    }

    // Output the global enumerations first, they will go in the main
    // header file by default, unless the enum specifies otherwise
    ProtocolHeaderFile enumfile(support);
    ProtocolSourceFile enumSourceFile(support);

    for(std::size_t i = 0; i < globalEnums.size(); i++)
    {
        EnumCreator* module = globalEnums.at(i);

        module->parseGlobal();
        enumfile.setModuleNameAndPath(module->getHeaderFileName(), module->getHeaderFilePath());

        if(support.supportbool && (support.language == ProtocolSupport::c_language))
            enumfile.writeIncludeDirective("stdbool.h", "", true);

        enumfile.write(module->getOutput());
        enumfile.makeLineSeparator();
        enumfile.flush();

        // If there is source-code available
        std::string source = module->getSourceOutput();

        if(!source.empty())
        {
            enumSourceFile.setModuleNameAndPath(module->getHeaderFileName(), module->getHeaderFilePath());

            enumSourceFile.write(source);
            enumSourceFile.makeLineSeparator();
            enumSourceFile.flush();

            fileNameList.push_back(enumSourceFile.fileName());
            filePathList.push_back(enumSourceFile.filePath());
        }

        // Keep a list of all the file names we used
        fileNameList.push_back(enumfile.fileName());
        filePathList.push_back(enumfile.filePath());
    }

    // Now parse the global structures
    for(std::size_t i = 0; i < structures.size(); i++)
    {
        ProtocolStructureModule* module = structures[i];

        // Parse its XML and generate the output
        module->parse();

        // Keep a list of all the file names
        fileNameList.push_back(module->getDefinitionFileName());
        filePathList.push_back(module->getDefinitionFilePath());
        fileNameList.push_back(module->getHeaderFileName());
        filePathList.push_back(module->getHeaderFilePath());
        fileNameList.push_back(module->getSourceFileName());
        filePathList.push_back(module->getSourceFilePath());
        fileNameList.push_back(module->getVerifySourceFileName());
        filePathList.push_back(module->getVerifySourceFilePath());
        fileNameList.push_back(module->getVerifyHeaderFileName());
        filePathList.push_back(module->getVerifyHeaderFilePath());
        fileNameList.push_back(module->getCompareSourceFileName());
        filePathList.push_back(module->getCompareSourceFilePath());
        fileNameList.push_back(module->getCompareHeaderFileName());
        filePathList.push_back(module->getCompareHeaderFilePath());
        fileNameList.push_back(module->getPrintSourceFileName());
        filePathList.push_back(module->getPrintSourceFilePath());
        fileNameList.push_back(module->getPrintHeaderFileName());
        filePathList.push_back(module->getPrintHeaderFilePath());
        fileNameList.push_back(module->getMapSourceFileName());
        filePathList.push_back(module->getMapSourceFilePath());
        fileNameList.push_back(module->getMapHeaderFileName());
        filePathList.push_back(module->getMapHeaderFilePath());

    }// for all top level structures

    // And the global packets. We want to sort the packets into two batches:
    // those packets which can be used by other packets; and those which cannot.
    // This way we can parse the first batch ahead of the second
    for(std::size_t i = 0; i < packets.size(); i++)
    {
        ProtocolPacket* packet = packets.at(i);

        if(!isFieldSet(packet->getElement(), "useInOtherPackets"))
            continue;

        // Parse its XML
        packet->parse();

        // The structures have been parsed, adding this packet to the list
        // makes it available for other packets to find as structure reference
        structures.push_back(packet);

        // Keep a list of all the file names
        fileNameList.push_back(packet->getDefinitionFileName());
        filePathList.push_back(packet->getDefinitionFilePath());
        fileNameList.push_back(packet->getHeaderFileName());
        filePathList.push_back(packet->getHeaderFilePath());
        fileNameList.push_back(packet->getSourceFileName());
        filePathList.push_back(packet->getSourceFilePath());
        fileNameList.push_back(packet->getVerifySourceFileName());
        filePathList.push_back(packet->getVerifySourceFilePath());
        fileNameList.push_back(packet->getVerifyHeaderFileName());
        filePathList.push_back(packet->getVerifyHeaderFilePath());
        fileNameList.push_back(packet->getCompareSourceFileName());
        filePathList.push_back(packet->getCompareSourceFilePath());
        fileNameList.push_back(packet->getCompareHeaderFileName());
        filePathList.push_back(packet->getCompareHeaderFilePath());
        fileNameList.push_back(packet->getPrintSourceFileName());
        filePathList.push_back(packet->getPrintSourceFilePath());
        fileNameList.push_back(packet->getPrintHeaderFileName());
        filePathList.push_back(packet->getPrintHeaderFilePath());
        fileNameList.push_back(packet->getMapSourceFileName());
        filePathList.push_back(packet->getMapSourceFilePath());
        fileNameList.push_back(packet->getMapHeaderFileName());
        filePathList.push_back(packet->getMapHeaderFilePath());

    }

    // And the packets which are not available for other packets
    for(std::size_t i = 0; i < packets.size(); i++)
    {
        ProtocolPacket* packet = packets.at(i);

        if(isFieldSet(packet->getElement(), "useInOtherPackets"))
            continue;

        // Parse its XML
        packet->parse();

        // Keep a list of all the file names
        fileNameList.push_back(packet->getDefinitionFileName());
        filePathList.push_back(packet->getDefinitionFilePath());
        fileNameList.push_back(packet->getHeaderFileName());
        filePathList.push_back(packet->getHeaderFilePath());
        fileNameList.push_back(packet->getSourceFileName());
        filePathList.push_back(packet->getSourceFilePath());
        fileNameList.push_back(packet->getVerifySourceFileName());
        filePathList.push_back(packet->getVerifySourceFilePath());
        fileNameList.push_back(packet->getVerifyHeaderFileName());
        filePathList.push_back(packet->getVerifyHeaderFilePath());
        fileNameList.push_back(packet->getCompareSourceFileName());
        filePathList.push_back(packet->getCompareSourceFilePath());
        fileNameList.push_back(packet->getCompareHeaderFileName());
        filePathList.push_back(packet->getCompareHeaderFilePath());
        fileNameList.push_back(packet->getPrintSourceFileName());
        filePathList.push_back(packet->getPrintSourceFilePath());
        fileNameList.push_back(packet->getPrintHeaderFileName());
        filePathList.push_back(packet->getPrintHeaderFilePath());
        fileNameList.push_back(packet->getMapSourceFileName());
        filePathList.push_back(packet->getMapSourceFilePath());
        fileNameList.push_back(packet->getMapHeaderFileName());
        filePathList.push_back(packet->getMapHeaderFilePath());

    }

    // Parse all of the documentation
    for(std::size_t i = 0; i < documents.size(); i++)
    {
        ProtocolDocumentation* doc = documents.at(i);

        doc->parse();
    }

    if(!nohelperfiles)
    {
        // Auto-generated files for coding
        ProtocolScaling(support).generate(fileNameList, filePathList);
        FieldCoding(support).generate(fileNameList, filePathList);
        ProtocolFloatSpecial(support).generate(fileNameList, filePathList);
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
    for(std::size_t i = 0; i < fileNameList.size(); i++)
        ProtocolFile::copyTemporaryFile(filePathList.at(i), fileNameList.at(i));

    // If we are putting the files in our local directory then we don't just want an empty string in our printout
    if(path.empty())
        path = "./";

    std::cout << "Generated protocol files in " << path << std::endl;

    return true;

}// ProtocolParser::parse


/*!
 * Parses a single XML file handling any require tags to flatten a file
 * heirarchy into a single flat structure
 * \param xmlFilename is the file to parse
 */
bool ProtocolParser::parseFile(std::string xmlFilename)
{
    // Path contains the path and file name and extension
    std::filesystem::path path(xmlFilename);

    // We allow each xml file to alter the global filenames used, but only for the context of that xml.
    ProtocolSupport localsupport(support);

    std::string absolutepathname;

    if(xmlFilename.at(0) == ':')
        absolutepathname = xmlFilename;
    else
        absolutepathname = std::filesystem::absolute(path).string();

    // Don't parse the same file twice
    if(contains(filesparsed, absolutepathname))
        return false;

    // Keep a record of what we have already parsed, so we don't parse the same file twice
    filesparsed.push_back(absolutepathname);

    std::cout << "Parsing file " << ProtocolFile::sanitizePath(path.parent_path().string()) << path.filename().string() << std::endl;

    std::fstream xmlFile(xmlFilename, std::ios_base::in);

    if(!xmlFile.is_open())
    {
        std::string warning = "error: Failed to open xml protocol file " + xmlFilename;
        std::cerr << warning << std::endl;
        return false;
    }

    currentxml = new XMLDocument();
    xmldocs.push_back(currentxml);
    std::string contents = std::string((std::istreambuf_iterator<char>(xmlFile)), std::istreambuf_iterator<char>());

    // Done with the file
    xmlFile.close();

    // Error parsing
    std::string error;
    //int errorLine, errorCol;

    currentxml = new XMLDocument();

    // Extract XML data
    if(currentxml->Parse(contents.c_str()) != XML_SUCCESS)
    {
        std::cerr << currentxml->ErrorStr() << std::endl;
        delete currentxml;

        if(xmldocs.size() > 0)
            currentxml = xmldocs.back();
        else
            currentxml = nullptr;

        return false;
    }
    else
        xmldocs.push_back(currentxml);

    // The outer most element
    XMLElement* docElem = currentxml->RootElement();

    // This element must have the "Protocol" tag
    if((docElem == nullptr) || (XMLUtil::StringEqual(docElem->Name(), "protocol") == false))
    {
        std::string warning = xmlFilename + ":0:0: error: 'Protocol' tag not found in file";
        std::cerr << warning << std::endl;
        return false;
    }

    // Protocol file options specified in the xml
    localsupport.parseFileNames(docElem->FirstAttribute());
    localsupport.sourcefile = xmlFilename;

    for(const XMLElement* element = docElem->FirstChildElement(); element != nullptr; element = element->NextSiblingElement())
    {
        std::string nodename = toLower(trimm(element->Name()));

        // Import another file recursively
        // File recursion is handled first so that ordering is preserved
        // This effectively creates a single flattened XML structure
        if( nodename == "require" )
        {
            std::string subfile = getAttribute("file", element->FirstAttribute());

            if( subfile.empty() )
            {
                std::string warning = xmlFilename + ": warning: file attribute missing from \"Require\" tag";
                std::cerr << warning << std::endl;
            }
            else
            {
                if(!endsWith(subfile, ".xml"))
                    subfile += ".xml";

                // The new file is relative to this file
                parseFile(ProtocolFile::sanitizePath(path.parent_path().string()) + subfile);
            }

        }
        else if( nodename == "struct" || nodename == "structure" )
        {
            ProtocolStructureModule* module = new ProtocolStructureModule( this, localsupport, api, version );

            // Remember the XML
            module->setElement(element);

            structures.push_back( module );
        }
        else if( nodename == "enum" || nodename == "enumeration" )
        {
            EnumCreator* Enum = new EnumCreator( this, nodename, localsupport );

            Enum->setElement(element);

            globalEnums.push_back( Enum );
            alldocumentsinorder.push_back( Enum );
        }
        // Define a packet
        else if( nodename == "packet" || nodename == "pkt" )
        {
            ProtocolPacket* packet = new ProtocolPacket( this, localsupport, api, version );

            packet->setElement(element);

            packets.push_back( packet );
            alldocumentsinorder.push_back( packet );
        }
        else if ( nodename == "doc" || contains(nodename, "document"))
        {
            ProtocolDocumentation* document = new ProtocolDocumentation( this, nodename, localsupport );

            document->setElement(element);

            documents.push_back( document );
            alldocumentsinorder.push_back( document );
        }
        else
        {
            //TODO
        }

    }

    return true;

}// ProtocolParser::parseFile


/*!
 * Create the header file for the top level module of the protocol
 * \param docElem is the "protocol" element from the DOM
 */
void ProtocolParser::createProtocolHeader(const XMLElement* docElem)
{
    // Build the header file
    header = new ProtocolHeaderFile(support);

    // The file name
    header->setModuleNameAndPath(name + "Protocol", support.outputpath);

    // Construct the file comment that goes in the \file block
    std::string filecomment = "\\mainpage " + name + " protocol stack\n\n" + comment + "\n\n";

    // The protocol enumeration API, which can be empty
    if(!api.empty())
    {
        // Make sure this is only a number
        bool ok = false;
        int64_t number = ShuntingYard::toInt(api, &ok);
        if(ok && number > 0)
        {
            // Turn it back into a string
            api = std::string(api);

            filecomment += "The protocol API enumeration is incremented anytime the protocol is changed in a way that affects compatibility with earlier versions of the protocol. The protocol enumeration for this version is: " + api + "\n\n";
        }
        else
            api.clear();

    }// if we have API enumeration

    // The protocol version string, which can be empty
    if(!version.empty())
        filecomment += "The protocol version is " + version + "\n\n";

    // A long comment that should be wrapped at 80 characters in the \file block
    header->setFileComment(filecomment);

    header->makeLineSeparator();

    // Includes
    if(support.supportbool)
        header->writeIncludeDirective("stdbool.h", "", true);

    header->writeIncludeDirective("stdint.h", std::string(), true);

    // Add other includes
    outputIncludes(name, *header, docElem);

    header->makeLineSeparator();

    // API macro
    if(!api.empty())
    {
        header->makeLineSeparator();
        header->write("//! \\return the protocol API enumeration\n");
        header->write("#define get" + name + "Api() " + api + "\n");
    }

    // Version macro
    if(!version.empty())
    {
        header->makeLineSeparator();
        header->write("//! \\return the protocol version string\n");
        header->write("#define get" + name + "Version() \""  + version + "\"\n");
    }

    // Translation macro
    header->makeLineSeparator();
    header->write("// Translation provided externally. The macro takes a `const char *` and returns a `const char *`\n");
    header->write("#ifndef translate" + name + "\n");
    header->write("    #define translate" + name + "(x) x\n");
    header->write("#endif");

    header->makeLineSeparator();

    // We need to flush this to disk now, because others may try to open this file and append it
    header->flush();

}// ProtocolParser::createProtocolHeader


void ProtocolParser::finishProtocolHeader(void)
{
    // We need to re-open this file because others may have written to it and
    // we want to append after their write (This is the whole reaons that
    // finishProtocolHeader() is separate from createProtocolHeader()
    header->setModuleNameAndPath(name + "Protocol", support.outputpath);

    header->makeLineSeparator();

    // We want these prototypes to be the last things written to the file, because support.pointerType may be defined above
    header->write("\n");
    header->write("// The prototypes below provide an interface to the packets.\n");
    header->write("// They are not auto-generated functions, but must be hand-written\n");
    header->write("\n");
    header->write("//! \\return the packet data pointer from the packet\n");
    header->write("uint8_t* get" + name + "PacketData(" + support.pointerType + " pkt);\n");
    header->write("\n");
    header->write("//! \\return the packet data pointer from the packet, const\n");
    header->write("const uint8_t* get" + name + "PacketDataConst(const " + support.pointerType + " pkt);\n");
    header->write("\n");
    header->write("//! Complete a packet after the data have been encoded\n");
    header->write("void finish" + name + "Packet(" + support.pointerType + " pkt, int size, uint32_t packetID);\n");
    header->write("\n");
    header->write("//! \\return the size of a packet from the packet header\n");
    header->write("int get" + name + "PacketSize(const " + support.pointerType + " pkt);\n");
    header->write("\n");
    header->write("//! \\return the ID of a packet from the packet header\n");
    header->write("uint32_t get" + name + "PacketID(const " + support.pointerType + " pkt);\n");
    header->write("\n");

    header->flush();
}


/*!
 * Output a long string of text which should be wrapped at 80 characters.
 * \param file receives the output
 * \param prefix precedes each line (for example "//" or " *"
 * \param text is the long text string to output. If text is empty
 *        nothing is output
 */
void ProtocolParser::outputLongComment(ProtocolFile& file, const std::string& prefix, const std::string& text)
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
std::string ProtocolParser::outputLongComment(const std::string& prefix, const std::string& text)
{
    return reflowComment(text, prefix, 80);

}// ProtocolParser::outputLongComment


/*!
 * Get a correctly reflowed comment from a DOM
 * \param e is the DOM to get the comment from
 * \return the correctly reflowed comment, which could be empty
 */
std::string ProtocolParser::getComment(const XMLElement* e)
{
    return reflowComment(e->Attribute("comment"));
}

/*!
 * Take a comment line which may have some unusual spaces and line feeds that
 * came from the xml formatting and reflow it for our needs.
 * \param text is the raw comment from the xml.
 * \param prefix precedes each line (for example "//" or " *"
 * \return the reflowed comment.
 */
std::string ProtocolParser::reflowComment(const std::string& text, const std::string& prefix, std::size_t charlimit)
{
    // Remove leading and trailing white space
    std::string output = trimm(text);

    // Convert to unix style line endings, just in case
    replaceinplace(output, "\r\n", "\n");

    // Separate by blocks that have \verbatim surrounding them
    std::vector<std::string> blocks = split(output, "\\verbatim");

    // Empty the output string so we can build the output up
    output.clear();

    for(std::size_t b = 0; b < blocks.size(); b++)
    {
        // odd blocks are "verbatim", even blocks are not
        if((b & 0x01) == 1)
        {
            // Separate the paragraphs, as given by single line feeds
            std::vector<std::string> paragraphs = split(blocks.at(b), "\n");

            if(prefix.empty())
            {
                for(std::size_t i = 0; i < paragraphs.size(); i++)
                    output += paragraphs[i] + "\n";
            }
            else
            {
                // Output with the prefix
                for(std::size_t i = 0; i < paragraphs.size(); i++)
                    output += prefix + " " + paragraphs[i] + "\n";
            }
        }
        else
        {
            // Separate the paragraphs, as given by dual line feeds
            std::vector<std::string> paragraphs = split(blocks.at(b), "\n\n");

            for(std::size_t i = 0; i < paragraphs.size(); i++)
            {
                // Replace line feeds with spaces
                replaceinplace(paragraphs[i], "\n", " ");

                // Replace tabs with spaces
                replaceinplace(paragraphs[i], "\t", " ");

                std::size_t length = 0;

                // Break it apart into words
                std::vector<std::string> list = split(paragraphs[i], " ");

                // Now write the words one at a time, wrapping at character length
                for (std::size_t j = 0; j < list.size(); j++)
                {
                    // Length of the word plus the preceding space
                    std::size_t wordLength = list.at(j).length() + 1;

                    if(charlimit > 0)
                    {
                        if((length != 0) && (length + wordLength > charlimit))
                        {
                            // Terminate the previous line
                            output += "\n";
                            length = 0;
                        }
                    }

                    // All lines in the comment block start with the prefix
                    if(length == 0)
                    {
                        output += prefix;
                        length += prefix.length();
                    }
                    else
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
 * Return a list of XNLNodes that are direct children and have a specific tag
 * name. This gets just children, not grand children.
 * \param node is the parent node.
 * \param tag is the tag name to look for.
 * \param tag2 is a second optional tag name to look for.
 * \param tag3 is a third optional tag name to look for.
 * \return a list of XMLNodes.
 */
std::vector<const XMLElement*> ProtocolParser::childElementsByTagName(const XMLElement* node, std::string tag, std::string tag2, std::string tag3)
{
    std::vector<const XMLElement*> list;

    // Now just the ones with the tag(s) we want
    for(const XMLElement* child = node->FirstChildElement(); child != nullptr; child = child->NextSiblingElement())
    {
        if(contains(child->Value(), tag))
            list.push_back(child);
        else if(contains(child->Value(), tag2))
            list.push_back(child);
        else if(contains(child->Value(), tag3))
            list.push_back(child);
    }

    return list;

}// ProtocolParser::childElementsByTagName


/*!
 * Return the value of an attribute from a node map
 * \param name is the name of the attribute to return. name is not case sensitive
 * \param attr is the root attribute from a DOM element.
 * \param defaultIfNone is returned if no name attribute is found.
 * \return the value of the name attribute, or defaultIfNone if none found
 */
std::string ProtocolParser::getAttribute(const std::string &name, const XMLAttribute *attr, const std::string &defaultIfNone)
{
    for( const XMLAttribute* a = attr; a; a = a->Next() )
    {
        if ( XMLUtil::StringEqual( a->Name(), name.c_str() ) )
        {
            return trimm(a->Value());
        }
    }

    return defaultIfNone;

}// ProtocolParser::getAttribute


/*!
 * Parse all enumerations which are direct children of a DomNode. The
 * enumerations will be stored in the global list
 * \param parent is the hierarchical name of the object which owns the new enumeration
 * \param node is parent node
 */
void ProtocolParser::parseEnumerations(const std::string& parent, const XMLNode* node)
{
    const XMLElement* element = node->ToElement();

    if(element == nullptr)
        return;

    // Interate through the child elements
    for(const XMLElement* e = element->FirstChildElement(); e != nullptr; e = e->NextSiblingElement())
    {
        // Look at those which are tagged "enum"
        if(toLower(e->Name()) == "enum")
            parseEnumeration(parent, e);
    }

}// ProtocolParser::parseEnumerations


/*!
 * Parse a single enumeration given by a DOM element. This will also
 * add the enumeration to the global list which can be searched with
 * lookUpEnumeration().
 * \param parent is the hierarchical name of the object which owns the new enumeration
 * \param element is the DomElement that represents this enumeration
 * \return a pointer to the newly created enumeration object.
 */
const EnumCreator* ProtocolParser::parseEnumeration(const std::string& parent, const XMLElement* element)
{
    EnumCreator* Enum = new EnumCreator(this, parent, support);

    Enum->setElement(element);
    Enum->parse();
    enums.push_back(Enum);

    return Enum;

}// ProtocolParser::parseEnumeration


/*!
 * Output all include directions which are direct children of a DomNode
 * \param parent is the hierarchical name of the owning object
 * \param file receives the output
 * \param node is parent node
 */
void ProtocolParser::outputIncludes(const std::string& parent, ProtocolFile& file, const XMLNode* node) const
{
    outputIncludes(parent, file, node->ToElement());

}// ProtocolParser::outputIncludes


/*!
 * Output all include directions which are direct children of an element
 * \param parent is the hierarchical name of the owning object
 * \param file receives the output
 * \param node is parent node
 */
void ProtocolParser::outputIncludes(const std::string& parent, ProtocolFile& file, const XMLElement* element) const
{
    if(element == nullptr)
        return;

    // Interate through the child elements
    for(const XMLElement* e = element->FirstChildElement(); e != nullptr; e = e->NextSiblingElement())
    {
        // Look at those which are tagged "include"
        if(toLower(trimm(e->Name())) == "include")
        {
            std::string include;
            std::string comment;
            bool global = false;

            for(const XMLAttribute* a = e->FirstAttribute(); a != nullptr; a = a->Next())
            {
                std::string attrname = toLower(trimm(a->Name()));

                if(attrname == "name")
                    include = trimm(a->Value());
                else if(attrname == "comment")
                    comment = trimm(a->Value());
                else if(attrname == "global")
                    global = ProtocolParser::isFieldSet(a->Value());
                else if(support.disableunrecognized == false)
                    ProtocolDocumentation::emitWarning(support.sourcefile, parent + ": " + include, "Unrecognized attribute", a);

            }// for all attributes

            if(!include.empty())
                file.writeIncludeDirective(include, comment, global);

        }// if element is an include

    }// for all elements

}// ProtocolParser::outputIncludes


/*!
 * Find the include name for a specific global structure type or enumeration type
 * \param typeName is the type to lookup
 * \return the file name to be included to reference this global structure type
 */
std::string ProtocolParser::lookUpIncludeName(const std::string& typeName) const
{
    for(std::size_t i = 0; i < structures.size(); i++)
    {
        if(structures.at(i)->typeName == typeName)
        {
            return structures.at(i)->getDefinitionFileName();
        }
    }

    for(std::size_t i = 0; i < packets.size(); i++)
    {
        if(packets.at(i)->typeName == typeName)
        {
            return packets.at(i)->getDefinitionFileName();
        }
    }

    for(std::size_t i = 0; i < globalEnums.size(); i++)
    {
        if((globalEnums.at(i)->getName() == typeName) || globalEnums.at(i)->isEnumerationValue(typeName))
        {
            return globalEnums.at(i)->getHeaderFileName();
        }
    }

    return std::string();
}


/*!
 * Find the global structure pointer for a specific type
 * \param typeName is the type to lookup
 * \return a pointer to the structure encodable, or NULL if it does not exist
 */
const ProtocolStructureModule* ProtocolParser::lookUpStructure(const std::string& typeName) const
{
    for(std::size_t i = 0; i < structures.size(); i++)
    {
        if(structures.at(i)->typeName == typeName)
        {
            return structures.at(i);
        }
    }


    for(std::size_t i = 0; i < packets.size(); i++)
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
const EnumCreator* ProtocolParser::lookUpEnumeration(const std::string& enumName) const
{
    for(std::size_t i = 0; i < globalEnums.size(); i++)
    {
        if(globalEnums.at(i)->getName() == enumName)
        {
            return globalEnums.at(i);
        }
    }

    for(std::size_t i = 0; i < enums.size(); i++)
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
 * \param text is the source text to search, which won't be modified
 * \return A new string that replaces any enumeration names with the value of the enumeration
 */
std::string ProtocolParser::replaceEnumerationNameWithValue(const std::string& text) const
{
    std::string replace = text;

    for(std::size_t i = 0; i < globalEnums.size(); i++)
    {
        replace = globalEnums.at(i)->replaceEnumerationNameWithValue(replace);
    }

    for(std::size_t i = 0; i < enums.size(); i++)
    {
        replace = enums.at(i)->replaceEnumerationNameWithValue(replace);
    }

    return replace;
}


/*!
 * Determine if text is part of an enumeration. This will compare against all
 * elements in all enumerations and return the enumeration name if a match is found.
 * \param text is the enumeration value string to compare.
 * \return The enumeration name if a match is found, or an empty string for no match.
 */
std::string ProtocolParser::getEnumerationNameForEnumValue(const std::string& text) const
{
    for(std::size_t i = 0; i < globalEnums.size(); i++)
    {
        if(globalEnums.at(i)->isEnumerationValue(text))
            return globalEnums.at(i)->getName();
    }

    for(std::size_t i = 0; i < enums.size(); i++)
    {
        if(enums.at(i)->isEnumerationValue(text))
            return enums.at(i)->getName();
    }

    return std::string();

}


/*!
 * Find the enumeration value with this name and return its comment, or an empty
 * string. This will search all the enumerations that the parser knows about to
 * find the enumeration value.
 * \param name is the name of the enumeration value to find
 * \return the comment string of the name enumeration element, or an empty
 *         string if name is not found
 */
std::string ProtocolParser::getEnumerationValueComment(const std::string& name) const
{
    std::string comment;

    for(std::size_t i = 0; i < globalEnums.size(); i++)
    {
        comment = globalEnums.at(i)->getEnumerationValueComment(name);
        if(!comment.empty())
            return comment;
    }

    for(std::size_t i = 0; i < enums.size(); i++)
    {
        comment = enums.at(i)->getEnumerationValueComment(name);
        if(!comment.empty())
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
void ProtocolParser::getStructureSubDocumentationDetails(std::string typeName, std::vector<int>& outline, std::string& startByte, std::vector<std::string>& bytes, std::vector<std::string>& names, std::vector<std::string>& encodings, std::vector<std::string>& repeats, std::vector<std::string>& comments) const
{
    for(std::size_t i = 0; i < structures.size(); i++)
    {
        if(structures.at(i)->typeName == typeName)
        {
            return structures.at(i)->getSubDocumentationDetails(outline, startByte, bytes, names, encodings, repeats, comments);
        }
    }


    for(std::size_t i = 0; i < packets.size(); i++)
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
void ProtocolParser::outputMarkdown(bool isBigEndian, std::string inlinecss)
{
    std::string basepath = support.outputpath;

    if (!docsDir.empty())
        basepath = docsDir;

    std::string filename = basepath + name + ".markdown";
    std::string filecontents = "\n\n";
    ProtocolFile file(filename, support, false);

    std::vector<std::string> packetids;
    for(std::size_t i = 0; i < packets.size(); i++)
        packets.at(i)->appendIds(packetids);

    removeDuplicates(packetids);

    /* Write protocol introductory information */
    if (hasAboutSection())
    {
        if(title.empty())
            filecontents += "# " + name + " Protocol\n\n";
        else
            filecontents += "# " + title + "\n\n";

        if(!comment.empty())
            filecontents += outputLongComment("", comment) + "\n\n";

        if(!version.empty())
            filecontents += title + " Protocol version is " + version + ".\n\n";

        if(!api.empty())
            filecontents += title + " Protocol API is " + api + ".\n\n";

    }

    for(std::size_t i = 0; i < alldocumentsinorder.size(); i++)
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
    if(!titlePage.empty())
        file.write("Title:" + title + "\n\n");

    // Specific header-level definitions are required for LaTeX compatibility
    if (latexEnabled)
    {
        file.write("Base Header Level: 1 \n");  // Base header level refers to the HTML output format
        file.write("LaTeX Header Level: " + std::to_string(latexHeader) + " \n"); // LaTeX header level can be set by user
        file.write("\n");
    }

    // Add stylesheet information (unless it is disabled entirely)
    if (!nocss)
    {
        // Open the style tag
        file.write("<style>\n");

        if(inlinecss.empty())
            file.write(getDefaultInlinCSS());
        else
            file.write(inlinecss);

        // Close the style tag
        file.write("</style>\n");

        file.write("\n");
    }

    if(tableOfContents)
    {
        std::string temp = getTableOfContents(filecontents);
        temp += "----------------------------\n\n";
        temp += filecontents;
        filecontents = temp;
    }

    if(!titlePage.empty())
    {
        std::string temp = titlePage;
        temp += "\n----------------------------\n\n";
        temp += filecontents;
        filecontents = temp;
    }

    // Add html page breaks at each ---
    replaceinplace(filecontents, "\n---", "<div class=\"page-break\"></div>\n\n\n---");

    // Deal with the degrees symbol, which doesn't render in html
    replaceinplace(filecontents, "°", "&deg;");

    file.write(filecontents);

    file.flush();

    // Write html documentation
    std::string htmlfile =  basepath + name + ".html";
    std::cout << "Writing HTML documentation to " << htmlfile << std::endl;
    #if defined(__APPLE__) && defined(__MACH__)
    std::system(("/usr/local/bin/MultiMarkdown " + filename + " > " + htmlfile).c_str());
    #else    
    std::system(("multimarkdown " + filename + " > " + htmlfile).c_str());
    #endif

    if (latexEnabled)
    {
        // Write LaTeX documentation
        std::string latexfile =  basepath + name + ".tex";
        std::cout << "Writing LaTeX documentation to " << latexfile << "\n";

        #if defined(__APPLE__) && defined(__MACH__)
        std::system(("/usr/local/bin/MultiMarkdown " + filename + " > " + latexfile + " --to=latex").c_str());
        #else
        std::system(("multimarkdown " + filename + " > " + latexfile + " --to=latex").c_str());
        #endif
    }
}


/*!
 * Get the table of contents, based on the file contents
 * \param filecontents are the file contents (without a TOC)
 * \return the table of contents data
 */
std::string ProtocolParser::getTableOfContents(const std::string& filecontents)
{
    std::string output;

    // The table of contents identifies all the heding references, which start
    // with #. Each heading reference is also prefaced by two line feeds. The
    // level of the heading (and toc line) is defined by the number of #s.
    std::vector<std::string> headings = split(filecontents, "\n\n#");

    if(headings.size() > 0)
        output = "<toctitle id=\"tableofcontents\">Table of contents</toctitle>\n";

    for(std::size_t i = 0; i < headings.size(); i++)
    {
        std::string refname;
        std::string text;
        int level = 1;

        // Pull the first line out of the heading group
        std::string line = headings.at(i).substr(0, headings.at(i).find("\n"));

        std::size_t prolog;
        for(prolog = 0; prolog < line.size(); prolog++)
        {
            char symbol = line.at(prolog);

            if(symbol == '#')       // Count the level
                level++;
            else if(symbol == ' ')  // Remove leading spaces
                continue;
            else
            {
                // Remove the prolog and get out
                line.erase(0, prolog);
                break;
            }

        }// for all characters

        // Special check in case we got a weird one
        if(line.empty())
            continue;

        // Now figure out the reference. It is either going to be given by some
        // embedded html, or by the id that markdown will put into the output html
        if(contains(line, "<a name=") && contains(line, "\""))
        {
            // The text is after the reference closure
            text = line.substr(line.find("</a>")+4);

            // In this case the reference name is contained within quotes
            std::size_t start = line.find("\"") + 1;
            refname = line.substr(start, line.find("\"", start) - start - 1);
        }
        else
        {
            // The text for the line is the line
            text = line;

            // In this case the reference name is the whole line, lower case, no spaces and other special characters
            refname = toLower(line);
            replaceinplace(refname, " ");
            replaceinplace(refname, "(");
            replaceinplace(refname, ")");
            replaceinplace(refname, "{");
            replaceinplace(refname, "}");
            replaceinplace(refname, "[");
            replaceinplace(refname, "]");
            replaceinplace(refname, "`");
            replaceinplace(refname, "\"");
            replaceinplace(refname, "*");
        }

        // Finally the actual line
        output += "<toc"+std::to_string(level)+"><a href=\"#" + refname + "\">" + text + "</a></toc"+std::to_string(level)+">\n";

    }// for all headings

    return output + "\n";

}// ProtocolParser::getTableOfContents


/*!
 * Return the string that describes the about section
 * \param isBigEndian should be true to describe a big endian protocol
 * \return the about section contents
 */
std::string ProtocolParser::getAboutSection(bool isBigEndian)
{
    std::string output;

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
bool ProtocolParser::isFieldSet(const XMLElement* e, const std::string& label)
{
    return isFieldSet(label, e->FirstAttribute());
}


/*!
 * Determine if the value of an attribute is either {'true','yes','1'}
 * \param attribname is the name of the attribute to test
 * \param map is the list of all attributes to search
 * \return true if the attribute value is "true", "yes", or "1"
 */
bool ProtocolParser::isFieldSet(const std::string& attribname, const XMLAttribute* map)
{
    return isFieldSet(ProtocolParser::getAttribute(attribname, map));
}


/*!
 * Determine if the value of an attribute is either {'true','yes','1'}
 * \param value is the attribute value to test
 * \return true if the attribute value is "true", "yes", or "1"
 */
bool ProtocolParser::isFieldSet(const std::string& value)
{
    bool result = false;

    if(contains(value, "true"))
        result = true;
    else if(contains(value, "yes"))
        result = true;
    else if(contains(value, "1"))
        result = true;

    return result;

}// ProtocolParser::isFieldSet


/*!
 * Determine if the field contains a given label, and the value is either {'false','no','0'}
 * \param e is the element from the DOM to test
 * \param label is the name of the attribute form the element to test
 * \return true if the attribute value is "true", "yes", or "1"
 */
bool ProtocolParser::isFieldClear(const XMLElement* e, const std::string& label)
{
    return isFieldSet(label, e->FirstAttribute());
}


/*!
 * Determine if the value of an attribute is either {'false','no','0'}
 * \param attribname is the name of the attribute to test
 * \param map is the list of all attributes to search
 * \return true if the attribute value is "false", "no", or "0"
 */
bool ProtocolParser::isFieldClear(const std::string& attribname, const XMLAttribute* map)
{
    return isFieldClear(ProtocolParser::getAttribute(attribname, map));
}


/*!
 * Determine if the value of an attribute is either {'false','no','0'}
 * \param value is the attribute value to test
 * \return true if the attribute value is "false", "no", or "0"
 */
bool ProtocolParser::isFieldClear(const std::string& value)
{
    bool result = false;

    if(contains(value, "false"))
        result = true;
    else if(contains(value, "no"))
        result = true;
    else if(contains(value, "0"))
        result = true;

    return result;
}


/*!
 * Get the string used for inline css. This must be bracketed in <style> tags in the html
 * \return the inline csss string
 */
std::string ProtocolParser::getDefaultInlinCSS(void)
{
    std::string css("\
body {\n\
    text-align:justify;\n\
    max-width: 25cm;\n\
    margin-left: auto;\n\
    margin-right: auto;\n\
    font-family: Georgia;\n\
    counter-reset: h1counter h2counter  h3counter toc1counter toc2counter toc3counter;\n\
 }\n\
\n\
table {\n\
   border: 1px solid #e0e0e0;\n\
   border-collapse: collapse;\n\
   margin-bottom: 25px;\n\
}\n\
\n\
th, td {\n\
    border: 1px solid #e0e0e0;\n\
    font-family: Courier, monospace;\n\
    font-size: 90%;\n\
    padding: 2px;\n\
}\n\
\n\
/*\n\
 * Alternate colors for the table, including the heading row\n\
 */\n\
th {\n\
background-color: #e0e0e0   \n\
}\n\
tr:nth-child(even){background-color: #e0e0e0}\n\
\n\
h1, h2, h3, h4, h5 { font-family: Arial; }\n\
h1 { font-size:120%; margin-bottom: 25px; }\n\
h2 { font-size:110%; margin-bottom: 15px; }\n\
h3 { font-size:100%; margin-bottom: 10px;}\n\
h4, li { font-size:100%; }\n\
\n\
caption{ font-family:Arial; font-size:85%;}\n\
\n\
code, pre, .codelike {\n\
    font-family: Courier, monospace;\n\
    font-size: 100%;\n\
    color: darkblue;\n\
}\n\
\n\
/*\n\
 * Counters for the main headings\n\
 */\n\
\n\
h1:before {\n\
    counter-increment: h1counter;\n\
    content: counter(h1counter) \"\\00a0 \";\n\
}\n\
h1 {\n\
    counter-reset: h2counter;\n\
}\n\
\n\
h2:before {\n\
    counter-increment: h2counter;\n\
    content: counter(h1counter) \".\" counter(h2counter) \"\\00a0 \";\n\
}\n\
h2 {\n\
    counter-reset: h3counter;\n\
}\n\
\n\
h3:before {\n\
  counter-increment: h3counter;\n\
  content: counter(h1counter) \".\" counter(h2counter) \".\" counter(h3counter) \"\\00a0 \";\n\
}\n\
\n\
/*\n\
 * The document title, centered\n\
 */\n\
doctitle {font-family: Arial; font-size:120%; font-weight: bold; margin-bottom:25px; text-align:center; display:block;}\n\
titlepagetext {text-align:center; display:block;}\n\
\n\
/*\n\
 * The table of contents formatting\n\
 */\n\
toctitle {font-family: Arial; font-size:120%; font-weight: bold; margin-bottom:25px; display:block;}\n\
toc1, toc2, toc3 {font-family: Arial; font-size:100%; margin-bottom:2px; display:block;}\n\
toc1 {text-indent: 0px;}\n\
toc2 {text-indent: 15px;}\n\
toc3 {text-indent: 30px;}\n\
\n\
toc1:before {\n\
    content: counter(toc1counter) \"\\00a0 \";\n\
    counter-increment: toc1counter;\n\
}\n\
toc1 {\n\
    counter-reset: toc2counter;\n\
}\n\
\n\
toc2:before {\n\
    content: counter(toc1counter) \".\" counter(toc2counter) \"\\00a0 \";\n\
    counter-increment: toc2counter;\n\
}\n\
toc2 {\n\
    counter-reset: toc3counter;\n\
}\n\
\n\
toc3:before {\n\
  content: counter(toc1counter) \".\" counter(toc2counter) \".\" counter(toc3counter) \"\\00a0 \";\n\
  counter-increment: toc3counter;\n\
}\n\
\n\
/* How it looks on a screen, notice the fancy hr blocks and lack of page breaks */\n\
@media screen {\n\
  body {\n\
    background-color: #f0f0f0;\n\
  }\n\
  .page-break { display: none; }\n\
  hr { \n\
    height: 25px; \n\
    border-style: solid; \n\
    border-color: gray; \n\
    border-width: 1px 0 0 0; \n\
    border-radius: 10px; \n\
  } \n\
  hr:before { \n\
    display: block; \n\
    content: \"\"; \n\
    height: 25px; \n\
    margin-top: -26px; \n\
    border-style: solid; \n\
    border-color: gray; \n\
    border-width: 0 0 1px 0; \n\
    border-radius: 10px; \n\
  }\n\
}\n\
\n\
/* How it looks when printed, hr turned off, in favor of page breaks*/\n\
@media print {\n\
  hr {display: none;}\n\
  body {background-color: white;}\n\
  .page-break{page-break-before: always;}\n\
}\n");

    return css;

}// ProtocolParser::getDefaultInlinCSS


/*!
 * Set the target path for writing output markdown documentation files
 * If no output path is set, the output directory for generated files is used
 * \param path is the desired output directory
 */
void ProtocolParser::setDocsPath(std::string path)
{
    std::error_code ec;
    if(std::filesystem::exists(path) || std::filesystem::create_directories(path, ec))
        docsDir = path;
    else
        docsDir = "";
}


/*!
 * Output the doxygen HTML documentation
 */
void ProtocolParser::outputDoxygen(void)
{
    std::string fileName = "ProtocolDoxyfile";
    std::fstream file(fileName, std::ios_base::out);

    if(!file.is_open())
    {
        std::cerr << "Failed to open " << fileName << std::endl;
        return;
    }

    // This file allows us to have project name specific documentation in the
    // doxygen configuration file via the @INCLUDE directive
    file << "PROJECT_NAME = \"" + name + " Protocol API\"\n";
    file.close();

    // This is a trick using raw strings to embed a resource
    static const std::string doxyfile = (
        #include "Doxyfile"
    );

    std::fstream doxfile("Doxyfile", std::ios_base::out);
    if(doxfile.is_open())
        doxfile << doxyfile;

    doxfile.close();

    // On the mac doxygen is a utility inside the Doxygen.app bundle.
    #if defined(__APPLE__) && defined(__MACH__)
    std::system("/Applications/Doxygen.app/Contents/Resources/doxygen Doxyfile");
    #else
    std::system("doxygen Doxyfile");
    #endif

    // Delete our temporary files
    ProtocolFile::deleteFile("Doxyfile");
    ProtocolFile::deleteFile(fileName);
}
