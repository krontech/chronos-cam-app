#ifndef EXTBROWSER_H
#define EXTBROWSER_H

#include <QWidget>
#include <QTableView>
#include <QTimer>
#include "fileinfomodel.h"
#include "extbrowserdelegate.h"
#include "movedirection.h"
#include "storagedevice_info.h"

namespace Ui {
class ExtBrowser;
}

class ExtBrowser : public QWidget
{
    Q_OBJECT

private:
    enum class DeviceAndPathState : unsigned int {
        list_devices,
        descend_to_device,
        ascend_from_device,
        browse_device
    };

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
            QString         const&  folder_to_descend_to = {} );
private:
    DeviceAndPathState
    get_state(
            MoveDirection const  direction);
private:
    void
    update_path_label();
public:
    void
    setup_path_and_model_data(
            MoveDirection const     direction,
            QString                 file_name = {} );

private slots:
    void on_extBrowserCloseButton_clicked();
    void on_selection_changed(const QItemSelection &, const QItemSelection &);
    void on_timer_tick();

private:
    Ui::ExtBrowser*     ui;
    FileInfoModel       m_model;
    ExtBrowserDelegate  m_delegate;
private:
    QStringList         m_current_path;
    StorageDevice_Info  m_current_device;
    BrowserMode const   m_mode;
    QTimer* const       m_timer;
};

#endif // EXTBROWSER_H
