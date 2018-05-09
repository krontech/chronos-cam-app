#ifndef WHITEBALANCEDIALOG_H
#define WHITEBALANCEDIALOG_H

#include <QDialog>

namespace Ui {
class whiteBalanceDialog;
}

class whiteBalanceDialog : public QDialog
{
	Q_OBJECT

public:
	explicit whiteBalanceDialog(QWidget *parent = 0);
	~whiteBalanceDialog();

private:
	Ui::whiteBalanceDialog *ui;
};

#endif // WHITEBALANCEDIALOG_H
