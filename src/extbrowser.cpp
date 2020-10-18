#include <QCheckBox>
#include "extbrowser.h"
#include "ui_extbrowser.h"

ExtBrowser::ExtBrowser(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ExtBrowser)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    this->move(0,0);

    m_model.append({    "FileA",            "mp4",  "5MB",   "Oct1,15:15"});
    m_model.append({    "Some other File",  "tiff", "100MB", "Oct14,08:43"});
    m_model.append({    "sloMo34File",      "mp4",  "1.3GB", "Oct5,17:24"});
    m_model.append({    "lastFile",         "mp4",  "932MB", "Nov1,12:00"});

    /*struct stat st;
    while (stat("/dev/sda", &st) == 0)
    {
        if (!S_ISBLK(st.st_mode))
        {
            break;
        }

        diskText = runCommand("ls -BghopqQt --group-directories-first /media/mmcblklpl");
        break;
    }*/

    //const QString diskText = runCommand("ls -BghopqQt --group-directories-first /media/mmcblklpl");

    ui->tableView->setModel( &m_model );
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
