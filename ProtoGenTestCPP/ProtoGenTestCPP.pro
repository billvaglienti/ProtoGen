QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        DemolinkProtocol.cpp \
        Engine.cpp \
        GPS.cpp \
        TelemetryPacket.cpp \
        base_types.cpp \
        bitfieldtest.cpp \
        compare/base_compare.cpp \
        compare/base_print.cpp \
        compare/compareDemolink.cpp \
        compare/printDemolink.cpp \
        definitions/verify.cpp \
        fielddecode.cpp \
        fieldencode.cpp \
        floatspecial.c \
        globaldependson.cpp \
        linkcode.cpp \
        main.cpp \
        map/base_map.cpp \
        map/mapDemolink.cpp \
        packetinterface.cpp \
        scaleddecode.cpp \
        scaledencode.cpp \
        verify/dateverify.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    DemolinkProtocol.h \
    Engine.h \
    GPS.h \
    OtherDefinitions.h \
    TelemetryPacket.h \
    base_types.h \
    bitfieldtest.h \
    compare/base_compare.h \
    compare/base_print.h \
    compare/compareDemolink.h \
    compare/printDemolink.h \
    definitions/EngineDefinitions.hpp \
    definitions/verify.h \
    fielddecode.h \
    fieldencode.h \
    floatspecial.h \
    globaldependson.h \
    globalenum.h \
    linkcode.h \
    map/base_map.h \
    map/mapDemolink.h \
    packetinterface.h \
    scaleddecode.h \
    scaledencode.h \
    verify/dateverify.h

INCLUDEPATH += \
    verify \
    compare \
    map \
    definitions
