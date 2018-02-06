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

#define KEYBOARD_BACKGROUND_BUTTON	"background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0, stop:0 rgba(0, 0, 0, 255), stop:1 rgba(255, 255, 255, 255));"
#define CHANGED_BACKGROUND_BUTTON	"background:lightgray;color:darkred;"

enum {
	KC_BACKSPACE = 1,
	KC_UP,
	KC_DOWN,
	KC_LEFT,
	KC_RIGHT,
	KC_ENTER,
	KC_CLOSE
};

namespace Ui {
class keyboard;
}

class keyboard : public QDialog
{
	Q_OBJECT
	
public:
	explicit keyboard(QWidget *parent = 0);
	QWidget * getLastFocsedWidget() {return lastFocusedWidget;}
	~keyboard();

public slots:
	void            show();
    void selectAllInFocusedWidget();

signals:
	void characterGenerated(QChar character);
	void codeGenerated(int code);

protected:
	bool event(QEvent *e);

private slots:
	void saveFocusWidget(QWidget *oldFocus, QWidget *newFocus);
	void buttonClicked(QWidget *w);

	void on_caps_clicked();

	void on_back_clicked();

	void on_space_clicked();

	void on_shift_clicked();

	void on_up_clicked();

	void on_down_clicked();

	void on_left_clicked();

	void on_right_clicked();

	void on_enter_clicked();

	void on_close_clicked();


private:
	bool capslock;
	bool shift;
	Ui::keyboard *ui;
	QWidget *lastFocusedWidget;
    QSignalMapper signalMapper;
};

#endif // KEYBOARD_H
