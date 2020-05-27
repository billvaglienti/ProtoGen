#include <QDateTime>
#include <iostream>
#include <math.h>
#include "bitfieldtest.hpp"
#include "floatspecial.hpp"
#include "GPS.hpp"
#include "Engine.hpp"
#include "TelemetryPacket.hpp"
#include "packetinterface.h"
#include "linkcode.hpp"
#include "fieldencode.hpp"

#define PI 3.141592653589793
#define PIf 3.141592653589793f
#define deg2rad(x) (PI*(x)/180.0)
#define rad2deg(x) (180.0*(x)/PI)
#define deg2radf(x) (PIf*(x)/180.0f)
#define rad2degf(x) (180.0f*(x)/PIf)

static int testLimits(void);
static int testConstantPacket(void);
static int testTelemetryPacket(void);
static int verifyTelemetryData(Telemetry_c telemetry);
static int testThrottleSettingsPacket(void);
static int testEngineSettingsPacket(void);
static int testEngineCommandPacket(void);
static int testGPSPacket(void);
static void fillOutGPSTest(GPS_c& gps);
static int verifyGPSData(GPS_c gps);
static int testKeepAlivePacket(void);
static int testVersionPacket(void);
static int verifyVersionData(Version_c version);
static int testZeroLengthPacket(void);
static int testBitfieldGroupPacket(void);
static int testMultiDimensionPacket(void);
static int testDefaultStringsPacket(void);

static int fcompare(double input1, double input2, double epsilon);

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    int Return = 1;

    std::cerr << "The next line should say: \"" << translateDemolink("") << "\"" << std::endl;
    std::cerr << packetIds_EnumComment(ENGINESETTINGS) << std::endl << std::endl;

    if(testLimits() == 0)
    {
        std::cout << "Limits failed test" << std::endl;
        Return = 0;
    }

    if(testSpecialFloat() == 0)
    {
        std::cout << "Special float failed test" << std::endl;
        Return = 0;
    }

    if(testBitfield() == 0)
    {
        std::cout << "Bitfield failed test" << std::endl;
        Return = 0;
    }

    if(testConstantPacket() == 0)
        Return = 0;

    if(testTelemetryPacket() == 0)
        Return = 0;

    if(testThrottleSettingsPacket()==0)
        Return = 0;

    if(testEngineSettingsPacket()==0)
        Return = 0;

    if(testEngineCommandPacket()==0)
        Return = 0;

    if(testGPSPacket()== 0)
        Return = 0;

    if(testKeepAlivePacket() == 0)
        Return = 0;

    if(testVersionPacket() == 0)
        Return = 0;

    if(testZeroLengthPacket() == 0)
        Return = 0;

    if(testBitfieldGroupPacket() == 0)
        Return = 0;

    if(testMultiDimensionPacket() == 0)
        Return = 0;

    if(testDefaultStringsPacket() == 0)
        Return = 0;

    if(Return == 1)
        std::cout << "All tests passed" << std::endl;

    return Return;
}


int testLimits(void)
{
    int32_t limittest = 513;

    if(limitMax(limittest, 1000) != 513)
        return 0;

    if(limitMax(limittest, 100) != 100)
        return 0;

    if(limitMin(limittest, 1000) != 1000)
        return 0;

    if(limitMin(limittest, 100) != 513)
        return 0;

    if(limitBoth(limittest, -100, 1000) != 513)
        return 0;

    if(limitBoth(limittest, 1000, 2000) != 1000)
        return 0;

    if(limitBoth(limittest, -100, 100) != 100)
        return 0;

    return 1;
}

