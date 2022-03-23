#include <QtGui>

#include "indicatewindow.h"
#include "ui_indicatewindow.h"

IndicateWindow::IndicateWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::IndicateWindow)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);

    this->move(0,0);
    this->setStyleSheet("background-color: rgba(255, 255, 255, 0%)");
}

IndicateWindow::~IndicateWindow()
{
    delete ui;
}

void IndicateWindow::setRecModeText(QString str)
{
    ui->lblRecMode->setStyleSheet("color: white;");
    ui->lblRecMode->setText(str);
    ui->lblRecMode->adjustSize();
    this->adjustSize();
    this->move( 0,
                QApplication::desktop()->screenGeometry().height() - height());
}

void IndicateWindow::setRunGunText(QString str)
{
    ui->lblRunGun->setStyleSheet("color: white;");
    ui->lblRunGun->setText(str);
    ui->lblRunGun->adjustSize();
    this->adjustSize();
    this->move( 0,
                QApplication::desktop()->screenGeometry().height() - height());
}
