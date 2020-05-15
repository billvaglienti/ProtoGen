#ifndef PROTOCOLBITFIELD_H
#define PROTOCOLBITFIELD_H


/*!
 * \file
 * Utilities for emitting inline bitfield code and testing code
 */


#include "protocolfile.h"
#include "protocolsupport.h"
#include "protocolparser.h"
#include <string>

class ProtocolBitfield
{
public:

    //! Perform the test generation, writing out the files
    static void generatetest(ProtocolSupport support);

    //! Compute the maximum value of a field
    static uint64_t maxvalueoffield(int numbits);

    //! Get the encode string for a bitfield
    static std::string getEncodeString(const std::string& spacing, const std::string& argument, const std::string& dataname, const std::string& dataindex, int bitcount, int numbits);

    //! Get the encode string for a bitfield
    static std::string getDecodeString(const std::string& spacing, const std::string& argument, const std::string& cast, const std::string& dataname, const std::string& dataindex, int bitcount, int numbits);

    //! Get the inner string that does a simple bitfield decode
    static std::string getInnerDecodeString(const std::string& dataname, const std::string& dataindex, int bitcount, int numbits);

private:

    //! Get the encode string for a complex bitfield (crossing byte boundaries)
    static std::string getComplexEncodeString(const std::string& spacing, const std::string& argument, const std::string& dataname, const std::string& dataindex, int bitcount, int numbits);

    //! Get the encode string for a complex bitfield (crossing byte boundaries)
    static std::string getComplexDecodeString(const std::string& spacing, const std::string& argument, const std::string& dataname, const std::string& dataindex, int bitcount, int numbits);

};

#endif // PROTOCOLBITFIELD_H
