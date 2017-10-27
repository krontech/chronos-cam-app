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
#ifndef STATUSWINDOW_H
#define STATUSWINDOW_H

#include <QWidget>

namespace Ui {
class StatusWindow;
}

class StatusWindow : public QWidget
{
	Q_OBJECT
	
public:
	explicit StatusWindow(QWidget *parent = 0);
	~StatusWindow();
	void setText(QString str);
	void setTimeout(int time) {timeout = time;}
	
private:
	Ui::StatusWindow *ui;
	QTimer * timer;
	int timeout;

protected:
	void showEvent( QShowEvent* event );

private slots:
	void on_StatusWindowTimer();
};

#endif // STATUSWINDOW_H
