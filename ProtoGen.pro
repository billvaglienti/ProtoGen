#-------------------------------------------------
#
# Project created by QtCreator 2014-09-02T16:26:57
#
#-------------------------------------------------

contains(QT_VERSION, ^4.*) {
    message("Cannot build ProtoGen with Qt version $${QT_VERSION}.")
    error("Use at least Qt 5.0")
}

TARGET = ProtoGen
TEMPLATE = app

QT       += core
QT       += xml
QT       -= gui

CONFIG   += console

!macx{
    CONFIG   -= app_bundle
}

macx{
    CONFIG   += app_bundle
}


#QMAKE_CXXFLAGS += -Wno-unused-parameter
#QMAKE_CXXFLAGS_WARN_ON = -wall -wno-unused-variable
#QMAKE_CXXFLAGS_WARN_ON += -wd4100

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
    shuntingyard.cpp \
    protocolcode.cpp \
    protocolbitfield.cpp \
    protocoldocumentation.cpp \
    xmllinelocator.cpp

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
    shuntingyard.h \
    protocolcode.h \
    protocolbitfield.h \
    protocoldocumentation.h \
    xmllinelocator.h

RESOURCES += \
    ProtoGen.qrc

CONFIG(debug, debug|release){
    DEFINES += _DEBUG
}

win32{
    CONFIG(release, debug|release){
        QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$shell_path($$PWD\exampleprotocol.xml)) $$quote($$shell_path($$PWD\ProtoGenInstall\exampleprotocol.xml)) $$escape_expand(\n\t)
        QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$shell_path($$PWD\LICENSE.txt)) $$quote($$shell_path($$PWD\ProtoGenInstall\LICENSE.txt)) $$escape_expand(\n\t)
        QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$shell_path($$shadowed($$PWD)\release\ProtoGen.exe)) $$quote($$shell_path($$PWD\ProtoGenInstall\ProtoGen.exe)) $$escape_expand(\n\t)
        QMAKE_POST_LINK += $$[QT_INSTALL_BINS]\windeployqt $$shell_path($$quote($$PWD\ProtoGenInstall\ProtoGen.exe)) $$escape_expand(\n\t)
        QMAKE_POST_LINK += $$quote(MultiMarkdown) $$quote($$shell_path($$PWD\README.md)) > $$quote($$shell_path($$PWD\ProtoGenInstall\ProtoGen.html)) $$escape_expand(\n\t)
        QMAKE_POST_LINK += $$quote(\Program Files\7-Zip\7z) a $$quote($$shell_path($$PWD\ProtoGenWin.zip)) $$quote($$shell_path($$PWD\ProtoGenInstall)) $$escape_expand(\n\t)
    }
}

macx{
    CONFIG(release, debug|release){
        # Build top level documentation
        QMAKE_POST_LINK += $$quote(/usr/local/bin/MultiMarkdown) $$quote($$shell_path($$PWD/README.md)) > $$quote($$shell_path($$PWD/ProtoGenInstall/ProtoGen.html)) $$escape_expand(\n\t)

        # Run macdeployqt to copy necessary frameworks
        QMAKE_POST_LINK +=$$[QT_INSTALL_BINS]/macdeployqt $$quote($$shadowed($$PWD)/ProtoGen.app) -no-plugins -always-overwrite $$escape_expand(\n\t)

        # Copy key files to the ProtoGenInstall directory
        QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$shell_path($$PWD\exampleprotocol.xml)) $$quote($$shell_path($$PWD/ProtoGenInstall/exampleprotocol.xml)) $$escape_expand(\n\t)
        QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$shell_path($$PWD\LICENSE.txt)) $$quote($$shell_path($$PWD/ProtoGenInstall/LICENSE.txt)) $$escape_expand(\n\t)
        QMAKE_POST_LINK += rm -r -d -f $$quote($$shell_path($$PWD/ProtoGenInstall/ProtoGen.app/)) $$escape_expand(\n\t)
        QMAKE_POST_LINK += $$QMAKE_COPY -r $$quote($$shell_path($$shadowed($$PWD)/ProtoGen.app)) $$quote($$shell_path($$PWD/ProtoGenInstall/ProtoGen.app/)) $$escape_expand(\n\t)

        QMAKE_POST_LINK += tar czvf $$quote($$shell_path($$PWD\ProtoGenMac.tgz)) -C $$quote($$shell_path($$PWD)) ProtoGenInstall$$escape_expand(\n\t)
    }
}

unix:!macx{
    CONFIG(release, debug|release){
        # Copy key files to the ProtoGenInstall directory
        QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$shell_path($$PWD\exampleprotocol.xml)) $$quote($$shell_path($$PWD\ProtoGenInstall)) $$escape_expand(\n\t)
        QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$shell_path($$PWD\LICENSE.txt)) $$quote($$shell_path($$PWD\ProtoGenInstall)) $$escape_expand(\n\t)
        QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$shell_path($$PWD\ProtoGen.sh)) $$quote($$shell_path($$PWD\ProtoGenInstall)) $$escape_expand(\n\t)
        QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$shell_path($$shadowed($$PWD)\ProtoGen)) $$quote($$shell_path($$PWD\ProtoGenInstall)) $$escape_expand(\n\t)

        QMAKE_POST_LINK += $$quote(multimarkdown) $$quote($$shell_path($$PWD\README.md)) > $$quote($$shell_path($$PWD\ProtoGenInstall/ProtoGen.html)) $$escape_expand(\n\t)
        QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$shell_path($$[QT_INSTALL_LIBS]/libQt5Xml.so.5)) $$quote($$shell_path($$PWD/ProtoGenInstall)) $$escape_expand(\n\t)
        QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$shell_path($$[QT_INSTALL_LIBS]/libQt5Core.so.5)) $$quote($$shell_path($$PWD/ProtoGenInstall)) $$escape_expand(\n\t)
        QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$shell_path($$[QT_INSTALL_LIBS]/libicudata.so.5?)) $$quote($$shell_path($$PWD/ProtoGenInstall)) $$escape_expand(\n\t)
        QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$shell_path($$[QT_INSTALL_LIBS]/libicui18n.so.5?)) $$quote($$shell_path($$PWD/ProtoGenInstall)) $$escape_expand(\n\t)
        QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$shell_path($$[QT_INSTALL_LIBS]/libicuuc.so.5?)) $$quote($$shell_path($$PWD/ProtoGenInstall)) $$escape_expand(\n\t)

        QMAKE_POST_LINK += tar czvf $$quote($$shell_path($$PWD\ProtoGenLinux.tgz)) -C $$quote($$shell_path($$PWD)) ProtoGenInstall$$escape_expand(\n\t)
    }
}

OTHER_FILES += \
    exampleprotocol.xml \
    README.md \
    LICENSE.txt

DISTFILES += \
    moredocsfile.txt \
    dependson.xml \
    licensetest.txt
