include ( ../mythconfig.mak )
include ( ../libs.pro )

QT += network xml sql widgets
TEMPLATE = lib
CONFIG += debug plugin 
TARGET = mythmpc
target.path = $${LIBDIR}/mythtv/plugins
INSTALLS += target

uifiles.path = /usr/share/mythtv/themes/default/
uifiles.files = theme/*


INSTALLS += uifiles

# Input
HEADERS += mpcui.h  mpcconf.h

SOURCES += main.cpp mpcui.cpp mpcconf.cpp

macx {
    QMAKE_LFLAGS += -flat_namespace -undefined suppress
}
