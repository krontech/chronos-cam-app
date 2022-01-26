#include "recmodewindow.h"
#include "ui_recmodewindow.h"
#include "camera.h"

#include <QListView>

recModeWindow::recModeWindow(QWidget *parent, Camera * cameraInst, ImagerSettings_t * settings) :
    QWidget(parent),
    ui(new Ui::recModeWindow)
{
	windowOpening = true;
    ui->setupUi(this);
    this->setWindowFlags(Qt::Dialog /*| Qt::WindowStaysOnTopHint*/ | Qt::FramelessWindowHint);
    this->move(0,0);

    camera = cameraInst;

    is = settings;
	sensor = camera->getSensorInfo();

    ui->chkDisableRing->setChecked(is->disableRingBuffer);
    ui->chkRunNGun->setChecked(camera->get_runngun());

	showLoopInformation();

	if (camera->liveSlowMotion)
	{
		is->mode = RECORD_MODE_LIVE;
	}

    switch(is->mode)
    {
        case RECORD_MODE_NORMAL:
            ui->radioNormal->setChecked(true);
            ui->stackedWidget->setCurrentIndex(0);
            ui->grpSegmented->setVisible(false);
            ui->chkDisableRing->setVisible(false);
            ui->chkRunNGun->setVisible(false);
            ui->lblNormal->setText("Records frames in a single buffer");
        break;

        case RECORD_MODE_SEGMENTED:
            ui->radioSegmented->setChecked(true);
            ui->stackedWidget->setCurrentIndex(0);
            ui->grpSegmented->setVisible(true);
            ui->chkDisableRing->setVisible(true);
            ui->chkRunNGun->setVisible(true);
            ui->lblNormal->setText("Records frames in a segmented buffer, one buffer per trigger");
        break;

		case RECORD_MODE_GATED_BURST:
			ui->radioGated->setChecked(true);
			ui->stackedWidget->setCurrentIndex(1);
			ui->grpSegmented->setVisible(false);
			ui->chkDisableRing->setVisible(false);
            ui->chkRunNGun->setVisible(false);
		break;

		case RECORD_MODE_LIVE:
			ui->radioLive->setChecked(true);
			ui->stackedWidget->setCurrentIndex(2);
			ui->grpSegmented->setVisible(false);
            ui->chkDisableRing->setVisible(false);
            ui->chkRunNGun->setVisible(false);
		break;

		case RECORD_MODE_FPN:
        default:
            /* Internal recording modes only. */
        break;
    }

	if (camera->liveOneShot)
	{
		ui->radioOneShot->setChecked(true);
	}
	else
	{
		ui->radioContinuous->setChecked(true);
	}


	ui->spinRecLengthFrames->setMaximum(camera->getMaxRecordRegionSizeFrames(&is->geometry));
	ui->spinRecLengthFrames->setValue(is->recRegionSizeFrames);

	ui->spinRecLengthSeconds->setMaximum((double)(camera->getMaxRecordRegionSizeFrames(&is->geometry)) * ((double) is->period / sensor.timingClock));
	ui->spinRecLengthSeconds->setValue((double)is->period / sensor.timingClock * is->recRegionSizeFrames);

	ui->spinSegmentCount->setValue(is->segments);

	ui->spinPrerecordFrames->setMaximum(camera->getMaxRecordRegionSizeFrames(&is->geometry) / 2);
	ui->spinPrerecordSeconds->setMaximum(ui->spinPrerecordFrames->maximum() * (double)is->period / sensor.timingClock);
	ui->spinPrerecordSeconds->setMinimum((double)is->period / sensor.timingClock);

	ui->spinPrerecordFrames->setValue(is->prerecordFrames);
	ui->spinPrerecordSeconds->setValue(((double)is->period / sensor.timingClock * is->prerecordFrames));

	ui->lblSegmentSize->setText("Segment size:\n" + QString::number(ui->spinRecLengthFrames->value() / ui->spinSegmentCount->value() * ((double) is->period / sensor.timingClock)) + " s\n(" + QString::number(ui->spinRecLengthFrames->value() / ui->spinSegmentCount->value()) + " frames)");

	ui->spinLoopLengthSeconds->setValue(camera->liveLoopTime);

	for (int fps = 15;  fps <= 60; fps *= 2)
	{
		ui->comboPlaybackRate->addItem(QString::number(fps));
	}

	int index = 0;
	int fps = 15;
	while (camera->playbackFps > fps)
	{
		index++;
		fps *= 2;
	}
	ui->comboPlaybackRate->setCurrentIndex(index);

    //Set QListView to change items in the combo box with qss
    ui->comboPlaybackRate->setView(new QListView);
    ui->comboPlaybackRate->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);


	windowOpening = false;
}

recModeWindow::~recModeWindow()
{
   delete ui;
}

void recModeWindow::on_cmdOK_clicked()
{
	QSettings appSettings;
	is->disableRingBuffer = (ui->radioSegmented->isChecked() && ui->chkDisableRing->isChecked());
    camera->set_runngun(ui->radioSegmented->isChecked() && ui->chkRunNGun->isChecked());

	camera->liveSlowMotion = false;

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
	else if(ui->radioLive->isChecked())
	{
		is->mode = RECORD_MODE_LIVE;
		camera->liveLoopTime = ui->spinLoopLengthSeconds->value();
		camera->liveSlowMotion = true;
		camera->liveOneShot = ui->radioOneShot->isChecked();
	}

	appSettings.setValue("recorder/liveLoopTime", camera->liveLoopTime);
	appSettings.setValue("recorder/liveLoopRecordTime", camera->liveLoopRecordTime);
	int index = ui->comboPlaybackRate->currentIndex();
	camera->playbackFps = 15 * (1 << index);
	appSettings.setValue("recorder/liveLoopPlaybackFps", camera->playbackFps);
	appSettings.setValue("recorder/liveMode", camera->liveSlowMotion);
	appSettings.setValue("recorder/liveOneShot", camera->liveOneShot);

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
    ui->chkDisableRing->setVisible(false);
    ui->chkRunNGun->setVisible(false);
}

