#-------------------------------------------------
#
# Project created by QtCreator 2014-01-21T12:40:03
#
#-------------------------------------------------

QT       += core gui

TARGET = visor
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    imagestitcher.cpp \
    objectrecognizer.cpp \
    sharedfunctions.cpp \
    customgraphicsview.cpp \
    customslider.cpp

HEADERS  += mainwindow.h \
    imagestitcher.h \
    objectrecognizer.h \
    sharedfunctions.h \
    customgraphicsview.h \
    customslider.h

FORMS    += mainwindow.ui

INCLUDEPATH +=  `pkg-config --cflags opencv`

LIBS += -L/usr/local/lib
LIBS +=         `pkg-config --libs opencv`
