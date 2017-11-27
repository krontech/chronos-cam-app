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
	char str[100];

	ui->setupUi(this);
	this->setWindowFlags(Qt::Dialog /*| Qt::WindowStaysOnTopHint*/ | Qt::FramelessWindowHint);
	this->move(0,0);

	connect(ui->cmdCancel, SIGNAL(clicked()), this, SLOT(close()));

	camera = cameraInst;

	ui->spinHRes->setSingleStep(camera->sensor->getHResIncrement());
	ui->spinHRes->setMinimum(camera->sensor->getMinHRes());
	ui->spinHRes->setMaximum(camera->sensor->getMaxHStride());

	ui->spinVRes->setSingleStep(camera->sensor->getVResIncrement());
	ui->spinVRes->setMinimum(camera->sensor->getMinVRes());
	ui->spinVRes->setMaximum(camera->sensor->getMaxVRes());

	ui->spinHOffset->setSingleStep(camera->sensor->getHResIncrement());
	ui->spinHOffset->setMinimum(0);
	ui->spinHOffset->setMaximum(camera->sensor->getMaxHStride() - camera->getImagerSettings().hOffset);

	ui->spinVOffset->setSingleStep(camera->sensor->getVResIncrement());
	ui->spinVOffset->setMinimum(0);
	ui->spinVOffset->setMaximum(camera->sensor->getMaxVRes() - camera->getImagerSettings().vRes);

	ui->spinHRes->setValue(camera->getImagerSettings().stride);
	ui->spinVRes->setValue(camera->getImagerSettings().vRes);
	ui->spinHOffset->setValue(camera->getImagerSettings().hOffset);
	ui->spinVOffset->setValue(camera->getImagerSettings().vOffset);

	ui->comboGain->addItem("0dB (x1)");
	ui->comboGain->addItem("6dB (x2)");
	ui->comboGain->addItem("12dB (x4)");
	ui->comboGain->addItem("18dB (x8)");
	ui->comboGain->addItem("24dB (x16)");
	ui->comboGain->setCurrentIndex(camera->getImagerSettings().gain);

	//Populate the common resolution combo box from the list of resolutions
	FILE * fp;
	char line[30];

	fp = fopen("resolutions", "r");
	if (fp != NULL)
	{
		while (fgets(line, 30, fp) != NULL) {
			//Remove any newline or carrage return and replace with null
			char *pos;
			if ((pos=strchr(line, '\r')) != NULL)
				*pos = '\0';
			if ((pos=strchr(line, '\n')) != NULL)
				*pos = '\0';

			//Get the resolution and compute the maximum frame rate to be appended after the resolution
			int hRes, vRes;

			sscanf(line, "%dx%d", &hRes, &vRes);

			int fr =  100000000.0 / (double)camera->sensor->getMinFramePeriod(hRes, vRes);
			qDebug() << "hres" << hRes << "vRes" << vRes << "mperiod" << camera->sensor->getMinFramePeriod(hRes, vRes) << "fr" << fr;
			char tmp[20];
			sprintf(tmp, " %d fps", fr);
			strcat(line, tmp);

			ui->comboRes->addItem(line);
		}

		fclose(fp);
	}





	//If the current image position is in the center, check the centered checkbox
	if(	camera->getImagerSettings().hOffset == round((camera->sensor->getMaxHRes() - camera->getImagerSettings().stride) / 2, camera->sensor->getHResIncrement()) &&
		camera->getImagerSettings().vOffset == round((camera->sensor->getMaxVRes() - camera->getImagerSettings().vRes) / 2, camera->sensor->getVResIncrement()))
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
	double framePeriod = (double)camera->getImagerSettings().period / 100000000.0;
	getSIText(str, framePeriod, 10, DEF_SI_OPTS, 8);
	ui->linePeriod->setText(str);

	//Set the frame rate
	double frameRate = 1.0 / framePeriod;
	getSIText(str, frameRate, ceil(log10(framePeriod*100000000.0)+1), DEF_SI_OPTS, 1000);
	ui->lineRate->setText(str);

	//Set the exposure
	double exposure = (double)camera->getImagerSettings().exposure / 100000000.0;
	getSIText(str, exposure, 10, DEF_SI_OPTS, 8);
	ui->lineExp->setText(str);


	ui->frameImage->setGeometry(QRect(ui->spinHOffset->value()/4, ui->spinVOffset->value()/4, ui->spinHRes->value()/4, ui->spinVRes->value()/4));

	updateInfoText();

}

