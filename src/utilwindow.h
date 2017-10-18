#ifndef UTILWINDOW_H
#define UTILWINDOW_H

#include <QWidget>
#include "camera.h"

namespace Ui {
class UtilWindow;
}

class UtilWindow : public QWidget
{
	Q_OBJECT
	
public:
	explicit UtilWindow(QWidget *parent, Camera * cameraInst);
	~UtilWindow();
	
private slots:
	void on_cmdSWUpdate_clicked();
	void onUtilWindowTimer();

	void on_cmdSetClock_clicked();

	void on_cmdAdcOffset_clicked();

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

	void on_cmdEjectUSB_clicked();

	void on_chkAutoSave_stateChanged(int arg1);

    void on_chkAutoRecord_stateChanged(int arg1);

    void on_cmdDefaults_clicked();

    void on_cmdRestoreSettings_clicked();

    void on_cmdBackupSettings_clicked();

private:
	Ui::UtilWindow *ui;
	Camera * camera;
	QTimer * timer;
	bool settingClock;
};

#endif // UTILWINDOW_H
