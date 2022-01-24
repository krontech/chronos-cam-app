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
#include <QMessageBox>
#include <QDebug>
#include <QTimer>

#include "statuswindow.h"
#include "util.h"
#include "camera.h"
#include "video.h"

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
    void saveNextSegment(VideoState state);

	void on_chkFocusAid_clicked(bool focusAidEnabled);

	void UtilWindow_closed();

	void on_expSlider_valueChanged(int shutterAngle);

	void recSettingsClosed();

	void on_cmdUtil_clicked();

	void on_cmdBkGndButton_clicked();

	void on_cmdDPCButton_clicked();

	void createNewPlaybackWindow();

	void keyPressEvent(QKeyEvent *ev);

	void on_state_valueChanged(const QVariant &value);
	void on_videoState_valueChanged(const QVariant &value);
	void on_exposurePeriod_valueChanged(const QVariant &value);
	void on_exposureMax_valueChanged(const QVariant &value);
	void on_exposureMin_valueChanged(const QVariant &value);
	void on_focusPeakingLevel_valueChanged(const QVariant &value);
	void on_wbTemperature_valueChanged(const QVariant &value);
    void on_rsResolution_valueChanged(const QVariant &value);

	void buttonsEnabled(bool en);
    void checkForCalibration(void);
    void checkForNfsStorage(void);
    void checkForSmbStorage(void);
    void checkForWebMount(void);

    void runTimer();
    void stopTimer();

private:
	void updateCurrentSettingsLabel(void);
	void updateExpSliderLimits(void);
	void updateBatteryData();
	bool okToStopLive();
	QMessageBox::StandardButton question(const QString &title, const QString &text, QMessageBox::StandardButtons = QMessageBox::Yes|QMessageBox::No);

	QMessageBox *prompt;
	Ui::CamMainWindow *ui;
	UserInterface * interface;
	StatusWindow * sw;
	QTimer *timer;
    QTimer *mountTimer;
	bool recording;
	bool lastShutterButton;
	int windowsAlwaysOpen;

	double batteryPercent;
	double batteryVoltage;
	bool batteryPresent;
	bool externalPower;

	bool autoSaveActive;

	bool apiUpdate = false;
	UInt32 expSliderFramePeriod;

	ImagerSettings_t is;

    CaKrontechChronosControlInterface iface;

    QVariantList nextSegments = {};
};

#endif // CAMMAINWINDOW_H
