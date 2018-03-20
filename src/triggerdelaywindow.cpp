#include "triggerdelaywindow.h"
#include "ui_triggerdelaywindow.h"
#include "camera.h"
#include <QDebug>
triggerDelayWindow::triggerDelayWindow(QWidget *parent, Camera * cameraInst, ImagerSettings_t * imagerSettings, double periodFromRecSettingsWindow) :
    QWidget(parent),
    ui(new Ui::triggerDelayWindow)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Dialog /*| Qt::WindowStaysOnTopHint*/ | Qt::FramelessWindowHint);
    this->move(0,0);

    camera = cameraInst;
    is = imagerSettings;

    period = periodFromRecSettingsWindow;
    recLenFrames = ((is->mode == RECORD_MODE_NORMAL || is->mode == RECORD_MODE_GATED_BURST) ? is->recRegionSizeFrames : is->recRegionSizeFrames / is->segments);

    ui->spinPreFrames->setMaximum(recLenFrames);
    ui->spinPreSeconds->setMaximum((double)recLenFrames * period);
    ui->comboKeepConstant->setCurrentIndex(camera->getTriggerDelayConstant());

    UInt32 position;
	 if(camera->getTriggerDelayConstant() == TRIGGERDELAY_PRERECORDSECONDS)
      position = recLenFrames + (camera->triggerPreRecordSeconds / period);
    else if(camera->getTriggerDelayConstant() == TRIGGERDELAY_SECONDS)
	  position = (camera->triggerPostSeconds / period);
    else if(camera->getTriggerDelayConstant() == TRIGGERDELAY_FRAMES)
	  position = (camera->triggerPostFrames);
    ui->horizontalSlider->setMaximum(camera->maxPostFramesRatio * max(recLenFrames, position));
    ui->horizontalSlider->setHighlightRegion(0, recLenFrames);
    updateControls(position);
}

triggerDelayWindow::~triggerDelayWindow()
{
    delete ui;
}

void triggerDelayWindow::on_cmdOK_clicked()
{
    camera->setTriggerDelayConstant(ui->comboKeepConstant->currentIndex());
    camera->setTriggerDelayValues(ui->spinPreRecSeconds->value() - ui->spinPreSeconds->value(),//positive value if pre-rec time is used, or negative value if pre-trigger time is used
                                  ui->spinPostSeconds->value(),
                                  ui->spinPostFrames->value());
    camera->io->setTriggerDelayFrames(ui->spinPostFrames->value());
    camera->maxPostFramesRatio = ui->horizontalSlider->maximum() / (double)max(recLenFrames, ui->horizontalSlider->value());
    close();
}

void triggerDelayWindow::on_cmdCancel_clicked()
{
    close();
}

void triggerDelayWindow::on_horizontalSlider_sliderMoved(int position)
{
    if(ui->horizontalSlider->hasFocus())
        updateControls(position);
}

void triggerDelayWindow::on_cmdZeroPercent_clicked()
{
    updateControls(recLenFrames);
}

void triggerDelayWindow::on_cmdFiftyPercent_clicked()
{
    updateControls(recLenFrames / 2);
}

void triggerDelayWindow::on_cmdHundredPercent_clicked()
{
    updateControls(0);
}

void triggerDelayWindow::updateControls(UInt32 postTriggerFrames)
{
    UInt32 pretriggerFrames  = max((Int32)recLenFrames - (Int32)postTriggerFrames, 0);
    UInt32 preRecFrames = max((Int32)postTriggerFrames - (Int32)recLenFrames, 0);

    if(postTriggerFrames > ui->horizontalSlider->maximum())
        ui->horizontalSlider->setMaximum(postTriggerFrames);

    if(!ui->horizontalSlider->hasFocus())   ui->horizontalSlider->setSliderPosition(postTriggerFrames);
    if(!ui->spinPreFrames->hasFocus())      ui->spinPreFrames->setValue(pretriggerFrames);
    if(!ui->spinPreSeconds->hasFocus())     ui->spinPreSeconds->setValue((double)pretriggerFrames * period);
    if(!ui->spinPostFrames->hasFocus())     ui->spinPostFrames->setValue(postTriggerFrames);
    if(!ui->spinPostSeconds->hasFocus())    ui->spinPostSeconds->setValue((double)postTriggerFrames * period);
    if(!ui->spinPreRecFrames->hasFocus())   ui->spinPreRecFrames->setValue(preRecFrames);
    if(!ui->spinPreRecSeconds->hasFocus())  ui->spinPreRecSeconds->setValue((double)preRecFrames * period);
}

void triggerDelayWindow::on_cmdMax_clicked()
{
    ui->horizontalSlider->setMaximum(ui->horizontalSlider->maximum() * 2);
    ui->horizontalSlider->setHighlightRegion(0, recLenFrames);
}

void triggerDelayWindow::on_spinPreSeconds_valueChanged(double arg1)
{
    if(ui->spinPreSeconds->hasFocus())
        updateControls(recLenFrames - (arg1 / period));
}

void triggerDelayWindow::on_spinPreFrames_valueChanged(int arg1)
{
    if(ui->spinPreFrames->hasFocus())
        updateControls(recLenFrames - arg1);
}

void triggerDelayWindow::on_spinPostSeconds_valueChanged(double arg1)
{
    if(ui->spinPostSeconds->hasFocus())
        updateControls(arg1 / period);
}

void triggerDelayWindow::on_spinPostFrames_valueChanged(int arg1)
{
    if(ui->spinPostFrames->hasFocus())
        updateControls(arg1);
}

void triggerDelayWindow::on_spinPreRecSeconds_valueChanged(double arg1)
{
    if(ui->spinPreRecSeconds->hasFocus())
        updateControls(recLenFrames + (arg1 / period));
}

void triggerDelayWindow::on_spinPreRecFrames_valueChanged(int arg1)
{
    if(ui->spinPreRecFrames->hasFocus())
        updateControls(recLenFrames + arg1);
}

void triggerDelayWindow::on_cmdResetToDefaults_clicked()
{
    updateControls(0);
    ui->horizontalSlider->setMaximum(max(ui->spinPostFrames->value(), recLenFrames));
}
