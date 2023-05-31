#-------------------------------------------------
#
# Project created by QtCreator 2016-08-01T09:58:12
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = fif
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp\
        codeeditor.cpp

HEADERS  += mainwindow.h\
        appvars.h\
        codeeditor.h

FORMS    += mainwindow.ui

RC_FILE = fif.rc

RESOURCES += \
    fif.qrc
