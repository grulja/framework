include(../common_top.pri)

INCLUDEPATH += ../stubs \
               ../dummyimplugin \
               ../dummyimplugin3 \

# Input
HEADERS += \
    ut_mimpluginmanager.h \
    ../stubs/mimsettings_stub.h \
    ../stubs/fakegconf.h \
    ../stubs/minputcontextconnection_stub.h \
    $$SRC_DIR/mimpluginmanager_p.h \
    $$SRC_DIR/minputmethodplugin.h \
    $$SRC_DIR/minputcontextconnection.h \
    $$SRC_DIR/mimapplication.h \

SOURCES += \
    ut_mimpluginmanager.cpp \
    ../stubs/fakegconf.cpp \
    ../stubs/minputcontextconnection_stub.cpp \
    ../stubs/minputcontextglibdbusconnection_serverinit_stub.cpp \
    $$SRC_DIR/minputcontextconnection.cpp \
    $$SRC_DIR/mimapplication.cpp \

CONFIG += plugin qdbus link_pkgconfig
PKGCONFIG += dbus-glib-1 dbus-1 gconf-2.0

LIBS += \
    $$SRC_DIR/lib$${MALIIT_PLUGINS_LIB}.so \
    -L ../plugins/ \
    -ldummyimplugin \
    -ldummyimplugin3 \

target.files += \
    $$TARGET \
    toolbar1.xml \
    toolbar2.xml \

include(../common_check.pri)
