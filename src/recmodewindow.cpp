#include "recmodewindow.h"
#include "ui_recmodewindow.h"
#include "camera.h"

recModeWindow::recModeWindow(QWidget *parent, Camera * cameraInst) :
    QWidget(parent),
    ui(new Ui::recModeWindow)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Dialog /*| Qt::WindowStaysOnTopHint*/ | Qt::FramelessWindowHint);
    this->move(0,0);

    camera = cameraInst;
    is = camera->getImagerSettings();

    ui->chkDisableRing->setChecked(is.disableRingBuffer);

    ui->chkSegmentedEnable->setChecked(is.segments > 1);

    switch(is.mode)
    {
        case RECORD_MODE_NORMAL:
            ui->radioNormal->setChecked(true);
            ui->stackedWidget->setCurrentIndex(0);
            ui->spinRecLengthFrames->setValue(is.recRegionSizeFrames);
            ui->spinRecLengthSeconds->setValue((double)is.period / 100000000.0 * is.recRegionSizeFrames);
            ui->spinSegmentCount->setValue(is.segments);
        break;

        case RECORD_MODE_GATED_BURST:
            ui->radioGated->setChecked(true);
            ui->stackedWidget->setCurrentIndex(1);
            ui->spinPrerecordFrames->setValue(is.prerecordFrames);
            ui->spinPrerecordSeconds->setValue(((double)is.period / 100000000.0 * is.prerecordFrames));
        break;
    }

}

recModeWindow::~recModeWindow()
{
    delete ui;
}

void recModeWindow::on_cmdOK_clicked()
{

    is.disableRingBuffer = ui->chkDisableRing->isChecked();

    if(ui->radioNormal->isChecked())
    {
        is.mode = RECORD_MODE_NORMAL;
        is.segments = ui->chkSegmentedEnable->isChecked() ? ui->spinSegmentCount->value() : 1;
        is.segmentLengthFrames = is.recRegionSizeFrames / is.segments;

    }
    else if(ui->radioGated->isChecked())
    {
        is.mode = RECORD_MODE_GATED_BURST;
        is.prerecordFrames = ui->spinPrerecordFrames->value();
    }

    camera->setImagerSettings(is);

    close();
}

void recModeWindow::on_cmdCancel_clicked()
{
    close();
}

void recModeWindow::on_radioNormal_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void recModeWindow::on_radioGated_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}
