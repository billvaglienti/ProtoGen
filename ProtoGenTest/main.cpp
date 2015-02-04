#include <QCoreApplication>
#include "floatspecial.h"
#include "bitfieldspecial.h"

int main(int argc, char *argv[])
{
    testSpecialFloat();
    testBitfield();

    QCoreApplication a(argc, argv);

    return a.exec();
}
