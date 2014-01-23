#-------------------------------------------------
#
# Project created by QtCreator 2014-01-21T12:40:03
#
#-------------------------------------------------

QT       += core gui

TARGET = visor
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

INCLUDEPATH +=  `pkg-config --cflags opencv`

LIBS +=         `pkg-config --libs opencv zbar`
