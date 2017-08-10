#ifndef CAMMAINWINDOW_H
#define CAMMAINWINDOW_H

#include <QDialog>
#include <QObject>
#include <QDebug>
#include <QTimer>

#include "statuswindow.h"
#include "util.h"
#include "camera.h"
#include "keyboard/widgetKeyBoard.h"

namespace Ui {
class CamMainWindow;
}

class CamMainWindow : public QDialog
{
	Q_OBJECT
	
public:
	explicit CamMainWindow(QWidget *parent = 0);
	~CamMainWindow();
	
private slots:
	void on_cmdClose_clicked();

	void on_cmdDebugWnd_clicked();

	void on_cmdRec_clicked();

	void on_cmdPlay_clicked();

	void on_cmdRecSettings_clicked();

	void on_cmdFPNCal_clicked();

	void on_cmdWB_clicked();

	void on_cmdIOSettings_clicked();

	void on_MainWindowTimer();

	void on_MainWindowTimeoutTimer();

	void on_cmdFocusAid_clicked();

	void on_expSlider_sliderMoved(int position);

	void recSettingsClosed();

	void on_cmdUtil_clicked();

	void on_cmdBkGndButton_clicked();

private:
	void updateRecordingState(bool recording);
	void updateCurrentSettingsLabel(void);
	Ui::CamMainWindow *ui;
	StatusWindow * sw;
	widgetKeyBoard  *myKeyboard;
	QTimer * timer;
	bool lastShutterButton;
	bool lastRecording;
	int bmsFifoFD;

	UInt8 battCapacityPercent;  //Battery data from ENEL4A.c
	UInt8 battSOHPercent;
	UInt32 battVoltage;
	UInt32 battCurrent;
	UInt32 battHiResCap;
	UInt32 battHiResSOC;
	UInt32 battVoltageCam;
	Int32 battCurrentCam;
	Int32 mbTemperature;
	UInt8 flags;
	UInt8 fanPWM;
};

#endif // CAMMAINWINDOW_H
