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

extern "C" {
#include "siText.h"
}
#include "math.h"
#include "defines.h"
#include "types.h"

#define DEF_SI_OPTS	SI_DELIM_SPACE | SI_SPACE_BEFORE_PREFIX

extern bool pych;

//Round an integer (x) to the nearest multiple of mult
template<typename T>
inline T round(T x, T mult) { T offset = (x) % (mult); return (offset >= mult/2) ? x - offset + mult : x - offset; }

RecSettingsWindow::RecSettingsWindow(QWidget *parent, Camera * cameraInst) :
	QWidget(parent),
	ui(new Ui::RecSettingsWindow)
{
	char str[100];

	// as non-static data member initializers can't happen in the .h, making sure it's set correct here.
	windowInitComplete = false;

	ui->setupUi(this);
	this->setWindowFlags(Qt::Dialog /*| Qt::WindowStaysOnTopHint*/ | Qt::FramelessWindowHint);
	this->move(0,0);

	is = new ImagerSettings_t;
	connect(ui->cmdCancel, SIGNAL(clicked()), this, SLOT(close()));

	camera = cameraInst;
	FrameGeometry maxSize = camera->sensor->getMaxGeometry();
	ImagerSettings_t isTemp = camera->getImagerSettings();          //Using new and memcpy because passing the address of a class variable was causing segfaults among others in the trigger delay window.
	memcpy((void *)is, (void *)(&isTemp), sizeof(ImagerSettings_t));

	ui->spinHRes->setSingleStep(camera->sensor->getHResIncrement());
	ui->spinHRes->setMinimum(camera->sensor->getMinHRes());
	ui->spinHRes->setMaximum(maxSize.hRes);

	ui->spinVRes->setSingleStep(camera->sensor->getVResIncrement());
	ui->spinVRes->setMinimum(camera->sensor->getMinVRes());
	ui->spinVRes->setMaximum(maxSize.vRes);

	ui->spinHOffset->setSingleStep(camera->sensor->getHResIncrement());
	ui->spinHOffset->setMinimum(0);
	ui->spinHOffset->setMaximum(maxSize.hRes - is->geometry.hOffset);

	ui->spinVOffset->setSingleStep(camera->sensor->getVResIncrement());
	ui->spinVOffset->setMinimum(0);
	ui->spinVOffset->setMaximum(maxSize.vRes - is->geometry.vRes);
	
	ui->spinHRes->setValue(is->geometry.hRes);
	ui->spinVRes->setValue(is->geometry.vRes);
	updateOffsetLimits();
	ui->spinHOffset->setValue(is->geometry.hOffset);
	ui->spinVOffset->setValue(is->geometry.vOffset);

	ui->comboGain->addItem("0dB (x1)");
	ui->comboGain->addItem("6dB (x2)");
	ui->comboGain->addItem("12dB (x4)");
	ui->comboGain->addItem("18dB (x8)");
	ui->comboGain->addItem("24dB (x16)");
	ui->comboGain->setCurrentIndex(is->gain);

	//Populate the common resolution combo box from the list of resolutions
	QFile fp;
	QString filename;
	QByteArray line;
	QString lineText;

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
		FrameGeometry fSize;

		if (line.isEmpty() || line.isNull())
			break;
		
		//Get the resolution and compute the maximum frame rate to be appended after the resolution
		sscanf(line.constData(), "%dx%d", &fSize.hRes, &fSize.vRes);
		fSize.bitDepth = BITS_PER_PIXEL;
		fSize.hOffset = fSize.vOffset = fSize.vDarkRows = 0;
		
		int fr =  100000000.0 / (double)camera->sensor->getMinFramePeriod(&fSize);
		qDebug() << "hres" << fSize.hRes << "vRes" << fSize.vRes << "mperiod" << camera->sensor->getMinFramePeriod(&fSize) << "fr" << fr;

		lineText.sprintf("%dx%d %d fps", fSize.hRes, fSize.vRes, fr);
		
		ui->comboRes->addItem(lineText);

		if ((fSize.hRes == is->geometry.hRes) && (fSize.vRes == is->geometry.vRes)) {
			ui->comboRes->setCurrentIndex(ui->comboRes->count() - 1);
		}
	}
	
	fp.close();
	

	//If the current image position is in the center, check the centered checkbox
	if(	is->geometry.hOffset == round((maxSize.hRes - is->geometry.hRes) / 2, camera->sensor->getHResIncrement()) &&
		is->geometry.vOffset == round((maxSize.vRes - is->geometry.vRes) / 2, camera->sensor->getVResIncrement()))
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


	//Set the frame period
    double framePeriod = (double)is->period / 100000000.0;
	getSIText(str, framePeriod, 10, DEF_SI_OPTS, 8);
	ui->linePeriod->setText(str);
	ui->linePeriod->setHasUnits(true);

	//Set the frame rate
	double frameRate = 1.0 / framePeriod;
	getSIText(str, frameRate, ceil(log10(framePeriod*100000000.0)+1), DEF_SI_OPTS, 1000);
	ui->lineRate->setText(str);
	ui->lineRate->setHasUnits(true);

	//Set the exposure
	double exposure = camera->sensor->getCurrentExposureDouble();
	getSIText(str, exposure, 10, DEF_SI_OPTS, 8);
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
	return size;
}