int testConstantPacket(void)
{
    testPacket_c pkt;
    Constant_c constant;
    unsigned constant5 = 0;

    constant.encode(&pkt, 127);

    if(pkt.length != (2 + 19 + 4 + 3*1 + 4 + 1 + 1) )
    {
        std::cout << "Constant packet has the wrong length" << std::endl;
        return 0;
    }

    if(pkt.pkttype != 23)
    {
        std::cout << "Constant packet has the wrong type" << std::endl;
        return 0;
    }

    if(Constant_c::decode(&pkt, constant.constant2, &constant.cos45, constant.sin45, &constant.constant3, &constant5, &constant.token))
    {
        constant.constant5 = constant5;

        if( (pkt.data[0] != 0x34)   ||
            (pkt.data[1] != 0x12)   ||
            (constant.token != 127) ||
            (strcmp(constant.constant2, "To be or not to be") != 0) ||
            fcompare(constant.cos45, 0.70710678118654752440084436210485f, 0.00000001f) ||
            fcompare(constant.sin45[0], 0.70710678118654752440084436210485f, 1.0/127) ||
            fcompare(constant.sin45[1], 0.70710678118654752440084436210485f, 1.0/127) ||
            fcompare(constant.sin45[2], 0.70710678118654752440084436210485f, 1.0/127) ||
            (constant.constant3 != 327612) ||
            (constant.constant5 != 13))
        {
            std::cout << "decodeConstantPacket() yielded incorrect data" << std::endl;
            return 0;
        }
    }
    else
    {
        std::cout << "decodeConstantPacket() failed" << std::endl;
        return 0;
    }


    constant.encode(&pkt);
    memset(&constant, 0, sizeof(constant));
    if(constant.decode(&pkt))
    {
        constant.constant5 = constant5;

        if( (pkt.data[0] != 0x34)   ||
            (pkt.data[1] != 0x12)   ||
            (constant.token != 127) ||
            (strcmp(constant.constant2, "To be or not to be") != 0) ||
            fcompare(constant.cos45, 0.70710678118654752440084436210485f, 0.00000001f) ||
            fcompare(constant.sin45[0], 0.70710678118654752440084436210485f, 1.0/127) ||
            fcompare(constant.sin45[1], 0.70710678118654752440084436210485f, 1.0/127) ||
            fcompare(constant.sin45[2], 0.70710678118654752440084436210485f, 1.0/127) ||
            (constant.constant3 != 327612) ||
            (constant.constant5 != 13))
        {
            std::cout << "decodeConstantPacketStructure() yielded incorrect data" << std::endl;
            return 0;
        }
    }
    else
    {
        std::cout << "decodeConstantPacketStructure() failed" << std::endl;
        return 0;
    }


    return 1;

}


int testTelemetryPacket(void)
{
    testPacket_c pkt;
    Telemetry_c telemetry;
    memset(&telemetry, 0, sizeof(telemetry));

    telemetry.insMode = insModeRun;

    telemetry.numGPSs = 1;
    fillOutGPSTest(telemetry.gpsData[0]);

    telemetry.ECEF[0] = 1;
    telemetry.ECEF[1] = 2;
    telemetry.ECEF[2] = 3;

    telemetry.numFueltanks = 3;
    telemetry.fuel[0] = 0;
    telemetry.fuel[1] = 0.001f;
    telemetry.fuel[2] = 1000.0f;

    telemetry.airDataIncluded = 1;
    telemetry.OAT = 300;
    telemetry.staticP = 101325;
    telemetry.dynamicP = 254;

    telemetry.laserStatus = 1;
    telemetry.laserAGL = 131.256f;

    telemetry.magIncluded = 1;
    telemetry.mag[0] = 12.56f;
    telemetry.mag[1] = 85.76f;
    telemetry.mag[2] = -999.9f;
    telemetry.compassHeading = deg2radf(-64.56f);

    telemetry.numControls = 14;
    for(int i = 0; i < telemetry.numControls; i++)
        telemetry.controls[i] = deg2rad(i);

    telemetry.encode(&pkt);

    if(pkt.length != (13 + 1*60 + 1 + 14*2 + 1 + 3*2 + 5 + 10 + 4 + 3*3) )
    {
        std::cout << "Telemetry packet has the wrong length" << std::endl;
        return 0;
    }

    if(pkt.pkttype != 21)
    {
        std::cout << "Telemetry packet has the wrong type" << std::endl;
        return 0;
    }

    memset(&telemetry, 0, sizeof(telemetry));
    if(telemetry.decode(&pkt))
    {
        if(verifyTelemetryData(telemetry) == 0)
        {
            std::cout << "decodeTelemetryPacketStructure() yielded incorrect data" << std::endl;
            return 0;
        }
    }
    else
    {
        std::cout << "decodeTelemetryPacketStructure() failed" << std::endl;
        return 0;
    }

    // Try again, but this time remove the magnetometer and verify the new size
    telemetry.magIncluded = 0;
    telemetry.mag[0] = telemetry.mag[1] = telemetry.mag[2] = telemetry.compassHeading = 0;
    telemetry.encode(&pkt);

    if(pkt.length != (13 + 1*60 + 1 + 14*2 + 1 + 3*2 + 5 + 2 + 4 + 3*3) )
    {
        std::cout << "Telemetry packet has the wrong length" << std::endl;
        return 0;
    }

    if(telemetry.decode(&pkt))
    {
        if(verifyTelemetryData(telemetry) == 0)
        {
            std::cout << "decodeTelemetryPacketStructure() yielded incorrect data" << std::endl;
            return 0;
        }
    }
    else
    {
        std::cout << "decodeTelemetryPacketStructure() failed" << std::endl;
        return 0;
    }

    return 1;
}


