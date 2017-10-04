#ifndef PLAYBACKWINDOW_H
#define PLAYBACKWINDOW_H

#include <QWidget>
#include <QDebug>

#include "statuswindow.h"
#include "util.h"
#include "camera.h"

namespace Ui {
class playbackWindow;
}

class playbackWindow : public QWidget
{
	Q_OBJECT
	
public:
	explicit playbackWindow(QWidget *parent = 0, Camera * cameraInst = NULL, bool autosave = false);
	~playbackWindow();

private slots:
	void on_verticalSlider_sliderMoved(int position);

	void on_verticalSlider_valueChanged(int value);

	void on_cmdPlayForward_pressed();

	void on_cmdPlayForward_released();

	void on_cmdPlayReverse_pressed();

	void on_cmdPlayReverse_released();

	void on_cmdSave_clicked();

	void on_cmdStopSave_clicked();

	void on_cmdSaveSettings_clicked();

	void on_cmdMarkIn_clicked();

	void on_cmdMarkOut_clicked();

	void updatePlayFrame();

	void checkForSaveDone();

	void on_cmdRateUp_clicked();

	void on_cmdRateDn_clicked();

private:
	Ui::playbackWindow *ui;
	Camera * camera;
	StatusWindow * sw;
	void updateStatusText();
	void updatePlayRateLabel(Int32 playbackRate);
	void setControlEnable(bool en);
	UInt32 markInFrame, markOutFrame;
	UInt32 lastPlayframe;
	QTimer * timer;
	QTimer * saveDoneTimer;
	Int32 playbackRate;
	bool autoSaveFlag;
};

#endif // PLAYBACKWINDOW_H
