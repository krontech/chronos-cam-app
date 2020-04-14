#include "expdialog.h"
#include "ui_expdialog.h"

ExpDialog::ExpDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ExpDialog)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);

    this->move(518,0);
}

ExpDialog::~ExpDialog()
{
    delete ui;
}
