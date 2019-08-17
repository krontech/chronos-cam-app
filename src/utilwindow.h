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
#ifndef UTILWINDOW_H
#define UTILWINDOW_H

#include <QWidget>
#include "camera.h"
#include <QDBusInterface>

namespace Ui {
class UtilWindow;
}

class UtilWindow : public QWidget
{
	Q_OBJECT
	
public:
	explicit UtilWindow(QWidget *parent, Camera * cameraInst);
	~UtilWindow();

signals:
	void moveCamMainWindow();
	
private slots:
	void on_cmdSWUpdate_clicked();
	void on_cmdNetUpdate_clicked();
	void onUtilWindowTimer();

	void on_cmdSetClock_clicked();

	void on_cmdColumnGain_clicked();

	void on_cmdBlackCalAll_clicked();

	void on_cmdCloseApp_clicked();

	void on_chkFPEnable_stateChanged(int arg1);

	void on_chkZebraEnable_stateChanged(int arg1);

	void on_comboFPColor_currentIndexChanged(int index);

	void on_radioFPSensLow_toggled(bool checked);

	void on_radioFPSensMed_toggled(bool checked);

	void on_radioFPSensHigh_toggled(bool checked);

	void on_cmdClose_clicked();

	void on_cmdAutoCal_clicked();

	void on_cmdWhiteRef_clicked();

	void on_cmdSetSN_clicked();

	void on_cmdSaveCal_clicked();

	void on_cmdRestoreCal_clicked();

	void on_linePassword_textEdited(const QString &arg1);

	void on_cmdEjectSD_clicked();
	void on_cmdFormatSD_clicked();

	void on_cmdEjectDisk_clicked();
	void on_cmdFormatDisk_clicked();

	void on_lineSshPassword_textEdited(const QString &password);
	void on_cmdSshApply_clicked();

	void on_chkAutoSave_stateChanged(int arg1);

	void on_chkAutoRecord_stateChanged(int arg1);

	void on_chkLiveRecord_stateChanged(int arg1);
	void on_liveRecordComboBox_currentIndexChanged(const QString &arg1);

	void on_chkDemoMode_stateChanged(int arg1);

	void on_autoPowerSetting_currentIndexChanged(int index);

	void on_chkShippingMode_clicked();

	void on_cmdDefaults_clicked();

	void on_cmdRestoreSettings_clicked();

	void on_cmdBackupSettings_clicked();

	void on_chkShowDebugControls_toggled(bool checked);

	void on_cmdRevertCalData_pressed();

	void on_comboDisableUnsavedWarning_currentIndexChanged(int index);

	void on_cmdClose_2_clicked();
	void on_cmdClose_3_clicked();
	void on_cmdClose_4_clicked();
	void on_cmdClose_5_clicked();

	void on_tabWidget_currentChanged(int index);
	
	int updateSoftware(char *updateLocation);
	void on_chkUiOnLeft_clicked();

private:
	Ui::UtilWindow *ui;
	Camera * camera;
	QTimer * timer;
	bool settingClock;
	bool okToSaveLocation = false;

	void formatStorageDevice(const char *blkdev);
	void statErrorMessage();
	void refreshDriveList();
	void saveFileDirectory();
};

#endif // UTILWINDOW_H
