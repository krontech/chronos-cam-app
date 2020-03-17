/****************************************************************************
 *  Copyright (C) 2013-2017 Kron Technologies Inc <http://www.krontech.ca>. *
 *                                                                          *
 *  This program is free software: you can redistribute it and/or modify    *
 *  it under the terms of the GNU General Public License as published by    *
 *  the Free Software Foundation, either version 3 of the License, or       *
 *  (at your option) any later version.                                     *
 *                                                                          *
 *  This program is distributed in the hope that it will be useful,         *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU General Public License for more details.                            *
 *                                                                          *
 *  You should have received a copy of the GNU General Public License       *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/

/*
 *
 * Todo:
 * Change video save to 2nd video port - Cannot do this, need SDK update
 * Fix save playback address updating, GPIO callback delay could cause duplicated frames
 * save filename
 * advanced save settings
 * low resolution bug
 * Image sensor external exposure trigger
 *
 *
 *
 * Bugs:
 * When input panel (keyboard) comes up, it should select the contents of the control that was clicked, but this does not work until the second click.
 *
 * 6516 frames, mark in 4249, mark out at full (mark out not pressed), save continued indefinitely
 * 752x480 problems with lines on edges
 *
 *
 **/
extern "C" {
#include "siText.h"
}

#include "mainwindow.h"
#include "cammainwindow.h"
#include <QApplication>
#include <QWSServer>
#include "linux/ti81xxfb.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include "util.h"
#include <QDir>
#include <QFile>

#include "defines.h"

#include "myinputpanelcontext.h"

volatile sig_atomic_t done = 0;

void term(int signum)
{
	QSettings settings;
	settings.sync();
	qApp->exit();
}

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	
	QCoreApplication::setOrganizationName("KronTech");
	QCoreApplication::setOrganizationDomain("krontech.ca");
	QCoreApplication::setApplicationName("camApp");
	QSettings settings;

	/* Change into the data directory. */
#ifdef DEBIAN
	chdir("/var/camera");
#else
	chdir("/opt/camera");
#endif

	QString currentPath(QDir::current().canonicalPath());
	QDir::addSearchPath("camApp", currentPath);
	QDir::addSearchPath("cal", currentPath + "/cal");
	QDir::addSearchPath("fpn", currentPath + "/userFPN");
	QDir::addSearchPath("fpn", currentPath + "/cal/factoryFPN");
	QDir::addSearchPath("fpga", currentPath);

	//Check for and create required directories
	checkAndCreateDir("cal");
	checkAndCreateDir("cal/factoryFPN");
	checkAndCreateDir("userFPN");

	//Set up SIGTERM handler to cleanly exit the application
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = term;
	sigaction(SIGTERM, &action, NULL);

	//Set frame buffer blending
	int fd = open ("/dev/fb0", O_RDWR);
	
	if (fd <= 0)
	{
		perror ("Could not open device\n");
		exit(1);
	}
	
	struct ti81xxfb_region_params  regp;
	if (ioctl(fd, TIFB_GET_PARAMS, &regp) < 0) {
		perror("TIFB_GET_PARAMS\n");
		close(fd);
		exit(1);
	}
	/*Set Pixel Alpha Blending*/
	regp.blendtype = TI81XXFB_BLENDING_PIXEL;
	if (ioctl(fd, TIFB_SET_PARAMS, &regp) < 0) {
		perror ("TIFB_SET_PARAMS.\n");
		close(fd);
		exit(1);
	}
	
	close(fd);
	
#ifdef Q_WS_QWS
//	app.setOverrideCursor( QCursor( Qt::BlankCursor ) );
	QWSServer::setCursorVisible( false );
	//QWSServer::setBackground(QBrush(Qt::black));
	QWSServer::setBackground(QBrush(Qt::transparent));  // have not tested
#endif
	
	// Load stylesheet from file, if one exists.
    //QFile fStyle("stylesheet.qss");
    //if (fStyle.open(QFile::ReadOnly)) {
        //QString sheet = QLatin1String(fStyle.readAll());
        //a.setStyleSheet(sheet);
        //fStyle.close();
    //}
    QFile styleFile(":/qss/lightstylesheet.qss"); //":/qss/lightstylesheet.qss"
    styleFile.open(QFile::ReadOnly);

    QString style(styleFile.readAll());
    a.setStyleSheet(style);

	//Disable stdout buffering so prints work rather than just filling the buffer.
//	setbuf(stdout, NULL);
	
	//Set the minimum size of buttons and other UI interaction elements
	QApplication::setGlobalStrut(QSize(40, 40));
	
	MyInputPanelContext *ic = new MyInputPanelContext;
	a.setInputContext(ic);
	
	printf("testing print 2\n");
	qDebug("Testing QDebug");
//	fflush(stdout);
	CamMainWindow w;
	w.setWindowFlags(Qt::FramelessWindowHint);

	QSettings appSettings;
	int displayPosition = (appSettings.value("camera/ButtonsOnLeft", false)).toBool() ? 0 : 600;
	w.move(displayPosition,0);

//	MainWindow w;
	w.show();
	
	return a.exec();
}
