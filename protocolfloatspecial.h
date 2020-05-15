#ifndef PROTOCOLFLOATSPECIAL_H
#define PROTOCOLFLOATSPECIAL_H

/*!
 * \file
 * Auto magically generate the float special functions
 */


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

    //! Generate the encode header file
    bool generateHeader(void);

    //! Generate the encode source file
    bool generateSource(void);

    ProtocolHeaderFile header;
    ProtocolSourceFile source;
    ProtocolSupport support;
};

#endif // PROTOCOLFLOATSPECIAL_H
