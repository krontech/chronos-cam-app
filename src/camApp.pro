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
target.path = /opt/camera

QMAKE_CFLAGS += -Dxdc_target_types__=ti/targets/std.h -D__TMS470__ -DPlatform_dm814x -DG_THREADS_MANDATORY -DG_DISABLE_CAST_CHECKS -DG_DISABLE_ASSERT -pthread -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp
QMAKE_CXXFLAGS += -Dxdc_target_types__=ti/targets/std.h -D__TMS470__ -DPlatform_dm814x -DG_THREADS_MANDATORY -DG_DISABLE_CAST_CHECKS -DG_DISABLE_ASSERT -pthread -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp -std=c++11
#QMAKE_CFLAGS += -DPlatform_dm814x -DG_THREADS_MANDATORY -DG_DISABLE_CAST_CHECKS -DG_DISABLE_ASSERT -pthread -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp
#QMAKE_CXXFLAGS += -Dxdc_target_types__=ti/targets/std.h -D__TMS470__ -DPlatform_dm814x -DG_THREADS_MANDATORY -DG_DISABLE_CAST_CHECKS -DG_DISABLE_ASSERT

## Fixes for GCC 4.x to 5.x compatibilitiy
QMAKE_CFLAGS += -Wno-unused-parameter -Wno-sign-compare -ffreestanding
QMAKE_CXXFLAGS += -Wno-unused-parameter -Wno-sign-compare -ffreestanding

TEMPLATE = app
INSTALLS += target
INCLUDEPATH += $${QT_SYSROOT}/usr/include \
               $${QT_SYSROOT}/usr/include/gstreamer-0.10 \
               $${QT_SYSROOT}/usr/include/glib-2.0 \
	       $${QT_SYSROOT}/usr/lib/glib-2.0/include

# Library notes:
# In OMX documentation, it says to include omxcore.av5T in the above list. However, this conflicts with
# OMX shared libraries used by gstreamer, solving this took THREE WEEKS of my life!
#
# Workaround is to include the following shared libaries that are included by libgstomx instead of omxcore.av5t
#
QMAKE_LIBDIR += $${QT_SYSROOT}/usr/lib $${QT_SYSROOT}/usr/lib/gstreamer-0.10
LIBS += -ldl -lgmodule-2.0 -lgobject-2.0 -lgstbase-0.10 -lgstreamer-0.10 -lm -lpthread -l:libxml2.so.2 -l:libz.so.1 -lgthread-2.0 -lrt -lglib-2.0
LIBS += -lgstapp-0.10
LIBS += -static-libstdc++
#LIBS += -lgstreamer-0.10 -lgobject-2.0 -lgthread-2.0 -lgmodule-2.0 -lrt -lglib-2.0

SOURCES += main.cpp\
	mainwindow.cpp \
    camera.cpp \
    lupa1300.cpp \
    spi.cpp \
    gpmc.cpp \
    video.cpp \
    cammainwindow.cpp \
    myinputpanelcontext.cpp \
    keyboard.cpp \
    playbackwindow.cpp \
    cameraLowLevel.cpp \
    recsettingswindow.cpp \
    gstLaunch.c \
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
    chronosVideoInterface.cpp

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
    lupa1300.h \
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
    gstLaunch.h \
    tools.h \
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
    chronosVideoInterface.h

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
    whitebalancedialog.ui

RESOURCES += \
    Images.qrc
