# Prerequisites
The following packages were must be installed prior to building the camera
application.

```
    sudo apt-get install qtcreator gcc-arm-linux-gnueabi
```

# Building and Installing QT
The Chronos camera application is built using QT version 4.8, and must
be cross compiled for a Cortex-A8 target. The generic ARM linux targets
need to be modified, so extract the QT4.8 sources, and then create a
new linux-omap2-g++ target as follows

```bash
tar -xzf qt-everywhere-opensource-src-4.8.7.tar.gz
cd qt-everywhere-opensource-src-4.8.7/mkspecs/qws/
mkdir linux-omap2-g++
cp linux-arm-g++/qplatformdefs.h linux-omap2-g++/
cat > linux-omap2-g++/qmake.conf << EOF
#
# qmake configuration for building with arm-linux-gnueabi-g++
#

include(../../common/linux.conf)
include(../../common/gcc-base-unix.conf)
include(../../common/g++-unix.conf)
include(../../common/qws.conf)

# Compiler flags for OMAP2
QMAKE_CFLAGS_RELEASE =   -O3 -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp
QMAKE_CXXFLAGS_RELEASE = -O3 -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp

# modifications to g++.conf
QMAKE_CC                = arm-linux-gnueabi-gcc
QMAKE_CXX               = arm-linux-gnueabi-g++
QMAKE_LINK              = arm-linux-gnueabi-g++
QMAKE_LINK_SHLIB        = arm-linux-gnueabi-g++

# modifications to linux.conf
QMAKE_AR                = arm-linux-gnueabi-ar cqs
QMAKE_OBJCOPY           = arm-linux-gnueabi-objcopy
QMAKE_STRIP             = arm-linux-gnueabi-strip
QMAKE_LIBS		+= -lts

load(qt_config)
EOF
```


The following shell script
demonstrates the configuration provided to QT when used with the Chronos
SDK.

```bash
#!/bin/bash
QTVER=4.8.7
QTPATH=../qt-everywhere-opensource-src-${QTVER}/
SYSROOT=/home/osk/Work/chronos-sdk/targetfs

## All the configure arguments.
${QTPATH}configure -prefix $(pwd)/install -embedded arm \
        -sysroot $SYSROOT -xplatform qws/linux-omap2-g++ \
        -depths 16,24,32 -no-mmx -no-3dnow -no-sse -no-sse2 -no-glib -no-cups \
        -no-largefile -no-accessibility -no-openssl -no-gtkstyle \
        -qt-mouse-pc -qt-mouse-linuxtp -qt-mouse-linuxinput \
        -plugin-mouse-linuxtp -plugin-mouse-pc -qt-mouse-tslib \
        -qtlibinfix E -fast -D TSLIMBOUSEHANDLER_DEBUG
```

After configuring, make and install QT by running the commands

```
make
make install
```

# Setting up QT Creator
The first step to setting up QT creator to build for the Chronos, is to
add the ARM cross compiler to QT. Navigate to `Tools -> Options`, and select
`Build & Run` on the left column, then add a new compiler under the
`Compilers` tab. The path of your G++ and GCC compiler can be found from
runnin the command `which arm-linux-gnueabi-gcc` from a console.

![QT Creator compiler configuration](/doc/qtcreator_compilers.png)

The second step requires adding our cross-compiled build of QT 4.8 to
QT Creator. Select the `QT Versions` tab from the options, and add a new
QT version. You should only need to locate the `qmake` tool, which should
be in the `install/bin` directory in the prefix that was provided while
configuring QT.

![QT Creator QT version configuration](/doc/qtcreator_qtversion.png)

Finally, a kit must be selected for the Chronos camera, which uses the
compiler and QT version that we set up earlier. Select the `Kits` tab
to add a new kit for the camera. The following settings are required:
* Device type: Generic Linux Device
* Sysroot: Path to the `targetfs` directory in the Chronos SDK
* Compiler: The G++ cross compiler we set up earlier.
* Qt Version: The cross compiled QT 4.8.7 that we set up earlier.
* Environment: Click `Change` to add a variable `EZSDK_PATH` set to the
    path of the Chronos SDK.

![QT Creator QT kit](/doc/qtcreator_kits.png)

# Building the Camera Application
Open the Chronos camera application from by navigating to
`File -> Open File or Project` and selecting the QT creator project
file `src/camApp.pro`. If this is your first time opening the
project, you will need to set up the kits by deselecting the default
`Desktop` kit, and selecting the `Camera` kit that we set up earlier.

![QT Creator project configuration](/doc/qtcreator_project.png)

Click on the  `Configure Project` button to select the kits, and then
build the application by navigating to `Build -> Build Project "camApp"`
or clicking on the hammer icon in the bottom left corner. When complete
the application will be output as `build-camApp-Camera-Debug/camApp`
