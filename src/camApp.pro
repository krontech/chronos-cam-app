#------------------------------------------------------------------------------
#   Copyright (C) 2013-2017 Kron Technologies Inc <http://www.krontech.ca>.
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
#------------------------------------------------------------------------------

QT       += core gui
QT       += dbus

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets multimedia

TARGET = camApp
CONFIG += qt console link_pkgconfig

INCLUDEPATH += $${QT_SYSROOT}/usr/include
QMAKE_CFLAGS += -pthread -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp
QMAKE_CXXFLAGS += -pthread -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp -std=c++11
QMAKE_LIBDIR += $${QT_SYSROOT}/usr/lib

## Fixes for GCC 4.x to 5.x compatibilitiy
QMAKE_CFLAGS += -Wno-unused-parameter -Wno-sign-compare -ffreestanding
QMAKE_CXXFLAGS += -Wno-unused-parameter -Wno-sign-compare -ffreestanding

TEMPLATE = app
INSTALLS += target
target.path = /opt/camera

LIBS += -lm -lpthread -lrt -static-libstdc++

## Some other stuff to install.
datafiles.path = /opt/camera
datafiles.files = data/resolutions
datafiles.files += data/shuttingDown.data
datafiles.files += data/stylesheet.qss
datafiles.files += data/frame-preview.png
INSTALLS += datafiles

conffiles.path = /etc
conffiles.files = data/chronos-gui.conf
INSTALLS += conffiles

## Tweaks for Debian builds.
exists( $${QT_SYSROOT}/etc/debian_version ) {
    target.path = /usr/bin
    datafiles.path = /var/camera
    DEFINES += DEBIAN=1
}

SOURCES += main.cpp\
	mainwindow.cpp \
    camera.cpp \
    video.cpp \
    cammainwindow.cpp \
    myinputpanelcontext.cpp \
    keyboard.cpp \
    playbackwindow.cpp \
    cameraLowLevel.cpp \
    recsettingswindow.cpp \
    util.cpp \
    savesettingswindow.cpp \
    userInterface.cpp \
    siText.c \
    iosettingswindow.cpp \
    camLineEdit.cpp \
    camspinbox.cpp \
    camtextedit.cpp \
    camdoublespinbox.cpp \
    utilwindow.cpp \
    statuswindow.cpp \
    recmodewindow.cpp \
    triggerdelaywindow.cpp \
    triggerslider.cpp \
    playbackslider.cpp \
    keyboardbase.cpp \
    keyboardnumeric.cpp \
    whitebalancedialog.cpp \
    chronosControlInterface.cpp \
    chronosVideoInterface.cpp \
    colorwindow.cpp \
    colordoublespinbox.cpp \
    errorStrings.cpp \
    control.cpp \
    myinputpanel.cpp \
    eeprom.c \
    networkconfig.cpp

## Generate version.cpp on every build
versionTarget.target = version.cpp
versionTarget.depends = FORCE
versionTarget.commands = $${_PRO_FILE_PWD_}/version.sh > $$versionTarget.target
QMAKE_EXTRA_TARGETS += versionTarget
QMAKE_CLEAN += $$versionTarget.target
PRE_TARGETDEPS += $$versionTarget.target
GENERATED_SOURCES += $$versionTarget.target

HEADERS  += mainwindow.h \
    camera.h \
    defines.h \
    types.h \
    video.h \
    cammainwindow.h \
    myinputpanelcontext.h \
    keyboard.h \
    playbackwindow.h \
    recsettingswindow.h \
    util.h \
    font.h \
    savesettingswindow.h \
    userInterface.h \
    siText.h \
    iosettingswindow.h \
    camLineEdit.h \
    camspinbox.h \
    camtextedit.h \
    camdoublespinbox.h \
    errorCodes.h \
    utilwindow.h \
    statuswindow.h \
    i2c/i2c-dev.h \
    recmodewindow.h \
    triggerdelaywindow.h \
    triggerslider.h \
    playbackslider.h \
    keyboardbase.h \
    keyboardnumeric.h \
    whitebalancedialog.h \
    chronosControlInterface.h \
    chronosVideoInterface.h \
    colorwindow.h \
    colordoublespinbox.h \
    frameGeometry.h \
    myinputpanel.h \
    ui_myinputpanelform.h \
    control.h \
    eeprom.h \
    networkconfig.h

FORMS    += mainwindow.ui \
    cammainwindow.ui \
    keyboard.ui \
    playbackwindow.ui \
    recsettingswindow.ui \
    savesettingswindow.ui \
    ramwindow.ui \
    iosettingswindow.ui \
    utilwindow.ui \
    statuswindow.ui \
    recmodewindow.ui \
    triggerdelaywindow.ui \
    keyboardnumeric.ui \
    whitebalancedialog.ui \
    colorwindow.ui

RESOURCES += \
    Images.qrc

DISTFILES += \
    stylesheet.qss \
    data/newstylesheet.qss \
    data/lightstylesheet.qss \
    lightstylesheet.qss \
    darkstylesheet.qss \
    light3stylesheet.qss \
    light3stylesheet.qss
