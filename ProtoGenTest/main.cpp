#include <QDateTime>
#include <iostream>
#include <math.h>
#include "floatspecial.h"
#include "bitfieldspecial.h"
#include "VersionPacket.h"
#include "KeepAlivePacket.h"
#include "GPS.h"
#include "Engine.h"
#include "packetinterface.h"

#define PI 3.141592653589793
#define deg2rad(x) (PI*(x)/180.0)
#define rad2deg(x) (180.0*(x)/PI)


static int testEngineSettingsPacket(void);
static int testEngineCommandPacket(void);
static int testGPSPacket(void);
static int verifyGPSData(GPS_t gps);
static int testKeepAlivePacket(void);
static int testVersionPacket(void);
static int verifyVersionData(Version_t version);

static int fcompare(double input1, double input2, double epsilon);


int main(int argc, char *argv[])
{

    if(testSpecialFloat() == 0)
    {
        std::cout << "Special float failed test" << std::endl;
        return 0;
    }

    if(testBitfield() == 0)
    {
        std::cout << "Bitfield failed test" << std::endl;
        return 0;
    }


    if(testEngineSettingsPacket()==0)
        return 0;

    if(testEngineCommandPacket()==0)
        return 0;

    if(testGPSPacket()== 0)
        return 0;

    if(testVersionPacket() == 0)
        return 0;

    if(testKeepAlivePacket() == 0)
        return 0;

    std::cout << "All tests passed" << std::endl;
    return 1;
}


int testEngineSettingsPacket(void)
{
    testPacket_t pkt;
    EngineSettings_t settings;

    if(getEngineSettingsMinDataLength() != 1)
    {
        std::cout << "Engine Settings minimum data length is wrong" << std::endl;
        return 0;
    }

    settings.gain[0] = 0.1f;
    settings.gain[1] = (float)(-PI);
    settings.gain[2] = 200.0f;
    settings.maxRPM = 8000;
    settings.mode = directRPM;

    encodeEngineSettingsPacketStructure(&pkt, &settings);

    if(pkt.length != 15 )
    {
        std::cout << "Engine settings packet has the wrong length" << std::endl;
        return 0;
    }

    if(pkt.pkttype != 11)
    {
        std::cout << "Engine settings packet has the wrong type" << std::endl;
        return 0;
    }

    if(decodeEngineSettingsPacketStructure(&pkt, &settings))
    {
        if( fcompare(settings.gain[0], 0.1f, 0.00000001)   ||
            fcompare(settings.gain[1], (float)(-PI), 0.00000001)    ||
            fcompare(settings.gain[2], 200.0f, 0.00000001) ||
            fcompare(settings.maxRPM, 8000, 1/4.0959375)   ||
            (settings.mode != directRPM))
        {
            std::cout << "decodeEngineSettingsPacketStructure() yielded incorrect data" << std::endl;
            return 0;
        }
    }
    else
    {
        std::cout << "decodeEngineSettingsPacketStructure() failed" << std::endl;
        return 0;
    }

    // now test the default case
    pkt.length = 1;
    if(decodeEngineSettingsPacketStructure(&pkt, &settings))
    {
         if( fcompare(settings.gain[0], 0.1f, 0.00000001) ||
             fcompare(settings.gain[1], 0.1f, 0.00000001) ||
             fcompare(settings.gain[2], 0.1f, 0.00000001) ||
             fcompare(settings.maxRPM, 10000, 1/4.0959375)||
             (settings.mode != directRPM))
        {
            std::cout << "decodeEngineSettingsPacketStructure() yielded incorrect default data" << std::endl;
            return 0;
        }
    }
    else
    {
        std::cout << "decodeEngineSettingsPacketStructure() failed with defaults" << std::endl;
        return 0;
    }


    return 1;

}


int testEngineCommandPacket(void)
{
    testPacket_t pkt;
    float command = 0.5678f;

    if(getEngineCommandMinDataLength() != 4)
    {
        std::cout << "Engine Command minimum data length is wrong" << std::endl;
        return 0;
    }

    encodeEngineCommandPacket(&pkt, command);


    if(pkt.length != 4 )
    {
        std::cout << "Engine command packet has the wrong length" << std::endl;
        return 0;
    }

    if(pkt.pkttype != 10)
    {
        std::cout << "Engine command packet has the wrong type" << std::endl;
        return 0;
    }

    if(decodeEngineCommandPacket(&pkt, &command))
    {
        if(fcompare(command, 0.5678, 0.0000001))
        {
            std::cout << "decodeEngineCommandPacket() yielded incorrect data" << std::endl;
            return 0;
        }
    }
    else
    {
        std::cout << "decodeEngineCommandPacket() failed" << std::endl;
        return 0;
    }

    return 1;

}

