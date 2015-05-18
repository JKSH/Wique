# -------------------------------------------------
# Project created by QtCreator 2010-11-21T17:03:48
# -------------------------------------------------
QT += network widgets sql
CONFIG += C++11
TARGET = Wique
TEMPLATE = app
SOURCES += main.cpp \
	database.cpp \
    datacoordinator.cpp \
    wikiquerier.cpp \
    gui/databaseui.cpp \
    gui/spreadsheetview.cpp
HEADERS += \
	database.h \
    datacoordinator.h \
    wikiquerier.h \
    gui/databaseui.h \
    gui/spreadsheetview.h

FORMS += \
    gui/databaseui.ui
