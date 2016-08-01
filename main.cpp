#include <QCoreApplication>
#include <QDomDocument>
#include <QStringList>
#include <QFile>
#include <QDir>
#include <iostream>

#include "protocolparser.h"
#include "xmllinelocator.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    ProtocolParser parser;

    // The list of arguments
    QStringList arguments = a.arguments();

    if(arguments.size() <= 1)
    {
        std::cout << "Protocol generator usage:" << std::endl;
        std::cout << "ProtoGen input.xml [outputpath] [-docs docspath] [-latex] [-no-doxygen] [-no-markdown] [-no-helper-files] [-no-unrecognized-warnings]" << std::endl;
        return 2;   // no input file
    }

    // We expect the input file here
    QString filename;

    // The output path
    QString path;

    // Skip the first argument "ProtoGen.exe"
    for(int i = 1; i < arguments.size(); i++)
    {
        QString arg = arguments.at(i);

        if(arg.contains("-no-doxygen", Qt::CaseInsensitive))
            parser.disableDoxygen(true);
        else if(arg.contains("-no-markdown", Qt::CaseInsensitive))
            parser.disableMarkdown(true);
        else if(arg.contains("-no-helper-files", Qt::CaseInsensitive))
            parser.disableHelperFiles(true);
        else if(arg.endsWith(".xml"))
            filename = arg;
        else if (arg.contains("-latex", Qt::CaseInsensitive))
            parser.setLaTeXSupport(true);
        else if (arg.contains("-no-unrecognized-warnings", Qt::CaseInsensitive))
            parser.disableUnrecognizedWarnings(true);
        else if(arg.endsWith(".css"))
        {
            QFile file(arg);
            if(file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                parser.setInlineCSS(file.readAll());
                file.close();
            }
            else
                std::cerr << "warning: Failed to open " << QDir::toNativeSeparators(arg).toStdString() << ", using default css" << std::endl;
        }
        else if (arg.startsWith("-docs"))
        {
            // Is there an argument following this?
            if (arguments.size() > (i + 1))
            {
                // The following argument is the directory path for documents
                QString docs = ProtocolFile::sanitizePath(arguments.at(++i));

                // If the directory already exists, or we can make it, then use it
                if(QDir(docs).exists() ||  QDir::current().mkdir(docs))
                    parser.setDocsPath(docs);
            }
        }
        else if((path.isEmpty()) && (arg != filename))
            path = arg;

    }// for all input arguments


    if(!filename.isEmpty())
    {
        if(parser.parse(filename, path))
            return 0;   // normal exit
        else
            return 1;   // input file in error
    }
    else
    {
        std::cerr << "error: must provide a protocol (*.xml) file." << std::endl;
        return 2;   // no input file
    }

}