int verifyTelemetryData(Telemetry_c telemetry)
{
    if(telemetry.insMode != insModeRun) return 0;

    if(telemetry.numGPSs != 1) return 0;
    if(verifyGPSData(telemetry.gpsData[0]) == 0) return 0;

    // ECEF are not encoded, so they should be zero from our memset
    if(telemetry.ECEF[0] != 0) return 0;
    if(telemetry.ECEF[1] != 0) return 0;
    if(telemetry.ECEF[2] != 0) return 0;

    if(telemetry.numFueltanks != 3) return 0;
    if(fcompare(telemetry.fuel[0], 0, 0.0001f)) return 0;
    if(fcompare(telemetry.fuel[1], 0.001f, 0.0001f)) return 0;
    if(fcompare(telemetry.fuel[2], 1000.0f, 0.0001f)) return 0;

    if(telemetry.airDataIncluded != 1) return 0;
    if(fcompare(telemetry.OAT, 300.0f, 200/256.0)) return 0;
    if(fcompare(telemetry.staticP, 101325.0f, 115000.0/65536.0)) return 0;
    if(fcompare(telemetry.dynamicP, 254.0f, 16200/65536.0)) return 0;

    if(telemetry.laserStatus != 1) return 0;
    if(fcompare(telemetry.laserAGL, 131.256, 150/65536.0)) return 0;

    // We don't test this one because we do both cases
    // if(telemetry.magIncluded != 1) return 0;

    if(telemetry.magIncluded)
    {
        if(fcompare(telemetry.mag[0], 12.56, 100000/32768.0)) return 0;
        if(fcompare(telemetry.mag[1], 85.76, 100000/32768.0)) return 0;
        if(fcompare(telemetry.mag[2], -999.9, 100000/32768.0)) return 0;
        if(fcompare(telemetry.compassHeading, deg2rad(-64.56), deg2rad(180)/32768.0)) return 0;
    }
    else
    {
        // these should have been set to zero and then not changed
        if(telemetry.mag[0] != 0) return 0;
        if(telemetry.mag[1] != 0) return 0;
        if(telemetry.mag[2] != 0) return 0;
        if(fcompare(telemetry.compassHeading, deg2rad(90), deg2rad(180)/32768.0)) return 0;
    }

    if(telemetry.numControls != 14) return 0;
    for(int i = 0; i < telemetry.numControls; i++)
    {
        if(fcompare(telemetry.controls[i], deg2rad(i), 1.5/32768.0)) return 0;
    }

    return 1;
}

