/****************************************************************************
 *  Copyright (C) 2013-2017 Kron Technologies Inc <http://www.krontech.ca>. *
 *                                                                          *
 *  This program is free software: you can redistribute it and/or modify    *
 *  it under the terms of the GNU General Public License as published by    *
 *  the Free Software Foundation, either version 3 of the License, or       *
 *  (at your option) any later version.                                     *
 *                                                                          *
 *  This program is distributed in the hope that it will be useful,         *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU General Public License for more details.                            *
 *                                                                          *
 *  You should have received a copy of the GNU General Public License       *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/
#include "recsettingswindow.h"
#include "ui_recsettingswindow.h"
#include "recmodewindow.h"
#include "triggerdelaywindow.h"
#include <QMessageBox>
#include <QResource>
#include <QDir>
#include "camLineEdit.h"

#include <QDebug>
#include <cstdio>
#include <QListView>

extern "C" {
#include "siText.h"
}
#include "math.h"
#include "defines.h"
#include "types.h"

#define DEF_SI_OPTS	SI_DELIM_SPACE | SI_SPACE_BEFORE_PREFIX


//Round an integer (x) to the nearest multiple of mult
template<typename T>
inline T round(T x, T mult) { T offset = (x) % (mult); return (offset >= mult/2) ? x - offset + mult : x - offset; }

RecSettingsWindow::RecSettingsWindow(QWidget *parent, Camera * cameraInst) :
	QWidget(parent),
	ui(new Ui::RecSettingsWindow)
{
	char str[100];
	UInt32 gain;

	// as non-static data member initializers can't happen in the .h, making sure it's set correct here.
	windowInitComplete = false;

	ui->setupUi(this);
	this->setWindowFlags(Qt::Dialog /*| Qt::WindowStaysOnTopHint*/ | Qt::FramelessWindowHint);
	this->move(0,0);

	is = new ImagerSettings_t;
	connect(ui->cmdCancel, SIGNAL(clicked()), this, SLOT(close()));

	camera = cameraInst;

	sensor = camera->getSensorInfo();
	camera->cinst->getImagerSettings(is);

	/* Adjust the size of the preview window to match the sensor. */
	if ((sensor.geometry.hRes * ui->frame->height()) > (sensor.geometry.vRes * ui->frame->width())) {
		previewScale = (sensor.geometry.hRes / ui->frame->width());
	} else {
		previewScale = (sensor.geometry.vRes / ui->frame->height());
	}
    ui->frame->setFixedSize(sensor.geometry.hRes / previewScale, sensor.geometry.vRes / previewScale); //sensor.geometry.vRes / previewScale

	ui->spinHRes->setSingleStep(sensor.hIncrement);
	ui->spinHRes->setMinimum(sensor.hMinimum);
	ui->spinHRes->setMaximum(sensor.geometry.hRes);

	ui->spinVRes->setSingleStep(sensor.vIncrement);
	ui->spinVRes->setMinimum(sensor.vMinimum);
	ui->spinVRes->setMaximum(sensor.geometry.vRes);

	ui->spinHOffset->setSingleStep(sensor.hIncrement);
	ui->spinHOffset->setMinimum(0);
	ui->spinHOffset->setMaximum(sensor.geometry.hRes - is->geometry.hOffset);

	ui->spinVOffset->setSingleStep(sensor.vIncrement);
	ui->spinVOffset->setMinimum(0);
	ui->spinVOffset->setMaximum(sensor.geometry.vRes - is->geometry.vRes);
	
	ui->spinHRes->setValue(is->geometry.hRes);
	ui->spinVRes->setValue(is->geometry.vRes);
	updateOffsetLimits();
	ui->spinHOffset->setValue(is->geometry.hOffset);
	ui->spinVOffset->setValue(is->geometry.vOffset);

	/* Populate the analog gain dropdown. */
	int gainIndex = 0;
	for (gain = sensor.minGain; gain <= sensor.maxGain; gain *= 2) {
		QString gainText;
		gainText.sprintf("%ddB (x%u)", int(6.0 * log2(gain)), gain);

		if (is->gain > gain) gainIndex++;
		ui->comboGain->addItem(gainText, QVariant(gain));
	}
	ui->comboGain->setCurrentIndex(gainIndex);
    //Set QListView to change items in the combo box with qss
    ui->comboGain->setView(new QListView);
    ui->comboGain->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	/* Populate the digital gain dropdown. */
	gainIndex = 0;
	for (gain = 1; gain <= 16; gain *= 2) {
		QString gainText;
		gainText.sprintf("%ddB (x%u)", int(6.0 * log2(gain)), gain);
		if (gain == round(is->digitalGain)) gainIndex = ui->comboDigGain->count();
		ui->comboDigGain->addItem(gainText, QVariant(gain));
	}
	ui->comboDigGain->setCurrentIndex(gainIndex);

    //Set QListView to change items in the combo box with qss
    ui->comboDigGain->setView(new QListView);
    ui->comboDigGain->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	//Populate the common resolution combo box from the list of resolutions
	QFile fp;
	QString filename;
	QByteArray line;
	QString lineText;
	int fps;
	FrameGeometry frameSize = sensor.geometry;
	FrameTiming timing;

	/* List the sensor's native resolution first. */
	frameSize.hOffset = frameSize.vOffset = frameSize.vDarkRows = 0;
	camera->cinst->getTiming(&frameSize, &timing);
	fps = (double)sensor.timingClock / timing.minFramePeriod;
	lineText.sprintf("%dx%d %d fps", frameSize.hRes, frameSize.vRes, fps);
	ui->comboRes->addItem(lineText);

    //Set QListView to change items in the combo box with qss
    ui->comboRes->setView(new QListView);
    ui->comboRes->setMaxVisibleItems(6);
    ui->comboRes->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	filename.append("camApp:resolutions");
	QFileInfo resolutionsFile(filename);
	if (resolutionsFile.exists() && resolutionsFile.isFile()) {
		fp.setFileName(filename);
		fp.open(QIODevice::ReadOnly);
		if(!fp.isOpen()) {
			qDebug("Error: resolutions file couldn't be opened");
		}
	}
	else {
		qDebug("Error: resolutions file isn't present");
	}

	while(true) {
		line = fp.readLine(30);

		if (line.isEmpty() || line.isNull())
			break;
		
		//Get the resolution and compute the maximum frame rate to be appended after the resolution
		sscanf(line.constData(), "%dx%d", &frameSize.hRes, &frameSize.vRes);
		frameSize.hOffset = frameSize.vOffset = frameSize.vDarkRows = 0;
		if (camera->cinst->getTiming(&frameSize, &timing) == SUCCESS) {
			int fps = sensor.timingClock / timing.minFramePeriod;
			qDebug("Common Res: %dx%d mPeriod=%d fps=%d", frameSize.hRes, frameSize.vRes, timing.minFramePeriod, fps);

			lineText.sprintf("%dx%d %d fps", frameSize.hRes, frameSize.vRes, fps);
			if (ui->comboRes->findText(lineText) < 0) {
				ui->comboRes->addItem(lineText);
				if ((frameSize.hRes == is->geometry.hRes) && (frameSize.vRes == is->geometry.vRes)) {
					ui->comboRes->setCurrentIndex(ui->comboRes->count() - 1);
				}
			}
		}
	}
	
	fp.close();
	

	//If the current image position is in the center, check the centered checkbox
	if(	is->geometry.hOffset == round((sensor.geometry.hRes - is->geometry.hRes) / 2, sensor.hIncrement) &&
		is->geometry.vOffset == round((sensor.geometry.vRes - is->geometry.vRes) / 2, sensor.vIncrement))
	{
		ui->chkCenter->setChecked(true);
		ui->spinHOffset->setEnabled(false);
		ui->spinVOffset->setEnabled(false);
	}
	else
	{
		ui->chkCenter->setChecked(false);
		ui->spinHOffset->setEnabled(true);
		ui->spinVOffset->setEnabled(true);
	}

	/* Get the current exposure and frame period */
	UInt32 fPeriod;
	UInt32 ePeriod;
	camera->cinst->getInt("framePeriod", &fPeriod);
	camera->cinst->getInt("exposurePeriod", &ePeriod);

	//Set the frame period
	getSIText(str, (double)fPeriod / sensor.timingClock, 10, DEF_SI_OPTS, 8);
	ui->linePeriod->setText(str);
	ui->linePeriod->setHasUnits(true);

	//Set the frame rate
	getSIText(str, (double)sensor.timingClock / fPeriod, 10, DEF_SI_OPTS, 1000);
	ui->lineRate->setText(str);
	ui->lineRate->setHasUnits(true);

	//Set the exposure
	getSIText(str, (double)ePeriod / sensor.timingClock, 10, DEF_SI_OPTS, 8);
	ui->lineExp->setText(str);
	ui->lineExp->setHasUnits(true);

	updateFramePreview();
	updateInfoText();

    qDebug() << "---- Rec Settings Window ---- Init complete";
    windowInitComplete = true;  //This is used to avoid control on_change events firing with incomplete values populated
}

