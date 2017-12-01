#include "triggerdelaywindow.h"
#include "ui_triggerdelaywindow.h"

triggerDelayWindow::triggerDelayWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::triggerDelayWindow)
{
    ui->setupUi(this);
}

triggerDelayWindow::~triggerDelayWindow()
{
    delete ui;
}
