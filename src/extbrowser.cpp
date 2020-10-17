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

    ui->tableView->setModel( &m_model );

    const auto rows_count = m_model.rowCount( QModelIndex{} );

    for( int i=0; i<rows_count; i++ )
    {
        auto item = m_model.index( i, 0 );
        QCheckBox* check_box = new QCheckBox("Add");
        ui->tableView->setIndexWidget(item, check_box);
    }
}

ExtBrowser::~ExtBrowser()
{
    delete ui;
}

#include <iostream>

void ExtBrowser::on_pushButton_clicked()
{
    std::cout<< "DINO" << std::endl;

    const QCheckBox* chkbox = qobject_cast<const QCheckBox*>( ui->tableView->indexWidget( ui->tableView->model()->index(0,0) ) );
    if ( nullptr != chkbox )
        std::cout<< chkbox->isChecked() << std::endl;

    std::cout<< "DINO" << std::endl;

    close();
}
