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
#include "frameimage.h"

#include <QDebug>
#include <cstdio>

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
	windowInitComplete = false;
	
	char str[100];

	ui->setupUi(this);
	this->setWindowFlags(Qt::Dialog /*| Qt::WindowStaysOnTopHint*/ | Qt::FramelessWindowHint);
	this->move(0,0);

    is = new ImagerSettings_t;
        //[gui2] connect(ui->cmdCancel, SIGNAL(clicked()), this, SLOT(close()));

	camera = cameraInst;

    ImagerSettings_t isTemp = camera->getImagerSettings();          //Using new and memcpy because passing the address of a class variable was causing segfaults among others in the trigger delay window.
    memcpy((void *)is, (void *)(&isTemp), sizeof(ImagerSettings_t));

	ui->spinHRes->setSingleStep(camera->sensor->getHResIncrement());
	ui->spinHRes->setMinimum(camera->sensor->getMinHRes());
	ui->spinHRes->setMaximum(camera->sensor->getMaxHStride());

	ui->spinVRes->setSingleStep(camera->sensor->getVResIncrement());
	ui->spinVRes->setMinimum(camera->sensor->getMinVRes());
	ui->spinVRes->setMaximum(camera->sensor->getMaxVRes());

	ui->spinHOffset->setSingleStep(camera->sensor->getHResIncrement());
	ui->spinHOffset->setMinimum(0);
    ui->spinHOffset->setMaximum(camera->sensor->getMaxHStride() - is->hOffset);

	ui->spinVOffset->setSingleStep(camera->sensor->getVResIncrement());
	ui->spinVOffset->setMinimum(0);
    ui->spinVOffset->setMaximum(camera->sensor->getMaxVRes() - is->vRes);

    ui->spinHRes->setValue(is->stride);
    ui->spinVRes->setValue(is->vRes);
    ui->spinHOffset->setValue(is->hOffset);
    ui->spinVOffset->setValue(is->vOffset);

    ui->comboGain->clear(); //Remove the placeholder from the .ui file.
	ui->comboGain->addItem("0dB (x1)");
	ui->comboGain->addItem("6dB (x2)");
	ui->comboGain->addItem("12dB (x4)");
	ui->comboGain->addItem("18dB (x8)");
	ui->comboGain->addItem("24dB (x16)");
    ui->comboGain->setCurrentIndex(is->gain);
    
    
    //Set the preview video mode to the full sensor, displayed in the little preview box.
    is->temporary = true;
    is->hRes = MAX_FRAME_SIZE_H;
    is->vRes = MAX_FRAME_SIZE_V;
    is->hOffset = 0;
    is->vOffset = 0;
    is->stride = is->hRes;
    is->gain = ui->comboGain->currentIndex();
    //is->period = camera->sensor->getActualFramePeriod(
    //    siText2Double("945.75 u"), is->hRes, is->vRes
    //) * 100000000.0;
    //is->exposure = camera->sensor->getActualIntegrationTime(
	//	siText2Double("940.75 u"), is->period, is->hRes, is->vRes
	//) * 100000000.0;
	is->displayArea = QRect(ui->frame->pos(), ui->frame->size());
	camera->setImagerSettings(*is);
    camera->setDisplaySettings(false, MAX_LIVE_FRAMERATE);
    
    if(CAMERA_FILE_NOT_FOUND == camera->loadFPNFromFile())
		camera->autoFPNCorrection(2, false, true);

	
	//Populate the common resolution combo box from the list of resolutions
	QFile fp;
	QString filename;
	QByteArray line;

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

	ui->comboRes->clear();
	while(true) {
		line = fp.readLine(30);
		if (line.isEmpty() || line.isNull())
			break;
		
		//Get the resolution and compute the maximum frame rate to be appended after the resolution
		int hRes, vRes;
		
		sscanf(line.constData(), "%dx%d", &hRes, &vRes);
		
		int fr =  100000000.0 / (double)camera->sensor->getMinFramePeriod(hRes, vRes);
		qDebug() << "hres" << hRes << "vRes" << vRes << "mperiod" << camera->sensor->getMinFramePeriod(hRes, vRes) << "fr" << fr;

		ui->comboRes->addItem(QString::fromUtf8("%1×%2 @ %3fps").arg(hRes).arg(vRes).arg(fr));
	}
	
	fp.close();
	

	//If the current image position is in the center, check the centered checkbox
    if(	is->hOffset == round((camera->sensor->getMaxHRes() - is->stride) / 2, camera->sensor->getHResIncrement()) &&
        is->vOffset == round((camera->sensor->getMaxVRes() - is->vRes) / 2, camera->sensor->getVResIncrement()))
	{
                //[gui2] ui->chkCenter->setChecked(true);
		ui->spinHOffset->setEnabled(false);
		ui->spinVOffset->setEnabled(false);
	}
	else
	{
                //[gui2] ui->chkCenter->setChecked(false);
		ui->spinHOffset->setEnabled(true);
		ui->spinVOffset->setEnabled(true);
	}


	//Set the frame period
    double framePeriod = (double)is->period / 100000000.0;
	getSIText(str, framePeriod, 10, DEF_SI_OPTS, 8);
        ui->linePeriod->setText(str);

	//Set the frame rate
	double frameRate = 1.0 / framePeriod;
	ui->lineRate->setText(QString::number(frameRate));

	//Set the exposure
    //double exposure = (double)is->exposure / 100000000.0;
    double exposure = (camera->sensor->getIntegrationTime());
	getSIText(str, exposure, 10, DEF_SI_OPTS, 8);
        ui->lineExp->setText(str);

    updateInfoText();
	updatePreviewBox();

	dragIsOccuring = false;
	connect(ui->frameImage, SIGNAL(moved()), this, SLOT(moveTransparentWidget()));

    qDebug() << "---- Rec Settings Window ---- Init complete";
    windowInitComplete = true;  //This is used to avoid control on_change events firing with incomplete values populated
}