int testGPSPacket(void)
{
    testPacket_t pkt;
    GPS_t gps;

    memset(&gps, 0, sizeof(gps));

    if(getGPSMinDataLength() != 25)
    {
        std::cout << "GPS minimum data length is wrong" << std::endl;
        return 0;
    }

    // 5 days, 11 hours, 32 minutes, 59 seconds, 251 ms
    gps.ITOW = ((((5*24) + 11)*60 + 32)*60 + 59)*1000 + 251;
    gps.Week = 1234;
    gps.PDOP = -2.13;
    gps.PositionLLA.altitude = 169.4;
    gps.PositionLLA.latitude = deg2rad(45.6980142);
    gps.PositionLLA.longitude = deg2rad(-121.5618339);
    gps.VelocityNED.north = 23.311;
    gps.VelocityNED.east = -42.399;
    gps.VelocityNED.down = -.006;
    gps.numSvInfo = 5;
    gps.svInfo[0].azimuth = deg2rad(91);
    gps.svInfo[0].elevation = deg2rad(77);
    gps.svInfo[0].healthy = 1;
    gps.svInfo[0].L1CNo = 50;
    gps.svInfo[0].L2CNo = 33;
    gps.svInfo[0].PRN = 12;
    gps.svInfo[0].tracked = 1;
    gps.svInfo[0].used = 1;
    gps.svInfo[0].visible = 1;

    // Just replicate the data
    gps.svInfo[1] = gps.svInfo[2] = gps.svInfo[3] = gps.svInfo[0];

    // Make a few changes
    gps.svInfo[1].PRN = 13;
    gps.svInfo[1].azimuth = deg2rad(-179.99);
    gps.svInfo[1].elevation = deg2rad(-23);
    gps.svInfo[2].PRN = 23;
    gps.svInfo[2].azimuth = deg2rad(179.1);
    gps.svInfo[2].elevation = deg2rad(66);
    gps.svInfo[3].PRN = 1;
    gps.svInfo[3].azimuth = deg2rad(90);
    gps.svInfo[3].elevation = deg2rad(0);
    gps.svInfo[3].healthy = 0;
    gps.svInfo[3].used = 0;

    encodeGPSPacket(&pkt, &gps);

    if(pkt.length != (25 + 5*6) )
    {
        std::cout << "GPS packet has the wrong length" << std::endl;
        return 0;
    }

    if(pkt.pkttype != 22)
    {
        std::cout << "GPS packet has the wrong type" << std::endl;
        return 0;
    }

    if(decodeGPSPacket(&pkt, &gps))
    {
        if(!verifyGPSData(gps))
        {
            std::cout << "decodeGPSPacket() yielded incorrect data" << std::endl;
            return 0;
        }
    }
    else
    {
        std::cout << "decodeGPSPacket() failed" << std::endl;
        return 0;
    }

    return 1;
}


