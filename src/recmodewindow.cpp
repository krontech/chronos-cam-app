#include <QDebug>
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
            ui->grpSegmented->setVisible(false);
        break;

        case RECORD_MODE_SEGMENTED:
            ui->radioSegmented->setChecked(true);
            ui->stackedWidget->setCurrentIndex(0);
            ui->grpSegmented->setVisible(true);
        break;

        case RECORD_MODE_GATED_BURST:
            ui->radioGated->setChecked(true);
            ui->stackedWidget->setCurrentIndex(1);
        break;
    }

    ui->spinRecLengthFrames->setValue(is.recRegionSizeFrames);
    ui->spinRecLengthFrames->setMaximum(camera->getMaxRecordRegionSizeFrames(is.hRes, is.vRes));
    ui->spinRecLengthSeconds->setValue((double)is.period / 100000000.0 * is.recRegionSizeFrames);
    ui->spinRecLengthSeconds->setMaximum((double)(camera->getMaxRecordRegionSizeFrames(is.hRes, is.vRes)) * ((double) is.period / 100000000.0));
    ui->spinSegmentCount->setValue(is.segments);
    ui->spinPrerecordFrames->setValue(is.prerecordFrames);
    ui->spinPrerecordSeconds->setValue(((double)is.period / 100000000.0 * is.prerecordFrames));



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
        is.recRegionSizeFrames = ui->spinRecLengthFrames->value();
        is.segments = 1;
        is.segmentLengthFrames = is.recRegionSizeFrames;

    }
    else if(ui->radioSegmented->isChecked())
    {
        is.mode = RECORD_MODE_SEGMENTED;
        is.recRegionSizeFrames = ui->spinRecLengthFrames->value();
        is.segments = ui->spinSegmentCount->value();
        is.segmentLengthFrames = is.recRegionSizeFrames / is.segments;
    }
    else if(ui->radioGated->isChecked())
    {
        is.mode = RECORD_MODE_GATED_BURST;
        is.prerecordFrames = ui->spinPrerecordFrames->value();
    }
    qDebug() << "---- Record mode window ---- Rec length frames = " << is.recRegionSizeFrames;
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
    ui->grpSegmented->setVisible(false);
}

void recModeWindow::on_radioSegmented_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
    ui->grpSegmented->setVisible(true);
}

void recModeWindow::on_radioGated_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}



void recModeWindow::on_cmdMax_clicked()
{
    ui->spinRecLengthFrames->setValue(camera->getMaxRecordRegionSizeFrames(is.hRes, is.vRes));
    ui->spinRecLengthSeconds->setValue(ui->spinRecLengthFrames->value() * (double) is.period / 100000000.0);
}

void recModeWindow::on_spinRecLengthSeconds_valueChanged(double arg1)
{

}

void recModeWindow::on_spinRecLengthSeconds_editingFinished()
{
    ui->spinRecLengthFrames->setValue(ui->spinRecLengthSeconds->value() / ((double) is.period / 100000000.0));
}

void recModeWindow::on_spinRecLengthFrames_editingFinished()
{
    ui->spinRecLengthSeconds->setValue((double)ui->spinRecLengthFrames->value() * ((double) is.period / 100000000.0));
}
