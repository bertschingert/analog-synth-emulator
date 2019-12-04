#-------------------------------------------------
#
# Project created by QtCreator 2019-12-04T09:42:52
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = analog-synth-emulator
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += main.cpp\
        mainwindow.cpp \
    RtMidi.cpp \
    midi.cpp \
    audio.cpp \
    RtAudio.cpp

HEADERS  += mainwindow.h \
    RtMidi.h \
    midi.h \
    audio.h \
    RtAudio.h

FORMS    += mainwindow.ui

CONFIG += C++14 \

LIBS += -L$$PWD/ -lrtaudio_static -lasound -lpthread

QMAKE_CXXFLAGS += -D__LINUX_ALSA__
QMAKE_CXXFLAGS_WARN_ON += -Wno-unknown-pragmas
