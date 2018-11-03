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
datafiles.files = $$files(data/*)
INSTALLS += datafiles

## Tweaks for Debian builds.
exists( $${QT_SYSROOT}/etc/debian_version ) {
    target.path = /usr/bin
    datafiles.path = /var/camera
    DEFINES += DEBIAN=1

    DEBPACKAGE = chronos-gui
    DEBFULLNAME = $$system(git config user.name)
    DEBEMAIL = $$system(git config user.email)
    DEBFILES = $$files(debian/*, true)
    DEBCONFIG = $$find(DEBFILES, "\\.in$")
    DEBFILES -= $$DEBCONFIG
    QMAKE_SUBSTITUTES += $$DEBCONFIG

    system($$QMAKE_MKDIR -p $${OUT_PWD}/debian $${OUT_PWD}/debian/source)
    system($$QMAKE_COPY $$DEBFILES $${OUT_PWD}/debian)
    system($${_PRO_FILE_PWD_}/changelog.sh > $${OUT_PWD}/debian/changelog)
}

SOURCES += main.cpp\
	mainwindow.cpp \
    camera.cpp \
    spi.cpp \
    gpmc.cpp \
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
    ramwindow.cpp \
    siText.c \
    dm8148PWM.cpp \
    io.cpp \
    iosettingswindow.cpp \
    camLineEdit.cpp \
    camspinbox.cpp \
    camtextedit.cpp \
    camdoublespinbox.cpp \
    lux1310.cpp \
    ecp5Config.cpp \
    utilwindow.cpp \
    statuswindow.cpp \
    eeprom.c \
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
    colordoublespinbox.cpp

## Generate version.cpp on every build
versionTarget.target = version.cpp
versionTarget.depends = FORCE
versionTarget.commands = $${_PRO_FILE_PWD_}/version.sh > $$versionTarget.target
QMAKE_EXTRA_TARGETS += versionTarget
QMAKE_CLEAN += $$versionTarget.target
PRE_TARGETDEPS += $$versionTarget.target
GENERATED_SOURCES += $$versionTarget.target

HEADERS  += mainwindow.h \
    gpmc.h \
    gpmcRegs.h \
    camera.h \
    spi.h \
    defines.h \
    types.h \
    cameraRegisters.h \
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
    ramwindow.h \
    siText.h \
    dm8148PWM.h \
    io.h \
    iosettingswindow.h \
    camLineEdit.h \
    camspinbox.h \
    camtextedit.h \
    camdoublespinbox.h \
    errorCodes.h \
    lux1310.h \
    ecp5Config.h \
    utilwindow.h \
    statuswindow.h \
    i2c/i2c-dev.h \
    eeprom.h \
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
    colordoublespinbox.h

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
    stylesheet.qss
