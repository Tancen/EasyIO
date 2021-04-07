QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += ../../../../src/

SOURCES += main.cpp\
        ../../../../src/EasyIOAutoBuffer.cpp \
        ../../../../src/EasyIOContext_linux.cpp \
        ../../../../src/EasyIOContext_win.cpp \
        ../../../../src/EasyIOEventLoopGroup.cpp \
        ../../../../src/EasyIOEventLoop_linux.cpp \
        ../../../../src/EasyIOEventLoop_win.cpp \
        ../../../../src/EasyIOTCPAcceptor.cpp \
        ../../../../src/EasyIOTCPClient_linux.cpp \
        ../../../../src/EasyIOTCPClient_win.cpp \
        ../../../../src/EasyIOTCPConnection_linux.cpp \
        ../../../../src/EasyIOTCPConnection_win.cpp \
        ../../../../src/EasyIOTCPServer_linux.cpp \
        ../../../../src/EasyIOTCPServer_win.cpp \
        ../../../../src/EasyIOUDPClient.cpp \
        ../../../../src/EasyIOUDPServer.cpp \
        ../../../../src/EasyIOUDPSocket_linux.cpp \
        ../../../../src/EasyIOUDPSocket_win.cpp \
        widget.cpp

HEADERS  += widget.h \
    ../../../../src/EasyIO.h \
    ../../../../src/EasyIOAutoBuffer.h \
    ../../../../src/EasyIOContext_linux.h \
    ../../../../src/EasyIOContext_win.h \
    ../../../../src/EasyIODef.h \
    ../../../../src/EasyIOEventLoopGroup.h \
    ../../../../src/EasyIOEventLoop_linux.h \
    ../../../../src/EasyIOEventLoop_win.h \
    ../../../../src/EasyIOIEventLoop.h \
    ../../../../src/EasyIOTCPAcceptor.h \
    ../../../../src/EasyIOTCPClient_linux.h \
    ../../../../src/EasyIOTCPClient_win.h \
    ../../../../src/EasyIOTCPConnection_linux.h \
    ../../../../src/EasyIOTCPConnection_win.h \
    ../../../../src/EasyIOTCPIClient.h \
    ../../../../src/EasyIOTCPIConnection.h \
    ../../../../src/EasyIOTCPIServer.h \
    ../../../../src/EasyIOTCPServer_linux.h \
    ../../../../src/EasyIOTCPServer_win.h \
    ../../../../src/EasyIOUDPClient.h \
    ../../../../src/EasyIOUDPIClient.h \
    ../../../../src/EasyIOUDPIServer.h \
    ../../../../src/EasyIOUDPISocket.h \
    ../../../../src/EasyIOUDPServer.h \
    ../../../../src/EasyIOUDPSocket_linux.h \
    ../../../../src/EasyIOUDPSocket_win.h \
    ../../../test_config.h

FORMS += \
    Widget.ui

windows:{
    LIBS += -lWs2_32
    msvc:{
        QMAKE_CFLAGS += /utf-8
        QMAKE_CXXFLAGS += /utf-8
    }
}



