ADS_OUT_ROOT = $${OUT_PWD}/../..

QT       += core gui widgets # openglwidgets

TARGET = CentralWidgetExample
DESTDIR = $${ADS_OUT_ROOT}/lib
TEMPLATE = app
CONFIG += c++14
CONFIG += debug_and_release
adsBuildStatic {
    DEFINES += ADS_STATIC
}

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    centralwidget.cpp \
    main.cpp \
    mainwindow.cpp \
    toolpanel.cpp

HEADERS += \
    centralwidget.h \
    mainwindow.h \
    toolpanel.h

FORMS += \
    mainwindow.ui

LIBS += -L$${ADS_OUT_ROOT}/lib
include(../../ads.pri)
INCLUDEPATH += ../../src
DEPENDPATH += ../../src