int testThrottleSettingsPacket(void)
{
    testPacket_c pkt;
    ThrottleSettings_c settings;

    if(settings.minLength() != 4)
    {
        std::cout << "Throttle Settings minimum data length is wrong" << std::endl;
        return 0;
    }

    settings.numCurvePoints = 5;
    settings.enableCurve = 1;
    settings.highPWM = 2000;
    settings.lowPWM = 1000;
    settings.defaultBitfield = 6;
    for(uint32_t i = 0; i < settings.numCurvePoints; i++)
    {
        settings.curvePoint[i].PWM = settings.lowPWM + i*100;
        settings.curvePoint[i].throttle = i*0.2f;
    }

    settings.encode(&pkt);

    if(pkt.length != (4+3*5+5) )
    {
        std::cout << "Throttle settings packet has the wrong length" << std::endl;
        return 0;
    }

    if(pkt.pkttype != 12)
    {
        std::cout << "Throttle settings packet has the wrong type" << std::endl;
        return 0;
    }

    memset(&settings, 0, sizeof(settings));
    if(settings.decode(&pkt))
    {
        if( (settings.numCurvePoints != 5) ||
            (settings.enableCurve != 1)    ||
            (settings.lowPWM != 1000)      ||
            (settings.highPWM != 2000)     ||
            (settings.defaultBitfield != 6))
        {
            std::cout << "decodeThrottleSettingsPacketStructure() yielded incorrect data" << std::endl;
            return 0;
        }

        for(uint32_t i = 0; i < settings.numCurvePoints; i++)
        {
            if( (settings.curvePoint[i].PWM != settings.lowPWM + i*100) ||
                fcompare(settings.curvePoint[i].throttle, i*0.2f, 1.0/255))
            {
                std::cout << "decodeThrottleSettingsPacketStructure() yielded incorrect data" << std::endl;
                return 0;
            }
        }
    }
    else
    {
        std::cout << "decodeThrottleSettingsPacketStructure() failed" << std::endl;
        return 0;
    }


    // simpler case using defaults
    memset(&settings, 0, sizeof(settings));
    settings.encode(&pkt);
    if(pkt.length != (4+5) )
    {
        std::cout << "Throttle settings packet (#2) has the wrong length" << std::endl;
        return 0;
    }

    // now test the default case
    pkt.length = 4;
    if(settings.decode(&pkt))
    {
        if( (settings.numCurvePoints != 0) ||
            (settings.enableCurve != 0)    ||
            (settings.lowPWM != 1100)      ||
            (settings.highPWM != 1900)     ||
            (settings.defaultBitfield != 0))
        {
            std::cout << "decodeThrottleSettingsPacketStructure() with defaults yielded incorrect data" << std::endl;
            return 0;
        }

    }
    else
    {
        std::cout << "decodeThrottleSettingsPacketStructure() with defaults failed" << std::endl;
        return 0;
    }

    return 1;

}



