#include "recmodewindow.h"
#include "ui_recmodewindow.h"
#include "camera.h"

recModeWindow::recModeWindow(QWidget *parent, Camera * cameraInst, ImagerSettings_t * settings) :
    QWidget(parent),
    ui(new Ui::recModeWindow)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Dialog /*| Qt::WindowStaysOnTopHint*/ | Qt::FramelessWindowHint);
    this->move(0,0);

    camera = cameraInst;
    is = settings;

    ui->chkDisableRing->setChecked(is->disableRingBuffer);
	ui->chkDisableRing->setVisible(false);

    switch(is->mode)
    {
        case RECORD_MODE_NORMAL:
            ui->radioNormal->setChecked(true);
            ui->stackedWidget->setCurrentIndex(0);
            ui->grpSegmented->setVisible(false);
            ui->lblNormal->setText("Records frames in a single buffer");
        break;

        case RECORD_MODE_SEGMENTED:
            ui->radioSegmented->setChecked(true);
            ui->stackedWidget->setCurrentIndex(0);
            ui->grpSegmented->setVisible(true);
            ui->lblNormal->setText("Records frames in a segmented buffer, one buffer per trigger");
        break;

        case RECORD_MODE_GATED_BURST:
            ui->radioGated->setChecked(true);
            ui->stackedWidget->setCurrentIndex(1);
		  ui->grpSegmented->setVisible(false);
        break;

        case RECORD_MODE_FPN:
        default:
            /* Internal recording modes only. */
        break;
    }

    ui->spinRecLengthFrames->setMaximum(camera->getMaxRecordRegionSizeFrames(is->hRes, is->vRes));
    ui->spinRecLengthFrames->setValue(is->recRegionSizeFrames);

    ui->spinRecLengthSeconds->setMaximum((double)(camera->getMaxRecordRegionSizeFrames(is->hRes, is->vRes)) * ((double) is->period / 100000000.0));
    ui->spinRecLengthSeconds->setValue((double)is->period / 100000000.0 * is->recRegionSizeFrames);

    ui->spinSegmentCount->setValue(is->segments);

    ui->spinPrerecordFrames->setMaximum(camera->getMaxRecordRegionSizeFrames(is->hRes, is->vRes) / 2);
    ui->spinPrerecordSeconds->setMaximum(ui->spinPrerecordFrames->maximum() * (double)is->period / 100000000.0);
    ui->spinPrerecordSeconds->setMinimum((double)is->period / 100000000.0);

    ui->spinPrerecordFrames->setValue(is->prerecordFrames);
    ui->spinPrerecordSeconds->setValue(((double)is->period / 100000000.0 * is->prerecordFrames));



    ui->lblSegmentSize->setText("Segment size:\n" + QString::number(ui->spinRecLengthFrames->value() / ui->spinSegmentCount->value() * ((double) is->period / 100000000.0)) + " s\n(" + QString::number(ui->spinRecLengthFrames->value() / ui->spinSegmentCount->value()) + " frames)");


}

recModeWindow::~recModeWindow()
{
    delete ui;
}

void recModeWindow::on_cmdOK_clicked()
{
    //is->disableRingBuffer = ui->chkDisableRing->isChecked();

    if(ui->radioNormal->isChecked())
    {
        is->mode = RECORD_MODE_NORMAL;
        is->recRegionSizeFrames = ui->spinRecLengthFrames->value();
        is->segments = 1;
        is->segmentLengthFrames = is->recRegionSizeFrames;

    }
    else if(ui->radioSegmented->isChecked())
    {
        is->mode = RECORD_MODE_SEGMENTED;
        is->recRegionSizeFrames = ui->spinRecLengthFrames->value();
        is->segments = ui->spinSegmentCount->value();
        is->segmentLengthFrames = is->recRegionSizeFrames / is->segments;
    }
    else if(ui->radioGated->isChecked())
    {
        is->mode = RECORD_MODE_GATED_BURST;
        is->prerecordFrames = ui->spinPrerecordFrames->value();
    }

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
    ui->lblNormal->setText("Records frames in a single buffer");
}

void recModeWindow::on_radioSegmented_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
    ui->grpSegmented->setVisible(true);
    ui->lblNormal->setText("Records frames in a segmented buffer, one buffer per trigger");
}

void recModeWindow::on_radioGated_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void recModeWindow::on_cmdMax_clicked()
{
    UInt32 recLenFrames = camera->getMaxRecordRegionSizeFrames(is->hRes, is->vRes);
    ui->spinRecLengthFrames->setValue(recLenFrames);
    ui->spinRecLengthSeconds->setValue((double)recLenFrames * (double) is->period / 100000000.0);
    ui->spinSegmentCount->setMaximum(min(SEGMENT_COUNT_MAX, recLenFrames));
    updateSegmentSizeText(ui->spinSegmentCount->value());
}

void recModeWindow::on_spinRecLengthSeconds_valueChanged(double arg1)
{
    if(ui->spinRecLengthSeconds->hasFocus())
    {
        UInt32 recLenFrames = arg1 / ((double) is->period / 100000000.0);

        if(recLenFrames < RECORD_LENGTH_MIN)
        {
            recLenFrames = RECORD_LENGTH_MIN;
            ui->spinRecLengthSeconds->setValue(recLenFrames * ((double) is->period / 100000000.0));

        }

        ui->spinRecLengthFrames->setValue(recLenFrames);
        ui->spinSegmentCount->setMaximum(min(SEGMENT_COUNT_MAX, recLenFrames));

        if(ui->radioSegmented->isChecked())
            updateSegmentSizeText(ui->spinSegmentCount->value());
    }

}

void recModeWindow::on_spinRecLengthFrames_valueChanged(int arg1)
{
    if(ui->spinRecLengthFrames->hasFocus())
    {
        ui->spinRecLengthSeconds->setValue((double)arg1 * ((double) is->period / 100000000.0));
        ui->spinSegmentCount->setMaximum(min(SEGMENT_COUNT_MAX, arg1));

        if(ui->radioSegmented->isChecked())
            updateSegmentSizeText(ui->spinSegmentCount->value());
    }
}

void recModeWindow::on_spinSegmentCount_valueChanged(int arg1)
{
    updateSegmentSizeText(arg1);
}

void recModeWindow::updateSegmentSizeText(UInt32 segmentCount)
{
    ui->lblSegmentSize->setText("Segment size:\n" +
        QString::number(ui->spinRecLengthFrames->value() / segmentCount * ((double) is->period / 100000000.0)) +
        " s\n(" + QString::number(ui->spinRecLengthFrames->value() / segmentCount) + " frames)");
}

void recModeWindow::on_spinPrerecordSeconds_valueChanged(double arg1)
{
    if(ui->spinPrerecordSeconds->hasFocus())
    {
        ui->spinPrerecordFrames->setValue(ui->spinPrerecordSeconds->value() / ((double) is->period / 100000000.0));
    }
}

void recModeWindow::on_spinPrerecordFrames_valueChanged(int arg1)
{
    if(ui->spinPrerecordFrames->hasFocus())
    {
        ui->spinPrerecordSeconds->setValue((double)ui->spinPrerecordFrames->value() * ((double) is->period / 100000000.0));
    }
}

//Update seconds spinbox so that it reflects the exact delay after editing is finished
void recModeWindow::on_spinPrerecordSeconds_editingFinished()
{
    ui->spinPrerecordFrames->setValue(ui->spinPrerecordSeconds->value() / ((double) is->period / 100000000.0));
    ui->spinPrerecordSeconds->setValue((double)ui->spinPrerecordFrames->value() * ((double) is->period / 100000000.0));
}
