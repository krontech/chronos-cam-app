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
#include <QCheckBox>
#include <QTime>
#include <QMessageBox>
#include "extbrowser.h"
#include "ui_extbrowser.h"
#include "util.h"
#include "extbrowserparser.h"

/// Assembles path to the descending folder
static
void
assemble_path_down(
        QStringList const&  current_path,
        QStringList      &  new_path,
        QString     const&  folder_to_descend_to )
{
    new_path = current_path;

    if ( 0 != folder_to_descend_to.length() )
    {
        new_path.append( folder_to_descend_to );
    }
}

/// Assembles path to the ascending folder
///
/// Folder to ascend to is the last folder in the current_path.
static
void
assemble_path_up(
        QStringList const&  current_path,
        QStringList      &  new_path )
{
    auto const current_path_length = current_path.length();

    assert ( 0 < current_path_length );

    new_path = current_path;
    new_path.pop_back();
}

/// Assembles path to the current folder
static
void
assemble_path_list(
        QStringList const&  current_path,
        QStringList      &  new_path )
{
    new_path = current_path;
}

/// Assembles path to the target folder
///
/// Returns the path as a string and fills the new_path
/// list, but doesn't update current_path; that will happen
/// later only if 'ls' system call doesn't fail.
static
QString
assemble_path(
        QStringList   const&  current_path,
        QStringList        &  new_path,
        QString       const&  folder_to_descend_to,
        MoveDirection const   direction )
{
    switch( direction )
    {
        /// folder has been opened
        case MoveDirection::descend:
            assemble_path_down(
                current_path,
                new_path,
                folder_to_descend_to );
            break;

        /// up link has been opened
        case MoveDirection::ascend:
            assemble_path_up(
                current_path,
                new_path );
            break;

        /// just listing the contents of the current folder
        case MoveDirection::list:
            assemble_path_list(
                current_path,
                new_path );
            break;

        default:
            assert (false);
    }

    return
        new_path.join( "/" );
}

/// Gets block storage devices (USB and eSATA external drives and SD cards)
///
/// This must not fail; there's no real handling. And this function
/// doesn't throw. If things do go wrong nothing particulary bad will
/// happen - no devices will be listed.
static
QList<StorageDevice_Info>
get_nonnetwork_storage_devices()
{
    std::string const command{ "lsblk -inpr -o mountpoint,size,label | grep \"/media/\"" };

    int status{0};

    QString const lsblk_output =
        runCommand(
            command.c_str(),
            &status );

    if ( -1 == status )
    {
        /// TODO : Failed getting data!
        return {};
    }

    return
        parse_lsblk_output( lsblk_output );
}

/// Get all storage devices - block devices and network devices
static
QList<StorageDevice_Info>
get_storage_devices()
{
    QList<StorageDevice_Info> storage_devices =
            get_nonnetwork_storage_devices();

    get_network_shares( storage_devices );

    return storage_devices;
}

void
ExtBrowser::update_current_path(
        QStringList& new_path )
{
    m_current_path.swap( new_path );
}

QString
ExtBrowser::move_to_folder_and_get_contents(
        MoveDirection   const   direction,
        QString         const&  folder_to_descend_to )
{
    QStringList new_path;

    QString const path =
        assemble_path(
            m_current_path,
            new_path,
            folder_to_descend_to,
            direction );

    assert ( !m_current_device.mount_folder.isEmpty() );

    std::string const command =
         (QString{ "ls -BghopqQt --group-directories-first" }
        + " \""
        + m_current_device.mount_folder + QChar{'/'} + path + QChar('\"')
        ).toStdString();

    int status{0};

    QString const ls_output =
        runCommand(
            command.c_str(),
            &status );

    if (   (-1 == status)
        || ls_output.isEmpty() )
    {
        /// TODO : Failed getting data!
        throw "ls failed";
    }

    update_current_path( new_path );

    return ls_output;
}

