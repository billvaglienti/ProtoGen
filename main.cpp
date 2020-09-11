#include <iostream>
#include <fstream>
#include "shuntingyard.h"
#include "protocolparser.h"

static void printHelp(void);

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

        if(startsWith(argument, "-help") || startsWith(argument, "-?"))
        {
            printHelp();
            return 0;
        }
        else if(startsWith(argument, "-v"))
        {
            std::cout << ProtocolParser::genVersion << std::endl;
            return 0;
        }

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
                startsWith(argument, "-li")           ||
                startsWith(argument, "-latex-header") ||
                isEqual(argument, "-s")               ||
                startsWith(argument, "-style")        ||
                startsWith(argument, "-ti") )
                arguments.push_back(argument + " " + trimm(argv[++i]));
        }
        else
            arguments.push_back(argument);

    }// for all arguments except the first one

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
    std::string licenseTemplate = liststartsWith(arguments, "-li");
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
    parser.showHiddenItems(contains(arguments, "-show-hidden"));
    parser.disableUnrecognizedWarnings(contains(arguments, "-no-unrecognized"));
    parser.setLaTeXSupport(contains(arguments, "-latex"));
    parser.disableCSS(contains(arguments, "-no-css"));
    parser.enableTableOfContents(contains(arguments, "-table-of-contents"));

    if(contains(arguments, "-lang-py"))
        parser.setLanguageOverride(ProtocolSupport::python_language);
    if(contains(arguments, "-lang-c"))
        parser.setLanguageOverride(ProtocolSupport::c_language);
    else if(contains(arguments, "-lang-cpp"))
        parser.setLanguageOverride(ProtocolSupport::cpp_language);

    std::string latexLevel = liststartsWith(arguments, "-latex-header");
    latexLevel = latexLevel.substr(latexLevel.find(" ") + 1);
    if(!latexLevel.empty())
    {
        bool ok = false;
        int lvl = (int)ShuntingYard::toInt(latexLevel, &ok);

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
    if(!css.empty() && endsWith(css, ".css"))
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

}// main


void printHelp(void)
{
    std::string help = "Protocol Generation Tool, version: " + ProtocolParser::genVersion;
    help += R"===(

Usage: ProtoGen inputfile.xml <outputpath> <otherinputfiles.xml> -options

  inputfile.xml      : Protocol defintion, first .xml file in arguments.

  outputpath         : Path for generated files (current working directory if
                       empty).
  -docs <path>       : Path for generated documentation files (default =
                       outputpath).
  -license <file>    : License template file which will be prepended to
                       generated files.
  -yes-doxygen       : Call doxygen to output developer-level documentation.

  -no-markdown       : Skip generation of user-level documentation.

  -no-about-section  : Skip generation of "About this ICD" section in
                       documentation output.
  -no-helper-files   : Skip creation of helper files not directly specifed by
                       protocol .xml files.
  -style path        : Specify a css file to override the default style for
                       HTML documentation.
  -no-css            : Skip generation of any css data in documentation files.

  -no-unrecognized   : Suppress warnings for unrecognized xml tags.

  -table-of-contents : Generate a table of contents in the markdown.

  -titlepage <file>  : Title page file with text at the beginning of the
                       markdown.
  -lang-py           : Force the output language to Python, overriding the language
                       specifier in the protocol file.
  -lang-c            : Force the output language to C, overriding the language
                       specifier in the protocol file.
  -lang-cpp          : Force the output language to C++, overriding the
                       language specifier in the protocol file.
  -version           : Prints just the version information.

)===";
    std::cout << help;
}
