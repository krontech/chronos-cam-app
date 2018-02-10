# Chronos Cam App
This repository contains the code for the built-in user interface of the Chronos Camera from [Kron Technologies](http://www.krontech.ca/). The following instructions detail how to build the Chronos Camera UI.



# Prerequisites
The recommended development environment for the Chronos camera application is
supported on Ubuntu 16.04 LTS. On a base Ubuntu installations, we will also need
to add the following packages:

```
    sudo apt install qtcreator gcc-arm-linux-gnueabi g++-arm-linux-gnueabi gdb-multiarch git
```

(Newer version of Ubuntu can also be used to build the camera application, but the instructions
may need to be modified to use GCC version 5 or older. This is usually done by substituting
`gcc-5` in place of `gcc`.)

You will also need a MicroSD card reader, to copy some files off the MicroSD card located in the bottom of the camera.

# Building and Installing QT
The Chronos camera application is built using QT version 4.8, and must
be cross compiled for a Cortex-A8 target. To do this, the generic ARM linux targets
need to be modified. First, grab the [QT4.8 source](https://download.qt.io/official_releases/qt/4.8/4.8.7/qt-everywhere-opensource-src-4.8.7.tar.gz) and extract it. I put the resulting folder in `~/Work/`, but you can put it anywhere you like. (Just remember to use your path when I reference my `~/Work` folder.) Next, in `~/Work/`, we'll create a
new linux-omap2-g++ target by running the following script:

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
You should now have a folder called `~/Work/qt-everywhere-opensource-src-4.8.7`, or whatever you called it.

Next, we need to copy in our `targetfs` folder. We'll take the MicroSD card from the bottom of our Chronos camera and copy everything on it, in ROOTFS, to `~/Work/chronos-sdk/targetfs/`. Some copy errors will pop up, but those files are not needed and you can safely ignore them. 🙂`targetfs` should now contain something that looks like a Linux root filesystem.

The following shell script demonstrates the configuration provided to QT
when used with the Chronos SDK. Assuming you put everything where I did. Copy the below content into a shell script, `~/Work/qt4-install/conf.sh`. (`qt4-install` will become our install directory. Again, you may use a different directory, but you'll have to update my paths.)

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

Run the shell script from `~/Work/qt4-install`. Select "open source" when prompted.

To solve a "tslib functionality test failed" error, you'll need to modify the include and library search paths by editing `QMAKE_INCDIR` and `QMAKE_LIBDIR` in `~/Work/qt-everywhere-opensource-src-4.8.7/mkspecs/qws/linux-omap2-g++`.

After configuration is complete, make and install QT by running `make && make install`. This will take 1 to 4ish hours, depending on how fast your computer is.

# ![qt-icon](/doc/qt_icon.png) Setting up QT Creator
To actually *build* the Chronos Cam App, we'll use QT Creator. (Which we installed at the beginning of this document.)

To set up QT creator, the first step is to
add the ARM cross compiler to QT. Run the program, and navigate to `Tools -> Options`, select
`Build & Run` on the left column, and add a new compiler under the
`Compilers` tab. The path of your G++ and GCC compiler can be found by
running the commands `which arm-linux-gnueabi-g++` and `which arm-linux-gnueabi-gcc` respectively. 

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

(If you want, you may plug your camera into your laptop with a Mini-USB cable now. This is not required, QT Creator will just let you know it can't connect if you don't.)

Click on `Next` and `Finish` to add the new debugging target to QT creator.

Finally, a kit must be selected for the Chronos camera, which uses the
compiler and QT version that we set up earlier. Still in the Options dialog, select the `Kits` tab
to add a new kit for the camera. The following settings are required:
* Device type: Generic Linux Device
* Sysroot: Path to the `targetfs` directory in the Chronos SDK
* Compiler: The G++ cross compiler we set up earlier.
* Qt Version: The cross compiled QT 4.8.7 that we set up earlier.

For debugging, you will also want to configure:
* Device: The Chronos Camera debugging target we set up earlier.
* Debugger: The Multiarch GDB debugger we set up earlier.

We can now close the Options dialog.



![QT Creator QT kit](/doc/qtcreator_kits.png)

# ![build-icon](/doc/build_icon.png) Building the Camera Application

Clone this repository (`git clone https://github.com/krontech/chronos-cam-app.git`) in `~/Work`.

Open the Chronos camera application from by navigating to
`File -> Open File or Project` and selecting the QT creator project
file `~/Work/chronos-cam-app/src/camApp.pro`. If this is your first time opening the
project, you will need to set up the kits by deselecting the default
`Desktop` kit, and selecting the `Camera` kit that we set up earlier.

![QT Creator project configuration](/doc/qtcreator_project.png)

Click on the  `Configure Project` button to select the kits. Click the "Run" toggle, in the "Camera" Build/Run box near the top. Delete the step 'check for available disk space', so only 'Upload files via SFTP' is left. Now, add a new step, 'run custom remote command'. Set it to `killall camApp; sleep 0.25 #Gracefully close the app, so we don't mess up the graphics driver state. Give it time to shut down.` Move it above 'Upload files via SFTP'.

Scrolling down a little, you'll find the Run box where you can specify 'Arguments' and 'Working directory'. Set 'Arguments' to `-qws`, and 'Working directory' to `/opt/camera`.

Finally, we build the application by navigating to `Build -> Build Project "camApp"`
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
