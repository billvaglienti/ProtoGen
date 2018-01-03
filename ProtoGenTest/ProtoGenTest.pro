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
    fielddecode.c \
    fieldencode.c \
    floatspecial.c \
    GPS.c \
    scaleddecode.c \
    scaledencode.c \
    TelemetryPacket.c \
    linkcode.c \
    packetinterface.c \
    bitfieldtest.c \
    definitions/verify.c \
    verify/dateverify.c \
    DemolinkProtocol.c \
    compare/compareDemolink.cpp

HEADERS += \
    indices.h \
    DemolinkProtocol.h \
    Engine.h \
    fielddecode.h \
    fieldencode.h \
    floatspecial.h \
    GPS.h \
    scaleddecode.h \
    scaledencode.h \
    TelemetryPacket.h \
    linkcode.h \
    packetinterface.h \
    definitions/EngineDefinitions.hpp \
    bitfieldtest.h \
    definitions/verify.h \
    OtherDefinitions.h \
    verify/dateverify.h \
    compare/compareDemolink.h

OTHER_FILES += \
    Doxyfile

INCLUDEPATH += ./ \
               ./definitions \
               ./verify

#protogen.target = $$PWD/Demolink.markdown
#protogen.commands = $$PWD/../ProtoGenInstall/ProtoGen.exe $$PWD/../exampleprotocol.xml $$PWD -no-doxygen
#protogen.depends = FORCE

#PRE_TARGETDEPS += $$PWD/Demolink.markdown
QMAKE_EXTRA_TARGETS += protogen
