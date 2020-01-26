/****************************************************************************
 *  Copyright (C) 2019-2020 Kron Technologies Inc <http://www.krontech.ca>. *
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
#ifndef UPDATEWINDOW_H
#define UPDATEWINDOW_H

#include <QDialog>
#include <QTimer>
#include <QProcess>

namespace Ui {
class UpdateWindow;
}

class UpdateWindow : public QDialog
{
	Q_OBJECT

public:
	explicit UpdateWindow(QWidget *parent = 0);
	~UpdateWindow();

private:
	Ui::UpdateWindow *ui;
	QTimer *mediaTimer;
	QProcess *aptCheck;
	int aptUpdateCount;

private slots:
	void checkForMedia();

	void started();
	void finished(int code, QProcess::ExitStatus status);
	void error(QProcess::ProcessError err);
	void readyReadStandardOutput();
	void updateClosed();

	void on_cmdApplyMedia_clicked();
	void on_cmdRescanNetwork_clicked();
	void on_comboGui_currentIndexChanged(int);
	void on_cmdApplyGui_clicked();
	void on_cmdListSoftware_clicked();
	void on_cmdReboot_clicked();
	void on_cmdQuit_clicked();
};

#endif // UPDATEWINDOW_H