RecSettingsWindow::~RecSettingsWindow()
{
	delete ui;
}




void RecSettingsWindow::on_cmdOK_clicked()
{
	ImagerSettings_t settings;



	settings.hRes = ui->spinHRes->value();		//pixels
	settings.vRes = ui->spinVRes->value();		//pixels
	settings.stride = ui->spinHRes->value();		//Number of pixels per line (allows for dark pixels in the last column), always multiple of 16
	settings.hOffset = ui->spinHOffset->value();	//Active area offset from left
	settings.vOffset = ui->spinVOffset->value();		//Active area offset from top
	settings.gain = ui->comboGain->currentIndex();
    settings.disableRingBuffer = camera->getImagerSettings().disableRingBuffer;
    settings.mode = camera->getImagerSettings().mode;
    settings.prerecordFrames = camera->getImagerSettings().prerecordFrames;
    settings.segmentLengthFrames = camera->getImagerSettings().segmentLengthFrames;
    settings.segments = camera->getImagerSettings().segments;
    settings.temporary = 0;

	double framePeriod = camera->sensor->getActualFramePeriod(siText2Double(ui->linePeriod->text().toStdString().c_str()),
													   ui->spinHRes->value(),
													   ui->spinVRes->value());

	settings.period = framePeriod * 100000000.0;

	double exp = camera->sensor->getActualIntegrationTime(siText2Double(ui->lineExp->text().toStdString().c_str()),
														  framePeriod,
														  ui->spinHRes->value(),
														  ui->spinVRes->value());


	settings.exposure = exp * 100000000.0;

	settings.temporary = 0;
	camera->setImagerSettings(settings);
    camera->setDisplaySettings(false, MAX_LIVE_FRAMERATE);

	if(CAMERA_FILE_NOT_FOUND == camera->loadFPNFromFile(FPN_FILENAME))
		camera->autoFPNCorrection(2, false, true);

	close();
}

/*
void RecSettingsWindow::on_cmdOK_clicked()
{
	ImagerSettings_t settings;

	settings.hRes = min(ui->spinHRes->value(), 1280);		//pixels
	settings.vRes = ui->spinVRes->value();		//pixels
	settings.stride = ui->spinHRes->value();		//Number of pixels per line (allows for dark pixels in the last column), always multiple of 16
	settings.hOffset = ui->spinHOffset->value();	//Active area offset from left
	settings.vOffset = ui->spinVOffset->value();		//Active area offset from top

	double framePeriodMin = (double)camera->sensor->getMinFramePeriod(settings.stride, settings.vRes) / 100000000.0;
	double framePeriod = siText2Double(ui->linePeriod->text().toStdString().c_str());

	if(framePeriod < framePeriodMin)
		framePeriod = framePeriodMin;

	settings.period = framePeriod * 100000000.0;

	double expMax = (double)camera->sensor->getMaxExposure(settings.period) / 100000000.0;
	double exp = siText2Double(ui->lineExp->text().toStdString().c_str());

	if(exp > expMax)
		exp = expMax;

	settings.exposure = exp * 100000000.0;

	camera->setImagerSettings(settings);
    camera->setDisplaySettings(false, MAX_LIVE_FRAMERATE);

	camera->loadFPNFromFile(FPN_FILENAME);

	close();
}
*/
void RecSettingsWindow::on_spinHRes_valueChanged(int arg1)
{
	updateOffsetLimits();


	ui->frameImage->setGeometry(QRect(ui->spinHOffset->value()/4, ui->spinVOffset->value()/4, ui->spinHRes->value()/4, ui->spinVRes->value()/4));
	updateInfoText();
}

