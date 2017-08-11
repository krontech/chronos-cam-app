#-------------------------------------------------
#
# Project created by QtCreator 2013-04-28T14:58:59
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets multimedia

TARGET = camApp
CONFIG += qt console link_pkgconfig
target.path = /home/root/qt

QMAKE_CFLAGS += -Dxdc_target_types__=ti/targets/std.h -D__TMS470__ -DPlatform_dm814x -DG_THREADS_MANDATORY -DG_DISABLE_CAST_CHECKS -DG_DISABLE_ASSERT -pthread -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp
QMAKE_CXXFLAGS += -Dxdc_target_types__=ti/targets/std.h -D__TMS470__ -DPlatform_dm814x -DG_THREADS_MANDATORY -DG_DISABLE_CAST_CHECKS -DG_DISABLE_ASSERT -pthread -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp
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

## Linker glue for TI OMX libraries and board support.
INCLUDEPATH += ${EZSDK_PATH}/component-sources/xdctools_3_23_03_53/packages \
                ${EZSDK_PATH}/component-sources/omx_05_02_00_48/include

QMAKE_LIBDIR += ${EZSDK_PATH}/component-sources/omx_05_02_00_48/lib \
        ${EZSDK_PATH}/component-sources/osal_1_22_01_09/packages/linuxdist/build/lib \
        ${EZSDK_PATH}/component-sources/osal_1_22_01_09/packages/linuxdist/cstubs/lib \
        ${EZSDK_PATH}/component-sources/framework_components_3_22_01_07/packages/ti/sdo/rcm/lib/debug \
        ${EZSDK_PATH}/component-sources/framework_components_3_22_01_07/packages/ti/sdo/fc/memutils/lib/release \
        ${EZSDK_PATH}/component-sources/osal_1_22_01_09/packages/ti/sdo/xdcruntime/linux/lib/debug \
        ${EZSDK_PATH}/component-sources/framework_components_3_22_01_07/packages/ti/sdo/fc/global/lib/debug \
        ${EZSDK_PATH}/component-sources/syslink_2_20_02_20/packages/ti/syslink/lib \
        ${EZSDK_PATH}/component-sources/linuxutils_3_22_00_02/packages/ti/sdo/linuxutils/cmem/lib \
        ${EZSDK_PATH}/component-sources/uia_1_01_01_14/packages/ti/uia/linux/lib

LIBS += -Xlinker --start-group -l:memcfg.av5T -l:domx.av5T -l:domx_delegates_shmem.av5T -l:timmosal.av5T -l:omxcfg.av5T -l:osal.a -l:cstubs.a \
        -l:rcm_syslink.av5T -l:memutils.av5T -l:osal_linux_470.av5T -l:fcsettings.av5T -l:syslink.a_debug -l:cmem.a470MV -l:servicemgr.a -Xlinker --end-group

# Library notes:
# In OMX documentation, it says to include omxcore.av5T in the above list. However, this conflicts with
# OMX shared libraries used by gstreamer, solving this took THREE WEEKS of my life!
#
# Workaround is to include the following shared libaries that are included by libgstomx instead of omxcore.av5t
#
QMAKE_LIBDIR += $${QT_SYSROOT}/usr/lib $${QT_SYSROOT}/usr/lib/gstreamer-0.10
LIBS += -lOMX_Core  -ldl -lgmodule-2.0 -lgobject-2.0 -lgstbase-0.10 -lgstreamer-0.10 -lm -lpthread -l:libxml2.so.2 -l:libz.so.1 -lgthread-2.0 -lrt -lglib-2.0
LIBS += -static-libstdc++
#LIBS += -lgstreamer-0.10 -lgobject-2.0 -lgthread-2.0 -lgmodule-2.0 -lrt -lglib-2.0

SOURCES += main.cpp\
	mainwindow.cpp \
    camera.cpp \
    lupa1300.cpp \
    spi.cpp \
    gpmc.cpp \
 #   vpss/ilmain.c \
 #   vpss/ilclient.c \
    vpss/ilclient_utils.c \
 #   vpss/Fb_blending.c \
 #   vpss/es_parser.c \
    vpss/semp.c \
 #   vpss/msgq.c \
    vpss/dm814x/platform_utils.c \
    video.cpp \
    cammainwindow.cpp \
    myinputpanelcontext.cpp \
    keyboard.cpp \
    playbackwindow.cpp \
    cameraLowLevel.cpp \
    recsettingswindow.cpp \
    videoRecord.cpp \
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
    eeprom.c

HEADERS  += mainwindow.h \
    gpmc.h \
    gpmcRegs.h \
    camera.h \
    lupa1300.h \
    spi.h \
    defines.h \
    types.h \
    cameraRegisters.h \
#    vpss/ilmain.h \
#    vpss/ilclient.h \
    vpss/ilclient_utils.h \
#    vpss/app_cfg.h \
#    vpss/es_parser.h \
    vpss/semp.h \
#    vpss/msgq.h \
    vpss/dm814x/platform_utils.h \
    video.h \
    cammainwindow.h \
    myinputpanelcontext.h \
    keyboard.h \
    playbackwindow.h \
    recsettingswindow.h \
    videoRecord.h \
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
    eeprom.h





FORMS    += mainwindow.ui \
    cammainwindow.ui \
    keyboard.ui \
    playbackwindow.ui \
    recsettingswindow.ui \
    savesettingswindow.ui \
    ramwindow.ui \
    iosettingswindow.ui \
    utilwindow.ui \
    statuswindow.ui

RESOURCES += \
    Images.qrc