int testEngineSettingsPacket(void)
{
    testPacket_c pkt;
    EngineSettings_c settings;

    if(settings.minLength() != 1)
    {
        std::cout << "Engine Settings minimum data length is wrong" << std::endl;
        return 0;
    }

    settings.gain[0] = 0.1f;
    settings.gain[1] = (float)(-PI);
    settings.gain[2] = 200.0f;
    settings.maxRPM = 8000;
    settings.mode = directRPM;

    settings.encode(&pkt);

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

    memset(&settings, 0, sizeof(settings));
    if(settings.decode(&pkt))
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
    memset(&settings, 0, sizeof(settings));
    if(settings.decode(&pkt))
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
    testPacket_c pkt;
    EngineCommand_c eng;

    eng.command = 0.5678f;

    if(eng.minLength() != 4)
    {
        std::cout << "Engine Command minimum data length is wrong" << std::endl;
        return 0;
    }

    eng.encode(&pkt);

    if(pkt.length != 5 )
    {
        std::cout << "Engine command packet has the wrong length" << std::endl;
        return 0;
    }

    if(pkt.pkttype != 10)
    {
        std::cout << "Engine command packet has the wrong type" << std::endl;
        return 0;
    }

    eng.command = 0;
    if(eng.decode(&pkt))
    {
        if(fcompare(eng.command, 0.5678, 0.0000001))
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
    testPacket_c pkt;
    GPS_c gps;

    memset(&gps, 0, sizeof(gps));

    if(GPS_c::minLength() != 25)
    {
        std::cout << "GPS minimum data length is wrong" << std::endl;
        return 0;
    }

    fillOutGPSTest(gps);
    gps.encode(&pkt);

    if(pkt.length != (25 + 5*7) )
    {
        std::cout << "GPS packet has the wrong length" << std::endl;
        return 0;
    }

    if(pkt.pkttype != 22)
    {
        std::cout << "GPS packet has the wrong type" << std::endl;
        return 0;
    }

    memset(&gps, 0, sizeof(gps));
    if(gps.decode(&pkt))
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


void fillOutGPSTest(GPS_c& gps)
{
    // 5 days, 11 hours, 32 minutes, 59 seconds, 251 ms
    gps.ITOW = ((((5*24) + 11)*60 + 32)*60 + 59)*1000 + 251;
    gps.Week = 1234;
    gps.PDOP = -2.13f;
    gps.PosLLA.altitude = 169.4;
    gps.PosLLA.latitude = deg2rad(45.6980142);
    gps.PosLLA.longitude = deg2rad(-121.5618339);
    gps.VelocityNED.north = 23.311f;
    gps.VelocityNED.east = -42.399f;
    gps.VelocityNED.down = -.006f;
    gps.numSvInfo = 5;
    gps.svInfo[0].azimuth = deg2radf(91);
    gps.svInfo[0].elevation = deg2radf(77);
    gps.svInfo[0].CNo[GPS_BAND_L1] = 50;
    gps.svInfo[0].CNo[GPS_BAND_L2] = 33;
    gps.svInfo[0].PRN = 12;
    gps.svInfo[0].healthy = true;
    gps.svInfo[0].tracked = true;
    gps.svInfo[0].used = true;
    gps.svInfo[0].visible = true;

    // Just replicate the data
    gps.svInfo[1] = gps.svInfo[2] = gps.svInfo[3] = gps.svInfo[0];

    // Make a few changes
    gps.svInfo[1].PRN = 13;
    gps.svInfo[1].azimuth = deg2radf(-179.99f);
    gps.svInfo[1].elevation = deg2radf(-23);
    gps.svInfo[2].PRN = 23;
    gps.svInfo[2].azimuth = deg2radf(179.1f);
    gps.svInfo[2].elevation = deg2radf(66);
    gps.svInfo[3].PRN = 1;
    gps.svInfo[3].azimuth = deg2radf(90);
    gps.svInfo[3].elevation = deg2radf(0);
    gps.svInfo[3].healthy = 0;
    gps.svInfo[3].used = 0;
}

int verifyGPSData(GPS_c gps)
{
    if(gps.ITOW != ((((5*24) + 11)*60 + 32)*60 + 59)*1000 + 251) return 0;
    if(gps.Week != 1234) return 0;
    if(fcompare(gps.PDOP, 0, 0.1)) return 0;
    if(fcompare(gps.PosLLA.altitude, 169.4, 1.0/1000)) return 0;
    if(fcompare(gps.PosLLA.latitude, deg2rad(45.6980142), 1.0/1367130551.152863)) return 0;
    if(fcompare(gps.PosLLA.longitude, deg2rad(-121.5618339), 1.0/683565275.2581217)) return 0;
    if(fcompare(gps.VelocityNED.north, 23.311, 1.0/100)) return 0;
    if(fcompare(gps.VelocityNED.east, -42.399, 1.0/100)) return 0;
    if(fcompare(gps.VelocityNED.down, -.006, 1.0/100)) return 0;
    if(gps.numSvInfo != 5) return 0;
    if(fcompare(gps.svInfo[0].azimuth, deg2rad(91), 1.0/40.42535554534142)) return 0;
    if(fcompare(gps.svInfo[0].elevation, deg2rad(77), 1.0/40.42535554534142)) return 0;
    if(gps.svInfo[0].CNo[GPS_BAND_L1] != 50) return 0;
    if(gps.svInfo[0].CNo[GPS_BAND_L2] != 33) return 0;
    if(gps.svInfo[0].PRN != 12) return 0;
    if(gps.svInfo[0].healthy != true) return 0;
    if(gps.svInfo[0].tracked != true) return 0;
    if(gps.svInfo[0].used != true) return 0;
    if(gps.svInfo[0].visible != true) return 0;

    if(fcompare(gps.svInfo[1].azimuth, deg2rad(-179.99), 1.0/40.42535554534142)) return 0;
    if(fcompare(gps.svInfo[1].elevation, deg2rad(-23), 1.0/40.42535554534142)) return 0;
    if(gps.svInfo[1].healthy != 1) return 0;
    if(gps.svInfo[1].CNo[GPS_BAND_L1] != 50) return 0;
    if(gps.svInfo[1].CNo[GPS_BAND_L2] != 33) return 0;
    if(gps.svInfo[1].PRN != 13) return 0;
    if(gps.svInfo[1].tracked != 1) return 0;
    if(gps.svInfo[1].used != 1) return 0;
    if(gps.svInfo[1].visible != 1) return 0;

    if(fcompare(gps.svInfo[2].azimuth, deg2rad(179.1), 1.0/40.42535554534142)) return 0;
    if(fcompare(gps.svInfo[2].elevation, deg2rad(66), 1.0/40.42535554534142)) return 0;
    if(gps.svInfo[2].healthy != 1) return 0;
    if(gps.svInfo[2].CNo[GPS_BAND_L1] != 50) return 0;
    if(gps.svInfo[2].CNo[GPS_BAND_L2] != 33) return 0;
    if(gps.svInfo[2].PRN != 23) return 0;
    if(gps.svInfo[2].tracked != 1) return 0;
    if(gps.svInfo[2].used != 1) return 0;
    if(gps.svInfo[2].visible != 1) return 0;

    if(fcompare(gps.svInfo[3].azimuth, deg2rad(90), 1.0/40.42535554534142)) return 0;
    if(fcompare(gps.svInfo[3].elevation, deg2rad(0), 1.0/40.42535554534142)) return 0;
    if(gps.svInfo[3].healthy != 0) return 0;
    if(gps.svInfo[3].CNo[GPS_BAND_L1] != 50) return 0;
    if(gps.svInfo[3].CNo[GPS_BAND_L2] != 33) return 0;
    if(gps.svInfo[3].PRN != 1) return 0;
    if(gps.svInfo[3].tracked != 1) return 0;
    if(gps.svInfo[3].used != 0) return 0;
    if(gps.svInfo[3].visible != 1) return 0;

    if(fcompare(gps.svInfo[4].azimuth, deg2rad(0), 1.0/40.42535554534142)) return 0;
    if(fcompare(gps.svInfo[4].elevation, deg2rad(0), 1.0/40.42535554534142)) return 0;
    if(gps.svInfo[4].healthy != 0) return 0;
    if(gps.svInfo[4].CNo[GPS_BAND_L1] != 0) return 0;
    if(gps.svInfo[4].CNo[GPS_BAND_L2] != 0) return 0;
    if(gps.svInfo[4].PRN != 0) return 0;
    if(gps.svInfo[4].tracked != 0) return 0;
    if(gps.svInfo[4].used != 0) return 0;
    if(gps.svInfo[4].visible != 0) return 0;

    return 1;

}// verifyGPSData


int testKeepAlivePacket(void)
{
    testPacket_c pkt;
    KeepAlive_c keepalive;

    if(keepalive.minLength() != 22)
    {
        std::cout << "KeepAlive packet minimum data length is wrong" << std::endl;
        return 0;
    }

    keepalive.encode(&pkt);

    if(pkt.length != (22))
    {
        std::cout << "KeepAlive packet has the wrong length" << std::endl;
        return 0;
    }

    if(pkt.pkttype != 0)
    {
        std::cout << "KeepAlive packet has the wrong type" << std::endl;
        return 0;
    }

    memset(&keepalive, 0, sizeof(keepalive));
    if(keepalive.decode(&pkt, &keepalive.api, keepalive.version))
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
    testPacket_c pkt, pkt2;
    Version_c version;

    if(Version_c::minLength() != 26)
    {
        std::cout << "Version packet minimum data length is wrong" << std::endl;
        return 0;
    }

    version.major = 1;
    version.minor = 2;
    version.sub = 3;
    version.patch = 4;
    pgstrncpy(version.description, "special testing version", sizeof(version.description));
    version.date.day = QDate::currentDate().day();
    version.date.month = QDate::currentDate().month();
    version.date.year = QDate::currentDate().year();
    version.board.assemblyNumber = 0x12345678;
    version.board.isCalibrated = 1;
    version.board.serialNumber = 0x98765432;
    version.board.manufactureDate.year = 2003;
    version.board.manufactureDate.month = 12;
    version.board.manufactureDate.day = 17;
    version.board.calibratedDate.year = 2069;
    version.board.calibratedDate.month = 7;
    version.board.calibratedDate.day = 20;
    pgstrncpy(version.board.description, "special testing version", sizeof(version.board.description));

    // Two different interfaces for encoding
    version.encode(&pkt);
    Version_c::encode(&pkt2, &version.board, version.major, version.minor, version.sub, version.patch, &version.date, version.description);

    if(pkt.length != (24 + strlen(version.description) + 1 + strlen(version.board.description) + 1))
    {
        std::cout << "Version packet has the wrong length" << std::endl;
        return 0;
    }

    if(pkt.pkttype != 20)
    {
        std::cout << "Version packet has the wrong type" << std::endl;
        return 0;
    }

    std::string diff = Version_c::compare("Version", &pkt, &pkt2);
    if(!diff.empty())
    {
        std::cout << "Structure encoded version packet is different than parameter encoded version packet: " << diff << std::endl;
        return 0;
    }

    memset(&version, 0, sizeof(version));
    if(version.decode(&pkt))
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

    memset(&version, 0, sizeof(version));
    if(Version_c::decode(&pkt2, &version.board, &version.major, &version.minor, &version.sub, &version.patch, &version.date, version.description))
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

    // Encode to and from text using structures
    std::string textversion = version.textPrint("Version");
    memset(&version, 0, sizeof(version));

    if((version.textRead("Version", textversion) != 18) || !verifyVersionData(version))
    {
        std::cout << "textPrintVersion_c() to textReadVersion_c() yielded incorrect data" << std::endl;
        return 0;
    }

    // Encode to and from text using packets
    textversion = version.textPrint("Testing", &pkt);
    memset(&version, 0, sizeof(version));

    if((version.textRead("Testing", textversion) != 18) || !verifyVersionData(version))
    {
        std::cout << "textPrintVersionPacket() to textReadVersion_c() yielded incorrect data" << std::endl;
        return 0;
    }

    return 1;

}// testVersionPacket


int verifyVersionData(Version_c version)
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
    if(version.board.manufactureDate.year != 2003) return 0;
    if(version.board.manufactureDate.month != 12) return 0;
    if(version.board.manufactureDate.day != 17) return 0;
    if(version.board.calibratedDate.year != 2069) return 0;
    if(version.board.calibratedDate.month != 7) return 0;
    if(version.board.calibratedDate.day != 20) return 0;
    if(strcmp(version.board.description, "special testing version") != 0) return 0;

    return 1;
}


int testZeroLengthPacket(void)
{
    testPacket_c pkt;


    if(Zero_c::minLength() != 0)
    {
        std::cout << "Zero length packet minimum data length is wrong" << std::endl;
        return 0;
    }

    Zero_c::encode(&pkt);

    if(pkt.length != 0)
    {
        std::cout << "Zero length packet has the wrong length" << std::endl;
        return 0;
    }

    if(pkt.pkttype != 24)
    {
        std::cout << "Zero length packet has the wrong type" << std::endl;
        return 0;
    }

    if(!Zero_c::decode(&pkt))
    {
        std::cout << "Zero length packet failed to decode" << std::endl;
        return 0;
    }

    return 1;

}


int testBitfieldGroupPacket(void)
{
    BitfieldTester_c bits;
    testPacket_c pkt;

    bits.field1 = 1111;
    bits.field2 = 1;
    bits.field3 = 20;
    bits.field4 = 44739242;
    bits.field5 = 1;
    bits.field6 = 23456248059221ULL;

    bits.encode(&pkt);

    if(pkt.length != 13)
    {
        std::cout << "Bitfield group packet length is wrong" << std::endl;
        return 0;
    }

    bits = BitfieldTester_c();
    bits.decode(&pkt);

    if( (bits.field1 != 1111) ||
        (bits.field2 != 1)    ||
        (bits.field3 != 20)   ||
        (bits.field4 != 44739242) ||
        (bits.field5 != 1)    ||
        (bits.field6 != 23456248059221ULL))
    {
        std::cout << "Bitfield group packet decoded wrong data" << std::endl;
        return 0;
    }

    return 1;
}


int testMultiDimensionPacket(void)
{
    lowPrecisionMultiTable_c table;
    MultiDimensionTable_c* hightable = &table;
    testPacket_c highpkt;
    testPacket_c lowpkt;

    table.numCols = table.numRows = 2;
    for(int row = 0; row < table.numRows; row++)
    {
        for(int col = 0; col < table.numCols; col++)
        {
            table.scaledData[row][col] = table.floatData[row][col] = row*col*(1.0f/3.0f);
            table.intData[row][col] = row + col;
            table.dates[row][col].day = row+1;
            table.dates[row][col].month = col+1;
            table.dates[row][col].year = 2017;
        }

    }

    hightable->encode(&highpkt);
    table.encode(&lowpkt);

    if((highpkt.pkttype != MULTIDIMENSIONTABLE) || (lowpkt.pkttype != LOWPREC_MULTIDIMENSIONTABLE))
    {
        std::cout << "Multi-dimensional packet types are wrong" << std::endl;
        return 0;
    }

    if(highpkt.length != (2 + table.numCols*table.numRows*(4 + 2 + 2 + Date_c::minLength())))
    {
        std::cout << "Multi-dimensional packet size is wrong" << std::endl;
        return 0;
    }

    if(lowpkt.length != (2 + table.numCols*table.numRows*(2 + 1 + 1 + smallDate_c::minLength())))
    {
        std::cout << "Low precision multi-dimensional packet size is wrong" << std::endl;
        return 0;
    }

    table = lowPrecisionMultiTable_c();
    if(!hightable->decode(&highpkt))
    {
        std::cout << "Multi-dimensional packet failed to decode" << std::endl;
        return 0;
    }

    if((table.numCols != 2) || (table.numRows != 2))
    {
        std::cout << "Multi-dimensional packet data are wrong" << std::endl;
        return 0;
    }

    for(int row = 0; row < table.numRows; row++)
    {
        for(int col = 0; col < table.numCols; col++)
        {
            if( fcompare(table.scaledData[row][col],  row*col*(1.0f/3.0f), 0.001f) ||
                fcompare(table.floatData[row][col],  row*col*(1.0f/3.0f), 0.001f)  ||
                (table.intData[row][col] != row + col)  ||
                (table.dates[row][col].day != row+1)    ||
                (table.dates[row][col].month != col+1)  ||
                (table.dates[row][col].year != 2017) )
            {
                std::cout << "Multi-dimensional packet data are wrong" << std::endl;
                return 0;

            }
        }
    }

    table = lowPrecisionMultiTable_c();
    if(!table.decode(&lowpkt))
    {
        std::cout << "Low precision multi-dimensional packet failed to decode" << std::endl;
        return 0;
    }

    if((table.numCols != 2) || (table.numRows != 2))
    {
        std::cout << "Low precision multi-dimensional packet data are wrong" << std::endl;
        return 0;
    }

    for(int row = 0; row < table.numRows; row++)
    {
        for(int col = 0; col < table.numCols; col++)
        {
            if( fcompare(table.scaledData[row][col],  row*col*(1.0f/3.0f), 0.02f) ||
                fcompare(table.floatData[row][col],  row*col*(1.0f/3.0f), 0.001f)  ||
                (table.intData[row][col] != row + col)  ||
                (table.dates[row][col].day != row+1)    ||
                (table.dates[row][col].month != col+1)  ||
                (table.dates[row][col].year != 2017) )
            {
                std::cout << "Low precision multi-dimensional packet data are wrong" << std::endl;
                return 0;

            }
        }
    }

    return 1;

}// testMultiDimensionPacket


int testDefaultStringsPacket(void)
{
    TestWeirdStuff_c test;
    testPacket_c pkt;

    test.Field0 = 0x12345678;
    pgstrncpy(test.Field3, "Field3", sizeof(test.Field3));
    pgstrncpy(test.Field4, "Field4", sizeof(test.Field4));

    test.encode(&pkt);

    if(pkt.length != 47 + 2*3*4)
    {
        std::cout << "Weird stuff packet length is wrong" << std::endl;
        return 0;
    }

    test = TestWeirdStuff_c();
    test.decode(&pkt);
    if( (test.Field0 != 0x12345678) ||
        (strcmp(test.Field3, "Field3") != 0) ||
        (strcmp(test.Field4, "Field4") != 0))
    {
        std::cout << "Weird stuff packet decoded to wrong data" << std::endl;
        return 0;
    }

    // Now test the default functions
    test = TestWeirdStuff_c();
    pkt.length = 40;
    test.decode(&pkt);
    if(strcmp(test.Field4, "secondtest") != 0)
    {
        std::cout << "Weird stuff packet field4 default failed" << std::endl;
        return 0;
    }

    test = TestWeirdStuff_c();
    pkt.length = 39;
    test.decode(&pkt);
    if(strcmp(test.Field3, "test") != 0)
    {
        std::cout << "Weird stuff packet field3 default failed" << std::endl;
        return 0;
    }

    test = TestWeirdStuff_c();
    pkt.length = 43;
    test.decode(&pkt);
    if(strcmp(test.Field4, "Fi") != 0)
    {
        std::cout << "Weird stuff packet field4 decode failed" << std::endl;
        return 0;
    }

    return 1;
}


int fcompare(double input1, double input2, double epsilon)
{
    if(fabs(input1 - input2) > epsilon)
        return 1;
    else
        return 0;
}

