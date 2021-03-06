TEMPLATE = app

QT += core jsonserializer
QT -= gui

CONFIG += console
CONFIG -= app_bundle

TARGET = qpmx
QMAKE_TARGET_DESCRIPTION = "qpmx package manager"

HEADERS += \
	installcommand.h \
	command.h \
	pluginregistry.h \
	qpmxformat.h \
	listcommand.h \
	compilecommand.h \
	generatecommand.h \
	topsort.h \
	searchcommand.h \
	uninstallcommand.h \
	initcommand.h \
	translatecommand.h \
	devcommand.h \
	preparecommand.h \
	publishcommand.h \
	createcommand.h \
	hookcommand.h \
	clearcachescommand.h \
	updatecommand.h \
	qbscommand.h \
	bridge.h

SOURCES += main.cpp \
	installcommand.cpp \
	command.cpp \
	pluginregistry.cpp \
	qpmxformat.cpp \
	listcommand.cpp \
	compilecommand.cpp \
	generatecommand.cpp \
	searchcommand.cpp \
	uninstallcommand.cpp \
	initcommand.cpp \
	translatecommand.cpp \
	devcommand.cpp \
	preparecommand.cpp \
	publishcommand.cpp \
	createcommand.cpp \
	hookcommand.cpp \
	clearcachescommand.cpp \
	updatecommand.cpp \
	qbscommand.cpp \
	bridge.cpp

RESOURCES += \
	qpmx.qrc

DISTFILES += \
	completitions/bash/qpmx \
	qbs/module.qbs \
	qbs/dep-base.qbs \
	qbs/MergedStaticLibrary.qbs

include(../submodules/qcliparser/qcliparser.pri)
include(../submodules/qpluginfactory/qpluginfactory.pri)
include(../submodules/qctrlsignals/qctrlsignals.pri)
include(../lib.pri)

include(../submodules/deployment/install.pri)
target.path = $$INSTALL_BINS
INSTALLS += target

unix {
	bashcomp.path = $${INSTALL_SHARE}/bash-completion/completions/
	bashcomp.files = completitions/bash/qpmx
	zshcomp.path = $${INSTALL_SHARE}/zsh/site-functions/
	zshcomp.files = completitions/zsh/_qpmx
	INSTALLS += bashcomp zshcomp
}
