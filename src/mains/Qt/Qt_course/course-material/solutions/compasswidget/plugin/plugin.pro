TEMPLATE     = lib
CONFIG      += qt warn_on release plugin
SOURCES  += plugin.cpp ../compass.cpp ../compass2.cpp
HEADERS  += plugin.h ../compass.h ../compass2.h
DESTDIR   = $(QTDIR)/plugins/designer
TARGET    = compass
INCLUDEPATH = ../ .
target.path=$$plugins.path
isEmpty(target.path):target.path=$$QT_PREFIX/plugins
unix:MOC_DIR = .moc
unix:OBJECTS_DIR = .obj
