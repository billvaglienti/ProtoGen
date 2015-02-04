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


SOURCES += main.cpp \
    packetinterface.c \
    bitfieldspecial.c \
    Board.c \
    Date.c \
    Engine.c \
    fielddecode.c \
    fieldencode.c \
    floatspecial.c \
    GPS.c \
    scaleddecode.c \
    scaledencode.c \
    TelemetryPacket.c \
    VersionPacket.c \
    KeepAlivePacket.c

HEADERS += \
    indices.h \
    bitfieldspecial.h \
    Board.h \
    Date.h \
    DemolinkProtocol.h \
    Engine.h \
    fielddecode.h \
    fieldencode.h \
    floatspecial.h \
    GPS.h \
    scaleddecode.h \
    scaledencode.h \
    TelemetryPacket.h \
    VersionPacket.h \
    KeepAlivePacket.h

OTHER_FILES += \
    Doxyfile