RecSettingsWindow::~RecSettingsWindow()
{
	delete is;
	delete ui;
}

FrameGeometry RecSettingsWindow::getResolution(void)
{
	FrameGeometry size;
	size.hRes = ui->spinHRes->value();
	size.vRes = ui->spinVRes->value();
	size.hOffset = ui->spinHOffset->value();
	size.vOffset = ui->spinVOffset->value();
	size.vDarkRows = 0;
	size.bitDepth = BITS_PER_PIXEL;
	size.minFrameTime = 0;
	return size;
}

void RecSettingsWindow::on_cmdOK_clicked()
{
	FrameTiming timing;
	bool videoFlip = false;
	int gainIndex;

	is->period = ui->linePeriod->siText() * sensor.timingClock;
	is->exposure = ui->lineExp->siText() * sensor.timingClock;
	is->gain = sensor.minGain;
	is->digitalGain = 1.0;
	is->geometry = getResolution();
	is->geometry.minFrameTime = (double)is->period / sensor.timingClock;
	gainIndex = ui->comboGain->currentIndex();
	if (gainIndex >= 0) {
		is->gain = ui->comboGain->itemData(gainIndex).toInt();
	}

	gainIndex = ui->comboDigGain->currentIndex();
	if (gainIndex >= 0) {
		is->digitalGain = ui->comboDigGain->itemData(gainIndex).toDouble();
	}
	/* Sanity check the requested frame and exposure timing */
	if (camera->cinst->getTiming(&is->geometry, &timing) == SUCCESS) {
		is->exposure = within(is->exposure, timing.exposureMin, timing.exposureMax);
		is->period = max(is->period, timing.minFramePeriod);
	}
	else {
		/* The API reported that this timing is not possible... bad times await. */
		qDebug("Invalid resolution and timing: %dx%d offset %dx%d",
			   is->geometry.hRes, is->geometry.vRes,
			   is->geometry.hOffset, is->geometry.vOffset);
	}

	camera->setImagerSettings(*is);
	camera->vinst->liveDisplay(videoFlip);

	/* If calibration is now suggested, we should start zero-time black cal. */
	if (camera->cinst->getProperty("calSuggested", false).toBool()) {
		camera->cinst->asyncCalibration({"analogCal", "zeroTimeBlackCal"}, false);
	}

	emit settingsChanged();
	
	close();
}

