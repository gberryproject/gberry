TEMPLATE = app
TARGET = mainui

QT += qml quick network widgets
CONFIG += c++11

include(project.pri)
SOURCES += main.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH = $$PWD/ui/gberry-lib $$PWD/ui/console-lib

CONFIG(debug, debug|release) {
    DEFINES += GBERRY_DEBUG_QML_IMPORT_PATH=$$PWD/ui/gberry-lib:$$PWD/ui/console-lib
}

target.path = $${DEPLOY_DIR}/bin/
INSTALLS += target

includeStaticLibrary("gberrylib", $${GBERRYLIB_SRC_DIR}, $${GBERRYLIB_BUILD_DIR})

includeStaticLibrary("qhttpserver", $${QHTTPSERVER_SRC_DIR}, $${QHTTPSERVER_BUILD_DIR})

includeSharedLibrary("consolelib", $${CONSOLELIB_SRC_DIR}, $${CONSOLELIB_BUILD_DIR})
