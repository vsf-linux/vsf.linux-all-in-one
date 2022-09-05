TEMPLATE = app
CONFIG += console

INCLUDEPATH += "./core"
INCLUDEPATH += "./include"
INCLUDEPATH += "./modules/inc"

SOURCES += components/elua/lapi.c \
           components/elua/lauxlib.c

SOURCES += main.c \
    core/evm.c \
    core/upy.c

HEADERS += \
    core/upy.h