ExtBrowser::DeviceAndPathState
ExtBrowser::get_state(
        MoveDirection const  direction)
{
    if ( m_current_device.mount_folder.isEmpty() )
    {
        if ( MoveDirection::list == direction )
        {
            return DeviceAndPathState::list_devices;
        }

        return DeviceAndPathState::descend_to_device;
    }

    if (   (m_current_path.isEmpty())
         & (MoveDirection::ascend == direction) )
    {
        return DeviceAndPathState::ascend_from_device;
    }

    return DeviceAndPathState::browse_device;
}

void
ExtBrowser::update_path_label()
{
    QString folder_path;
    if ( 0 < m_current_path.size() )
    {
        folder_path =
              QString{'/'}
            + m_current_path.join( "/" );
    }

    ui->extBrowserPathLabel->setText(
          QString{"Path: /"}
        + m_current_device.label
        + folder_path );
}

void
ExtBrowser::setup_path_and_model_data_impl (
        MoveDirection const  direction,
        QString              file_name )
{
    DeviceAndPathState const state =
        get_state( direction );

    QList<FileInfo> model_data;

    switch( state )
    {
        case DeviceAndPathState::ascend_from_device:
        {
            m_current_device = {};

            /// fallthrough
        }

        case DeviceAndPathState::list_devices:
        {
            auto const storage_devices =
                get_storage_devices();

            for( int i=0; i<storage_devices.size(); ++i )
            {
                model_data.append(
                    {   storage_devices.at(i).label,
                        true });
            }

            break;
        }

        case DeviceAndPathState::descend_to_device:
        {
            auto const storage_devices =
                get_storage_devices();

            for( int i=0; i<storage_devices.size(); ++i )
            {
                auto const device = storage_devices.at(i);
                if ( device.label == file_name )
                {
                    m_current_device = device;
                    break;
                }
            }

            file_name.clear();

            /// fallthrough
        }

        case DeviceAndPathState::browse_device:
        {
            QString const ls_output =
                move_to_folder_and_get_contents(
                    direction,
                    file_name );

            model_data =
                parse_ls_output(
                    ls_output,
                    BrowserMode::folder_selector == m_mode );

            break;
        }

        default: assert ( false );
    }

    m_model.set_data( model_data );

    update_path_label();
}

void
ExtBrowser::clear_selection()
{
    ui->tableView->clearSelection();
    ui->extBrowserSelectedCountLabel->setText( "0 files selected" );
    ui->extBrowserOpenButton->setEnabled( false );
    ui->extBrowserDeselectAllButton->setEnabled( false );
    ui->extBrowserDeleteSelectedButton->setEnabled( false );
}

void
ExtBrowser::setup_path_and_model_data(
        MoveDirection const  direction,
        QString              file_name )
{
    try
    {
        setup_path_and_model_data_impl(
            direction,
            file_name );
    }
    catch(...)
    {
        m_current_path      = {};
        m_current_device    = {};

        /// this call should never throw!
        setup_path_and_model_data_impl( MoveDirection::list );
    }

    clear_selection();

    bool const is_root_folder = m_current_device.mount_folder.isEmpty();
    ui->extBrowserSelectButton->setEnabled( ! is_root_folder );
}

/// Formats the current time as a string
static
QString
get_current_time_as_string()
{
    QTime const t = QTime::currentTime();

    QString const time_str =
         QString::number( t.hour() ).rightJustified( 2, '0' )
        +QChar(':')
        +QString::number( t.minute() ).rightJustified( 2, '0' );

    return time_str;
}

