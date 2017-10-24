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
#ifndef RAMWINDOW_H
#define RAMWINDOW_H

#include <QWidget>

#include "util.h"
#include "camera.h"

namespace Ui {
class ramWindow;
}

class ramWindow : public QWidget
{
	Q_OBJECT
	
public:
	explicit ramWindow(QWidget *parent = 0, Camera * cameraInst = NULL);
	~ramWindow();
	
private slots:
	void on_cmdSet_clicked();

	void on_cmdRead_clicked();

private:
	Camera * camera;
	Ui::ramWindow *ui;
};

#endif // RAMWINDOW_H
