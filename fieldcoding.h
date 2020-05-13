#ifndef FIELDCODING_H
#define FIELDCODING_H

#include "protocolscaling.h"

class FieldCoding : public ProtocolScaling
{
public:

    FieldCoding(ProtocolSupport sup);

    //! Perform the generation, writing out the files
    bool generate(std::vector<std::string>& fileNameList, std::vector<std::string>& filePathList);

protected:

    //! Get a human readable type name like "unsigned 3 byte integer".
    std::string getReadableTypeName(int type);

    //! Generate the encode header file
    bool generateEncodeHeader(void);

    //! Generate the encode source file
    bool generateEncodeSource(void);

    //! Generate the helper functions in the encode source file
    void generateEncodeHelpers(void);

    //! Generate the decode header file
    bool generateDecodeHeader(void);

    //! Generate the decode source file
    bool generateDecodeSource(void);

    //! Generate the one line brief comment for the encode function
    std::string briefEncodeComment(int type, bool bigendian);

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

    //! Generate the one line brief comment for the decode function
    std::string briefDecodeComment(int type, bool bigendian);

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

    //! List of built in type names
    std::vector<std::string> typeNames;

    //! List of type names in function signature
    std::vector<std::string> typeSigNames;

    //! Size of built in types
    QList<int> typeSizes;

    QList<bool> typeUnsigneds;
};

#endif // FIELDCODING_H
