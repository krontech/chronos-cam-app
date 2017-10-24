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
#include <QtGui>

#include "statuswindow.h"
#include "ui_statuswindow.h"

StatusWindow::StatusWindow(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::StatusWindow)
{
	ui->setupUi(this);
	this->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);

	this->move(0,0);
	//this->adjustSize();
	timeout = 0;
	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(on_StatusWindowTimer()));

}

StatusWindow::~StatusWindow()
{
	delete timer;
	delete ui;
}

void StatusWindow::setText(QString str)
{
	ui->label->setText(str);
	ui->label->adjustSize();
	this->adjustSize();
	this->move(( QApplication::desktop()->screenGeometry().width() - width()) / 2,
				(QApplication::desktop()->screenGeometry().height() - height()) / 2);
}

void StatusWindow::showEvent( QShowEvent* event )
{
	QWidget::showEvent( event );
	if(timeout)
		timer->start(timeout);
}

void StatusWindow::on_StatusWindowTimer()
{
	this->hide();
	timeout = 0;
}
