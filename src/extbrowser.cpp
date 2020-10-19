#include <QCheckBox>
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
    assert ( folder_to_descend_to.length() );

    new_path = current_path;
    new_path.append( folder_to_descend_to );

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
assemble_path_stay(
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

        case MoveDirection::stay:
            return
                assemble_path_stay(
                    current_path,
                    new_path );
    }

    assert (true);
}

void
ExtBrowser::update_current_path(
        QStringList& new_path )
{
    current_path.swap( new_path );
}

QString
ExtBrowser::move_to_folder_and_get_contents(
        MoveDirection   const   direction,
        QString         const&  folder_to_descend_to )
{
    QStringList new_path;

    QString const path =
        assemble_path(
            current_path,
            new_path,
            folder_to_descend_to,
            direction );

    std::string const command =
         (QString{ "ls -BghopqQt --group-directories-first" }
        + " "
        + QString{"/media/mmcblk1p1/"} + path
        ).toStdString();

    int status{0};

    QString const ls_output =
        runCommand(
            command.c_str(),
            &status );

    if ( -1 == status )
    {
        /// TODO : Failed getting data!
        return {};
    }

    update_current_path( new_path );

    return ls_output;
}

ExtBrowser::ExtBrowser(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ExtBrowser)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    this->move(0,0);

    {
        QString const ls_output =
            move_to_folder_and_get_contents( MoveDirection::stay );

        assert ( true == is_at_root() );

        auto const model_data =
            parse_ls_output(
                ls_output,
                true );

        m_model.set_data( model_data );
    }

    ui->tableView->setModel( &m_model );

    m_delegate.set_model(
        &m_model,
        this );

    ui->tableView->setItemDelegate( &m_delegate );

    ui->tableView->setSelectionBehavior( QAbstractItemView::SelectRows );
    ui->tableView->setSelectionMode( QAbstractItemView::MultiSelection );
}

ExtBrowser::~ExtBrowser()
{
    delete ui;
}

void ExtBrowser::on_pushButton_clicked()
{
    close();
}