RecSettingsWindow::~RecSettingsWindow()
{
    delete is;
	delete ui;
}




void RecSettingsWindow::on_cmdOK_clicked()
{
	//ImagerSettings_t settings;

    is->hRes = ui->spinHRes->value();       //pixels
    is->vRes = ui->spinVRes->value();       //pixels
    is->hOffset = ui->spinHOffset->value(); //Active area offset from left
    is->vOffset = ui->spinVOffset->value(); //Active area offset from top
    is->stride = is->hRes;                  //Number of pixels per line (allows for dark pixels in the last column), always multiple of 16
    is->gain = ui->comboGain->currentIndex();
	is->displayArea = QRect(0, 0, DISPLAY_SIZE_H, DISPLAY_SIZE_V);
//    is->recRegionSizeFrames = is->recRegionSizeFrames;
//    is->disableRingBuffer = is->disableRingBuffer;
//    is->mode = is->mode;
//    is->prerecordFrames = is->prerecordFrames;
//    is->segmentLengthFrames = is->segmentLengthFrames;
//    is->segments = is->segments;
//    is->temporary = 0;

	double framePeriod = camera->sensor->getActualFramePeriod(siText2Double(ui->linePeriod->text().toStdString().c_str()),
													   ui->spinHRes->value(),
													   ui->spinVRes->value());

    is->period = framePeriod * 100000000.0;

	double exp = camera->sensor->getActualIntegrationTime(siText2Double(ui->lineExp->text().toStdString().c_str()),
														  framePeriod,
														  ui->spinHRes->value(),
														  ui->spinVRes->value());


    is->exposure = exp * 100000000.0;

    is->temporary = false;
    camera->setImagerSettings(*is);
    camera->setDisplaySettings(false, MAX_LIVE_FRAMERATE);

	if(CAMERA_FILE_NOT_FOUND == camera->loadFPNFromFile())
		camera->autoFPNCorrection(2, false, true);

	close();
}

void RecSettingsWindow::on_spinHRes_valueChanged(int arg1)
{
    if(windowInitComplete)
    {
        updateOffsetLimits();


        updatePreviewBox();
        updateInfoText();

        is->recRegionSizeFrames = camera->getMaxRecordRegionSizeFrames(ui->spinHRes->value(), ui->spinVRes->value());
        qDebug() << "---- Rec Settings Window ---- hres =" << ui->spinHRes->value() << "vres =" << ui->spinVRes->value() << "recRegionSizeFrames =" << is->recRegionSizeFrames;
    }
}

