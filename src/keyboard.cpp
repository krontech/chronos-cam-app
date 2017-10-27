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
#include "keyboard.h"
#include "ui_keyboard.h"

keyboard::keyboard(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::keyboard)
{
	ui->setupUi(this);
	capslock = false;

	connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)),
			this, SLOT(saveFocusWidget(QWidget*,QWidget*)));

	signalMapper.setMapping(ui->Q, ui->Q);
	signalMapper.setMapping(ui->W, ui->W);
	signalMapper.setMapping(ui->E, ui->E);
	signalMapper.setMapping(ui->R, ui->R);
	signalMapper.setMapping(ui->T, ui->T);
	signalMapper.setMapping(ui->Y, ui->Y);
	signalMapper.setMapping(ui->U, ui->U);
	signalMapper.setMapping(ui->I, ui->I);
	signalMapper.setMapping(ui->O, ui->O);
	signalMapper.setMapping(ui->P, ui->P);
	signalMapper.setMapping(ui->A, ui->A);
	signalMapper.setMapping(ui->S, ui->S);
	signalMapper.setMapping(ui->D, ui->D);
	signalMapper.setMapping(ui->F, ui->F);
	signalMapper.setMapping(ui->G, ui->G);
	signalMapper.setMapping(ui->H, ui->H);
	signalMapper.setMapping(ui->J, ui->J);
	signalMapper.setMapping(ui->K, ui->K);
	signalMapper.setMapping(ui->L, ui->L);
	signalMapper.setMapping(ui->Z, ui->Z);
	signalMapper.setMapping(ui->X, ui->X);
	signalMapper.setMapping(ui->C, ui->C);
	signalMapper.setMapping(ui->V, ui->V);
	signalMapper.setMapping(ui->B, ui->B);
	signalMapper.setMapping(ui->N, ui->N);
	signalMapper.setMapping(ui->M, ui->M);
	signalMapper.setMapping(ui->num0, ui->num0);
	signalMapper.setMapping(ui->num1, ui->num1);
	signalMapper.setMapping(ui->num2, ui->num2);
	signalMapper.setMapping(ui->num3, ui->num3);
	signalMapper.setMapping(ui->num4, ui->num4);
	signalMapper.setMapping(ui->num5, ui->num5);
	signalMapper.setMapping(ui->num6, ui->num6);
	signalMapper.setMapping(ui->num7, ui->num7);
	signalMapper.setMapping(ui->num8, ui->num8);
	signalMapper.setMapping(ui->num9, ui->num9);
	signalMapper.setMapping(ui->numdot, ui->numdot);



	connect(ui->Q, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->W, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->E, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->R, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->T, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->Y, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->U, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->I, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->O, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->P, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->A, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->S, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->D, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->F, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->G, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->H, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->J, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->K, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->L, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->Z, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->X, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->C, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->V, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->B, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->N, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->M, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->num0, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->num1, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->num2, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->num3, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->num4, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->num5, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->num6, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->num7, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->num8, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->num9, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->numdot, SIGNAL(clicked()), &signalMapper, SLOT(map()));

	connect(&signalMapper, SIGNAL(mapped(QWidget*)),
			this, SLOT(buttonClicked(QWidget*)));
}

keyboard::~keyboard()
{
	delete ui;
}


void keyboard::show()
{
	//QDesktopWidget * qdw = QApplication::desktop();

	this->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
	QWidget::show();
	//Moving after showing because window height isn't set until show()
	this->move(0,QApplication::desktop()->screenGeometry().height() - height());


}



bool keyboard::event(QEvent *e)
{
	switch (e->type()) {

	case QEvent::WindowActivate:
		if (lastFocusedWidget)
		{
			lastFocusedWidget->activateWindow();
			qDebug() << "keyboard::event lastFocusedWidget activated" << lastFocusedWidget;
		}
		break;
	default:
		break;
	}

	return QWidget::event(e);
}



void keyboard::saveFocusWidget(QWidget * /*oldFocus*/, QWidget *newFocus)
{
	if (newFocus != 0 && !this->isAncestorOf(newFocus)) {
		lastFocusedWidget = newFocus;
		qDebug() << "keyboard::saveFocusWidget lastFocusedWidget set to" << newFocus;
	}
}



void keyboard::buttonClicked(QWidget *w)
{
	QPushButton * pb = (QPushButton *)w;
	QChar chr = pb->text()[0];
	emit characterGenerated(capslock || shift ? chr : chr.toLower());
	if(shift)
	{
		shift = false;
		ui->shift->setStyleSheet(QString(""));
	}
}

void keyboard::on_caps_clicked()
{
	if(capslock)
	{
		capslock = false;
		ui->caps->setStyleSheet(QString(""));
	}
	else
	{
		capslock = true;
		ui->caps->setStyleSheet(QString(KEYBOARD_BACKGROUND_BUTTON));
	}
}

void keyboard::on_back_clicked()
{
	emit codeGenerated(KC_BACKSPACE);
}

void keyboard::on_space_clicked()
{
	emit characterGenerated(QChar(' '));
}

void keyboard::on_shift_clicked()
{
	if(shift)
	{
		shift = false;
		ui->shift->setStyleSheet(QString(""));
	}
	else
	{
		shift = true;
		ui->shift->setStyleSheet(QString(KEYBOARD_BACKGROUND_BUTTON));
	}
}

void keyboard::on_up_clicked()
{
	emit codeGenerated(KC_UP);
}

void keyboard::on_down_clicked()
{
	emit codeGenerated(KC_DOWN);
}

void keyboard::on_left_clicked()
{
	emit codeGenerated(KC_LEFT);
}

void keyboard::on_right_clicked()
{
	emit codeGenerated(KC_RIGHT);
}

void keyboard::on_enter_clicked()
{
	emit codeGenerated(KC_ENTER);
}

void keyboard::on_close_clicked()
{
	emit codeGenerated(KC_CLOSE);
}