ExtBrowser::ExtBrowser(
        BrowserMode const       mode,
        Camera*                 camInst,
        Ui::saveSettingsWindow* ui_ssw,
        QWidget*                parent
    )
    :   QWidget                 (parent)
    ,   ui                      (new Ui::ExtBrowser)
    ,   camera                  (camInst)
    ,   ui_save_settings_window (ui_ssw)
    ,   m_mode                  (mode)
    ,   m_timer                 (new QTimer(this))
{
    ui->setupUi(this);

    /// setup clock
    connect(m_timer, SIGNAL(timeout()), this, SLOT(on_timer_tick()));
    ui->extBrowserClock->setText( QString{"Current time: "} + get_current_time_as_string() );
    m_timer->start(1000);

    /// enable/disable GUI elements depending on the mode (folder selection / browsing)
    ui->extBrowserOpenButton->setEnabled( false );
    ui->extBrowserDeselectAllButton->setEnabled( false );
    ui->extBrowserDeleteSelectedButton->setEnabled( false );
    ui->extBrowserSelectButton->setEnabled( false );

    if ( BrowserMode::file_browser == m_mode )
    {
        ui->extBrowserSelectButton->hide();
    }

    if ( BrowserMode::folder_selector == m_mode )
    {
        ui->extBrowserCloseButton->setText( "Cancel" );
        ui->extBrowserDeleteSelectedButton->hide();
        ui->extBrowserDeselectAllButton->hide();
        ui->extBrowserSelectedCountLabel->hide();
    }

    /// window setup
    this->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    this->move(0,0);

    assert ( m_current_device.mount_folder.isEmpty() );

    /// fill model data
    setup_path_and_model_data( MoveDirection::list );

    ui->tableView->setModel( &m_model );

    /// style the table
    ui->tableView->verticalHeader()->hide();
    ui->tableView->setColumnWidth( 0, 430 );
    ui->tableView->setColumnWidth( 1,  80 );
    ui->tableView->setColumnWidth( 2,  80 );
    ui->tableView->setColumnWidth( 3, 125 );

    ui->tableView->setAlternatingRowColors(true);
    ui->tableView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    ui->tableView->setShowGrid(false);

    QHeaderView* verticalHeader = ui->tableView->verticalHeader();
    verticalHeader->setResizeMode(QHeaderView::Fixed);
    verticalHeader->setDefaultSectionSize(40);

    /// set model and delegate
    m_delegate.set_model(
        &m_model );

    ui->tableView->setItemDelegate( &m_delegate );

    /// setup selection behaviour
    ui->tableView->setSelectionBehavior( QAbstractItemView::SelectRows );
    ui->tableView->setSelectionMode( QAbstractItemView::MultiSelection );

    connect(
        ui->tableView->selectionModel(),
        SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
        SLOT(on_selection_changed(const QItemSelection &, const QItemSelection &))
    );
}

ExtBrowser::~ExtBrowser()
{
    m_timer->stop();
    delete m_timer;
    delete ui;
}

void ExtBrowser::on_extBrowserCloseButton_clicked()
{
    close();
}

void ExtBrowser::on_selection_changed(
        const QItemSelection& sel,
        const QItemSelection& desel )
{
    auto const selection_model = ui->tableView->selectionModel();
    auto const index_list      = selection_model->selectedRows();

    int const number_of_selected_elements = index_list.length();

    QString const text = QString::number( number_of_selected_elements ) + " files selected";

    ui->extBrowserSelectedCountLabel->setText( text );

    if ( 1 == number_of_selected_elements )
    {
        auto const row = index_list.at(0).row();

        auto const& file_info =
            m_model.get_data().at( row );

        ui->extBrowserOpenButton->setEnabled( false == file_info.is_file() );
    }
    else
    {
        ui->extBrowserOpenButton->setEnabled( false );
    }

    ui->extBrowserDeselectAllButton->setEnabled( 0 != number_of_selected_elements );

    /// if any of the items selected is the 'Up folder'
    /// disable the delete button
    for(
        int i=0;
        i < number_of_selected_elements;
        ++i )
    {
        auto const row = index_list.at(i).row();

        auto const& file_info =
            m_model.get_data().at( row );

        assert ( file_info.is_valid() );

        if ( file_info.is_up_link() )
        {
            ui->extBrowserDeleteSelectedButton->setEnabled( false );
            return;
        }
    }

    bool const is_root_folder = m_current_device.mount_folder.isEmpty();

    ui->extBrowserDeleteSelectedButton->setEnabled( 0 != number_of_selected_elements && (! is_root_folder) );
}

void ExtBrowser::on_timer_tick()
{
    ui->extBrowserClock->setText( QString{"Current time: "} + get_current_time_as_string() );
}

