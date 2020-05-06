#ifndef PROTOCOLSTRUCTUREMODULE_H
#define PROTOCOLSTRUCTUREMODULE_H

#include "protocolstructure.h"
#include "protocolfile.h"
#include "enumcreator.h"
#include "protocolbitfield.h"
#include <QString>
#include <QDomElement>

class ProtocolStructureModule : public ProtocolStructure
{
public:

    //! Construct the structure parsing object, with details about the overall protocol
    ProtocolStructureModule(ProtocolParser* parse, ProtocolSupport supported, const QString& protocolApi, const QString& protocolVersion);

    //! Parse a packet from the DOM
    void parse(void) Q_DECL_OVERRIDE;

    //! Reset our data contents
    void clear(void) Q_DECL_OVERRIDE;

    //! Destroy the protocol packet
    ~ProtocolStructureModule(void);

    //! Return the include directives needed for this encodable
    void getIncludeDirectives(QStringList& list) const Q_DECL_OVERRIDE;

    //! Return the include directives that go into source code for this encodable
    void getSourceIncludeDirectives(QStringList& list) const Q_DECL_OVERRIDE;

    //! Return the include directives needed for this encodable's init and verify functions
    void getInitAndVerifyIncludeDirectives(QStringList& list) const Q_DECL_OVERRIDE;

    //! Return the include directives needed for this encodable's map functions
    void getMapIncludeDirectives(QStringList& list) const Q_DECL_OVERRIDE;

    //! Return the include directives needed for this encodable's compare functions
    void getCompareIncludeDirectives(QStringList& list) const Q_DECL_OVERRIDE;

    //! Return the include directives needed for this encodable's print functions
    void getPrintIncludeDirectives(QStringList& list) const Q_DECL_OVERRIDE;

    //! Get the name of the header file that encompasses this structure definition
    QString getDefinitionFileName(void) const {return structHeader->fileName();}

    //! Get the name of the header file that encompasses this structure interface functions
    QString getHeaderFileName(void) const {return header.fileName();}

    //! Get the name of the source file for this structure
    QString getSourceFileName(void) const {return source.fileName();}

    //! Get the path of the header file that encompasses this structure definition
    QString getDefinitionFilePath(void) const {return structHeader->filePath();}

    //! Get the path of the header file that encompasses this structure interface functions
    QString getHeaderFilePath(void) const {return header.filePath();}

    //! Get the path of the source file for this structure
    QString getSourceFilePath(void) const {return source.filePath();}

    //! Get the name of the header file that encompasses this structure verify functions
    QString getVerifyHeaderFileName(void) const {return (verifyHeader == nullptr) ? QString() : verifyHeader->fileName();}

    //! Get the name of the source file that encompasses this structure verify functions
    QString getVerifySourceFileName(void) const {return (verifySource == nullptr) ? QString() : verifySource->fileName();}

    //! Get the path of the header file that encompasses this structure verify functions
    QString getVerifyHeaderFilePath(void) const {return (verifyHeader == nullptr) ? QString() : verifyHeader->filePath();}

    //! Get the path of the source file that encompasses this structure verify functions
    QString getVerifySourceFilePath(void) const {return (verifySource == nullptr) ? QString() : verifySource->filePath();}

    //! Get the name of the header file that encompasses this structure comparison functions
    QString getCompareHeaderFileName(void) const {return (compareHeader == nullptr) ? QString() : compareHeader->fileName();}

    //! Get the name of the source file that encompasses this structure comparison functions
    QString getCompareSourceFileName(void) const {return (compareSource == nullptr) ? QString() : compareSource->fileName();}

    //! Get the path of the header file that encompasses this structure comparison functions
    QString getCompareHeaderFilePath(void) const {return (compareHeader == nullptr) ? QString() : compareHeader->filePath();}

    //! Get the path of the source file that encompasses this structure comparison functions
    QString getCompareSourceFilePath(void) const {return (compareSource == nullptr) ? QString() : compareSource->filePath();}

    //! Get the name of the header file that encompasses this structure comparison functions
    QString getPrintHeaderFileName(void) const {return (printHeader == nullptr) ? QString() : printHeader->fileName();}

    //! Get the name of the source file that encompasses this structure comparison functions
    QString getPrintSourceFileName(void) const {return (printSource == nullptr) ? QString() : printSource->fileName();}

