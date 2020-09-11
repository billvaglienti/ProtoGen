#ifndef FIELDCODING_H
#define FIELDCODING_H

#include "protocolscaling.h"
#include <QString>
#include <QTextStream>

//! \class FieldCoding generates functions to go from types to bytes or bytes to types
class FieldCoding : public ProtocolScaling
{
public:

    FieldCoding(ProtocolSupport sup);

    //! Perform the generation, writing out the files
    bool generate(std::vector<std::string>& fileNameList, std::vector<std::string>& filePathList);

protected:

    //! List of built in type names
    std::vector<std::string> typeNames;

    //! List of type names in function signature
    std::vector<std::string> typeSigNames;

    //! Size of built in types
    std::vector<int> typeSizes;

    //! list of bools to determine if a type is signed
    std::vector<bool> typeUnsigneds;

};

//! \class Interface class for either python or C and C++ for fieldcoding
class FieldcodingInterface
{

public:

    FieldcodingInterface(const ProtocolSupport &sup, const std::vector<std::string> &typeNames, const std::vector<std::string> &typeSigNames,
           const std::vector<int> &typeSizes, const std::vector<bool> &typeUnsigneds);

    virtual ~FieldcodingInterface() {;}

    //! Generate the encode header file
    virtual bool generateEncodeHeader(ProtocolHeaderFile &header) = 0;

    //! Generate the encode source file
    virtual bool generateEncodeSource(ProtocolSourceFile &source) = 0;

    //! Generate the decode header file
    virtual bool generateDecodeHeader(ProtocolHeaderFile &header) = 0;

    //! Generate the decode source file
    virtual bool generateDecodeSource(ProtocolSourceFile &source) = 0;

protected:

    //! List of built in type names
    const std::vector<std::string> &typeNames;

    //! List of type names in function signature
    const std::vector<std::string> &typeSigNames;

    //! Size of built in types
    const std::vector<int> &typeSizes;

    //! list of bools to determine if a type is signed
    const std::vector<bool> &typeUnsigneds;

    //! Whats supported by the protocol
    const ProtocolSupport &support;

};


//! \class PythonCoding for encoding and decoding functions in python
class PythonCoding : public FieldcodingInterface
{
public:

    PythonCoding(const ProtocolSupport &sup, const std::vector<std::string> &typeNames, const std::vector<std::string> &typeSigNames,
                 const std::vector<int> &typeSizes, const std::vector<bool> &typeUnsigneds);

    //! Generate the encode header file
    bool generateEncodeHeader(ProtocolHeaderFile &header);

    //! Generate the encode source file
    bool generateEncodeSource(ProtocolSourceFile &source);

    //! Generate the decode header file
    bool generateDecodeHeader(ProtocolHeaderFile &header);

    //! Generate the decode source file
    bool generateDecodeSource(ProtocolSourceFile &source);

protected:

    //! Encodes a simple explanation of the encode process
    std::string briefEncodeComment(int type, bool bigendian);

    //! Encodes a simple explanation of the decode process
    std::string briefDecodeComment(int type, bool bigendian);

    //! Get a human readable type name like "unsigned 3 byte integer"
    std::string getReadableTypeName(int type);

    //! generate the signature for encode and decode
    std::string pySignature(int type, bool bigendian, bool encode);

    //! generate the format string for base types
    std::string pyFormat(int type, bool bigendian);

    //! generate the second format string for the special size case
    std::string secondFormat(int fullSize, bool bigendian, bool unSigned);

    //! Generate the full encode function and all its peices
    std::string fullPyEncodeFunction(int type, bool bigendian);

    //! Generate the doc string for the encode functions
    std::string pyEncodeComment(int type, bool bigendian);

    //! build the function from the signature, comment and format string
    std::string pyEncodeFunction(std::string signature, std::string comment, std::string format, int type, bool bigendian);

    //! encode the non standard size types including 24, 40, 48, 56
    std::string pyEncodeSpecialSize(std::string function, int type, bool bigendian);

    //! encode the non standard size floats including 16 and 24
    std::string pyEncodeSpecialFloat(std::string function, int type, bool bigendian);

    //! Generate the full decode function and all its peices
    std::string fullPyDecodeFunction(int type, bool bigendian);

    //! Generate the doc string for the decode functions
    std::string pyDecodeComment(int type, bool bigendian);

    //! build the function from the signature, comment and format string
    std::string pyDecodeFunction(std::string signature, std::string comment, std::string format, int type, bool bigendian);

    //! decode the non standard size types like 24, 40, 48, 56
    std::string pyDecodeSpecialSize(std::string function, int type, bool bigendian);

    //! decode the non standard size floats including 16 and 24
    std::string pyDecodeSpecialFloat(std::string function, int type, bool bigendian);


private:
    //! true if the size is non standard
    bool specialSize;
};


//! \class CandCppCoding for encoding and decoding functions in C or C++
class CandCppCoding : public FieldcodingInterface
{
public:

    CandCppCoding(const ProtocolSupport &sup, const std::vector<std::string> &typeNames, const std::vector<std::string> &typeSigNames,
                  const std::vector<int> &typeSizes, const std::vector<bool> &typeUnsigneds);

    //! Generate the encode header file
    bool generateEncodeHeader(ProtocolHeaderFile &header);

    //! Generate the encode source file
    bool generateEncodeSource(ProtocolSourceFile &source);

    //! Generate the decode header file
    bool generateDecodeHeader(ProtocolHeaderFile &header);

    //! Generate the decode source file
    bool generateDecodeSource(ProtocolSourceFile &source);

protected:

    //! Get a human readable type name like "unsigned 3 byte integer".
    std::string getReadableTypeName(int type);

    //! Generate the one line brief comment for the encode function
    std::string briefEncodeComment(int type, bool bigendian);

    //! Generate the one line brief comment for the decode function
    std::string briefDecodeComment(int type, bool bigendian);

    //! Generate the helper functions in the encode source file
    void generateEncodeHelpers(void);

    //! Generate the full comment for the encode function
    std::string fullEncodeComment(int type, bool bigendian);

    //! Generate the encode function signature
    std::string encodeSignature(int type, bool bigendian);

    //! Generate the full encode function
    std::string fullEncodeFunction(int type, bool bigendian);

    //! Generate the float encode function
    std::string floatEncodeFunction(int type, bool bigendian);

    //! Generate the integer encode function
    std::string integerEncodeFunction(int type, bool bigendian);

    //! Generate the full comment for the decode function
    std::string fullDecodeComment(int type, bool bigendian);

    //! Generate the decode function signature
    std::string decodeSignature(int type, bool bigendian);

    //! Generate the full decode function
    std::string fullDecodeFunction(int type, bool bigendian);

    //! Generate the float decode function
    std::string floatDecodeFunction(int type, bool bigendian);

    //! Generate the integer decode function
    std::string integerDecodeFunction(int type, bool bigendian);

};


#endif // FIELDCODING_H





