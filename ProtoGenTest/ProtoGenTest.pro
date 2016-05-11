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

QMAKE_CXXFLAGS += -Wno-unused-parameter

QMAKE_CFLAGS += -std=c99
QMAKE_CFLAGS += -pedantic
QMAKE_CFLAGS += -Wall


SOURCES += main.cpp \
    bitfieldspecial.c \
    Engine.c \
    fielddecode.c \
    fieldencode.c \
    floatspecial.c \
    GPS.c \
    scaleddecode.c \
    scaledencode.c \
    TelemetryPacket.c \
    linkcode.c \
    packetinterface.c

HEADERS += \
    indices.h \
    bitfieldspecial.h \
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
    packetinterface.h

OTHER_FILES += \
    Doxyfile
