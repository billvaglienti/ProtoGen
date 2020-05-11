#ifndef PROTOCOLSUPPORT_H
#define PROTOCOLSUPPORT_H

#include <QString>
#include "tinyxml2.h"

using namespace tinyxml2;

class ProtocolSupport
{
public:
    ProtocolSupport();

    //! Parse attributes from the ProtocolTag
    void parse(const XMLAttribute* map);

    //! Parse the global file names
    void parseFileNames(const XMLAttribute* map);

    //! Return the list of attributes understood by ProtocolSupport
    QStringList getAttriblist(void) const;

    //! The type of language being output
    typedef enum
    {
        c_language,                 //!< Standard (C99) language rules, also the default
        cpp_language,               //!< C++ language rules
        python_language             //!< Python language rules
    }LanguageType;

    //! Set the language override option, call this before the parse function
    void setLanguageOverride(LanguageType lang) {enablelanguageoverride = true; language = lang;}

    LanguageType language;          //!< Enumerator specifying the language type
    int maxdatasize;                //!< Maximum number of data bytes in a packet, 0 if no limit
    bool int64;                     //!< true if support for integers greater than 32 bits is included
    bool float64;                   //!< true if support for double precision is included
    bool specialFloat;              //!< true if support for float16 and float24 is included
    bool bitfield;                  //!< true if support for bitfields is included
    bool longbitfield;              //!< true to support long bitfields
    bool bitfieldtest;              //!< true to output the bitfield test function
    bool disableunrecognized;       //!< true to disable warnings about unrecognized attributes
    bool bigendian;                 //!< Protocol bigendian flag
    bool supportbool;               //!< true if support for 'bool' is included
    bool limitonencode;             //!< true to enforce verification limits on encode
    bool compare;                   //!< True if the compare function is output for all structures
    bool print;                     //!< True if the textPrint and textRead function is output for all structures
    bool mapEncode;                 //!< True if the mapEncode and mapDecode function is output for all structures
    QString globalFileName;         //!< File name to be used if a name is not given
    QString globalVerifyName;       //!< Verify file name to be used if a name is not given
    QString globalCompareName;      //!< Comparison file name to be used if a name is not given
    QString globalPrintName;        //!< Print file name to be used if a name is not given
    QString globalMapName;          //!< Map file name to be used if a name is not given
    QString outputpath;             //!< path to output files to
    QString packetStructureSuffix;  //!< Name to use at end of encode/decode Packet structure functions
    QString packetParameterSuffix;  //!< Name to use at end of encode/decode Packet parameter functions
    QString protoName;              //!< Name of the protocol
    QString prefix;                 //!< Prefix name
    QString pointerType;            //!< Packet pointer type - default is "void*"
    QString licenseText;            //!< License text to be added to each generated file

protected:

    //! Set to true to enable the language override feature
    bool enablelanguageoverride;
};

#endif // PROTOCOLSUPPORT_H