void RecSettingsWindow::on_spinHRes_valueChanged(int arg1)
{
    if(windowInitComplete)
    {
		FrameGeometry fSize;

		updateOffsetLimits();
		updateInfoText();
		updateFramePreview();

		fSize = getResolution();
		is->recRegionSizeFrames = camera->getMaxRecordRegionSizeFrames(&fSize);
		qDebug() << "---- Rec Settings Window ---- hres =" << fSize.hRes << "vres =" << fSize.vRes << "recRegionSizeFrames =" << is->recRegionSizeFrames;
    }
}

void RecSettingsWindow::on_spinHRes_editingFinished()
{
	ui->spinHRes->setValue(max(sensor.hMinimum, round((UInt32)ui->spinHRes->value(), sensor.hIncrement)));

	updateFramePreview();
	updateInfoText();
}

void RecSettingsWindow::on_spinVRes_valueChanged(int arg1)
{
    if(windowInitComplete)
    {
		FrameGeometry fSize;
		updateOffsetLimits();
		updateInfoText();
		updateFramePreview();

		fSize = getResolution();
		is->recRegionSizeFrames = camera->getMaxRecordRegionSizeFrames(&fSize);
		qDebug() << "---- Rec Settings Window ---- hres =" << fSize.hRes << "vres =" << fSize.vRes << "recRegionSizeFrames =" << is->recRegionSizeFrames;
    }
}

