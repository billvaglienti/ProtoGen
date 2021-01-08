#-------------------------------------------------
#
# Project created by QtCreator 2014-09-18T14:24:01
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = ProtoGenTest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

#QMAKE_CXXFLAGS += -Wno-unused-parameter
#QMAKE_CFLAGS += -std=c99
#QMAKE_CFLAGS += -pedantic
#QMAKE_CFLAGS += -Wall


SOURCES += main.cpp \
    Engine.c \
    base_types.c \
    compare/base_compare.cpp \
    compare/base_print.cpp \
    fielddecode.c \
    fieldencode.c \
    floatspecial.c \
    GPS.c \
    globaldependson.c \
    globalenum.c \
    map/base_map.cpp \
    scaleddecode.c \
    scaledencode.c \
    TelemetryPacket.c \
    linkcode.c \
    packetinterface.c \
    bitfieldtest.c \
    definitions/verify.c \
    verify/dateverify.c \
    DemolinkProtocol.c \
    compare/compareDemolink.cpp \
    compare/printDemolink.cpp \
    map/mapDemolink.cpp

HEADERS += \
    base_types.h \
    compare/base_compare.hpp \
    compare/base_print.hpp \
    globaldependson.h \
    indices.h \
    DemolinkProtocol.h \
    Engine.h \
    fielddecode.h \
    fieldencode.h \
    floatspecial.h \
    GPS.h \
    map/base_map.hpp \
    scaleddecode.h \
    scaledencode.h \
    TelemetryPacket.h \
    linkcode.h \
    packetinterface.h \
    definitions/EngineDefinitions.hpp \
    bitfieldtest.h \
    definitions/verify.h \
    OtherDefinitions.h \
    translation.h \
    verify/dateverify.h \
    compare/compareDemolink.hpp \
    compare/printDemolink.hpp \
    map/mapDemolink.hpp

OTHER_FILES += \
    Doxyfile

INCLUDEPATH += ./ \
               ./definitions \
               ./verify \
               ./compare \
               ./map

#protogen.target = $$PWD/Demolink.markdown
#protogen.commands = $$PWD/../ProtoGenInstall/ProtoGen.exe $$PWD/../exampleprotocol.xml $$PWD -no-doxygen
#protogen.depends = FORCE

#PRE_TARGETDEPS += $$PWD/Demolink.markdown
QMAKE_EXTRA_TARGETS += protogen
