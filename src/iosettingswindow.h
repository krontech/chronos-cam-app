#ifndef IOSETTINGSWINDOW_H
#define IOSETTINGSWINDOW_H

#include <QWidget>
#include <QDebug>
#include <QTimer>
#include <util.h>

#include "camera.h"

namespace Ui {
class IOSettingsWindow;
}

class IOSettingsWindow : public QWidget
{
	Q_OBJECT
	
public:
	explicit IOSettingsWindow(QWidget *parent = 0, Camera *cameraInst = NULL);
	~IOSettingsWindow();

private slots:
	void on_cmdOK_clicked();

	void on_cmdApply_clicked();

	void on_updateTimer();

	void updateIO();

	void on_radioIO1TriggeredShutter_toggled(bool checked);

	void on_radioIO1ShutterGating_toggled(bool checked);

	void on_radioIO2TriggeredShutter_toggled(bool checked);

	void on_radioIO2ShutterGating_toggled(bool checked);

private:
	Ui::IOSettingsWindow *ui;
	Camera * camera;
	UInt32 lastIn;
	QTimer * timer;
};

#endif // IOSETTINGSWINDOW_H
