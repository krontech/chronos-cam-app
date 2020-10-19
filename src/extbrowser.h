#ifndef EXTBROWSER_H
#define EXTBROWSER_H

#include <QWidget>
#include <QTableView>
#include "fileinfomodel.h"
#include "extbrowserdelegate.h"
#include "movedirection.h"

namespace Ui {
class ExtBrowser;
}

class ExtBrowser : public QWidget
{
    Q_OBJECT

public:
    explicit ExtBrowser(QWidget *parent = 0);
    ~ExtBrowser();

private:
    void
    update_current_path(
            QStringList& new_path );
public:
    QString
    move_to_folder_and_get_contents(
            MoveDirection   const   direction,
            QString         const&  folder_to_descend_to = QString{} );
public:
    bool
    is_at_root() const
    {
        return 0 == current_path.length();
    }

private slots:
    void on_pushButton_clicked();

private:
    Ui::ExtBrowser*     ui;
    FileInfoModel       m_model;
    ExtBrowserDelegate  m_delegate;
private:
    QStringList         current_path;
};

#endif // EXTBROWSER_H
