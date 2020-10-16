#ifndef EXTBROWSER_H
#define EXTBROWSER_H

#include <QWidget>
#include <QTableView>
#include "fileinfomodel.h"

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
    Ui::ExtBrowser* ui;
    QTableView      m_view;
    FileInfoModel   m_model;
};

#endif // EXTBROWSER_H
