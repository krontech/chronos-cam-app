#include "extbrowser.h"
#include "ui_extbrowser.h"

ExtBrowser::ExtBrowser(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ExtBrowser)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    this->move(0,0);
}

ExtBrowser::~ExtBrowser()
{
    delete ui;
}

void ExtBrowser::on_pushButton_clicked()
{
    close();
}
