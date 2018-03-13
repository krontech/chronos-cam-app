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
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <QtGui>
#include <QtCore>
#include <QWidget>
#include <QDialog>
#include "keyboardbase.h"


namespace Ui {
class keyboard;
}

class keyboard : public keyboardBase
{
	Q_OBJECT
	
public:
	explicit keyboard(QWidget *parent = 0);
	~keyboard();



private slots:

	void on_caps_clicked();

	void on_space_clicked();

	void on_shift_clicked();

	void buttonClicked(QWidget *w);


private:
	Ui::keyboard *ui;
	bool capslock;
	bool shift;
};

#endif // KEYBOARD_H
