#include <QCheckBox>
#include <QTime>
#include "extbrowser.h"
#include "ui_extbrowser.h"
#include "util.h"
#include "extbrowserparser.h"

/// methods
static
QString
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

    return
          new_path.join( "/" );
}

static
QString
assemble_path_up(
        QStringList const&  current_path,
        QStringList      &  new_path )
{
    auto const current_path_length = current_path.length();

    assert ( 0 < current_path_length );

    new_path = current_path;
    new_path.pop_back();

    return
          new_path.join( "/" );
}

static
QString
assemble_path_list(
        QStringList const&  current_path,
        QStringList      &  new_path )
{
    new_path = current_path;

    return
          new_path.join( "/" );
}

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
        case MoveDirection::descend:
            return
                assemble_path_down(
                    current_path,
                    new_path,
                    folder_to_descend_to );

        case MoveDirection::ascend:
            return
                assemble_path_up(
                    current_path,
                    new_path );

        case MoveDirection::list:
            return
                assemble_path_list(
                    current_path,
                    new_path );
    }

    assert (false);

    return {};
}

static
QList<StorageDevice_Info>
get_storage_devices()
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

    QList<StorageDevice_Info> const storage_devices =
            parse_lsblk_output( lsblk_output );

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
          QString{'/'}
        + m_current_device.label
        + folder_path );
}

void
ExtBrowser::setup_path_and_model_data(
        MoveDirection const  direction,
        QString              file_name )
{
    try
    {
        DeviceAndPathState const state =
            get_state( direction );

        QList<FileInfo> model_data;

        switch( state )
        {
            case DeviceAndPathState::ascend_from_device:
            {
                m_current_device = {};
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
    catch(...)
    {
        m_current_path      = {};
        m_current_device    = {};

        setup_path_and_model_data( MoveDirection::list );
    }
}

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
        BrowserMode const mode,
        QWidget*          parent
    )
        :   QWidget (parent)
        ,   ui      (new Ui::ExtBrowser)
        ,   m_mode  (mode)
        ,   m_timer (new QTimer(this))
{
    ui->setupUi(this);

    connect(m_timer, SIGNAL(timeout()), this, SLOT(on_timer_tick()));
    ui->extBrowserClock->setSegmentStyle(QLCDNumber::Flat);
    ui->extBrowserClock->display( get_current_time_as_string() );
    m_timer->start(1000);

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

    this->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    this->move(0,0);

    assert ( m_current_device.mount_folder.isEmpty() );

    setup_path_and_model_data( MoveDirection::list );

    ui->tableView->setModel( &m_model );

    m_delegate.set_model(
        &m_model,
        this );

    ui->tableView->setItemDelegate( &m_delegate );

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
}

void ExtBrowser::on_timer_tick()
{
    ui->extBrowserClock->display( get_current_time_as_string() );
}
