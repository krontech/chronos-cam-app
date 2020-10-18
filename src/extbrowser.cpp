#include <QCheckBox>
#include "extbrowser.h"
#include "ui_extbrowser.h"
#include "util.h"

QList<FileInfo>
parse_ls_output(
        QString const&  ls_output,
        bool    const   is_root );

ExtBrowser::ExtBrowser(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ExtBrowser)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    this->move(0,0);

    /*struct stat st;
    while (stat("/dev/sda", &st) == 0)
    {
        if (!S_ISBLK(st.st_mode))
        {
            break;
        }

        diskText = runCommand("ls -BghopqQt --group-directories-first /media/mmcblk1p1");
        break;
    }*/

    int status;
    QString const ls_output =
        runCommand(
            "ls -BghopqQt --group-directories-first /media/mmcblk1p1",
            &status );

    if ( -1 == status )
    {
        /// TODO : Failed getting data!
    }

    auto const model_data = parse_ls_output( ls_output, true );

    m_model.set_data( model_data );

    //std::cout << ls_output.toStdString() << std::endl;

    ui->tableView->setModel( &m_model );

    m_delegate.set_model( &m_model );

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