void RecSettingsWindow::on_cmdOK_clicked()
{
	is->gain = ui->comboGain->currentIndex();
	is->geometry = getResolution();

	UInt32 period = camera->sensor->getActualFramePeriod(ui->linePeriod->siText(), &is->geometry);
	UInt32 intTime = camera->sensor->getActualIntegrationTime(ui->lineExp->siText(), period, &is->geometry);

	is->period = period;
	is->exposure = intTime;
	is->temporary = 0;

	if (pych)
	{
		//add
		FrameGeometry *geo = &is->geometry;
		camera->cinst->setResolution(geo);
	}
	else
	{
		camera->updateTriggerValues(*is);
	}

	camera->setImagerSettings(*is);
	camera->vinst->liveDisplay(is->geometry.hRes, is->geometry.vRes);
	camera->liveColumnCalibration();

	if(CAMERA_FILE_NOT_FOUND == camera->loadFPNFromFile()) {
		if (pych)
		{
			// add this
		}
		else
		{
			camera->fastFPNCorrection();
		}
	}

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
	ui->spinHRes->setValue(round((UInt32)ui->spinHRes->value(), camera->sensor->getHResIncrement()));

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
	ui->spinVRes->setValue(round((UInt32)ui->spinVRes->value(), camera->sensor->getVResIncrement()));

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
	ui->spinHOffset->setValue(round((UInt32)ui->spinHOffset->value(), camera->sensor->getHResIncrement()));

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
	ui->spinVOffset->setValue(round((UInt32)ui->spinVOffset->value(), camera->sensor->getVResIncrement()));

	updateFramePreview();
	updateInfoText();
}

void RecSettingsWindow::updateOffsetLimits()
{
	FrameGeometry maxSize = camera->sensor->getMaxGeometry();
	ui->spinHOffset->setMaximum(maxSize.hRes - ui->spinHRes->value());
	ui->spinVOffset->setMaximum(maxSize.vRes - ui->spinVRes->value());

	if(ui->chkCenter->checkState())
	{
		ui->spinHOffset->setValue(round((maxSize.hRes - ui->spinHRes->value()) / 2, camera->sensor->getHResIncrement()));
		ui->spinVOffset->setValue(round((maxSize.vRes - ui->spinVRes->value()) / 2, camera->sensor->getVResIncrement()));
	}
}

void RecSettingsWindow::on_chkCenter_toggled(bool checked)
{
	if(checked)
	{
		FrameGeometry maxSize = camera->sensor->getMaxGeometry();
		ui->spinHOffset->setValue(round((maxSize.hRes - ui->spinHRes->value()) / 2, camera->sensor->getHResIncrement()));
		ui->spinVOffset->setValue(round((maxSize.vRes - ui->spinVRes->value()) / 2, camera->sensor->getVResIncrement()));
	}

	ui->spinHOffset->setEnabled(!checked);
	ui->spinVOffset->setEnabled(!checked);
}


