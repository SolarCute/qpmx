TEMPLATE = lib

QT += core
QT -= gui

TARGET = qpmsource
VERSION = $$QPMXVER
CONFIG += plugin

DESTDIR = $$OUT_PWD/../qpmx

HEADERS += \
		qpmsourceplugin.h

SOURCES += \
		qpmsourceplugin.cpp

DISTFILES += qpmsource.json
json_target.target = moc_qpmsourceplugin.o
json_target.depends += $$PWD/qpmsource.json
QMAKE_EXTRA_TARGETS += json_target

include(../../install.pri)
target.path = $${INSTALL_PLUGINS}/qpmx
INSTALLS += target

