#include <QCoreApplication>
#include <QDomDocument>
#include <QStringList>
#include <QFile>
#include <QDir>
#include <iostream>
#include "protocolparser.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    int Return = 1;
    bool nodoxygen = false;
    bool nomarkdown = false;
    bool nohelperfiles = false;
    QString inlinecss;

    QString docs;

    bool latexSupportOn = false;

    // The list of arguments
    QStringList arguments = a.arguments();

    if(arguments.size() <= 1)
    {
        std::cout << "Protocol generator usage:" << std::endl;
        std::cout << "ProtoGen input.xml [outputpath] [-docs docspath] [-latex] [-no-doxygen] [-no-markdown] [-no-helper-files]" << std::endl;
        return 0;
    }

    // We expect the input file here
    QString filename = a.arguments().at(1);

    // The output path
    QString path;

    // Skip the first argument "ProtoGen.exe"
    for(int i = 1; i < arguments.size(); i++)
    {
        QString arg = arguments.at(i);

        if(arg.contains("-no-doxygen", Qt::CaseInsensitive))
            nodoxygen = true;
        else if(arg.contains("-no-markdown", Qt::CaseInsensitive))
            nomarkdown = true;        
        else if(arg.contains("-no-helper-files", Qt::CaseInsensitive))
            nohelperfiles = true;
        else if(arg.endsWith(".xml"))
            filename = arg;
        else if (arg.contains("-latex", Qt::CaseInsensitive))
            latexSupportOn = true;
        else if(arg.endsWith(".css"))
        {
            QFile file(arg);
            if(file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                inlinecss = file.readAll();
                file.close();
            }
            else
                std::cout << "Failed to open " << arg.toStdString() << "; using default css" << std::endl;
        }
        else if (arg.startsWith("-docs")) {
            //Is there an argument following this?
            if (arguments.size() > (i + 1)) {
                docs = arguments.at(i+1);

                QDir docDir(QDir::current());

                if (docDir.exists() || docDir.mkdir(docs)) //Markdown directory is sane
                {
                    //Skip the next argument;
                    i++;
                } else {
                    docs = "";
                }
            }
        }
        else if((path.isEmpty()) && (arg != filename))
            path = arg;
    }

    if(!filename.isEmpty())
    {
        QDomDocument doc("protogen");

        QFile file(filename);
        if (file.open(QIODevice::ReadOnly))
        {
            if (doc.setContent(&file))
            {
                ProtocolParser parser;

                parser.setLaTeXSupport(latexSupportOn);

                if (!docs.isEmpty())
                    parser.setDocsPath(docs);

                // Set our working directory
                if(!path.isEmpty())
                {
                    QDir dir(QDir::current());

                    // The path could be absolute or relative to
                    // our current path, this works either way

                    // Make sure the path exists
                    dir.mkpath(path);

                    // Now set it as the path to use
                    QDir::setCurrent(path);
                }

                if(parser.parse(doc, nodoxygen, nomarkdown, nohelperfiles, inlinecss))
                    Return = 1;

            }
            else
            {
                std::cout << "failed to validate xml from file: " << filename.toStdString() << std::endl;
                Return = 0;
            }

            file.close();
        }
        else
        {
            std::cout << "failed to open protocol file: " << filename.toStdString() << std::endl;
            Return = 0;
        }

    }
    else
    {
        std::cout << "must provide a protocol file." << std::endl;
        Return = 0;
    }

    if (Return == 1)
        std::cout << "Generated protocol files in " << path.toStdString() << std::endl;

    return Return;
}
