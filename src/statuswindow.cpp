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