void RecSettingsWindow::on_spinHRes_editingFinished()
{

	ui->spinHRes->setValue(round((UInt32)ui->spinHRes->value(), camera->sensor->getHResIncrement()));

	updatePreviewBox();
	qDebug() << "editing finished ";
	updateInfoText();
}

void RecSettingsWindow::on_spinVRes_valueChanged(int arg1)
{
    if(windowInitComplete)
    {
        updateOffsetLimits();
        updatePreviewBox();
        updateInfoText();
        is->recRegionSizeFrames = camera->getMaxRecordRegionSizeFrames(ui->spinHRes->value(), ui->spinVRes->value());
        qDebug() << "---- Rec Settings Window ---- hres =" << ui->spinHRes->value() << "vres =" << ui->spinVRes->value() << "recRegionSizeFrames =" << is->recRegionSizeFrames;
    }
}

void RecSettingsWindow::on_spinVRes_editingFinished()
{
	ui->spinVRes->setValue(round((UInt32)ui->spinVRes->value(), camera->sensor->getVResIncrement()));

	updatePreviewBox();
	updateInfoText();
}

void RecSettingsWindow::on_spinHOffset_valueChanged(int arg1)
{
	updatePreviewBox();
	updateInfoText();
}

void RecSettingsWindow::on_spinHOffset_editingFinished()
{
	ui->spinHOffset->setValue(round((UInt32)ui->spinHOffset->value(), camera->sensor->getHResIncrement()));

	updatePreviewBox();
	updateInfoText();
}

void RecSettingsWindow::on_spinVOffset_valueChanged(int arg1)
{
	updatePreviewBox();
	updateInfoText();
}

void RecSettingsWindow::on_spinVOffset_editingFinished()
{
	ui->spinVOffset->setValue(round((UInt32)ui->spinVOffset->value(), camera->sensor->getVResIncrement()));

	updatePreviewBox();
	updateInfoText();
}

void RecSettingsWindow::updateOffsetLimits()
{
	ui->spinHOffset->setMaximum(camera->sensor->getMaxHStride() - ui->spinHRes->value());
	ui->spinVOffset->setMaximum(camera->sensor->getMaxVRes() - ui->spinVRes->value());

    //[gui2] if(ui->chkCenter->checkState())
    if(true)
	{
		ui->spinHOffset->setValue(round((camera->sensor->getMaxHRes() - ui->spinHRes->value()) / 2, camera->sensor->getHResIncrement()));
		ui->spinVOffset->setValue(round((camera->sensor->getMaxVRes() - ui->spinVRes->value()) / 2, camera->sensor->getVResIncrement()));
	}
}

void RecSettingsWindow::on_chkCenter_toggled(bool checked)
{
	if(checked)
	{
		ui->spinHOffset->setValue(round((camera->sensor->getMaxHRes() - ui->spinHRes->value()) / 2, camera->sensor->getHResIncrement()));
		ui->spinVOffset->setValue(round((camera->sensor->getMaxVRes() - ui->spinVRes->value()) / 2, camera->sensor->getVResIncrement()));
	}

	ui->spinHOffset->setEnabled(!checked);
	ui->spinVOffset->setEnabled(!checked);
}


void RecSettingsWindow::on_cmdMax_clicked()
{
	double framePeriod = camera->sensor->getMinMasterFramePeriod(ui->spinHRes->value(), ui->spinVRes->value());
	char str[100];

	getSIText(str, framePeriod, 10, DEF_SI_OPTS, 8);
	qDebug() << framePeriod;
        ui->linePeriod->setText(str);

	double frameRate = 1.0 / framePeriod;
	qDebug() << frameRate;
	ui->lineRate->setText(QString::number(frameRate));

	//Make sure exposure is within limits
	double exp = camera->sensor->getActualIntegrationTime(siText2Double(ui->lineExp->text().toStdString().c_str()),
														  framePeriod,
														  ui->spinHRes->value(),
														  ui->spinVRes->value());

	//Format the entered value nicely
	getSIText(str, exp, 10, DEF_SI_OPTS, 8);
	qDebug() << exp;
        ui->lineExp->setText(str);

}

