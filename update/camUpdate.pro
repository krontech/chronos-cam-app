#-------------------------------------------------
#
# Project created by QtCreator 2018-10-26T14:36:02
#
#-------------------------------------------------

QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = camUpdate
TEMPLATE = app
INSTALLS += target

QMAKE_CXXFLAGS += -std=c++11

target.path = /usr/bin

SOURCES += main.cpp \
        updatewindow.cpp

HEADERS  += updatewindow.h

FORMS    += updatewindow.ui

