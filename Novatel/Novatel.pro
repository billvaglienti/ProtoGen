QT       += core
QT       -= gui

TARGET = Novatel
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

QMAKE_CXXFLAGS += -Wno-unused-parameter

QMAKE_CXXFLAGS_WARN_ON = -Wall -Wno-unused-parameter

SOURCES += main.cpp \
    bitfieldspecial.c \
    fielddecode.c \
    fieldencode.c \
    floatspecial.c \
    scaleddecode.c \
    scaledencode.c \
    NovatelPackets.c \
    NovatelStructures.c \
    NovatelPacket.c \
    NovatelShim.c

HEADERS += \
    bitfieldspecial.h \
    fielddecode.h \
    fieldencode.h \
    floatspecial.h \
    scaleddecode.h \
    scaledencode.h \
    NovatelPackets.h \
    NovatelProtocol.h \
    NovatelStructures.h \
    NovatelPacket.h \
    NovatelShim.h

DISTFILES += \
    novatelprotocol.xml
