#include "triggerdelaywindow.h"
#include "ui_triggerdelaywindow.h"
#include "camera.h"
#include <QDebug>

triggerDelayWindow::triggerDelayWindow(QWidget *parent, Camera * cameraInst, ImagerSettings_t *settings) :
	QWidget(parent),
	ui(new Ui::triggerDelayWindow)
{
	ui->setupUi(this);
	this->setWindowFlags(Qt::Dialog /*| Qt::WindowStaysOnTopHint*/ | Qt::FramelessWindowHint);
	this->move(0,0);

	camera = cameraInst;

    if (camera->guiMode == 1)
    {
        QFile styleFile(":/qss/darkstylesheet.qss");
        styleFile.open(QFile::ReadOnly);

        QString style(styleFile.readAll());
        this->setStyleSheet(style);
    }
    else
    {
        QFile styleFile(":/qss/lightstylesheet.qss");
        styleFile.open(QFile::ReadOnly);

        QString style(styleFile.readAll());
        this->setStyleSheet(style);
    }

	is = settings;
	period = (double)is->period / camera->getSensorInfo().timingClock;
	if ((is->mode == RECORD_MODE_NORMAL) || (is->mode == RECORD_MODE_GATED_BURST)) {
		recLenFrames = is->recRegionSizeFrames;
	}
	else {
		recLenFrames = (is->recRegionSizeFrames / is->segments);
	}

	ui->spinPreFrames->setMaximum(recLenFrames);
	ui->spinPreSeconds->setMaximum((double)recLenFrames * period);

	ui->horizontalSlider->setMaximum(max(recLenFrames, is->recTrigDelay));
	ui->horizontalSlider->setHighlightRegion(0, recLenFrames);
	updateControls(is->recTrigDelay);
}

triggerDelayWindow::~triggerDelayWindow()
{
	delete ui;
}

void triggerDelayWindow::on_cmdOK_clicked()
{
	is->recTrigDelay = ui->spinPostFrames->value();
	close();
}

void triggerDelayWindow::on_cmdCancel_clicked()
{
	close();
}

void triggerDelayWindow::on_horizontalSlider_sliderMoved(int position)
{
	if(ui->horizontalSlider->hasFocus()) {
		updateControls(position);
	}
}

void triggerDelayWindow::on_cmdZeroPercent_clicked()
{
	ui->horizontalSlider->clearFocus();
	updateControls(recLenFrames);
}

void triggerDelayWindow::on_cmdFiftyPercent_clicked()
{
	ui->horizontalSlider->clearFocus();
	updateControls(recLenFrames / 2);
}

void triggerDelayWindow::on_cmdHundredPercent_clicked()
{
	ui->horizontalSlider->clearFocus();
	updateControls(0);
}

void triggerDelayWindow::updateControls(UInt32 postTriggerFrames)
{
	UInt32 preTriggerFrames  = max((Int32)recLenFrames - (Int32)postTriggerFrames, 0);
	UInt32 preRecFrames = max((Int32)postTriggerFrames - (Int32)recLenFrames, 0);

	if(postTriggerFrames > ui->horizontalSlider->maximum()) {
		ui->horizontalSlider->setMaximum(postTriggerFrames);
	}

	if(!ui->horizontalSlider->hasFocus())   ui->horizontalSlider->setSliderPosition(postTriggerFrames);
	if(!ui->spinPreFrames->hasFocus())      ui->spinPreFrames->setValue(preTriggerFrames);
	if(!ui->spinPreSeconds->hasFocus())     ui->spinPreSeconds->setValue((double)preTriggerFrames * period);
	if(!ui->spinPostFrames->hasFocus())     ui->spinPostFrames->setValue(postTriggerFrames);
	if(!ui->spinPostSeconds->hasFocus())    ui->spinPostSeconds->setValue((double)postTriggerFrames * period);
	if(!ui->spinPreRecFrames->hasFocus())   ui->spinPreRecFrames->setValue(preRecFrames);
	if(!ui->spinPreRecSeconds->hasFocus())  ui->spinPreRecSeconds->setValue((double)preRecFrames * period);
}

void triggerDelayWindow::on_spinPreSeconds_valueChanged(double arg1)
{
	if(ui->spinPreSeconds->hasFocus()) {
        updateControls(recLenFrames - (arg1 / period));
	}
}

void triggerDelayWindow::on_spinPreFrames_valueChanged(int arg1)
{
	if(ui->spinPreFrames->hasFocus()) {
		updateControls(recLenFrames - arg1);
	}
}

void triggerDelayWindow::on_spinPostSeconds_valueChanged(double arg1)
{
	if(ui->spinPostSeconds->hasFocus()) {
		updateControls(arg1 / period);
	}
}

void triggerDelayWindow::on_spinPostFrames_valueChanged(int arg1)
{
	if(ui->spinPostFrames->hasFocus()) {
		updateControls(arg1);
	}
}

void triggerDelayWindow::on_spinPreRecSeconds_valueChanged(double arg1)
{
	if(ui->spinPreRecSeconds->hasFocus()) {
		updateControls(recLenFrames + (arg1 / period));
	}
}

void triggerDelayWindow::on_spinPreRecFrames_valueChanged(int arg1)
{
	if(ui->spinPreRecFrames->hasFocus()) {
		updateControls(recLenFrames + arg1);
	}
}

void triggerDelayWindow::on_cmdResetToDefaults_clicked()
{
	updateControls(0);
	ui->horizontalSlider->setMaximum(recLenFrames);
}

void triggerDelayWindow::on_cmdMorePreRecTime_clicked()
{
	ui->horizontalSlider->setMaximum(ui->horizontalSlider->maximum() * 2);
	ui->horizontalSlider->setHighlightRegion(0, recLenFrames);
}

void triggerDelayWindow::on_horizontalSlider_sliderReleased()
{
	ui->horizontalSlider->clearFocus();
}