void RecSettingsWindow::on_cmdMax_clicked()
{
	FrameGeometry fSize = getResolution();
	UInt32 period = camera->sensor->getMinFramePeriod(&fSize);
	char str[100];

	getSIText(str, (double)period / camera->sensor->getFramePeriodClock(), 10, DEF_SI_OPTS, 8);
	ui->linePeriod->setText(str);

	double frameRate = (double)camera->sensor->getFramePeriodClock() / period;
	getSIText(str, frameRate, ceil(log10(period)+1), DEF_SI_OPTS, 1000);

	ui->lineRate->setText(str);

	//Make sure exposure is within limits
	UInt32 intTime = camera->sensor->getActualIntegrationTime(ui->lineExp->siText(), period, &fSize);

	//Format the entered value nicely
	getSIText(str, (double)intTime / camera->sensor->getIntegrationClock(), 10, DEF_SI_OPTS, 8);
	ui->lineExp->setText(str);

}

void RecSettingsWindow::on_linePeriod_returnPressed()
{
	char str[100];
	FrameGeometry fSize = getResolution();
	UInt32 period = camera->sensor->getActualFramePeriod(ui->linePeriod->siText(), &fSize);

	//format the entered value nicely
	getSIText(str, (double)period / camera->sensor->getFramePeriodClock(), 10, DEF_SI_OPTS, 8);
	ui->linePeriod->setText(str);

	//Set the frame rate
	double frameRate = (double)camera->sensor->getFramePeriodClock() / period;
	getSIText(str, frameRate, ceil(log10(period)+1), DEF_SI_OPTS, 1000);

	ui->lineRate->setText(str);

	//Make sure exposure is within limits
	UInt32 intTime = camera->sensor->getActualIntegrationTime(ui->lineExp->siText(), period, &fSize);

	//Format the entered value nicely
	getSIText(str, (double)intTime / camera->sensor->getIntegrationClock(), 10, DEF_SI_OPTS, 8);
	ui->lineExp->setText(str);
}

void RecSettingsWindow::on_lineRate_returnPressed()
{
	FrameGeometry frameSize = getResolution();
	UInt32 period;
	double frameRate = ui->lineRate->siText();
	char str[100];

	if(0.0 == frameRate)
		return;

	period = camera->sensor->getActualFramePeriod(1.0 / frameRate, &frameSize);

	//Set the frame period box
	getSIText(str, (double)period / camera->sensor->getFramePeriodClock(), 10, DEF_SI_OPTS, 8);
	ui->linePeriod->setText(str);

	//Refill the frame rate box with the nicely formatted value
	frameRate = (double)camera->sensor->getFramePeriodClock() / period;
	getSIText(str, frameRate, ceil(log10(period)+1), DEF_SI_OPTS, 1000);

	ui->lineRate->setText(str);

	//Make sure exposure is within limits
	UInt32 intTime = camera->sensor->getActualIntegrationTime(ui->lineExp->siText(), period, &frameSize);

	//Format the entered value nicely
	getSIText(str, (double)intTime / camera->sensor->getIntegrationClock(), 10, DEF_SI_OPTS, 8);
	ui->lineExp->setText(str);
}

void RecSettingsWindow::on_lineExp_returnPressed()
{
	FrameGeometry frameSize = getResolution();
	UInt32 period = camera->sensor->getActualFramePeriod(ui->linePeriod->siText(), &frameSize);
	UInt32 intTime = camera->sensor->getActualIntegrationTime(ui->lineExp->siText(), period, &frameSize);
	char str[100];

	//Format the entered value nicely
	getSIText(str, (double)intTime / camera->sensor->getIntegrationClock(), 10, DEF_SI_OPTS, 8);
	ui->lineExp->setText(str);
}

