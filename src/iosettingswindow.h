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
	void getIoTriggerConfig(QVariantMap &config);
	void getIoShutterConfig(QVariantMap &config, QString expMode);
	void getIoSettings(void);
	UInt32 getIoLevels(void);

	void setIoConfig1(QVariantMap &config);
	void setIoConfig2(QVariantMap &config);
	void setIoConfig3(QVariantMap &config);
	void setIoSettings(void);


	Ui::IOSettingsWindow *ui;
	Camera * camera;
	UInt32 lastIn;
	QTimer * timer;
};

#endif // IOSETTINGSWINDOW_H
