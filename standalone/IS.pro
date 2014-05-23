#-------------------------------------------------
#
# Project created by QtCreator 2014-01-21T12:40:03
#
#-------------------------------------------------

QT       += core gui

TARGET = IS
TEMPLATE = app


SOURCES += mainIS.cpp\
    imagestitcher.cpp \
    sharedfunctions.cpp \ 
    StitchingHandler.cpp

HEADERS  += imagestitcher.h \
    sharedfunctions.h \
	StitchingHandler.h

INCLUDEPATH +=  `pkg-config --cflags opencv`

DESTDIR = bin

LIBS += -L/usr/local/lib
LIBS += `pkg-config --libs opencv`
