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

	void keyPressEvent(QKeyEvent *ev);
	void on_cmdClose_clicked();
	void saveSettingsClosed();

	void on_cmdLoop_clicked();

private:
	Ui::playbackWindow *ui;
	Camera * camera;
	StatusWindow * sw;
	void updateStatusText();
	void updatePlayRateLabel(Int32 playbackRate);
	void setControlEnable(bool en);

	UInt32 markInFrame, markOutFrame;
	UInt32 markInFrameOld;
	UInt32 lastPlayframe;
	QTimer * timer;
	QTimer * saveDoneTimer;
	Int32 playbackExponent;
	UInt32 playFrame;
	bool autoSaveFlag;
	bool settingsWindowIsOpen;

signals:
	void finishedSaving();
	void enableSaveSettingsButtons(bool);
};

#endif // PLAYBACKWINDOW_H