/*
void RecSettingsWindow::on_cmdMax_clicked()
{
	double framePeriod = (double)camera->sensor->getMinFramePeriod(ui->spinHRes->value(), ui->spinVRes->value()) / 100000000.0;
	char str[100];
	UInt32 framePeriodInt;

	//Round up to the next 10ns period
	framePeriod = ceil(framePeriod * (100000000.0)) / 100000000.0;
	framePeriodInt = framePeriod * 100000000.0;
	getSIText(str, framePeriod, 10, DEF_SI_OPTS, 8);
	qDebug() << framePeriod;
	ui->linePeriod->setText(str);

	double frameRate = 1.0 / framePeriod;
	qDebug() << frameRate;
	getSIText(str, frameRate, ceil(log10(framePeriodInt)+1), DEF_SI_OPTS, 1000);

	ui->lineRate->setText(str);

	//Make sure exposure is within limits
	double expMax = (double)camera->sensor->getMaxExposure(framePeriodInt) / 100000000.0;
	double exp = siText2Double(ui->lineExp->text().toStdString().c_str());

	if(exp > expMax)
		exp = expMax;

	//Round to nearest 10ns period
	exp = round(exp * (100000000.0)) / 100000000.0;

	//Format the entered value nicely
	getSIText(str, exp, 10, DEF_SI_OPTS, 8);
	qDebug() << exp;
	ui->lineExp->setText(str);

}
*/

void RecSettingsWindow::on_linePeriod_returnPressed()
{
	char str[100];

	double framePeriod = camera->sensor->getActualFramePeriod(siText2Double(ui->linePeriod->text().toStdString().c_str()),
													   ui->spinHRes->value(),
													   ui->spinVRes->value());

	//format the entered value nicely
	getSIText(str, framePeriod, 10, DEF_SI_OPTS, 8);
	qDebug() << framePeriod;
        ui->linePeriod->setText(str);

	//Set the frame rate
	double frameRate = 1.0 / framePeriod;

	ui->lineRate->setText(QString::number(frameRate));

	//Make sure exposure is within limits
	double exp = camera->sensor->getActualIntegrationTime(siText2Double(ui->lineExp->text().toStdString().c_str()),
														  framePeriod,
														  ui->spinHRes->value(),
														  ui->spinVRes->value());

	//Format the entered value nicely
	getSIText(str, exp, 10, DEF_SI_OPTS, 8);
	qDebug() << exp;
        ui->lineExp->setText(str);

}
/*
void RecSettingsWindow::on_linePeriod_returnPressed()
{
	double framePeriod;
	double framePeriodMin = (double)camera->sensor->getMinFramePeriod(ui->spinHRes->value(), ui->spinVRes->value()) / 100000000.0;
	UInt32 framePeriodInt;
	char str[100];
	framePeriod = siText2Double(ui->linePeriod->text().toStdString().c_str());

	if(framePeriod < framePeriodMin)
		framePeriod = framePeriodMin;

	//Round to nearest 10ns period
	framePeriod = round(framePeriod * (100000000.0)) / 100000000.0;
	framePeriodInt = framePeriod * 100000000.0;

	//format the entered value nicely
	getSIText(str, framePeriod, 10, DEF_SI_OPTS, 8);
	qDebug() << framePeriod;
	ui->linePeriod->setText(str);

	//Set the frame rate
	double frameRate = 1.0 / framePeriod;
	qDebug() << frameRate;
	getSIText(str, frameRate, ceil(log10(framePeriodInt)+1), DEF_SI_OPTS, 1000);

	ui->lineRate->setText(str);

	//Make sure exposure is within limits
	double expMax = (double)camera->sensor->getMaxExposure(framePeriodInt) / 100000000.0;
	double exp = siText2Double(ui->lineExp->text().toStdString().c_str());

	if(exp > expMax)
		exp = expMax;

	//Round to nearest 10ns period
	exp = round(exp * (100000000.0)) / 100000000.0;

	//Format the entered value nicely
	getSIText(str, exp, 10, DEF_SI_OPTS, 8);
	qDebug() << exp;
	ui->lineExp->setText(str);

}
*/

