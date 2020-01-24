#-------------------------------------------------
#
# Project created by QtCreator 2018-10-26T14:36:02
#
#-------------------------------------------------

QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets


TARGET = update
TEMPLATE = app
INSTALLS += target

target.path = /var/camera

SOURCES += main.cpp\
        updatewindow.cpp

HEADERS  += updatewindow.h

FORMS    += updatewindow.ui