void RecSettingsWindow::on_spinVRes_editingFinished()
{
	ui->spinVRes->setValue(max(sensor.vMinimum, round((UInt32)ui->spinVRes->value(), sensor.vIncrement)));

	updateFramePreview();
	updateInfoText();
}

void RecSettingsWindow::on_spinHOffset_valueChanged(int arg1)
{
	updateFramePreview();
	updateInfoText();
}

void RecSettingsWindow::on_spinHOffset_editingFinished()
{
	ui->spinHOffset->setValue(round((UInt32)ui->spinHOffset->value(), sensor.hIncrement));

	updateFramePreview();
	updateInfoText();
}

void RecSettingsWindow::on_spinVOffset_valueChanged(int arg1)
{
	updateFramePreview();
	updateInfoText();
}

void RecSettingsWindow::on_spinVOffset_editingFinished()
{
	ui->spinVOffset->setValue(round((UInt32)ui->spinVOffset->value(), sensor.vIncrement));

	updateFramePreview();
	updateInfoText();
}

void RecSettingsWindow::updateOffsetLimits()
{
	ui->spinHOffset->setMaximum(sensor.geometry.hRes - ui->spinHRes->value());
	ui->spinVOffset->setMaximum(sensor.geometry.vRes - ui->spinVRes->value());

	if(ui->chkCenter->checkState())
	{
		ui->spinHOffset->setValue(round((sensor.geometry.hRes - ui->spinHRes->value()) / 2, sensor.hIncrement));
		ui->spinVOffset->setValue(round((sensor.geometry.vRes - ui->spinVRes->value()) / 2, sensor.vIncrement));
	}
}

void RecSettingsWindow::on_chkCenter_toggled(bool checked)
{
	if(checked)
	{
		ui->spinHOffset->setValue(round((sensor.geometry.hRes - ui->spinHRes->value()) / 2, sensor.hIncrement));
		ui->spinVOffset->setValue(round((sensor.geometry.vRes - ui->spinVRes->value()) / 2, sensor.vIncrement));
	}

	ui->spinHOffset->setEnabled(!checked);
	ui->spinVOffset->setEnabled(!checked);
}

void RecSettingsWindow::clampExposure(FrameTiming *timing, UInt32 fPeriod)
{
	UInt32 expPeriod = ui->lineExp->siText() * sensor.timingClock;
	char str[100];

	if (expPeriod > fPeriod) expPeriod = fPeriod;
	if (expPeriod > timing->exposureMax) expPeriod = timing->exposureMax;
	if (expPeriod < timing->exposureMin) expPeriod = timing->exposureMin;
	getSIText(str, (double)expPeriod / sensor.timingClock, 10, DEF_SI_OPTS, 8);
	ui->lineExp->setText(str);
}
/*
void CamMainWindow::on_exposurePeriod_valueChanged(const QVariant &value)
{
    apiUpdate = true;
    ui->expSlider->setValue(value.toInt());
    apiUpdate = false;
    updateCurrentSettingsLabel();
}
*/

