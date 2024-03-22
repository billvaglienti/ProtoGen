#ifndef PROTOCOLSTRUCTUREMODULE_H
#define PROTOCOLSTRUCTUREMODULE_H

#include "protocolstructure.h"
#include "protocolfile.h"
#include <string>

class ProtocolStructureModule : public ProtocolStructure
{
public:

    //! Construct the structure parsing object, with details about the overall protocol
    ProtocolStructureModule(ProtocolParser* parse, ProtocolSupport supported);

    //! Parse a packet from the DOM
    void parse(void) override;

    //! Reset our data contents
    void clear(void) override;

    //! Destroy the protocol packet
    ~ProtocolStructureModule(void);

    //! Return the include directives needed for this encodable
    void getIncludeDirectives(std::vector<std::string>& list) const override;

    //! Return the include directives that go into source code for this encodable
    void getSourceIncludeDirectives(std::vector<std::string>& list) const override;

    //! Return the include directives needed for this encodable's init and verify functions
    void getInitAndVerifyIncludeDirectives(std::vector<std::string>& list) const override;

    //! Return the include directives needed for this encodable's map functions
    void getMapIncludeDirectives(std::vector<std::string>& list) const override;

    //! Return the include directives needed for this encodable's compare functions
    void getCompareIncludeDirectives(std::vector<std::string>& list) const override;

    //! Return the include directives needed for this encodable's print functions
    void getPrintIncludeDirectives(std::vector<std::string>& list) const override;

    //! Get the string which identifies this encodable in a CAN DBC file
    std::string getDBCMessageString(uint32_t ID) const;

    //! Get the string which comments this encodable in a CAN DBC file
    std::string getDBCMessageComment(uint32_t ID) const;

    //! Get the string which comments this encodables enumerations in a CAN DBC file
    std::string getDBCMessageEnum(uint32_t ID) const;

    //! Get the name of the header file that encompasses this structure definition
    std::string getDefinitionFileName(void) const {return structHeader->fileName();}

    //! Get the name of the header file that encompasses this structure interface functions
    std::string getHeaderFileName(void) const {return header.fileName();}

    //! Get the name of the source file for this structure
    std::string getSourceFileName(void) const {return source.fileName();}

    //! Get the path of the header file that encompasses this structure definition
    std::string getDefinitionFilePath(void) const {return structHeader->filePath();}

    //! Get the path of the header file that encompasses this structure interface functions
    std::string getHeaderFilePath(void) const {return header.filePath();}

    //! Get the path of the source file for this structure
    std::string getSourceFilePath(void) const {return source.filePath();}

    //! Get the name of the header file that encompasses this structure verify functions
    std::string getVerifyHeaderFileName(void) const {return (verifyHeader == nullptr) ? std::string() : verifyHeader->fileName();}

    //! Get the name of the source file that encompasses this structure verify functions
    std::string getVerifySourceFileName(void) const {return (verifySource == nullptr) ? std::string() : verifySource->fileName();}

    //! Get the path of the header file that encompasses this structure verify functions
    std::string getVerifyHeaderFilePath(void) const {return (verifyHeader == nullptr) ? std::string() : verifyHeader->filePath();}

    //! Get the path of the source file that encompasses this structure verify functions
    std::string getVerifySourceFilePath(void) const {return (verifySource == nullptr) ? std::string() : verifySource->filePath();}

    //! Get the name of the header file that encompasses this structure comparison functions
    std::string getCompareHeaderFileName(void) const {return (compareHeader == nullptr) ? std::string() : compareHeader->fileName();}

    //! Get the name of the source file that encompasses this structure comparison functions
    std::string getCompareSourceFileName(void) const {return (compareSource == nullptr) ? std::string() : compareSource->fileName();}

    //! Get the path of the header file that encompasses this structure comparison functions
    std::string getCompareHeaderFilePath(void) const {return (compareHeader == nullptr) ? std::string() : compareHeader->filePath();}

    //! Get the path of the source file that encompasses this structure comparison functions
    std::string getCompareSourceFilePath(void) const {return (compareSource == nullptr) ? std::string() : compareSource->filePath();}

    //! Get the name of the header file that encompasses this structure comparison functions
    std::string getPrintHeaderFileName(void) const {return (printHeader == nullptr) ? std::string() : printHeader->fileName();}

    //! Get the name of the source file that encompasses this structure comparison functions
    std::string getPrintSourceFileName(void) const {return (printSource == nullptr) ? std::string() : printSource->fileName();}

    //! Get the path of the header file that encompasses this structure comparison functions
    std::string getPrintHeaderFilePath(void) const {return (printHeader == nullptr) ? std::string() : printHeader->filePath();}

    //! Get the path of the source file that encompasses this structure comparison functions
    std::string getPrintSourceFilePath(void) const {return (printSource == nullptr) ? std::string() : printSource->filePath();}

    //! Get the name of the header file that encompasses this structure map functions
    std::string getMapHeaderFileName(void) const {return (mapHeader == nullptr) ? std::string() : mapHeader->fileName();}

    //! Get the name of the source file that encompasses this structure map functions
    std::string getMapSourceFileName(void) const {return (mapSource == nullptr) ? std::string() : mapSource->fileName();}

    //! Get the path of the header file that encompasses this structure map functions
    std::string getMapHeaderFilePath(void) const {return (mapHeader == nullptr) ? std::string() : mapHeader->filePath();}

    //! Get the path of the source file that encompasses this structure map functions
    std::string getMapSourceFilePath(void) const {return (mapSource == nullptr) ? std::string() : mapSource->filePath();}

protected:

    //! Setup the files, which accounts for all the ways the files can be organized for this structure.
    void setupFiles(std::string moduleName,
                    std::string defheadermodulename,
                    std::string verifymodulename,
                    std::string comparemodulename,
                    std::string printmodulename,
                    std::string mapmodulename,
                    bool forceStructureDeclaration = true, bool outputUtilities = true);

    //! Create utility functions for structure lengths
    std::string createUtilityFunctions(const std::string& spacing) const override;

    //! Issue warnings for the structure module.
    void issueWarnings(const XMLAttribute* map);

    //! Write data to the source and header files to encode and decode this structure and all its children
    void createStructureFunctions(void);

    //! Create the functions that encode/decode sub stuctures.
    void createSubStructureFunctions(void);

    //! Write data to the source and header files to encode and decode this structure but not its children
    void createTopLevelStructureFunctions(void);

    //! Write data to the source and header files for helper functions for this structure but not its children
    void createTopLevelStructureHelperFunctions(void);

    //! Get the text used to print a formatted string function
    static std::string getToFormattedStringFunction(void);

    //! Get the text used to extract text for text read functions
    static std::string getExtractTextFunction(void);

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
};

#endif // PROTOCOLSTRUCTUREMODULE_H