void RecSettingsWindow::on_lineRate_returnPressed()
{
	double framePeriod;
	double frameRate;
	char str[100];

	frameRate = siText2Double(ui->lineRate->text().toStdString().c_str());

	if(0.0 == frameRate)
		return;

	framePeriod = camera->sensor->getActualFramePeriod(1.0 / frameRate,
													   ui->spinHRes->value(),
													   ui->spinVRes->value());;


	//Set the frame period box
	getSIText(str, framePeriod, 10, DEF_SI_OPTS, 8);
	qDebug() << framePeriod;
        ui->linePeriod->setText(str);

	//Refill the frame rate box with the nicely formatted value
	frameRate = 1.0 / framePeriod;
	qDebug() << frameRate;

	ui->lineRate->setText(QString::number(frameRate));

	//Make sure exposure is within limits
	double exp = camera->sensor->getActualIntegrationTime(siText2Double(ui->lineExp->text().toStdString().c_str()),
														  framePeriod,
														  ui->spinHRes->value(),
														  ui->spinVRes->value());

	//Format the entered value nicely
	getSIText(str, exp, 10, DEF_SI_OPTS, 8);
	qDebug() << exp;
        ui->lineExp->setText(str);

}

/*
void RecSettingsWindow::on_lineRate_returnPressed()
{
	double framePeriod;
	double frameRate;
	double framePeriodMin = (double)camera->sensor->getMinFramePeriod(ui->spinHRes->value(), ui->spinVRes->value()) / 100000000.0;
	UInt32 framePeriodInt;
	char str[100];

	frameRate = siText2Double(ui->lineRate->text().toStdString().c_str());

	if(0.0 == frameRate)
		return;

	framePeriod = 1.0 / frameRate;

	if(framePeriod < framePeriodMin)
		framePeriod = framePeriodMin;

	//Round to nearest 10ns period
	framePeriod = round(framePeriod * (100000000.0)) / 100000000.0;
	framePeriodInt = framePeriod * 100000000.0;

	//Set the frame period box
	getSIText(str, framePeriod, 10, DEF_SI_OPTS, 8);
	qDebug() << framePeriod;
	ui->linePeriod->setText(str);

	//Refill the frame rate box with the nicely formatted value
	frameRate = 1.0 / framePeriod;
	qDebug() << frameRate;
	getSIText(str, frameRate, ceil(log10(framePeriodInt)+1), DEF_SI_OPTS, 1000);

	ui->lineRate->setText(str);

	//Make sure exposure is within limits
	double expMax = (double)camera->sensor->getMaxExposure(framePeriodInt) / 100000000.0;
	double exp = siText2Double(ui->lineExp->text().toStdString().c_str());

	if(exp > expMax)
		exp = expMax;

	//Round to nearest 10ns period
	exp = round(exp * (100000000.0)) / 100000000.0;

	//Format the entered value nicely
	getSIText(str, exp, 10, DEF_SI_OPTS, 8);
	qDebug() << exp;
	ui->lineExp->setText(str);

}
*/

void RecSettingsWindow::on_lineExp_returnPressed()
{
	char str[100];

	double framePeriod = camera->sensor->getActualFramePeriod(siText2Double(ui->linePeriod->text().toStdString().c_str()),
													   ui->spinHRes->value(),
													   ui->spinVRes->value());


	//Make sure exposure is within limits
	double exp = camera->sensor->getActualIntegrationTime(siText2Double(ui->lineExp->text().toStdString().c_str()),
														  framePeriod,
														  ui->spinHRes->value(),
														  ui->spinVRes->value());

	//Format the entered value nicely
	getSIText(str, exp, 10, DEF_SI_OPTS, 8);
	qDebug() << exp;
        ui->lineExp->setText(str);
}

