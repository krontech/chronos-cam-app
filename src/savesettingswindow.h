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

public slots:
	void setControlEnable(bool en);
private slots:
	void on_cmdClose_clicked();

	void on_cmdUMount_clicked();

	void on_cmdRefresh_clicked();

	void on_spinBitrate_valueChanged(double arg1);

	void updateDrives();

	void on_spinFramerate_valueChanged(int arg1);

	void on_spinMaxBitrate_valueChanged(int arg1);

    void on_comboSaveFormat_currentIndexChanged(int index);

    void on_lineFilename_textEdited(const QString &arg1);

private:
	void refreshDriveList();
	void updateBitrate();

	Ui::saveSettingsWindow *ui;
	QTimer * timer;
	UInt32 driveCount;
};

#endif // SAVESETTINGSWINDOW_H
