#ifndef EXTBROWSER_H
#define EXTBROWSER_H

/****************************************************************************
 *  Copyright (C) 2013-2020 Kron Technologies Inc <http://www.krontech.ca>. *
 *                                                                          *
 *  This program is free software: you can redistribute it and/or modify    *
 *  it under the terms of the GNU General Public License as published by    *
 *  the Free Software Foundation, either version 3 of the License, or       *
 *  (at your option) any later version.                                     *
 *                                                                          *
 *  This program is distributed in the hope that it will be useful,         *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU General Public License for more details.                            *
 *                                                                          *
 *  You should have received a copy of the GNU General Public License       *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/

#include "camera.h"

#include <QWidget>
#include <QTableView>
#include <QTimer>
#include "fileinfomodel.h"
#include "extbrowserdelegate.h"
#include "movedirection.h"
#include "storagedevice_info.h"
#include "ui_savesettingswindow.h"

namespace Ui {
class ExtBrowser;
}

class ExtBrowser : public QWidget
{
    Q_OBJECT

private:
    /// The state of the device/folder/direction combo.
    /// Devices are presented as a folder, although they are not,
    /// and some special handling is needed when crossing the boundary.
    ///
    /// Two members need to be updated: m_current_device and m_current_path.
    /// This enum helps simplify the state keeping.
    enum class DeviceAndPathState : unsigned int {
        list_devices,
        descend_to_device,
        ascend_from_device,
        browse_device
    };

public:
    /// External storage browser is configurable (but not extendable).
    /// It comes in the modes: folder selection mode (for picking
    /// device/folder) and browsing mode (for confirming the data was
    /// actually written to storage and deleting files).
    enum class BrowserMode : unsigned int {
        folder_selector,
        file_browser
    };
public:
    /// Constructor
    ///
    /// The mode of the external storage browser is selected by arguments
    /// passed to the constructor; only mode argument -> browsing mode.
    /// Add to that camera instance and saveSettingsWindow ui -> folder
    /// selection mode.
    explicit
    ExtBrowser(
            BrowserMode const       mode,
            Camera*                 camInst = 0,
            Ui::saveSettingsWindow* ui = 0,
            QWidget*                parent = 0);
    ~ExtBrowser();

private:
    /// After all system calls have been complited successfully,
    /// update the current path.
    void
    update_current_path(
            QStringList& new_path );
private:
    /// Get the contents of the folder (as a string containing
    /// the output of 'ls' system call) and possibly move to a
    /// different folder.
    ///
    /// This method might throw if 'ls' fails.
    QString
    move_to_folder_and_get_contents(
            MoveDirection   const   direction,
            QString         const&  folder_to_descend_to = {} );
private:
    /// Get the state of the device/folder/direction combo.
    /// Devices are presented as a folder, although they are not,
    /// and some special handling is needed when crossing the boundary.
    DeviceAndPathState
    get_state(
            MoveDirection const  direction);
private:
    /// Updates the 'path' label on the GUI
    void
    update_path_label();
private:
    /// When current folder is changed the selection releated GUI elements
    /// are not updated automatically. This method does it manually.
    void
    clear_selection();
private:
    /// Updates current device/folder (based on input) and fills model data
    /// with the contents of the current folder.
    ///
    /// May throw.
    void
    setup_path_and_model_data_impl (
            MoveDirection const  direction,
            QString              file_name = {} );
public:
    /// Updates current device/folder (based on input) and fills model data
    /// with the contents of the current folder.
    ///
    /// Never throws. On 'ls' failure (which might happen if the current device
    /// is ejected mid browsing) resets to the 'top' and lists devices.
    void
    setup_path_and_model_data(
            MoveDirection const     direction,
            QString                 file_name = {} );

private slots:
    void on_extBrowserCloseButton_clicked();
    void on_selection_changed(const QItemSelection &, const QItemSelection &);
    void on_timer_tick();
    void on_extBrowserOpenButton_clicked();
    void on_extBrowserDeselectAllButton_clicked();
    void on_extBrowserDeleteSelectedButton_clicked();
    void on_extBrowserSelectButton_clicked();

private:
    Ui::ExtBrowser*         ui;
    FileInfoModel           m_model;
    ExtBrowserDelegate      m_delegate;
    Camera*                 camera;
    Ui::saveSettingsWindow* ui_save_settings_window;
private:
    QStringList             m_current_path;
    StorageDevice_Info      m_current_device;
    BrowserMode const       m_mode;
    QTimer* const           m_timer;
};

#endif // EXTBROWSER_H