/*
void RecSettingsWindow::on_lineExp_returnPressed()
{
	char str[100];
	UInt32 framePeriodInt;
	double framePeriodMin = (double)camera->sensor->getMinFramePeriod(ui->spinHRes->value(), ui->spinVRes->value()) / 100000000.0;
	double framePeriod = siText2Double(ui->linePeriod->text().toStdString().c_str());

	if(framePeriod < framePeriodMin)
		framePeriod = framePeriodMin;

	//Round to nearest 10ns period
	framePeriod = round(framePeriod * (100000000.0)) / 100000000.0;
	framePeriodInt = framePeriod * 100000000.0;

	double expMax = (double)camera->sensor->getMaxExposure(framePeriodInt) / 100000000.0;
	double exp = siText2Double(ui->lineExp->text().toStdString().c_str());

	if(exp > expMax)
		exp = expMax;

	//Round to nearest 10ns period
	exp = round(exp * (100000000.0)) / 100000000.0;

	//Format the entered value nicely
	getSIText(str, exp, 10, DEF_SI_OPTS, 8);
	qDebug() << exp;
	ui->lineExp->setText(str);
}

*/

void RecSettingsWindow::on_cmdExpMax_clicked()
{
	char str[100];

	double framePeriod = camera->sensor->getActualFramePeriod(siText2Double(ui->linePeriod->text().toStdString().c_str()),
													   ui->spinHRes->value(),
													   ui->spinVRes->value());


	//Make sure exposure is within limits
	double exp = camera->sensor->getMaxIntegrationTime(framePeriod,
														  ui->spinHRes->value(),
														  ui->spinVRes->value());

	//Format the entered value nicely
	getSIText(str, exp, 10, DEF_SI_OPTS, 8);
	qDebug() << exp;
        ui->lineExp->setText(str);
}

/*
void RecSettingsWindow::on_cmdExpMax_clicked()
{
	char str[100];
	UInt32 framePeriodInt;
	double framePeriodMin = (double)camera->sensor->getMinFramePeriod(ui->spinHRes->value(), ui->spinVRes->value()) / 100000000.0;
	double framePeriod = siText2Double(ui->linePeriod->text().toStdString().c_str());

	if(framePeriod < framePeriodMin)
		framePeriod = framePeriodMin;

	//Round to nearest 10ns period
	framePeriod = round(framePeriod * (100000000.0)) / 100000000.0;
	framePeriodInt = framePeriod * 100000000.0;

	//Set to maximum exposure
	double exp = (double)camera->sensor->getMaxExposure(framePeriodInt) / 100000000.0;

	//Format the entered value nicely
	getSIText(str, exp, 10, DEF_SI_OPTS, 8);
	qDebug() << exp;
	ui->lineExp->setText(str);
}
*/

void RecSettingsWindow::updateInfoText()
{
	char str[300];
	char maxRateStr[30];
	char minPeriodStr[30];
	char maxExposureStr[30];

	double fp = camera->sensor->getMinMasterFramePeriod(ui->spinHRes->value(), ui->spinVRes->value());
	getSIText(minPeriodStr, fp, 10, DEF_SI_OPTS, 8);
	getSIText(maxRateStr, 1.0/fp, ceil(log10(fp*100000000.0)+1), DEF_SI_OPTS, 1000);
	getSIText(maxExposureStr, (double)camera->sensor->getMaxExposure(fp*100000000.0) / 100000000.0, 10, DEF_SI_OPTS, 8);

	sprintf(str, "Max rate for this resolution:\r\n%sfps\r\nMin Period: %ss", maxRateStr, minPeriodStr);
        //[gui2] ui->lblInfo->setText(str);
}

