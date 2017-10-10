
TARGET = AppLauncher
TEMPLATE = app

QT -= gui

CONFIG += c++11
unix:CONFIG += console
CONFIG += qt
CONFIG += warn_on
CONFIG += release
CONFIG -= app_bundle

win32 {
    #RC_ICONS = AppLauncher.ico
    QMAKE_TARGET_PRODUCT = "Application launcher"
    QMAKE_TARGET_DESCRIPTION = "Launch the application with arguments in settings"
    QMAKE_TARGET_COMPANY = "Home"
    QMAKE_TARGET_COPYRIGHT = "Copyright 2008-2017 WangHong. All rights reserved."
}

unix:DESTDIR += $$_PRO_FILE_PWD_
win32:DESTDIR += $$_PRO_FILE_PWD_/release
OBJECTS_DIR += objects
MOC_DIR += objects
RCC_DIR += objects
UI_DIR += objects

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += main.cpp \
    controller.cpp \
    subprocmgr.cpp \
    subprocess.cpp \
    settings.cpp

HEADERS += \
    controller.h \
    subprocmgr.h \
    subprocess.h \
    settings.h

DISTFILES +=
