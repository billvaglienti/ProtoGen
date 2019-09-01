#ifndef PROTOCOLBITFIELD_H
#define PROTOCOLBITFIELD_H


/*!
 * \file
 * Utilities for emitting inline bitfield code and testing code
 */


#include "protocolfile.h"
#include "protocolsupport.h"
#include "protocolparser.h"
#include <QList>
#include <QStringList>

class ProtocolBitfield
{
public:

    //! Perform the test generation, writing out the files
    static void generatetest(ProtocolSupport support);

    //! Compute the maximum value of a field
    static uint64_t maxvalueoffield(int numbits);

    //! Get the encode string for a bitfield
    static QString getEncodeString(QString spacing, QString argument, QString dataname, QString dataindex, int bitcount, int numbits);

    //! Get the encode string for a bitfield
    static QString getDecodeString(QString spacing, QString argument, QString cast, QString dataname, QString dataindex, int bitcount, int numbits);

    //! Get the inner string that does a simple bitfield decode
    static QString getInnerDecodeString(QString dataname, QString dataindex, int bitcount, int numbits);

private:

    //! Get the encode string for a complex bitfield (crossing byte boundaries)
    static QString getComplexEncodeString(QString spacing, QString argument, QString dataname, QString dataindex, int bitcount, int numbits);

    //! Get the encode string for a complex bitfield (crossing byte boundaries)
    static QString getComplexDecodeString(QString spacing, QString argument, QString dataname, QString dataindex, int bitcount, int numbits);

};

#endif // PROTOCOLBITFIELD_H
