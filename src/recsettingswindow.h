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
#ifndef RECSETTINGSWINDOW_H
#define RECSETTINGSWINDOW_H

#include <QWidget>
#include <QCloseEvent>

#include "camera.h"

namespace Ui {
class RecSettingsWindow;
}

class RecSettingsWindow : public QWidget
{
	Q_OBJECT
	
public:
	explicit RecSettingsWindow(QWidget *parent = 0, Camera * cameraInst = NULL);
	~RecSettingsWindow();

private slots:
	void on_cmdOK_clicked();

	void on_spinHRes_valueChanged(int arg1);

	void on_spinHRes_editingFinished();

	void on_spinVRes_valueChanged(int arg1);

	void on_spinVRes_editingFinished();

	void on_spinHOffset_valueChanged(int arg1);

	void on_spinHOffset_editingFinished();

	void on_spinVOffset_valueChanged(int arg1);

	void on_spinVOffset_editingFinished();

	void on_chkCenter_toggled(bool checked);

	void on_cmdMax_clicked();

	void on_linePeriod_returnPressed();

	void on_lineRate_returnPressed();

	void on_lineExp_returnPressed();

	void on_cmdExpMax_clicked();

	void closeEvent(QCloseEvent *event);

	void on_comboRes_activated(const QString &arg1);

    void on_cmdRecMode_clicked();

    void on_cmdDelaySettings_clicked();

private:
	void updateOffsetLimits();
	void updateFrameImage();
	void updateInfoText();
	void setResFromText(char * str);
	Ui::RecSettingsWindow *ui;
	Camera * camera;
    ImagerSettings_t * is;
    bool windowInitComplete;

signals:
	void settingsChanged();

};

#endif // RECSETTINGSWINDOW_H
