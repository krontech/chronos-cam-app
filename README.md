# Prerequisites
The recommended development environment for the Chronos camera application is
supported on Ubuntu 16.04 LTS. On a base Ubuntu installations, we will also need
to add the following packages:

```
    sudo apt-get install qtcreator gcc-arm-linux-gnueabi g++-arm-linux-gnueabi gdb-multiarch
```

Newer version of Ubuntu can also be used to build the camera application, but the instructions
may need to be modified to use GCC version 5 or older. This is usually done by substituting
`gcc-5` in place of `gcc`

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

The following shell script demonstrates the configuration provided to QT
when used with the Chronos SDK. This assumes you have extracted the QT 
Everywhere package to ~/Work/, and have placed your targetfs in
~/Work/chronos-sdk/targetfs
Put the below content into a shell script, for example conf.sh.
Place this file in the QT install directory, for example ~/Work/qt4-install/

```bash
#!/bin/bash
QTVER=4.8.7
QTPATH=~/Work/qt-everywhere-opensource-src-${QTVER}/
SYSROOT=~/Work/chronos-sdk/targetfs

## All the configure arguments.
${QTPATH}configure -prefix $(pwd)/install -embedded arm \
        -sysroot ${SYSROOT} -xplatform qws/linux-omap2-g++ \
        -depths 16,24,32 -no-mmx -no-3dnow -no-sse -no-sse2 -no-glib -no-cups \
        -no-largefile -no-accessibility -no-openssl -no-gtkstyle \
        -qt-mouse-pc -qt-mouse-linuxtp -qt-mouse-linuxinput \
        -plugin-mouse-linuxtp -plugin-mouse-pc -qt-mouse-tslib \
        -qtlibinfix E -fast
```

Run the shell script ./conf.sh, from the QT install directory. After configuration is complete, make and install QT by running the commands

```
make
make install
```

# ![qt-icon](/doc/qt_icon.png) Setting up QT Creator
The first step to setting up QT creator to build for the Chronos, is to
add the ARM cross compiler to QT. Navigate to `Tools -> Options`, and select
`Build & Run` on the left column, then add a new compiler under the
`Compilers` tab. The path of your G++ and GCC compiler can be found from
running the command `which arm-linux-gnueabi-gcc` from a console.

![QT Creator compiler configuration](/doc/qtcreator_compilers.png)

The second step requires adding our cross-compiled build of QT 4.8 to
QT Creator. Select the `QT Versions` tab from the options, and add a new
QT version. You should only need to locate the `qmake` tool, which should
be in the `install/bin` directory in the prefix that was provided while
configuring QT.

![QT Creator QT version configuration](/doc/qtcreator_qtversion.png)

This step is optional, but to add debugging support, select the `Debuggers`
tab and add the `gdb-multiarch` cross debugger to QT creator. You can find
the path of the debugger by running the command `which gdb-multiarch` from
a console.

![QT Creator debugger configuration](/doc/qtcreator_debuggers.png)

To add the Chronos camera as a debugging target, select `Devices` on the
left column and click the `Add` button to add a new target. Select the
`Generic Linux Device` and click on `Start Wizard`. You will be prompted
for the connection details, which should be set as follows:
 * Name to identify this configuration: Chronos Camera
 * Host name or IP address: 192.168.12.1
 * Username to log into the device: root
 * Authentication type: Password
 * User's Password: Your Awesome Super Secret Root Password On the Camera

![Chronos SSH login and kill](/doc/qtcreator_device_wizard.png)

Click on `Next` and `Finish` to add the new debugging target to QT creator.

Finally, a kit must be selected for the Chronos camera, which uses the
compiler and QT version that we set up earlier. Select the `Kits` tab
to add a new kit for the camera. The following settings are required:
* Device type: Generic Linux Device
* Sysroot: Path to the `targetfs` directory in the Chronos SDK
* Compiler: The G++ cross compiler we set up earlier.
* Qt Version: The cross compiled QT 4.8.7 that we set up earlier.

For debugging, you will also want to configure:
* Device: The Chronos Camera debugging target we set up earlier.
* Debugger: The Multiarch GDB debugger we set up earlier.

![QT Creator QT kit](/doc/qtcreator_kits.png)

# ![build-icon](/doc/build_icon.png) Building the Camera Application
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

# ![debug-icon](/doc/debug_icon.png) Debugging the Camera Application
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

![Chronos SSH login and kill](/doc/chronos_ssh_login.png)

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

![Chronos SSH login and kill](/doc/qtcreator_live_debug.png)

[1]: In a future update of the camera, SSH access will be disabled until a
root password is configured via the camera application. This is intended
to prevent an attacker from logging into an unconfigured camera via the
Ethernet port.

[2]: The SSH daemon on the camera does not support hash functions newer than
SHA-1, which has been deprecated as of Ubuntu 16.04 LTS. Thus the SSH
KexAlgorithms must be explicitly extended to successfully log in. This will
be addressed in a future update before enabling Ethernet access.
