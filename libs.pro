LIBVERSION = 0.28

INCLUDEPATH += $${SYSROOT}$${PREFIX}/include/mythtv
INCLUDEPATH += $${SYSROOT}$${PREFIX}/include/mythtv/libmythbase
INCLUDEPATH += $${SYSROOT}$${PREFIX}/include/mythtv/libmythui
INCLUDEPATH += $${SYSROOT}$${PREFIX}/include/mythtv/libmyth
DEPENDPATH *= $${INCLUDEPATH}

LIBS += -L$${LIBDIR} $$EXTRA_LIBS -lmythbase-$$LIBVERSION
LIBS += -lmyth-$$LIBVERSION -lmythui-$$LIBVERSION 