void RecSettingsWindow::updatePreviewBox()
{
	int padding = 2; //Make room for the borders of the preview box.
	float scaleX = MAX_FRAME_SIZE_H / 1.0 / (ui->frame->width() - padding);
	float scaleY = MAX_FRAME_SIZE_V / 1.0 / (ui->frame->height() - padding);
	
	int outerLeft   = 0;
	int outerTop    = 0;
	int outerRight  = ui->frame->width();
	int outerBottom = ui->frame->height();
	
	int innerLeft   = round(ui->spinHOffset->value() / scaleX + padding / 2);
	int innerTop    = round(ui->spinVOffset->value() / scaleY + padding / 2);
	int innerRight  = round(innerLeft + ui->spinHRes->value() / scaleX);
	int innerBottom = round(innerTop  + ui->spinVRes->value() / scaleY);
	
	//Passepartout components. Four, for the four sides. We can't cut holes in stuff very easily, and this was simpler and more visual than figuring out how masks work.
	ui->passepartoutLeft->setGeometry(
		QRect(outerLeft, outerTop, innerLeft, outerBottom-outerTop)
	);
	ui->passepartoutTop->setGeometry(
		QRect(innerLeft, outerTop, innerRight-innerLeft, innerTop)
	);
	ui->passepartoutRight->setGeometry(
		QRect(innerRight, outerTop, outerRight-innerRight, outerBottom-outerTop)
	);
	ui->passepartoutBottom->setGeometry(
		QRect(innerLeft, innerBottom, innerRight-innerLeft, outerBottom-innerBottom)
	);
	
	//Inner rectangle, draws black border inside passepartout.
	ui->frameImage->setGeometry(
		QRect(innerLeft, innerTop, innerRight-innerLeft, innerBottom-innerTop)
	);
}


void RecSettingsWindow::setResFromText(char * str)
{
	int hRes, vRes;
	int hOffset, vOffset;

	sscanf(str, "%d", &hRes);
	sscanf(str + std::string(str).find_first_not_of("0123456879") + 1, "%d", &vRes); //Skip over whatever that middle × gets munged to here.
	hOffset = round((camera->sensor->getMaxHRes() - hRes) / 2, camera->sensor->getHResIncrement());
	vOffset = round((camera->sensor->getMaxVRes() - vRes) / 2, camera->sensor->getVResIncrement());

	if(camera->sensor->isValidResolution(hRes, vRes, hOffset, vOffset)) {
		ui->spinHRes->setValue(hRes);
		ui->spinVRes->setValue(vRes);
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

void RecSettingsWindow::on_comboGain_activated()
{
	is->gain = ui->comboGain->currentIndex();
	
	camera->setImagerSettings(*is);
    camera->setDisplaySettings(false, MAX_LIVE_FRAMERATE);
	
	//TODO DDR: Figure out why the preview is sometimes bluegray after this.
}


void RecSettingsWindow::on_cmdRecMode_clicked()
{
    is->hRes = ui->spinHRes->value();		//pixels
    is->vRes = ui->spinVRes->value();		//pixels
    is->stride = ui->spinHRes->value();		//Number of pixels per line (allows for dark pixels in the last column), always multiple of 16
    is->hOffset = ui->spinHOffset->value();	//Active area offset from left
    is->vOffset = ui->spinVOffset->value();		//Active area offset from top
    is->gain = ui->comboGain->currentIndex();
	is->displayArea = QRect(0, 0, DISPLAY_SIZE_H, DISPLAY_SIZE_V); //Set the video output back to fullscreen.
//    is->recRegionSizeFrames = is->recRegionSizeFrames;
//    is->disableRingBuffer = is->disableRingBuffer;
//    is->mode = is->mode;
//    is->prerecordFrames = is->prerecordFrames;
//    is->segmentLengthFrames = is->segmentLengthFrames;
//    is->segments = is->segments;
//    is->temporary = 0;

    double framePeriod = camera->sensor->getActualFramePeriod(siText2Double(ui->linePeriod->text().toStdString().c_str()),
                                                       ui->spinHRes->value(),
                                                       ui->spinVRes->value());

    is->period = framePeriod * 100000000.0;

    double exp = camera->sensor->getActualIntegrationTime(siText2Double(ui->lineExp->text().toStdString().c_str()),
                                                          framePeriod,
                                                          ui->spinHRes->value(),
                                                          ui->spinVRes->value());


    is->exposure = exp * 100000000.0;

    is->temporary = false;


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
        triggerDelayWindow *w = new triggerDelayWindow(NULL, camera, is);
        //w->camera = camera;
        w->setAttribute(Qt::WA_DeleteOnClose);
        w->show();
    }
}

void RecSettingsWindow::moveTransparentWidget(){
	qDebug()<<"moveTransparentWidget";
	//ui->frameImage->
	//int newX = FrameImage.x + (QCursor::pos().x + )

}
