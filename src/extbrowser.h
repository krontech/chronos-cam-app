#ifndef EXTBROWSER_H
#define EXTBROWSER_H

#include <QWidget>
#include <QTableView>
#include "fileinfomodel.h"
#include "extbrowserdelegate.h"

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
    Ui::ExtBrowser*     ui;
    FileInfoModel       m_model;
    ExtBrowserDelegate  m_delegate;
};

#endif // EXTBROWSER_H