int verifyGPSData(GPS_t gps)
{
    if(gps.ITOW != ((((5*24) + 11)*60 + 32)*60 + 59)*1000 + 251) return 0;
    if(gps.Week != 1234) return 0;
    if(fcompare(gps.PDOP, 0, 0.1)) return 0;
    if(fcompare(gps.PositionLLA.altitude, 169.4, 1.0/1000)) return 0;
    if(fcompare(gps.PositionLLA.latitude, deg2rad(45.6980142), 1.0/1367130551.152863)) return 0;
    if(fcompare(gps.PositionLLA.longitude, deg2rad(-121.5618339), 1.0/683565275.2581217)) return 0;
    if(fcompare(gps.VelocityNED.north, 23.311, 1.0/100)) return 0;
    if(fcompare(gps.VelocityNED.east, -42.399, 1.0/100)) return 0;
    if(fcompare(gps.VelocityNED.down, -.006, 1.0/100)) return 0;
    if(gps.numSvInfo != 5) return 0;
    if(fcompare(gps.svInfo[0].azimuth, deg2rad(91), 1.0/40.42535554534142)) return 0;
    if(fcompare(gps.svInfo[0].elevation, deg2rad(77), 1.0/40.42535554534142)) return 0;
    if(gps.svInfo[0].healthy != 1) return 0;
    if(gps.svInfo[0].L1CNo != 50) return 0;
    if(gps.svInfo[0].L2CNo != 33) return 0;
    if(gps.svInfo[0].PRN != 12) return 0;
    if(gps.svInfo[0].tracked != 1) return 0;
    if(gps.svInfo[0].used != 1) return 0;
    if(gps.svInfo[0].visible != 1) return 0;

    if(fcompare(gps.svInfo[1].azimuth, deg2rad(-179.99), 1.0/40.42535554534142)) return 0;
    if(fcompare(gps.svInfo[1].elevation, deg2rad(-23), 1.0/40.42535554534142)) return 0;
    if(gps.svInfo[1].healthy != 1) return 0;
    if(gps.svInfo[1].L1CNo != 50) return 0;
    if(gps.svInfo[1].L2CNo != 33) return 0;
    if(gps.svInfo[1].PRN != 13) return 0;
    if(gps.svInfo[1].tracked != 1) return 0;
    if(gps.svInfo[1].used != 1) return 0;
    if(gps.svInfo[1].visible != 1) return 0;

    if(fcompare(gps.svInfo[2].azimuth, deg2rad(179.1), 1.0/40.42535554534142)) return 0;
    if(fcompare(gps.svInfo[2].elevation, deg2rad(66), 1.0/40.42535554534142)) return 0;
    if(gps.svInfo[2].healthy != 1) return 0;
    if(gps.svInfo[2].L1CNo != 50) return 0;
    if(gps.svInfo[2].L2CNo != 33) return 0;
    if(gps.svInfo[2].PRN != 23) return 0;
    if(gps.svInfo[2].tracked != 1) return 0;
    if(gps.svInfo[2].used != 1) return 0;
    if(gps.svInfo[2].visible != 1) return 0;

    if(fcompare(gps.svInfo[3].azimuth, deg2rad(90), 1.0/40.42535554534142)) return 0;
    if(fcompare(gps.svInfo[3].elevation, deg2rad(0), 1.0/40.42535554534142)) return 0;
    if(gps.svInfo[3].healthy != 0) return 0;
    if(gps.svInfo[3].L1CNo != 50) return 0;
    if(gps.svInfo[3].L2CNo != 33) return 0;
    if(gps.svInfo[3].PRN != 1) return 0;
    if(gps.svInfo[3].tracked != 1) return 0;
    if(gps.svInfo[3].used != 0) return 0;
    if(gps.svInfo[3].visible != 1) return 0;

    if(fcompare(gps.svInfo[4].azimuth, deg2rad(0), 1.0/40.42535554534142)) return 0;
    if(fcompare(gps.svInfo[4].elevation, deg2rad(0), 1.0/40.42535554534142)) return 0;
    if(gps.svInfo[4].healthy != 0) return 0;
    if(gps.svInfo[4].L1CNo != 0) return 0;
    if(gps.svInfo[4].L2CNo != 0) return 0;
    if(gps.svInfo[4].PRN != 0) return 0;
    if(gps.svInfo[4].tracked != 0) return 0;
    if(gps.svInfo[4].used != 0) return 0;
    if(gps.svInfo[4].visible != 0) return 0;

    return 1;

}// verifyGPSData


int testKeepAlivePacket(void)
{
    testPacket_t pkt;
    KeepAlive_t keepalive;

    if(getKeepAliveMinDataLength() != 3)
    {
        std::cout << "KeepAlive packet minimum data length is wrong" << std::endl;
        return 0;
    }

    encodeKeepAlivePacketStructure(&pkt);

    if(pkt.length != (2 + strlen("1.0.0.a") + 1))
    {
        std::cout << "KeepAlive packet has the wrong length" << std::endl;
        return 0;
    }

    if(pkt.pkttype != 0)
    {
        std::cout << "KeepAlive packet has the wrong type" << std::endl;
        return 0;
    }

    if(decodeKeepAlivePacket(&pkt, &keepalive.api, keepalive.version))
    {
        if(keepalive.api != 1)
        {
            std::cout << "decodeKeepAlivePacket() yielded incorrect data" << std::endl;
            return 0;
        }

        if(strcmp(keepalive.version, "1.0.0.a") != 0)
        {
            std::cout << "decodeKeepAlivePacket() yielded incorrect data" << std::endl;
            return 0;
        }
    }
    else
    {
        std::cout << "decodeKeepAlivePacket() failed" << std::endl;
        return 0;
    }

    return 1;

}// testKeepAlivePacket