void RecSettingsWindow::on_cmdExpMax_clicked()
{
	FrameGeometry frameSize = getResolution();
	UInt32 period = camera->sensor->getActualFramePeriod(ui->linePeriod->siText(), &frameSize);
	UInt32 intTime = camera->sensor->getMaxIntegrationTime(period, &frameSize);
	double exp = (double)intTime / camera->sensor->getIntegrationClock();
	char str[100];

	//Format the entered value nicely
	getSIText(str, exp, 10, DEF_SI_OPTS, 8);
	qDebug() << exp;
	ui->lineExp->setText(str);
}

void RecSettingsWindow::updateInfoText()
{
	FrameGeometry frameSize = getResolution();
	char str[300];
	char maxRateStr[30];
	char minPeriodStr[30];
	char maxExposureStr[30];

	UInt32 fclocks = camera->sensor->getMinFramePeriod(&frameSize);
	double fperiod = (double)fclocks / camera->sensor->getFramePeriodClock();
	UInt32 expclocks = camera->sensor->getMaxIntegrationTime(fclocks, &frameSize);

	getSIText(minPeriodStr, fperiod, 10, DEF_SI_OPTS, 8);
	getSIText(maxRateStr, 1.0/fperiod, ceil(log10(fclocks)+1), DEF_SI_OPTS, 1000);
	getSIText(maxExposureStr, (double)expclocks / camera->sensor->getIntegrationClock(), 10, DEF_SI_OPTS, 8);

	sprintf(str, "Max rate for this resolution:\r\n%sfps\r\nMin Period: %ss", maxRateStr, minPeriodStr);
	ui->lblInfo->setText(str);
}

void RecSettingsWindow::updateFramePreview()
{
	FrameGeometry fSize = getResolution();
	ui->frameImage->setGeometry(QRect(fSize.hOffset/4, fSize.vOffset/4, fSize.hRes/4, fSize.vRes/4));
}

void RecSettingsWindow::setResFromText(char * str)
{
	FrameGeometry maxSize = camera->sensor->getMaxGeometry();
	FrameGeometry frameSize;

	sscanf(str, "%dx%d", &frameSize.hRes, &frameSize.vRes);
	frameSize.hOffset = round((maxSize.hRes - frameSize.hRes) / 2, camera->sensor->getHResIncrement());
	frameSize.vOffset = round((maxSize.vRes - frameSize.vRes) / 2, camera->sensor->getVResIncrement());
	frameSize.vDarkRows = 0;
	frameSize.bitDepth = BITS_PER_PIXEL;

	if(camera->sensor->isValidResolution(&frameSize)) {
		ui->spinHRes->setValue(frameSize.hRes);
		ui->spinVRes->setValue(frameSize.vRes);
		on_cmdMax_clicked();
		on_cmdExpMax_clicked();
	}
}

void RecSettingsWindow::closeEvent(QCloseEvent *event)
{
	emit settingsChanged();
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

	UInt32 period = camera->sensor->getActualFramePeriod(ui->linePeriod->siText(), &is->geometry);
	UInt32 intTime = camera->sensor->getActualIntegrationTime(ui->lineExp->siText(), period, &is->geometry);

	is->period = period;
	is->exposure = intTime;
    is->temporary = 0;

    recModeWindow *w = new recModeWindow(NULL, camera, is);
    //w->camera = camera;
    w->setAttribute(Qt::WA_DeleteOnClose);
    w->show();
}

void RecSettingsWindow::on_cmdDelaySettings_clicked()
{
    if(is->mode == RECORD_MODE_GATED_BURST)
    {
        QMessageBox msg;
        msg.setText("Record mode is set to Gated Burst. This mode has no adjustable trigger settings.");
        msg.exec();
        return;
    }
    else
    {
		triggerDelayWindow *w = new triggerDelayWindow(NULL, camera, is, ui->linePeriod->siText());
        //w->camera = camera;
        w->setAttribute(Qt::WA_DeleteOnClose);
        w->show();
    }
}
