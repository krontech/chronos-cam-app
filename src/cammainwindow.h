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
#ifndef CAMMAINWINDOW_H
#define CAMMAINWINDOW_H

#include <QDialog>
#include <QObject>
#include <QDebug>
#include <QTimer>

#include "statuswindow.h"
#include "util.h"
#include "camera.h"

namespace Ui {
class CamMainWindow;
}

class CamMainWindow : public QDialog
{
	Q_OBJECT

public:
	explicit CamMainWindow(QWidget *parent = 0);
	short getWindowsAlwaysOpen();
	~CamMainWindow();

public slots:
	void updateCamMainWindowPosition();
private slots:
	void on_cmdClose_clicked();

	void on_cmdDebugWnd_clicked();

	void on_cmdRec_clicked();

	void on_cmdPlay_clicked();

	void playFinishedSaving();

	void on_cmdRecSettings_clicked();

	void on_cmdFPNCal_clicked();

	void on_cmdWB_clicked();

	void on_cmdIOSettings_clicked();

	void on_MainWindowTimer();
	void on_newVideoSegment(VideoStatus *st);

	void on_chkFocusAid_clicked(bool focusAidEnabled);

	void UtilWindow_closed();

	void on_expSlider_valueChanged(int shutterAngle);

	void recSettingsClosed();

	void on_cmdUtil_clicked();

	void on_cmdBkGndButton_clicked();

	void on_cmdDPCButton_clicked();

	void createNewPlaybackWindow();

	void keyPressEvent(QKeyEvent *ev);

private:
	void updateRecordingState(bool recording);
	void updateCurrentSettingsLabel(void);
	void updateExpSliderLimits(void);
	Ui::CamMainWindow *ui;
	StatusWindow * sw;
	QTimer *timer;
	bool lastShutterButton;
	bool lastRecording;
	int bmsFifoFD;
	int windowsAlwaysOpen;

	double battCapacityPercent;  //Battery data from ENEL4A.c
	UInt8 battSOHPercent;
	UInt32 battVoltage;
	UInt32 battCurrent;
	UInt32 battHiResCap;
	UInt32 battHiResSOC;
	double battVoltageCam;
	double battCurrentCam;
	Int32 mbTemperature;
	UInt8 flags;
	UInt8 fanPWM;
	bool battPresent;

	bool autoSaveActive;
};

#endif // CAMMAINWINDOW_H