void RecSettingsWindow::on_spinHRes_editingFinished()
{

	ui->spinHRes->setValue(round((UInt32)ui->spinHRes->value(), camera->sensor->getHResIncrement()));

	ui->frameImage->setGeometry(QRect(ui->spinHOffset->value()/4, ui->spinVOffset->value()/4, ui->spinHRes->value()/4, ui->spinVRes->value()/4));
	qDebug() << "editing finished ";
	updateInfoText();
}

void RecSettingsWindow::on_spinVRes_valueChanged(int arg1)
{
	updateOffsetLimits();
	ui->frameImage->setGeometry(QRect(ui->spinHOffset->value()/4, ui->spinVOffset->value()/4, ui->spinHRes->value()/4, ui->spinVRes->value()/4));
	updateInfoText();
}

void RecSettingsWindow::on_spinVRes_editingFinished()
{
	ui->spinVRes->setValue(round((UInt32)ui->spinVRes->value(), camera->sensor->getVResIncrement()));

	ui->frameImage->setGeometry(QRect(ui->spinHOffset->value()/4, ui->spinVOffset->value()/4, ui->spinHRes->value()/4, ui->spinVRes->value()/4));
	updateInfoText();
}

void RecSettingsWindow::on_spinHOffset_valueChanged(int arg1)
{
	ui->frameImage->setGeometry(QRect(ui->spinHOffset->value()/4, ui->spinVOffset->value()/4, ui->spinHRes->value()/4, ui->spinVRes->value()/4));
	updateInfoText();
}

void RecSettingsWindow::on_spinHOffset_editingFinished()
{
	ui->spinHOffset->setValue(round((UInt32)ui->spinHOffset->value(), camera->sensor->getHResIncrement()));

	ui->frameImage->setGeometry(QRect(ui->spinHOffset->value()/4, ui->spinVOffset->value()/4, ui->spinHRes->value()/4, ui->spinVRes->value()/4));
	updateInfoText();
}

void RecSettingsWindow::on_spinVOffset_valueChanged(int arg1)
{
	ui->frameImage->setGeometry(QRect(ui->spinHOffset->value()/4, ui->spinVOffset->value()/4, ui->spinHRes->value()/4, ui->spinVRes->value()/4));
	updateInfoText();
}

void RecSettingsWindow::on_spinVOffset_editingFinished()
{
	ui->spinVOffset->setValue(round((UInt32)ui->spinVOffset->value(), camera->sensor->getVResIncrement()));

	ui->frameImage->setGeometry(QRect(ui->spinHOffset->value()/4, ui->spinVOffset->value()/4, ui->spinHRes->value()/4, ui->spinVRes->value()/4));
	updateInfoText();
}

void RecSettingsWindow::updateOffsetLimits()
{
	ui->spinHOffset->setMaximum(camera->sensor->getMaxHStride() - ui->spinHRes->value());
	ui->spinVOffset->setMaximum(camera->sensor->getMaxVRes() - ui->spinVRes->value());

	if(ui->chkCenter->checkState())
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
	getSIText(str, frameRate, ceil(log10(framePeriod * 100000000.0)+1), DEF_SI_OPTS, 1000);

	ui->lineRate->setText(str);

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
	qDebug() << frameRate;
	getSIText(str, frameRate, ceil(log10(framePeriod*100000000.0)+1), DEF_SI_OPTS, 1000);

	ui->lineRate->setText(str);

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
	getSIText(str, frameRate, ceil(log10(framePeriod * 100000000.0)+1), DEF_SI_OPTS, 1000);

	ui->lineRate->setText(str);

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
	ui->lblInfo->setText(str);
}


void RecSettingsWindow::setResFromText(char * str)
{
	int hRes, vRes;

	sscanf(str, "%dx%d", &hRes, &vRes);

	if(camera->sensor->isValidResolution(hRes, vRes, (camera->sensor->getMaxHRes() - hRes) / 2 & 0xFFFFFFFE, (camera->sensor->getMaxVRes() - vRes) / 2 & 0xFFFFFFFE))
	{
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

void RecSettingsWindow::on_cmdRecMode_clicked()
{
    recModeWindow *w = new recModeWindow(NULL, camera);
    //w->camera = camera;
    w->setAttribute(Qt::WA_DeleteOnClose);
    w->show();
}