void recModeWindow::on_radioSegmented_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
    ui->grpSegmented->setVisible(true);
    ui->lblNormal->setText("Records frames in a segmented buffer, one buffer per trigger");
    ui->chkDisableRing->setVisible(true);
    ui->chkRunNGun->setVisible(true);
}

void recModeWindow::on_radioGated_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->chkDisableRing->setVisible(false);
    ui->chkRunNGun->setVisible(false);
}

void recModeWindow::on_radioLive_clicked()
{
	ui->stackedWidget->setCurrentIndex(2);
    ui->chkDisableRing->setVisible(false);
    ui->chkRunNGun->setVisible(false);
}

void recModeWindow::on_cmdMax_clicked()
{
	UInt32 recLenFrames = camera->getMaxRecordRegionSizeFrames(&is->geometry);
    ui->spinRecLengthFrames->setValue(recLenFrames);
	ui->spinRecLengthSeconds->setValue((double)recLenFrames * (double) is->period / sensor.timingClock);
    ui->spinSegmentCount->setMaximum(min(SEGMENT_COUNT_MAX, recLenFrames));
    updateSegmentSizeText(ui->spinSegmentCount->value());
}

void recModeWindow::on_spinRecLengthSeconds_valueChanged(double arg1)
{
    if(ui->spinRecLengthSeconds->hasFocus())
    {
		UInt32 recLenFrames = arg1 / ((double) is->period / sensor.timingClock);

        if(recLenFrames < RECORD_LENGTH_MIN)
        {
            recLenFrames = RECORD_LENGTH_MIN;
			ui->spinRecLengthSeconds->setValue(recLenFrames * ((double) is->period / sensor.timingClock));

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
		ui->spinRecLengthSeconds->setValue((double)arg1 * ((double) is->period / sensor.timingClock));
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
		QString::number(ui->spinRecLengthFrames->value() / segmentCount * ((double) is->period / sensor.timingClock)) +
        " s\n(" + QString::number(ui->spinRecLengthFrames->value() / segmentCount) + " frames)");
}

void recModeWindow::on_spinPrerecordSeconds_valueChanged(double arg1)
{
    if(ui->spinPrerecordSeconds->hasFocus())
    {
		ui->spinPrerecordFrames->setValue(ui->spinPrerecordSeconds->value() / ((double) is->period / sensor.timingClock));
    }
}

void recModeWindow::on_spinPrerecordFrames_valueChanged(int arg1)
{
    if(ui->spinPrerecordFrames->hasFocus())
    {
		ui->spinPrerecordSeconds->setValue((double)ui->spinPrerecordFrames->value() * ((double) is->period / sensor.timingClock));
    }
}

//Update seconds spinbox so that it reflects the exact delay after editing is finished
void recModeWindow::on_spinPrerecordSeconds_editingFinished()
{
	ui->spinPrerecordFrames->setValue(ui->spinPrerecordSeconds->value() / ((double) is->period / sensor.timingClock));
	ui->spinPrerecordSeconds->setValue((double)ui->spinPrerecordFrames->value() * ((double) is->period / sensor.timingClock));
}

void recModeWindow::on_spinLoopLengthSeconds_valueChanged()
{
	showLoopInformation();
}

void recModeWindow::showLoopInformation()
{
	double recordTime = calcRecordTime();
	camera->liveLoopRecordTime = recordTime;
	ui->lblRecordTime->setText(QString::number((int)(recordTime * 1000)) + " ms");
	ui->lblRecordFrames->setText(QString::number(calcRecordFrames())
			+ " @ " + QString::number(1 / camera->liveFramePeriod, 'f', 0) + " fps");
	ui->lblSlowFactor->setText(QString::number(calcSlowFactor(),'f', 3));
}

double recModeWindow::calcRecordTime()
{
	UInt32 playFps = camera->playbackFps;
	double frameRate;
	//camera->cinst->getFloat("frameRate", &frameRate);
	frameRate = 1.0 / camera->liveFramePeriod;
	double recordTime = ui->spinLoopLengthSeconds->value() * playFps / frameRate;
	qDebug() << recordTime;
	return recordTime;
}

UInt32 recModeWindow::calcRecordFrames()
{
	UInt32 playFps = camera->playbackFps;
	UInt32 recordFrames = ui->spinLoopLengthSeconds->value() * playFps;
	return recordFrames;
}

double recModeWindow::calcSlowFactor()
{
	UInt32 playFps = camera->playbackFps;
	double frameRate;
	//camera->cinst->getFloat("frameRate", &frameRate);
	frameRate = 1.0 / camera->liveFramePeriod;
	double slowFactor = frameRate / playFps;
	return slowFactor;
}

void recModeWindow::on_comboPlaybackRate_currentIndexChanged(int index)
{
	if (!windowOpening)
	{
		camera->playbackFps = 15 * (1 << ui->comboPlaybackRate->currentIndex());
		showLoopInformation();
	}
}
