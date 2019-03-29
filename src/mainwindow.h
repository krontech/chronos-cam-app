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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "camera.h"
#include "types.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
	Camera * camera;

private slots:

	void on_cmdWrite_clicked();

	void on_cmdAdvPhase_clicked();

	void on_cmdSeqOn_clicked();

	void on_cmdSeqOff_clicked();

	void on_cmdSetOffset_clicked();

	void on_cmdTrigger_clicked();

	void on_cmdRam_clicked();

	void on_cmdReset_clicked();

	void on_cmdFPN_clicked();

	void on_cmdFrameNumbers_clicked();

	void on_cmdDecPhase_clicked();

	void on_cmdGC_clicked();

	void on_cmdSaveOC_clicked();

	void on_cmdSaveFrame_clicked();

private:
	Ui::MainWindow *ui;
	void writePixel12(UInt32 pixel, UInt32 offset, UInt16 value);
};

#endif // MAINWINDOW_H