    //! Get the path of the header file that encompasses this structure comparison functions
    QString getPrintHeaderFilePath(void) const {return (printHeader == nullptr) ? QString() : printHeader->filePath();}

    //! Get the path of the source file that encompasses this structure comparison functions
    QString getPrintSourceFilePath(void) const {return (printSource == nullptr) ? QString() : printSource->filePath();}

    //! Get the name of the header file that encompasses this structure map functions
    QString getMapHeaderFileName(void) const {return (mapHeader == nullptr) ? QString() : mapHeader->fileName();}

    //! Get the name of the source file that encompasses this structure map functions
    QString getMapSourceFileName(void) const {return (mapSource == nullptr) ? QString() : mapSource->fileName();}

    //! Get the path of the header file that encompasses this structure map functions
    QString getMapHeaderFilePath(void) const {return (mapHeader == nullptr) ? QString() : mapHeader->filePath();}

    //! Get the path of the source file that encompasses this structure map functions
    QString getMapSourceFilePath(void) const {return (mapSource == nullptr) ? QString() : mapSource->filePath();}

protected:

    //! Setup the files, which accounts for all the ways the files can be organized for this structure.
    void setupFiles(QString moduleName,
                    QString defheadermodulename,
                    QString verifymodulename,
                    QString comparemodulename,
                    QString printmodulename,
                    QString mapmodulename,
                    bool forceStructureDeclaration = true, bool outputUtilities = true);

    //! Issue warnings for the structure module.
    void issueWarnings(const QDomNamedNodeMap& map);

    //! Write data to the source and header files to encode and decode this structure and all its children
    void createStructureFunctions(void);

    //! Create the functions that encode/decode sub stuctures.
    void createSubStructureFunctions(void);

    //! Write data to the source and header files to encode and decode this structure but not its children
    void createTopLevelStructureFunctions(void);

    //! Write data to the source and header files for helper functions for this structure but not its children
    void createTopLevelStructureHelperFunctions(void);

    //! Get the text used to extract text for text read functions
    static QString getExtractTextFunction(void);

    // These files are always used
    ProtocolSourceFile source;          //!< The source file (*.c)
    ProtocolHeaderFile header;          //!< The header file (*.h)

    // These files are optional - the outputs could be in source or header
    ProtocolHeaderFile _structHeader;   //!< Optional header file for the structure definition
    ProtocolSourceFile _verifySource;   //!< Optional source file for verify code (*.c)
    ProtocolHeaderFile _verifyHeader;   //!< Optional header file for verify code (*.h)
    ProtocolSourceFile _compareSource;  //!< Optional source file for comparison code (*.cpp)
    ProtocolHeaderFile _compareHeader;  //!< Optional header file for comparison code (*.h)
    ProtocolSourceFile _printSource;    //!< Optional source file for print code (*.cpp)
    ProtocolHeaderFile _printHeader;    //!< Optional header file for print code (*.h)
    ProtocolSourceFile _mapSource;      //!< Optional source file for map code (*.cpp)
    ProtocolHeaderFile _mapHeader;      //!< Optional header file for map code (*.h)

    // These are the pointers that get aliased to the correct output file
    ProtocolHeaderFile* structHeader;   //!< Pointer to the header file for the structure definition
    ProtocolSourceFile* verifySource;   //!< Pointer to the source file for verify code (*.c)
    ProtocolHeaderFile* verifyHeader;   //!< Pointer to the header file for verify code (*.h)
    ProtocolSourceFile* compareSource;  //!< Pointer to the source file for comparison code (*.cpp)
    ProtocolHeaderFile* compareHeader;  //!< Pointer to the header file for comparison code (*.h)
    ProtocolSourceFile* printSource;    //!< Pointer to the source file for print code (*.cpp)
    ProtocolHeaderFile* printHeader;    //!< Pointer to the header file for print code (*.h)
    ProtocolSourceFile* mapSource;      //!< Pointer to the source file for map code (*.cpp)
    ProtocolHeaderFile* mapHeader;      //!< Pointer to the header file for map code (*.h)

    QString api;                    //!< The protocol API enumeration
    QString version;                //!< The version string
};

#endif // PROTOCOLSTRUCTUREMODULE_H
