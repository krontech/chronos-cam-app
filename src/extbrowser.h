#ifndef EXTBROWSER_H
#define EXTBROWSER_H

#include <QWidget>

namespace Ui {
class ExtBrowser;
}

class ExtBrowser : public QWidget
{
    Q_OBJECT

public:
    explicit ExtBrowser(QWidget *parent = 0);
    ~ExtBrowser();

private slots:
    void on_pushButton_clicked();

private:
    Ui::ExtBrowser *ui;
};

#endif // EXTBROWSER_H
