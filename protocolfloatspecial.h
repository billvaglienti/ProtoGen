#ifndef PROTOCOLFLOATSPECIAL_H
#define PROTOCOLFLOATSPECIAL_H

//! \file Auto magically generate the float special functions

#include "protocolfile.h"
#include "protocolsupport.h"
#include <string>

class ProtocolFloatSpecial
{
public:
    ProtocolFloatSpecial(ProtocolSupport protocolsupport);

    //! Perform the generation, writing out the files
    bool generate(std::vector<std::string>& fileNameList, std::vector<std::string>& filePathList);

protected:

    ProtocolHeaderFile header;
    ProtocolSourceFile source;
    ProtocolSupport support;
};


class FloatSpecialInterface
{

public:

    FloatSpecialInterface(const ProtocolSupport &sup);

    virtual ~FloatSpecialInterface() {;}

    //! Generate the encode header file
    virtual bool generateHeader(ProtocolHeaderFile &header) = 0;

    //! Generate the encode source file
    virtual bool generateSource(ProtocolSourceFile &source) = 0;


protected:

    //! Key information
    const ProtocolSupport &support;


};


class PythonFloatSpecial : public FloatSpecialInterface
{
public:

    PythonFloatSpecial(const ProtocolSupport &sup) : FloatSpecialInterface(sup) {;}

    //! Generate the encode header file
    bool generateHeader(ProtocolHeaderFile &header);

    //! Generate the encode source file
    bool generateSource(ProtocolSourceFile &source);


};


class CandCppFloatSpecial : public FloatSpecialInterface
{
public:

    CandCppFloatSpecial(const ProtocolSupport &sup) : FloatSpecialInterface(sup) {;}

    //! Generate the encode header file
    bool generateHeader(ProtocolHeaderFile &header);

    //! Generate the encode source file
    bool generateSource(ProtocolSourceFile &source);

};

#endif // PROTOCOLFLOATSPECIAL_H
