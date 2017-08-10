#ifndef SAVESETTINGSWINDOW_H
#define SAVESETTINGSWINDOW_H

#include "camera.h"

#include <QWidget>
#include <QTimer>

namespace Ui {
class saveSettingsWindow;
}

class saveSettingsWindow : public QWidget
{
	Q_OBJECT
	
public:
	explicit saveSettingsWindow(QWidget *parent = 0, Camera * camInst = 0);
	~saveSettingsWindow();
	double bitsPerPixel;
	UInt32 framerate;
	char filename[1000];
	Camera * camera;
	
private slots:
	void on_cmdClose_clicked();

	void on_cmdUMount_clicked();

	void on_cmdRefresh_clicked();

	void on_spinBitrate_valueChanged(double arg1);

	void updateDrives();

	void on_spinFramerate_valueChanged(int arg1);

	void on_spinMaxBitrate_valueChanged(int arg1);

private:
	void refreshDriveList();

	Ui::saveSettingsWindow *ui;
	QTimer * timer;
	UInt32 driveCount;
};

#endif // SAVESETTINGSWINDOW_H
