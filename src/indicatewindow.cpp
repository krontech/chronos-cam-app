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

void IndicateWindow::setRGInfoText(QString str)
{
    ui->lblRGInfo->setStyleSheet("color: white;");
    ui->lblRGInfo->setText(str);
    ui->lblRGInfo->adjustSize();
    this->adjustSize();
}

void IndicateWindow::setTriggerText(QString str)
{
    ui->lblTrigger->setStyleSheet("color: white;");
    ui->lblTrigger->setText(str);
    ui->lblTrigger->adjustSize();
    this->adjustSize();
}

void IndicateWindow::setEstimatedTime(int seconds)
{
    QString estTime;
    if (seconds == 0) {
        estTime = "";
    }
    else {
        int textMinutes = seconds / 60;
        int textSeconds = seconds % 60;
        estTime = QString::number(textMinutes) + "m" + QString::number(textSeconds) + "s";
    }

    ui->lblEstTime->setStyleSheet("color: white;");
    ui->lblEstTime->setText(estTime);
    ui->lblEstTime->adjustSize();
    this->adjustSize();
}
