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
    // We allow ProtocolBitfield to access our protected members
    friend class ProtocolBitfield;

public:

    //! Construct the structure parsing object, with details about the overall protocol
    ProtocolStructureModule(ProtocolParser* parse, ProtocolSupport supported, const QString& protocolApi, const QString& protocolVersion);

    //! Parse a packet from the DOM
    virtual void parse(void) Q_DECL_OVERRIDE;

    //! Reset our data contents
    virtual void clear(void) Q_DECL_OVERRIDE;

    //! Destroy the protocol packet
    ~ProtocolStructureModule(void);

    //! Return the include directives needed for this encodable
    virtual void getIncludeDirectives(QStringList& list) const Q_DECL_OVERRIDE;

    //! Return the include directives needed for this encodable's init and verify functions
    virtual void getInitAndVerifyIncludeDirectives(QStringList& list) const Q_DECL_OVERRIDE;

    //! Get the name of the header file that encompasses this structure definition
    QString getDefinitionFileName(void) const {return structfile->fileName();}

    //! Get the name of the header file that encompasses this structure interface functions
    QString getHeaderFileName(void) const {return header.fileName();}

    //! Get the name of the source file for this structure
    QString getSourceFileName(void) const {return source.fileName();}

    //! Get the path of the header file that encompasses this structure definition
    QString getDefinitionFilePath(void) const {return structfile->filePath();}

    //! Get the path of the header file that encompasses this structure interface functions
    QString getHeaderFilePath(void) const {return header.filePath();}

    //! Get the path of the source file for this structure
    QString getSourceFilePath(void) const {return source.filePath();}

    //! Get the name of the header file that encompasses this structure verify functions
    QString getVerifyHeaderFileName(void) const {return verifyheaderfile->fileName();}

    //! Get the name of the source file that encompasses this structure verify functions
    QString getVerifySourceFileName(void) const {return verifysourcefile->fileName();}

    //! Get the path of the header file that encompasses this structure verify functions
    QString getVerifyHeaderFilePath(void) const {return verifyheaderfile->filePath();}

    //! Get the path of the source file that encompasses this structure verify functions
    QString getVerifySourceFilePath(void) const {return verifysourcefile->filePath();}

    //! Get the name of the header file that encompasses this structure comparison functions
    QString getCompareHeaderFileName(void) const {return compareHeader.fileName();}

    //! Get the name of the source file that encompasses this structure comparison functions
    QString getCompareSourceFileName(void) const {return compareSource.fileName();}

    //! Get the path of the header file that encompasses this structure comparison functions
    QString getCompareHeaderFilePath(void) const {return compareHeader.filePath();}

    //! Get the path of the source file that encompasses this structure comparison functions
    QString getCompareSourceFilePath(void) const {return compareSource.filePath();}

    //! Get the name of the header file that encompasses this structure comparison functions
    QString getPrintHeaderFileName(void) const {return printHeader.fileName();}

    //! Get the name of the source file that encompasses this structure comparison functions
    QString getPrintSourceFileName(void) const {return printSource.fileName();}

    //! Get the path of the header file that encompasses this structure comparison functions
    QString getPrintHeaderFilePath(void) const {return printHeader.filePath();}

    //! Get the path of the source file that encompasses this structure comparison functions
    QString getPrintSourceFilePath(void) const {return printSource.filePath();}

protected:

    //! Setup the files, which accounts for all the ways the files can be organized for this structure.
    void setupFiles(QString moduleName, QString defheadermodulename, QString verifymodulename, QString comparemodulename, QString printmodulename, bool forceStructureDeclaration = true, bool outputUtilities = true, const ProtocolStructureModule* redefines = NULL);

    //! Issue warnings for the structure module.
    void issueWarnings(const QDomNamedNodeMap& map);

    //! Write data to the source and header files to encode and decode this structure and all its children
    void createStructureFunctions(const ProtocolStructureModule* redefines = NULL);

    //! Create the functions that encode/decode sub stuctures.
    void createSubStructureFunctions(const ProtocolStructureModule* redefines = NULL);

    //! Write data to the source and header files to encode and decode this structure but not its children
    void createTopLevelStructureFunctions(const ProtocolStructureModule* redefines = NULL);

    ProtocolSourceFile source;      //!< The source file (*.c)
    ProtocolHeaderFile header;      //!< The header file (*.h)
    ProtocolHeaderFile defheader;   //!< The header file name for the structure definition
    ProtocolSourceFile verifySource;//!< The source file for verify code (*.c)
    ProtocolHeaderFile verifyHeader;//!< The header file for verify code (*.h)
    ProtocolSourceFile compareSource;       //!< The source file for comparison code (*.cpp)
    ProtocolHeaderFile compareHeader;       //!< The header file for comparison code (*.h)
    ProtocolSourceFile printSource;         //!< The source file for print code (*.cpp)
    ProtocolHeaderFile printHeader;         //!< The header file for print code (*.h)
    ProtocolHeaderFile* structfile;         //!< Reference to the file that holds the structure definition
    ProtocolHeaderFile* verifyheaderfile;   //!< Reference to the file that holds the verify prototypes
    ProtocolSourceFile* verifysourcefile;   //!< Reference to the file that holds the verify source code
    QString api;                    //!< The protocol API enumeration
    QString version;                //!< The version string
    bool encode;                    //!< True if the encode function is output
    bool decode;                    //!< True if the decode function is output
    bool compare;                   //!< True if the comparison function is output
    bool print;                     //!< True if the textPrint function is output
};

#endif // PROTOCOLSTRUCTUREMODULE_H
