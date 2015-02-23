#-------------------------------------------------
#
# Project created by QtCreator 2014-09-02T16:26:57
#
#-------------------------------------------------

QT       += core
QT       += xml

QT       -= gui

TARGET = ProtoGen
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

QMAKE_CXXFLAGS += -Wno-unused-parameter

QMAKE_CXXFLAGS_WARN_ON = -Wall -Wno-unused-parameter

SOURCES += main.cpp \
    protocolparser.cpp \
    protocolpacket.cpp \
    protocolfield.cpp \
    enumcreator.cpp \
    protocolfile.cpp \
    protocolscaling.cpp \
    fieldcoding.cpp \
    encodable.cpp \
    protocolstructure.cpp \
    protocolstructuremodule.cpp \
    protocolsupport.cpp \
    encodedlength.cpp \
    shuntingyard.cpp

HEADERS += \
    protocolparser.h \
    protocolpacket.h \
    protocolfield.h \
    enumcreator.h \
    protocolfile.h \
    protocolscaling.h \
    fieldcoding.h \
    encodable.h \
    protocolstructure.h \
    protocolstructuremodule.h \
    protocolsupport.h \
    encodedlength.h \
    shuntingyard.h

RESOURCES += \
    ProtoGen.qrc

CONFIG(debug, debug|release){
    DEFINES += _DEBUG
}

win32{
    CONFIG(release, debug|release){
        QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$shell_path($$PWD\exampleprotocol.xml)) $$quote($$shell_path($$PWD\ProtoGenInstall\exampleprotocol.xml)) $$escape_expand(\n\t)
        QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$shell_path($$shadowed($$PWD)\release\ProtoGen.exe)) $$quote($$shell_path($$PWD\ProtoGenInstall\ProtoGen.exe)) $$escape_expand(\n\t)
        QMAKE_POST_LINK += $$[QT_INSTALL_BINS]\windeployqt $$shell_path($$quote($$PWD\ProtoGenInstall\ProtoGen.exe)) $$escape_expand(\n\t)
        QMAKE_POST_LINK += $$quote(MultiMarkdown) $$quote($$shell_path($$PWD\README.md)) > $$quote($$shell_path($$PWD\ProtoGenInstall\ProtoGen.html)) $$escape_expand(\n\t)
        QMAKE_POST_LINK += $$quote(\Program Files\7-Zip\7z) a $$quote($$shell_path($$PWD\ProtoGenWin.zip)) $$quote($$shell_path($$PWD\ProtoGenInstall)) $$escape_expand(\n\t)
    }
}

macx{
    CONFIG(release, debug|release){
        # Copy key files to the ProtoGenInstall directory
        QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$shell_path($$PWD\exampleprotocol.xml)) $$quote($$shell_path($$PWD\ProtoGenInstall\exampleprotocol.xml)) $$escape_expand(\n\t)
        QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$shell_path($$shadowed($$PWD)/ProtoGen)) $$quote($$shell_path($$PWD\ProtoGenInstall\ProtoGen)) $$escape_expand(\n\t)

        # Build top level documentation
        QMAKE_POST_LINK += $$quote(/usr/local/bin/MultiMarkdown) $$quote($$shell_path($$PWD/README.md)) > $$quote($$shell_path($$PWD/ProtoGenInstall/ProtoGen.html)) $$escape_expand(\n\t)

        # ProtoGen depends on QtXML, copy it over, and then reset the paths in both the library and ProtoGen
        QMAKE_POST_LINK += $$QMAKE_COPY_DIR $$quote($$shell_path($$[QT_INSTALL_LIBS]/QtXml.framework/Versions/5/QtXml)) $$quote($$shell_path($$PWD/ProtoGenInstall/QtXml)) $$escape_expand(\n\t)
        QMAKE_POST_LINK += install_name_tool -id @executable_path/QtXml $$quote($$shell_path($$PWD/ProtoGenInstall/QtXml)) $$escape_expand(\n\t)
        QMAKE_POST_LINK += install_name_tool -change $$[QT_INSTALL_LIBS]/QtXml.framework/Versions/5/QtXml @executable_path/QtXml $$quote($$shell_path($$PWD/ProtoGenInstall/ProtoGen)) $$escape_expand(\n\t)

        # QtXml depends on QtCore, but it will look in the wrong place for it, so fix that too.
        QMAKE_POST_LINK += install_name_tool -change $$[QT_INSTALL_LIBS]/QtCore.framework/Versions/5/QtCore @executable_path/QtCore $$quote($$shell_path($$PWD/ProtoGenInstall/QtXml)) $$escape_expand(\n\t)

        # ProtoGen depends on QtCore, copy it over, and then reset the paths in both the library and ProtoGen
        QMAKE_POST_LINK += $$QMAKE_COPY_DIR $$quote($$shell_path($$[QT_INSTALL_LIBS]/QtCore.framework/Versions/5/QtCore)) $$quote($$shell_path($$PWD/ProtoGenInstall/QtCore)) $$escape_expand(\n\t)
        QMAKE_POST_LINK += install_name_tool -id @executable_path/QtCore $$quote($$shell_path($$PWD/ProtoGenInstall/QtCore)) $$escape_expand(\n\t)
        QMAKE_POST_LINK += install_name_tool -change $$[QT_INSTALL_LIBS]/QtCore.framework/Versions/5/QtCore @executable_path/QtCore $$quote($$shell_path($$PWD/ProtoGenInstall/ProtoGen)) $$escape_expand(\n\t)

        # Standard library that ProtoGen needs, however this should already be on the users machine
        # QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$shell_path(/usr/lib/libstdc++.6.dylib)) $$quote($$shell_path($$PWD\ProtoGenInstall)) $$escape_expand(\n\t)
        # QMAKE_POST_LINK += install_name_tool -id @executable_path/libstdc++.6.dylib $$quote($$shell_path($$PWD/ProtoGenInstall/libstdc++.6.dylib)) $$escape_expand(\n\t)
        # QMAKE_POST_LINK += install_name_tool -change /usr/lib/libstdc++.6.dylib @executable_path/libstdc++.6.dylib $$quote($$shell_path($$PWD/ProtoGenInstall/ProtoGen)) $$escape_expand(\n\t)

        # Standard library that ProtoGen needs, however this should already be on the users machine
        # QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$shell_path(/usr/lib/libSystem.B.dylib)) $$quote($$shell_path($$PWD\ProtoGenInstall)) $$escape_expand(\n\t)
        # QMAKE_POST_LINK += install_name_tool -id @executable_path/libSystem.B.dylib $$quote($$shell_path($$PWD/ProtoGenInstall/libSystem.B.dylib)) $$escape_expand(\n\t)
        # QMAKE_POST_LINK += install_name_tool -change /usr/lib/libSystem.B.dylib @executable_path/libSystem.B.dylib $$quote($$shell_path($$PWD/ProtoGenInstall/ProtoGen)) $$escape_expand(\n\t)

        QMAKE_POST_LINK += zip -j -r $$quote($$shell_path($$PWD/ProtoGenMac.zip)) $$quote($$shell_path($$PWD\ProtoGenInstall)) $$escape_expand(\n\t)
    }
}

OTHER_FILES += \
    ProtoGenInstall/ProtoGen.txt \
    exampleprotocol.xml \
    README.md
