TEMPLATE = app
TARGET = tests

QT       += core network websockets
# TODO: should be separated to gui and non gui libs
#QT       -= gui
QT += gui widgets

CONFIG   += console c++11
CONFIG   -= app_bundle

include(../project.pri)

SOURCES += main.cpp \
    test_commtcp.cpp \
    utils/testtcpserver.cpp \
    test_channelmanager.cpp \
    test_communication_integration.cpp \
    test_messagereader.cpp \
    utils/testchannel.cpp

HEADERS += \
    utils/testtcpserver.h \
    utils/testchannel.h

INCLUDEPATH += ../src
LIBS += -L../src -lconsolelib


includeStaticLibrary("gberrylib", $${GBERRYLIB_SRC_DIR}, $${GBERRYLIB_BUILD_DIR})

includeStaticLibrary("gmock", $${GMOCK_SRC_DIR}, $${GMOCK_BUILD_DIR})

includeStaticLibrary("gtest", $${GTEST_SRC_DIR}, $${GTEST_BUILD_DIR})

includeStaticLibrary("qhttpserver", $${QHTTPSERVER_SRC_DIR}, $${QHTTPSERVER_BUILD_DIR})