void RecSettingsWindow::on_cmdMax_clicked()
{
	FrameGeometry fSize = getResolution();
	FrameTiming timing;
	char str[100];

	camera->cinst->getTiming(&fSize, &timing);
	getSIText(str, (double)timing.minFramePeriod / sensor.timingClock, 10, DEF_SI_OPTS, 8);
	ui->linePeriod->setText(str);

	getSIText(str, (double)sensor.timingClock / timing.minFramePeriod, ceil(log10(timing.minFramePeriod)+1), DEF_SI_OPTS, 1000);
	ui->lineRate->setText(str);

	//Make sure exposure is within limits
	clampExposure(&timing, timing.minFramePeriod);
}

void RecSettingsWindow::on_linePeriod_returnPressed()
{
	FrameGeometry fSize = getResolution();
	FrameTiming timing;
	char str[100];
	double frameTime = ui->linePeriod->siText();
	UInt32 framePeriod;

	/* Sanitize the frame time. */
	fSize.minFrameTime = frameTime;
	camera->cinst->getTiming(&fSize, &timing);
	framePeriod = max(frameTime * sensor.timingClock, timing.minFramePeriod);

	//format the entered value nicely
	getSIText(str, (double)framePeriod / sensor.timingClock, 10, DEF_SI_OPTS, 8);
	ui->linePeriod->setText(str);

	//Set the frame rate
	getSIText(str, (double)sensor.timingClock / framePeriod, ceil(log10(framePeriod)+1), DEF_SI_OPTS, 1000);
	ui->lineRate->setText(str);

	//Make sure exposure is within limits
	clampExposure(&timing, framePeriod);
}

void RecSettingsWindow::on_lineRate_returnPressed()
{
	FrameGeometry fSize = getResolution();
	FrameTiming timing;
	double frameRate = ui->lineRate->siText();
	UInt32 framePeriod;
	char str[100];

	if(0.0 == frameRate)
		return;

	/* Sanitize the frame time. */
	fSize.minFrameTime = 1.0 / frameRate;
	camera->cinst->getTiming(&fSize, &timing);
	framePeriod = max(sensor.timingClock / frameRate, timing.minFramePeriod);

	//Set the frame period box
	getSIText(str, (double)framePeriod / sensor.timingClock, 10, DEF_SI_OPTS, 8);
	ui->linePeriod->setText(str);

	//Refill the frame rate box with the nicely formatted value
	getSIText(str, (double)sensor.timingClock / framePeriod, ceil(log10(framePeriod)+1), DEF_SI_OPTS, 1000);
	ui->lineRate->setText(str);

	//Make sure exposure is within limits
	clampExposure(&timing, framePeriod);
}

void RecSettingsWindow::on_lineExp_returnPressed()
{
	FrameGeometry frameSize = getResolution();
	FrameTiming timing;

	frameSize.minFrameTime = ui->linePeriod->siText();
	camera->cinst->getTiming(&frameSize, &timing);
	clampExposure(&timing, ui->linePeriod->siText() * sensor.timingClock);
}

void RecSettingsWindow::on_cmdExpMax_clicked()
{
	FrameGeometry frameSize = getResolution();
	FrameTiming timing;
	UInt32 fPeriod = ui->linePeriod->siText() * sensor.timingClock;
	UInt32 expPeriod = fPeriod;
	char str[100];

	frameSize.minFrameTime = ui->linePeriod->siText();
	camera->cinst->getTiming(&frameSize, &timing);

	if (expPeriod > timing.exposureMax) expPeriod = timing.exposureMax;
	if (expPeriod < timing.exposureMin) expPeriod = timing.exposureMin;
	getSIText(str, (double)expPeriod / sensor.timingClock, 10, DEF_SI_OPTS, 8);
	ui->lineExp->setText(str);
}

