TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    bitfieldspecial.c \
    fielddecode.c \
    fieldencode.c \
    floatspecial.c \
    scaleddecode.c \
    scaledencode.c \
    NovatelPackets.c \
    NovatelStructures.c \
    NovatelProtocol.c

HEADERS += \
    bitfieldspecial.h \
    fielddecode.h \
    fieldencode.h \
    floatspecial.h \
    scaleddecode.h \
    scaledencode.h \
    NovatelPackets.h \
    NovatelProtocol.h \
    NovatelStructures.h
