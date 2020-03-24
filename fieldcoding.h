#ifndef FIELDCODING_H
#define FIELDCODING_H

#include "protocolscaling.h"

class FieldCoding : public ProtocolScaling
{
public:

    FieldCoding(ProtocolSupport sup);

    //! Perform the generation, writing out the files
    bool generate(void);

protected:

    //! Get a human readable type name like "unsigned 3 byte integer".
    QString getReadableTypeName(int type);

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
    QString briefEncodeComment(int type, bool bigendian);

    //! Generate the full comment for the encode function
    QString fullEncodeComment(int type, bool bigendian);

    //! Generate the encode function signature
    QString encodeSignature(int type, bool bigendian);

    //! Generate the full encode function
    QString fullEncodeFunction(int type, bool bigendian);

    //! Generate the float encode function
    QString floatEncodeFunction(int type, bool bigendian);

    //! Generate the integer encode function
    QString integerEncodeFunction(int type, bool bigendian);

    //! Generate the one line brief comment for the decode function
    QString briefDecodeComment(int type, bool bigendian);

    //! Generate the full comment for the decode function
    QString fullDecodeComment(int type, bool bigendian);

    //! Generate the decode function signature
    QString decodeSignature(int type, bool bigendian);

    //! Generate the full decode function
    QString fullDecodeFunction(int type, bool bigendian);

    //! Generate the float decode function
    QString floatDecodeFunction(int type, bool bigendian);

    //! Generate the integer decode function
    QString integerDecodeFunction(int type, bool bigendian);

    //! List of built in type names
    QStringList typeNames;

    //! List of type names in function signature
    QStringList typeSigNames;

    //! Size of built in types
    QList<int> typeSizes;

    QList<bool> typeUnsigneds;
};

#endif // FIELDCODING_H