void RecSettingsWindow::updateFramePreview()
{
	FrameGeometry fSize = getResolution();
	ui->frameImage->setGeometry(QRect(fSize.hOffset/previewScale, fSize.vOffset/previewScale, fSize.hRes/previewScale, fSize.vRes/previewScale));
	ui->frame->repaint();
}

void RecSettingsWindow::updateInfoText()
{
	FrameGeometry frameSize = getResolution();
	FrameTiming timing;
	char str[300];
	char maxRateStr[30];
	char minPeriodStr[30];
	char maxExposureStr[30];

	camera->cinst->getTiming(&frameSize, &timing);

	getSIText(minPeriodStr, (double)timing.minFramePeriod / sensor.timingClock, 10, DEF_SI_OPTS, 8);
	getSIText(maxRateStr, (double)sensor.timingClock / timing.minFramePeriod, ceil(log10(timing.minFramePeriod)+1), DEF_SI_OPTS, 1000);
	getSIText(maxExposureStr, (double)timing.exposureMax / sensor.timingClock, 10, DEF_SI_OPTS, 8);

	sprintf(str, "Max rate for this resolution:\r\n%sfps\r\nMin Period: %ss", maxRateStr, minPeriodStr);
	ui->lblInfo->setText(str);
}

void RecSettingsWindow::setResFromText(char * str)
{
	SensorInfo_t si = camera->getSensorInfo();
	FrameGeometry frameSize;

	sscanf(str, "%dx%d", &frameSize.hRes, &frameSize.vRes);
	frameSize.hOffset = round((si.geometry.hRes - frameSize.hRes) / 2, si.hIncrement);
	frameSize.vOffset = round((si.geometry.vRes - frameSize.vRes) / 2, si.vIncrement);
	frameSize.vDarkRows = 0;
	frameSize.bitDepth = BITS_PER_PIXEL;

	if(camera->isValidResolution(&frameSize)) {
		ui->spinHRes->setValue(frameSize.hRes);
		ui->spinVRes->setValue(frameSize.vRes);
		on_cmdMax_clicked();
		on_cmdExpMax_clicked();
	}
}

void RecSettingsWindow::closeEvent(QCloseEvent *event)
{
	QEvent ev(QEvent::CloseSoftwareInputPanel);
	QApplication::sendEvent((QObject *)qApp->inputContext(), &ev);
	event->accept();
}


void RecSettingsWindow::on_comboRes_activated(const QString &arg1)
{
	QByteArray ba = arg1.toLatin1();

	setResFromText(ba.data());
}

void RecSettingsWindow::on_cmdRecMode_clicked()
{
	is->geometry = getResolution();
	is->gain = ui->comboGain->currentIndex();
	is->period = ui->linePeriod->siText() * sensor.timingClock;
	is->exposure = ui->lineExp->siText() * sensor.timingClock;
	camera->liveFramePeriod = is->period / 1e9;		//this is needed for calculations that have not yet been sent to the API

	recModeWindow *w = new recModeWindow(NULL, camera, is);
	w->setAttribute(Qt::WA_DeleteOnClose);
	w->show();
}

void RecSettingsWindow::on_cmdDelaySettings_clicked()
{
	is->geometry = getResolution();
	is->gain = ui->comboGain->currentIndex();
	is->period = ui->linePeriod->siText() * sensor.timingClock;
	is->exposure = ui->lineExp->siText() * sensor.timingClock;

	if(is->mode == RECORD_MODE_GATED_BURST)
	{
		QMessageBox msg;
		msg.setText("Record mode is set to Gated Burst. This mode has no adjustable trigger settings.");
		msg.exec();
		return;
    }
    else
	{
		triggerDelayWindow *w = new triggerDelayWindow(NULL, camera, is);
		w->setAttribute(Qt::WA_DeleteOnClose);
		w->show();
	}
}
