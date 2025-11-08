QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = gwmon
TEMPLATE = app

SOURCES	     = main.cpp \
               SystemTray.cpp \
    axsmbus.cpp \
    i2cbusses.c
HEADERS      = SystemTray.h \
    axsmbus.h
RESOURCES    = images.qrc


unix:!macx: LIBS += -li2c
