QT       += core gui opengl openglwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    viewerwidget.cpp

HEADERS += \
    mainwindow.h \
    viewerwidget.h

INCLUDEPATH += ../UglylabLib

LIBS += -L/home/o/Projects/QtProjects/BUILDS/build-UglylabLib-Desktop_Qt_6_9_0-Debug -lUglylabLib

DISTFILES += \
    LICENSE \
    README.md
