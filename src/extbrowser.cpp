#include "extbrowser.h"
#include "ui_extbrowser.h"

ExtBrowser::ExtBrowser(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ExtBrowser)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    this->move(0,0);

/*    m_layout.addWidget(&m_view, 0, 0, 1, 1);
    m_layout.addWidget(&m_button, 1, 0, 1, 1);
    connect(&m_button, SIGNAL(clicked()), &m_dialog, SLOT(open()));*/
    m_model.append({    "FileA",            "mp4",  "5MB",   "Oct1,15:15"});
    m_model.append({    "Some other File",  "tiff", "100MB", "Oct14,08:43"});
    m_model.append({    "sloMo34File",      "mp4",  "1.3GB", "Oct5,17:24"});
    m_model.append({    "lastFile",         "mp4",  "932MB", "Nov1,12:00"});
    m_view.setModel( &m_model );
/*    m_proxy.setSourceModel(&m_model);
    m_proxy.setFilterKeyColumn(2);
    m_view.setModel(&m_proxy);*/
/*    m_dialog.setLabelText("Enter registration number fragment to filter on. Leave empty to clear filter.");
    m_dialog.setInputMode(QInputDialog::TextInput);*/
}

ExtBrowser::~ExtBrowser()
{
    delete ui;
}

void ExtBrowser::on_pushButton_clicked()
{
    close();
}
