# Chronos Cam App
This repository contains the code for the built-in user interface of the Chronos Camera
from [Kron Technologies](http://www.krontech.ca/). The following instructions detail how
to build the Chronos Camera UI.

# Prerequisites
The recommended development environment for the Chronos camera application is
supported on Ubuntu 16.04 LTS. On a base Ubuntu installations, we will also need
to add the following packages:

```
    sudo apt install qtcreator gcc-arm-linux-gnueabi g++-arm-linux-gnueabi gdb-multiarch \
        libdbus-1-dev git
```

(Newer version of Ubuntu can also be used to build the camera application, but the instructions
may need to be modified to use GCC version 5 or older. This is usually done by substituting
`gcc-5` in place of `gcc`.)

You will also need a MicroSD card reader, to copy some files off the MicroSD card located in
the bottom of the camera. Or by creating a new root filesystem using the Debian debootstrap
tool (instructions TBD).

# Building and Installing QT
The Chronos camera application is built using QT version 4.8, and must be cross compiled for
a Cortex-A8 target. To do this, the generic ARM linux targets need to be modified. First, grab
the [Qt 4.8.7 sources](https://download.qt.io/archive/qt/4.8/4.8.7/qt-everywhere-opensource-src-4.8.7.tar.gz)
and extract them. We put the resulting folder in `~/Work/chronos-sdk`, but you can put them anywhere
you like. (Just remember to use your path when I reference my `~/Work/chronos-sdk` folder.) Next,
in `~/Work/chronos-sdk/`, we'll create a new linux-omap2-g++ target by running the following script:

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

QMAKE_LIBS              += -lts
DEFINES                 += QT_KEYPAD_NAVIGATION

load(qt_config)
EOF
```

Next, we need to create a copy of the root filesystem for the Chronos Camera. We'll
take the MicroSD card from the bottom of the Chronos camera and copy everything from
the ROOTFS partition and save it to to `~/Work/chronos-sdk/targetfs/`. You may
encounter some copy errors for files that are owned by the root user, but those files
are not needed and you can safely ignore them. The `targetfs` directory should now
contain something that looks like a Linux root filesystem.

The following shell script demonstrates the configuration provided to QT when used with
the Chronos SDK. Copy the content below into a shell script that will be run from the
QT build directory, such as `~/Work/chronos-sdk/qt4-build/qtconfing.sh`. Note that Qt
must be built in a directory that is different from the source directory (this is sometimes
referred to an out-of-tree build), and must be installed into a directory that is also
different from the build directory.

```bash
#!/bin/bash
QTVER=4.8.7
QTPATH=~/Work/chronos-sdk/qt-everywhere-opensource-src-${QTVER}/
QTINSTALL=~/Work/chronos-sdk/qt4-install
SYSROOT=~/Work/chronos-sdk/targetfs

## Specialization for Debian targets
if [ -e ${SYSROOT}/etc/debian_version ]; then
        QTDEBOPTS="-no-rpath -system-zlib -system-libtiff -system-libpng -system-libjpeg"
fi

## All the configure arguments.
eval ${QTPATH}configure -opensource -fast \
        -hostprefix ${QTINSTALL} ${QTDEBOPTS} \
        -embedded arm -qtlibinfix E -prefix /usr \
        -libdir /usr/lib/qt4 \
        -plugindir /usr/lib/qt4/plugins \
        -importdir /usr/lib/qt4/imports \
        -translationdir /usr/share/qt4/translations \
        -sysroot ${SYSROOT} -xplatform qws/linux-omap2-g++ \
        -nomake demos -nomake examples \
        -no-largefile -no-accessibility -no-openssl \
        -no-gtkstyle -no-glib -no-cups -no-multimedia \
        -no-webkit -no-javascript-jit \
        -plugin-mouse-linuxtp -plugin-mouse-pc -plugin-mouse-tslib \
        -qt-gfx-linuxfb -plugin-gfx-transformed \
        -dbus -ldbus-1 -lpthread \
        -I${SYSROOT}/usr/include/dbus-1.0 \
        -I${SYSROOT}/usr/lib/dbus-1.0/include \
        -I${SYSROOT}/usr/lib/arm-linux-gnueabi/dbus-1.0/include
```

Run the shell script from `~/Work/chronos-sdk/qt4-build` to configure Qt.
When configuration is complete, make and install QT by running `make && make install`.
This will take 1 to 4ish hours, depending on how fast your computer is.

# ![qt-icon](/doc/images/qt_icon.png) Setting up QT Creator
To actually *build* the Chronos Cam App, we'll use QT Creator.

To set up QT creator, the first step is to add the ARM cross compiler to
QT. Run the program, and navigate to `Tools -> Options`, select `Build & Run`
on the left column, and add a new compiler under the `Compilers` tab. The
path of your G++ and GCC compiler can be found by running the commands 
`which arm-linux-gnueabi-g++` and `which arm-linux-gnueabi-gcc` respectively. 

![QT Creator compiler configuration](/doc/images/qtcreator_compilers.png)

The second step requires adding our cross-compiled build of QT 4.8 to
QT Creator. Select the `QT Versions` tab from the options, and add a new
QT version. You should only need to locate the `qmake` tool, which should
be in the `bin` directory of the hostprefix that was provided while
configuring QT.

![QT Creator QT version configuration](/doc/images/qtcreator_qtversion.png)

This step is optional, but to add debugging support, select the `Debuggers`
tab and add the `gdb-multiarch` cross debugger to QT creator. You can find
the path of the debugger by running the command `which gdb-multiarch` from
a console.

![QT Creator debugger configuration](/doc/images/qtcreator_debuggers.png)

To add the Chronos camera as a debugging target, select `Devices` on the
left column and click the `Add` button to add a new target. Select the
`Generic Linux Device` and click on `Start Wizard`. You will be prompted
for the connection details, which should be set as follows:
 * Name to identify this configuration: Chronos Camera
 * Host name or IP address: 192.168.12.1
 * Username to log into the device: root
 * Authentication type: Password
 * User's Password: Your Awesome Super Secret Root Password On the Camera

![Chronos SSH login and kill](/doc/images/qtcreator_device_wizard.png)

(If you want, you may plug your camera into your laptop with a Mini-USB cable
now. This is not required, QT Creator will just let you know it can't connect
if you don't.)

Click on `Next` and `Finish` to add the new debugging target to QT creator.

Finally, a kit must be selected for the Chronos camera, which uses the
compiler and QT version that we set up earlier. Still in the `Options` dialog,
select `Build & Run` from the left column, and click on the `Kits` tab to add
a new kit for the camera. The following settings are required:
* Device type: Generic Linux Device
* Sysroot: Path to the `targetfs` directory in the Chronos SDK
* Compiler: The G++ cross compiler we set up earlier.
* Qt Version: The cross compiled QT 4.8.7 that we set up earlier.

For debugging, you will also want to configure:
* Device: The Chronos Camera debugging target we set up earlier.
* Debugger: The Multiarch GDB debugger we set up earlier.

We can now close the Options dialog.

![QT Creator QT kit](/doc/images/qtcreator_kits.png)

# ![build-icon](/doc/images/build_icon.png) Building the Camera Application

Clone this repository (`git clone https://github.com/krontech/chronos-cam-app.git`) in `~/Work`.

Open the Chronos camera application from by navigating to
`File -> Open File or Project` and selecting the QT creator project
file `~/Work/chronos-cam-app/src/camApp.pro`. If this is your first
time opening the project, you will need to set up the kits by deselecting
the default `Desktop` kit, and selecting the `Camera` kit that we set up
earlier. Click on the  `Configure Project` button to select the kits.

![QT Creator project configuration](/doc/images/qtcreator_project.png)

Next, we will need to configure how the camera application is deployed
and executed on the camera. Start by selecting the `Projects` button from
the left column of QT Creator, and clicking on the `Run` toggle button.

In the Deployment Steps, use the `Add Deploy Step` dropdown menu to add a
new `Run custom remote command` step, set the command line of our new step
to `killall camApp; sleep 1`, and sort it head of the `Upload files via SFTP`
step. Then, depending on your target operating system, you will need to make
the following adjustmnets to your Run configuration:

| Change            | Arago                            | Debian                           |
|-------------------|----------------------------------|----------------------------------|
| Arguments         | `-qws -display transformed:rot0` | `-qws -display transformed:rot0` |
| Working Directory | `/opt/camera`                    | `/var/camera`                    |
| Deploy Steps      | Remove `Check for Available Disk Space` | No Changes                |
| Environment `QWS_MOUSE_PROTO` | Unset                | `Tslib:/dev/input/event0`        |

![QT Creator Run Settings](/doc/images/qtcreator_run_settings.png)

Finally, we build the application by navigating to `Build -> Build Project "camApp"`
or clicking on the hammer icon in the bottom left corner. When complete
the application will be output as `build-camApp-Camera-Debug/camApp`

# ![debug-icon](/doc/images/debug_icon.png) Debugging the Camera Application
Debugging the camera application is done using a remote GDB connection to
the Linux operating system running on the Chronos camera. The easiest way
to establish this connection is to connect the Mini-USB on the Chronos to
your Ubuntu machine and powering on the camera. When the camera has finished
booting, it will appear as a USB network adapter with an IP address of
192.168.12.1.

There is no default password set on the camera, and until one has been set,
the SSH login will accept any password that you type in. You can change
the password by running the `passwd` command<sup>1</sup>.

Before we can launch the Camera application through QT creator, we must first
log into the Camera via SSH<sup>2</sup> and terminate the camApp process that started
at boot. To do this, log into the camera from a console by running the
command `ssh -oKexAlgorithms=+diffie-hellman-group1-sha1 root@192.168.12.1`.
Once you are logged into the camera, run the command `killall camApp` to
terminate the `camApp` process.

![Chronos SSH login and kill](/doc/images/chronos_ssh_login.png)

As a word of caution, the OMX video coprocessor on the DM8148 SoC is quite
fragile and can become deadlocked if the camera application is not shutdown
gracefully. This may occur if your camera application crashes during debug,
or if you neglect to terminate the running `camApp` process before starting
debug. When this occurs, your only recourse is to reboot the camera.

To start the camera application with interactive debugging, navigate to the
`Debug -> Start Debugging -> Start Debugging` menu, or click on the green
Play button with a bug icon in the bottom left hand corner. This will upload
the compiled `camApp` to the camera, and start it under a remote GDB server.
When the debugger is running, the play button will be replaced by a blue
Pause button with a bug icon.

![Chronos SSH login and kill](/doc/images/qtcreator_live_debug.png)

[1]: In a future update of the camera, SSH access will be disabled until a
root password is configured via the camera application. This is intended
to prevent an attacker from logging into an unconfigured camera via the
Ethernet port.

[2]: The SSH daemon on the camera does not support hash functions newer than
SHA-1, which has been deprecated as of Ubuntu 16.04 LTS. Thus the SSH
KexAlgorithms must be explicitly extended to successfully log in. This will
be addressed in a future update before enabling Ethernet access.

