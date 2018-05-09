#include "whitebalancedialog.h"
#include "ui_whitebalancedialog.h"

whiteBalanceDialog::whiteBalanceDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::whiteBalanceDialog)
{
	ui->setupUi(this);
}

whiteBalanceDialog::~whiteBalanceDialog()
{
	delete ui;
}
