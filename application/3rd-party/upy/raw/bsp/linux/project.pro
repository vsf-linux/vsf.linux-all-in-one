TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/../../core
INCLUDEPATH += $$PWD/../../include
INCLUDEPATH += $$PWD/../../luat/lualib
INCLUDEPATH += $$PWD/../../luat/include
INCLUDEPATH += $$PWD/../../luat/packages/lua-cjson
INCLUDEPATH += $$PWD/../../components/freertos/include
INCLUDEPATH += $$PWD/../../components/mbedtls/include
INCLUDEPATH += $$PWD/../../components/mbedtls/include/mbedtls
INCLUDEPATH += $$PWD/../../components/webclient/inc

DEFINES += LUAT_CONF_DISABLE_ROTABLE
DEFINES += LUAT_USE_FS_VFS

unix {
    INCLUDEPATH += $$PWD/../../components/freertos/portable/ThirdParty/GCC/Posix
    INCLUDEPATH += $$PWD/../../components/freertos/portable/ThirdParty/GCC/Posix/utils
    LIBS += -lpthread
}

win32 {
    INCLUDEPATH += $$PWD/../../components/freertos/portable/MSVC-MingW
    INCLUDEPATH += $$PWD/../../lib/x86/linux
    LIBS += -lwinmm
}

include($$PWD/qt/lualib.pri)
include($$PWD/qt/luat.pri)
include($$PWD/qt/freertos.pri)
include($$PWD/qt/mbedtls.pri)
include($$PWD/qt/port.pri)
include($$PWD/qt/upy.pri)

SOURCES += \
        main.c \
    lnewstate.c

HEADERS += \
    include/luat_conf_bsp.h
