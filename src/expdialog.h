#ifndef EXPDIALOG_H
#define EXPDIALOG_H

#include <QWidget>

namespace Ui {
class ExpDialog;
}

class ExpDialog : public QWidget
{
    Q_OBJECT

public:
    explicit ExpDialog(QWidget *parent = 0);
    ~ExpDialog();

private:
    Ui::ExpDialog *ui;
};

#endif // EXPDIALOG_H
