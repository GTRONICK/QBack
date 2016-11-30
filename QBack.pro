#-------------------------------------------------
#
# Project created by QtCreator 2016-08-01T09:01:50
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QBack
TEMPLATE = app


SOURCES += main.cpp\
        bumain.cpp \
    worker.cpp \
    logviewer.cpp

HEADERS  += bumain.h \
    worker.h \
    logviewer.h

FORMS    += bumain.ui \
    logviewer.ui

RESOURCES += \
    img/icons.qrc

OTHER_FILES +=
