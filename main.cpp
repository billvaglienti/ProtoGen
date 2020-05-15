#include <iostream>
#include <fstream>

#include "shuntingyard.h"
#include "protocolparser.h"
#include "xmllinelocator.h"

int main(int argc, char *argv[])
{
    std::vector<std::string> arguments;

    // First argument is the application name - skip that one
    for(int i = 1; i < argc; i++)
    {
        std::string argument = trimm(std::string(argv[i]));

        // All leading "--" are converted to "-" here
        while(startsWith(argument, "--"))
            argument.erase(0, 1);

        if(argument.empty())
            continue;

        // Some arguments require that the following argument be a special
        // string, like "-license afile.txt". Other arguments do not depend
        // on following arguments like "-no-helper-files". We group arguments
        // together that need to go together. All such special arguments
        // start with "-".
        if((argument.front() == '-') && (i < argc - 1))
        {
            // These are the arguments that need followers
            if( startsWith(argument, "-d")            ||
                isEqual(argument, "-l")               ||
                startsWith(argument, "-latex-header") ||
                isEqual(argument, "-s")               ||
                startsWith(argument, "-style")        ||
                startsWith(argument, "-t") )
                arguments.push_back(argument + " " + trimm(argv[++i]));
        }
        else
            arguments.push_back(argument);

    }// for all arguments except the first one



    /*
    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName( "Protogen" );
    QCoreApplication::setApplicationVersion(QString().fromStdString(ProtocolParser::genVersion));

    QCommandLineParser argParser;

    argParser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    argParser.setApplicationDescription("Protocol generation tool");
    argParser.addHelpOption();
    argParser.addVersionOption();

    argParser.addPositionalArgument("input", "Protocol defintion file, .xml");
    argParser.addPositionalArgument("outputpath", "Path for generated protocol files (default = current working directory)");

    argParser.addOption({{"d", "docs"}, "Path for generated documentation files (default = outputpath)", "docpath"});
    argParser.addOption({"license", "Path to license template file which will be prepended to each generated file", "license"});
    argParser.addOption({"show-hidden-items", "Show all items in documentation even if they are marked as 'hidden'"});
    argParser.addOption({"latex", "Enable extra documentation output required for LaTeX integration"});
    argParser.addOption({{"l", "latex-header-level"}, "LaTeX header level", "latexlevel"});
    argParser.addOption({"yes-doxygen", "Call doxygen to output developer-level documentation"});
    argParser.addOption({"no-markdown", "Skip generation of user-level documentation"});
    argParser.addOption({"no-about-section", "Skip generation of \"About this ICD\" section in documentation output"});
    argParser.addOption({"no-helper-files", "Skip creation of helper files not directly specifed by protocol .xml file"});
    argParser.addOption({{"s", "style"}, "Specify a css file to override the default style for HTML documentation", "cssfile"});
    argParser.addOption({"no-css", "Skip generation of any css data in documentation files"});
    argParser.addOption({"no-unrecognized-warnings", "Suppress warnings for unrecognized xml tags"});
    argParser.addOption({"table-of-contents", "Generate a table of contents"});
    argParser.addOption({{"t", "titlepage"}, "Path to title page file with text that will above at the beginning of the markdown", "titlefile"});
    argParser.addOption({"lang-c", "Force the output language to C, overriding the language specifier in the protocol file"});
    argParser.addOption({"lang-cpp", "Force the output language to C++, overriding the language specifier in the protocol file"});

    argParser.process(a);
    */

    ProtocolParser parser;

    // Process the positional arguments
    std::vector<std::string> otherfiles;
    std::string filename, path;

    for(std::size_t i = 0; i < arguments.size(); i++)
    {
        // Positional arguments do not have a "-" at the beginning
        if(startsWith(arguments.at(i), "-"))
            continue;

        // The first ".xml" argument is the main file, but you can
        // have more xml files after that if you want.
        if(endsWith(arguments.at(i), ".xml"))
        {
            if(filename.empty())
                filename = arguments.at(i);
            else
                otherfiles.push_back(arguments.at(i));
        }
        else
            path = arguments.at(i);
    }

    if(filename.empty())
    {
        std::cerr << "error: must provide a protocol (*.xml) file." << std::endl;
        return 2;   // no input file
    }

    // License template file
    std::string licenseTemplate = liststartsWith(arguments, "-l");
    licenseTemplate = licenseTemplate.substr(licenseTemplate.find(" ") + 1);
    if(!licenseTemplate.empty())
    {       
        std::fstream file(licenseTemplate, std::ios_base::in);

        if (file.is_open())
        {
            std::string contents = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

            // Pull all the data from the file, and convert to Qt standard line endings
            parser.setLicenseText(replaceinplace(contents, "\r\n", "\n"));

            file.close();
        }
        else
        {
            std::cerr << "warning: could not open license file '" << licenseTemplate << "'" << std::endl;
        }
    }

    // Documentation directory
    std::string docs = liststartsWith(arguments, "-d");
    docs = docs.substr(docs.find(" ") + 1);
    if (!docs.empty() && !contains(arguments, "-no-markdown"))
        parser.setDocsPath(ProtocolFile::sanitizePath(docs));

    // Process the optional arguments
    parser.disableDoxygen(!contains(arguments, "-yes-doxygen"));
    parser.disableMarkdown(contains(arguments, "-no-markdown"));
    parser.disableHelperFiles(contains(arguments, "-no-helper-files"));
    parser.disableAboutSection(contains(arguments, "-no-about-section"));
    parser.showHiddenItems(contains(arguments, "-show-hidden-items"));
    parser.disableUnrecognizedWarnings(contains(arguments, "-no-unrecognized-warnings"));
    parser.setLaTeXSupport(contains(arguments, "-latex"));
    parser.disableCSS(contains(arguments, "-no-css"));
    parser.enableTableOfContents(contains(arguments, "-table-of-contents"));

    if(contains(arguments, "-lang-c"))
        parser.setLanguageOverride(ProtocolSupport::c_language);
    else if(contains(arguments, "-lang-cpp"))
        parser.setLanguageOverride(ProtocolSupport::cpp_language);

    std::string latexLevel = liststartsWith(arguments, "-latex-header");
    latexLevel = latexLevel.substr(latexLevel.find(" ") + 1);
    if(!latexLevel.empty())
    {
        bool ok = false;
        int lvl = ShuntingYard::toInt(latexLevel, &ok);

        if (ok)
        {
            parser.setLaTeXLevel(lvl);
        }
        else
        {
            std::cerr << "warning: -latex-header-level argument '" << latexLevel << "' is invalid.";
        }
    }

    std::string css = liststartsWith(arguments, "-style");
    if(css.empty())
        css = liststartsWith(arguments, "-s ");

    css = css.substr(docs.find(" ") + 1);
    if(!css.empty() && endsWith(css, ".css", Qt::CaseInsensitive))
    {
        std::fstream file(css, std::ios_base::in);

        if (file.is_open())
        {
            // Pull all the data from the file
            parser.setInlineCSS(std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()));
            file.close();
        }
        else
        {
            std::cerr << "warning: Failed to open '" << css << "', using default css" << std::endl;
        }
    }

    std::string titlePage = liststartsWith(arguments, "-title");
    titlePage = titlePage.substr(titlePage.find(" ") + 1);
    if(!titlePage.empty())
    {
        std::fstream file(titlePage, std::ios_base::in);

        if (file.is_open())
        {
            parser.setTitlePage(std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()));
            file.close();
        }
        else
        {
            std::cerr << "warning: Failed to open '" << titlePage << "', skipping title page output" << std::endl;
        }
    }

    if (parser.parse(filename, path, otherfiles))
    {
        // Normal exit
        return 0;
    }
    else
    {
        // Input file in error
        return 1;
    }

}
