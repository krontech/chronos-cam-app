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
    enum class BrowserMode : unsigned int {
        folder_selector,
        file_browser
    };
public:
    explicit
    ExtBrowser(
            BrowserMode const mode,
            QWidget*          parent = 0);
    ~ExtBrowser();

private:
    void
    update_current_path(
            QStringList& new_path );
private:
    QString
    move_to_folder_and_get_contents(
            MoveDirection   const   direction,
            QString         const&  folder_to_descend_to = QString{} );
private:
    bool
    is_at_root() const
    {
        return 0 == m_current_path.length();
    }
public:
    void
    setup_path_and_model_data(
            MoveDirection const     direction,
            QString       const&    file_name = QString{} );

private slots:
    void on_extBrowserCloseButton_clicked();

private:
    Ui::ExtBrowser*     ui;
    FileInfoModel       m_model;
    ExtBrowserDelegate  m_delegate;
private:
    QStringList         m_current_path;
    BrowserMode const   m_mode;
};

#endif // EXTBROWSER_H
