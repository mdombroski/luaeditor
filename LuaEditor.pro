#-------------------------------------------------
#
# Project created by QtCreator 2016-09-10T19:59:27
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = LuaEditor
TEMPLATE = app


SOURCES += main.cpp\
		MainWindow.cpp \
	LuaForm.cpp \
	LuaHighlighter.cpp \
	LuaThread.cpp \
	CodeEditor.cpp

HEADERS  += MainWindow.h \
	LuaForm.h \
	LuaHighlighter.h \
	LuaThread.h \
	CodeEditor.h

FORMS    += MainWindow.ui \
	LuaForm.ui


CONFIG += link_pkgconfig
PKGCONFIG = lua5.3-c++

DISTFILES += \
    README.md