void ExtBrowser::on_extBrowserOpenButton_clicked()
{
    auto const selection_model = ui->tableView->selectionModel();
    auto const index_list      = selection_model->selectedRows();

    assert ( 1 == index_list.length() );

    auto const row = index_list.at(0).row();

    auto const& file_info =
        m_model.get_data().at( row );

    MoveDirection direction = MoveDirection::descend;
    if ( file_info.is_up_link() )
    {
        direction = MoveDirection::ascend;
    } else {
        assert ( file_info.is_folder() );
    }

    setup_path_and_model_data(
        direction,
        file_info.get_name() );
}

void ExtBrowser::on_extBrowserDeselectAllButton_clicked()
{
    ui->tableView->clearSelection();
}

void ExtBrowser::on_extBrowserDeleteSelectedButton_clicked()
{
    auto const selection_model = ui->tableView->selectionModel();
    auto const index_list      = selection_model->selectedRows();

    int const number_of_selected_elements = index_list.length();

    assert ( 0 < number_of_selected_elements );
    assert ( false == m_current_device.mount_folder.isEmpty() );

    QString folder_path;
    if ( 0 < m_current_path.size() )
    {
        folder_path =
              QString{'/'}
            + m_current_path.join( "/" );
    }

    QString const directory_path =
          m_current_device.mount_folder
        + folder_path
        + QChar{'/'};

    std::string const directory_path_std = directory_path.toStdString();

    std::string command{ "rm -fr " };

    /// format the delete command
    for(
        int i=0;
        i < number_of_selected_elements;
        ++i )
    {
        auto const row = index_list.at(i).row();

        auto const& file_info =
            m_model.get_data().at( row );

        assert ( file_info.is_valid() );
        assert ( false == file_info.is_up_link() );

        command += '\"';
        command += directory_path_std;
        command += file_info.get_name().toStdString();
        command += "\" ";
    }

    /// need to flush the storage buffers
    command += ";sync";

    /// query user for confirmation
    QMessageBox::StandardButton const reply =
        QMessageBox::question(
            this,
            "Warning!",
                  QString{"Are you sure you want to delete "}
                + QString::number(number_of_selected_elements)
                + " files?",
            QMessageBox::Yes|QMessageBox::No);

    if ( QMessageBox::Yes != reply )
    {
        return;
    }

    int status{0};

    /// Delete files.
    /// It doesn't matter if it fails.
    QString const rm_output =
        runCommand(
            command.c_str(),
            &status );

    setup_path_and_model_data(
        MoveDirection::list );
}


void ExtBrowser::on_extBrowserSelectButton_clicked()
{
    assert ( 0 != camera );
    assert ( 0 != ui_save_settings_window );
    assert ( ! m_current_device.mount_folder.isEmpty() );

    QSettings settings;

    /// select the device
    {
        auto const storage_device   = m_current_device;
        auto&      combo_device     = *ui_save_settings_window->comboDrive;
        int  const combo_size       = combo_device.count();

        int index = 0;

        /// find the index of the current device in the comboBox list
        for(
            ;
            index < combo_size;
            ++index )
        {
            QString const item = combo_device.itemText( index );
            if ( item.startsWith( storage_device.mount_folder+QChar{' '} ) )
            {
                break;
            }
        }

        if ( index >= combo_size )
        {
            return;
        }

        combo_device.setCurrentIndex( index );

        const char * path;

        if(combo_device.isEnabled() && (index >= 0)) {
            path = combo_device.itemData(index).toString().toAscii().constData();
        }
        else {
            //No valid paths selected
            path = "";
        }

        strcpy(camera->cinst->fileDirectory, path);
        settings.setValue("recorder/fileDirectory", camera->cinst->fileDirectory);
    }

    /// select the folder
    {
        QString folder_path =
              m_current_path.join( "/" );

        if ( ! folder_path.isEmpty() )
        {
            folder_path += QChar{'/'};
        }

        std::string const folder_path_std = folder_path.toStdString();

        ui_save_settings_window->lineFoldername->setText( folder_path );

        strcpy(camera->cinst->fileFolder, folder_path_std.c_str());
        settings.setValue("recorder/fileFolder", camera->cinst->fileFolder);
    }

    close();
}
