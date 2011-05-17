include(../common_top.pri)

QUICK_DIR = ../../input-method-quick
INCLUDEPATH += \
    $$SRC_DIR \
    $$QUICK_DIR \
    
DEFINES += UNIT_TEST
QT += core gui declarative

# Input
HEADERS += \
    ut_minputmethodquickplugin.h \
    $$SRC_DIR/mabstractinputmethod.h \
    $$SRC_DIR/mabstractinputmethodhost.h \
    $$SRC_DIR/minputmethodhost.h \
    $$QUICK_DIR/minputmethodquick.h \
    $$QUICK_DIR/minputmethodquickplugin.h \

SOURCES += \
    ut_minputmethodquickplugin.cpp \
    $$SRC_DIR/mabstractinputmethod.cpp \
    $$SRC_DIR/mabstractinputmethodhost.cpp \
    $$SRC_DIR/minputmethodhost.cpp \
    $$QUICK_DIR/minputmethodquick.cpp \
    $$QUICK_DIR/minputmethodquickplugin.cpp \

LIBS += \
    $$SRC_DIR/libmeegoimframework.so.0 \
    $$QUICK_DIR/libmeegoimquick.so.0 \

RESOURCES = \
    ut_minputmethodquickplugin.qrc

include(../common_check.pri)