int testVersionPacket(void)
{
    testPacket_t pkt, pkt2;
    Version_t version;

    if(getVersionMinDataLength() != 25)
    {
        std::cout << "Version packet minimum data length is wrong" << std::endl;
        return 0;
    }

    version.major = 1;
    version.minor = 2;
    version.sub = 3;
    version.patch = 4;
    strcpy(version.description, "special testing version");
    version.date.day = QDate::currentDate().day();
    version.date.month = QDate::currentDate().month();
    version.date.year = QDate::currentDate().year();
    version.board.assemblyNumber = 0x12345678;
    version.board.isCalibrated = 1;
    version.board.serialNumber = 0x98765432;
    version.board.manufactureDate.year = 1903;
    version.board.manufactureDate.month = 12;
    version.board.manufactureDate.day = 17;
    version.board.calibratedDate.year = 1969;
    version.board.calibratedDate.month = 7;
    version.board.calibratedDate.day = 20;

    // Two different interfaces the encoding
    encodeVersionPacketStructure(&pkt, &version);
    encodeVersionPacket(&pkt2, &version.board, version.major, version.minor, version.sub, version.patch, &version.date, version.description);

    if(pkt.length != (24 + strlen(version.description) + 1))
    {
        std::cout << "Version packet has the wrong length" << std::endl;
        return 0;
    }

    if(pkt.pkttype != 20)
    {
        std::cout << "Version packet has the wrong type" << std::endl;
        return 0;
    }

    for(int i = 0; i < pkt.length + 2; i++)
    {
        if(((uint8_t*)&pkt)[i] != ((uint8_t*)&pkt2)[i])
        {
            std::cout << "Structure encoded version packet is different than parameter encoded version packet" << std::endl;
            return 0;
        }
    }

    if(decodeVersionPacketStructure(&pkt, &version))
    {
        if(!verifyVersionData(version))
        {
            std::cout << "decodeVersionPacketStructure() yielded incorrect data" << std::endl;
            return 0;
        }
    }
    else
    {
        std::cout << "decodeVersionPacketStructure() failed" << std::endl;
        return 0;
    }

    if(decodeVersionPacket(&pkt2, &version.board, &version.major, &version.minor, &version.sub, &version.patch, &version.date, version.description))
    {
        if(!verifyVersionData(version))
        {
            std::cout << "decodeVersionPacket() yielded incorrect data" << std::endl;
            return 0;
        }
    }
    else
    {
        std::cout << "decodeVersionPacket() failed" << std::endl;
        return 0;
    }

    return 1;

}// testVersionPacket


int verifyVersionData(Version_t version)
{
    if(version.major != 1) return 0;
    if(version.minor != 2) return 0;
    if(version.sub != 3) return 0;
    if(version.patch != 4) return 0;
    if(strcmp(version.description, "special testing version") != 0) return 0;
    if(version.date.day != QDate::currentDate().day()) return 0;
    if(version.date.month != QDate::currentDate().month()) return 0;
    if(version.date.year != QDate::currentDate().year()) return 0;
    if(version.board.assemblyNumber != 0x12345678) return 0;
    if(version.board.isCalibrated != 1) return 0;
    if(version.board.serialNumber != 0x98765432) return 0;
    if(version.board.manufactureDate.year != 1903) return 0;
    if(version.board.manufactureDate.month != 12) return 0;
    if(version.board.manufactureDate.day != 17) return 0;
    if(version.board.calibratedDate.year != 1969) return 0;
    if(version.board.calibratedDate.month != 7) return 0;
    if(version.board.calibratedDate.day != 20) return 0;

    return 1;
}


int fcompare(double input1, double input2, double epsilon)
{
    if(fabs(input1 - input2) > epsilon)
        return 1;
    else
        return 0;
}
