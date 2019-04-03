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
#include <QMessageBox>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <QSettings>

#include "cameraRegisters.h"
#include "userInterface.h"
#include "mainwindow.h"
#include "playbackwindow.h"
#include "recsettingswindow.h"
#include "iosettingswindow.h"
#include "utilwindow.h"
//#include "statuswindow.h"
#include "cammainwindow.h"
#include "whitebalancedialog.h"
#include "ui_cammainwindow.h"
#include "util.h"
#include "whitebalancedialog.h"

#include "lux1310.h"
#include "lux2100.h"


extern "C" {
#include "siText.h"
}

extern bool pych;

#define DEF_SI_OPTS	SI_DELIM_SPACE | SI_SPACE_BEFORE_PREFIX

GPMC * gpmc;
ImageSensor * sensor;
Camera * camera;
Video * vinst;
Control * cinst;
UserInterface * userInterface;
bool focusAidEnabled = false;


CamMainWindow::CamMainWindow(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::CamMainWindow)
{
	QSettings appSettings;
	CameraErrortype retVal;
	ui->setupUi(this);

	gpmc = new GPMC();
	camera = new Camera();
	vinst = new Video();
    cinst = new Control();

    userInterface = new UserInterface();

	/* FIXME: We need a better method for detecting the attached sensor. */
	if (false)
		sensor = new LUX2100();
	else
		sensor = new LUX1310();

	battCapacityPercent = 0;

	gpmc->init();
	userInterface->init();
    retVal = camera->init(gpmc, vinst, cinst, sensor, userInterface, 16*1024/32*1024*1024, true);

	if(retVal != SUCCESS)
	{
		QMessageBox msg;
		msg.setText(QString("Camera init failed, error") + QString::number((Int32)retVal));
		msg.exec();
	}
	ui->cmdWB->setEnabled(camera->getIsColor());
	ui->chkFocusAid->setChecked(camera->getFocusPeakEnable());

	const char * myfifo = "/var/run/bmsFifo";


	/* open, read, and display the message from the FIFO */
	bmsFifoFD = ::open(myfifo, O_RDONLY|O_NONBLOCK);

	sw = new StatusWindow;
\
    if (pych)
    {

    }
    else
    {
        updateExpSliderLimits();
    }
	updateCurrentSettingsLabel();

	lastShutterButton = camera->ui->getShutterButton();
	lastRecording = camera->getIsRecording();
	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(on_MainWindowTimer()));
	timer->start(16);

	connect(camera->vinst, SIGNAL(newSegment(VideoStatus *)), this, SLOT(on_newVideoSegment(VideoStatus *)));

	if (appSettings.value("debug/hideDebug", true).toBool()) {
		ui->cmdDebugWnd->setVisible(false);
		ui->cmdClose->setVisible(false);
		ui->cmdDPCButton->setVisible(false);
	}

	if (camera->get_autoRecord()) {
		camera->setRecSequencerModeNormal();
		camera->startRecording();
	}
	if (camera->get_autoSave()) autoSaveActive = true;
	else                        autoSaveActive = false;


	if(camera->UpsideDownDisplay && camera->RotationArgumentIsSet()){
		camera->upsideDownTransform(2);//2 for upside down, 0 for normal
	} else  camera->UpsideDownDisplay = false;//if the rotation argument has not been added, this should be set to false

	if( (camera->ButtonsOnLeft) ^ (camera->UpsideDownDisplay) ){
		camera->updateVideoPosition();
	}

	//record the number of widgets that are open before any other windows can be opened
	QWidgetList qwl = QApplication::topLevelWidgets();
	windowsAlwaysOpen = qwl.count();
}

CamMainWindow::~CamMainWindow()
{
	timer->stop();
	::close(bmsFifoFD);
	delete sw;

	delete ui;
	delete camera;
}

void CamMainWindow::on_cmdClose_clicked()
{
	qApp->exit();
}


void CamMainWindow::on_cmdDebugWnd_clicked()
{
	MainWindow *w = new MainWindow;
	w->camera = camera;
	w->setAttribute(Qt::WA_DeleteOnClose);
	w->show();
}

void CamMainWindow::on_cmdRec_clicked()
{
	if(camera->getIsRecording())
	{
		camera->stopRecording();
//		ui->cmdRec->setText("Record");
//		ui->cmdPlay->setEnabled(true);
	}
	else
	{
		//If there is unsaved video in RAM, prompt to start record.  unsavedWarnEnabled values: 0=always, 1=if not reviewed, 2=never
		if(false == camera->recordingData.hasBeenSaved && (0 != camera->unsavedWarnEnabled && (2 == camera->unsavedWarnEnabled || !camera->videoHasBeenReviewed)))
		{
			QMessageBox::StandardButton reply;
			reply = QMessageBox::question(this, "Unsaved video in RAM", "Start recording anyway and discard the unsaved video in RAM?", QMessageBox::Yes|QMessageBox::No);
			if(QMessageBox::Yes != reply)
				return;
		}

		camera->setRecSequencerModeNormal();
		camera->startRecording();
		if (camera->get_autoSave()) autoSaveActive = true;

//		ui->cmdRec->setText("Stop");
//		ui->cmdPlay->setEnabled(false);
	}

}

void CamMainWindow::on_cmdPlay_clicked()
{
	if(camera->getIsRecording()) {
		QMessageBox::StandardButton reply;
		reply = QMessageBox::question(this, "Stop recording?", "This action will stop recording; is this okay?", QMessageBox::Yes|QMessageBox::No);
		if(QMessageBox::Yes != reply)
			return;
		autoSaveActive = false;
		camera->stopRecording();
		delayms(100);
	}
	createNewPlaybackWindow();
}

void CamMainWindow::createNewPlaybackWindow(){
	playbackWindow *w = new playbackWindow(NULL, camera);
	if(camera->get_autoRecord()) connect(w, SIGNAL(finishedSaving()),this, SLOT(playFinishedSaving()));
	//w->camera = camera;
	w->setAttribute(Qt::WA_DeleteOnClose);
	w->show();
	//w->setGeometry(0, 0,w->width(), w->height());
}

void CamMainWindow::playFinishedSaving()
{
	qDebug("--- Play Finished ---");
	if (camera->get_autoSave()) autoSaveActive = true;
	if (camera->get_autoRecord() && camera->autoRecord) {
		delayms(100);//delay needed or else the record may not always automatically start
		camera->setRecSequencerModeNormal();
		camera->startRecording();
		qDebug("--- started recording ---");
	}
}


void CamMainWindow::on_cmdRecSettings_clicked()
{
	if(camera->getIsRecording()) {
		QMessageBox::StandardButton reply;
		reply = QMessageBox::question(this, "Stop recording?", "This action will stop recording; is this okay?", QMessageBox::Yes|QMessageBox::No);
		if(QMessageBox::Yes != reply)
			return;
		autoSaveActive = false;
		camera->stopRecording();
	}
	RecSettingsWindow *w = new RecSettingsWindow(NULL, camera);
	connect(w, SIGNAL(settingsChanged()),this, SLOT(recSettingsClosed()));
	//w->camera = camera;
	w->setAttribute(Qt::WA_DeleteOnClose);
	w->show();

	/*while(w->isHidden() == false)
	  delayms(100);

	  qDebug() << "deleting window";
	  delete w;*/
}


void CamMainWindow::on_cmdFPNCal_clicked()//Black cal
{
	if(camera->getIsRecording()) {
		QMessageBox::StandardButton reply;
		reply = QMessageBox::question(this, "Stop recording?", "This action will stop recording and erase the video; is this okay?", QMessageBox::Yes|QMessageBox::No);
		if(QMessageBox::Yes != reply)
			return;
		autoSaveActive = false;
		camera->stopRecording();
		delayms(100);
	}
	else {
			//If there is unsaved video in RAM, prompt to start record
			QMessageBox::StandardButton reply;
			if(false == camera->recordingData.hasBeenSaved)	reply = QMessageBox::question(this, "Unsaved video in RAM", "Performing black calibration will erase the unsaved video in RAM. Continue?", QMessageBox::Yes|QMessageBox::No);
			else											reply = QMessageBox::question(this, "Start black calibration?", "Will start black calibration. Continue?", QMessageBox::Yes|QMessageBox::No);

			if(QMessageBox::Yes != reply)
				return;
	}
	sw->setText("Performing black calibration...");
	sw->show();
	QCoreApplication::processEvents();
	camera->liveColumnCalibration();
	camera->autoFPNCorrection(16, true);
	sw->hide();
}

void CamMainWindow::on_cmdWB_clicked()
{
	if(camera->getIsRecording()) {
		QMessageBox::StandardButton reply;
		reply = QMessageBox::question(this, "Stop recording?", "This action will stop recording and erase the video; is this okay?", QMessageBox::Yes|QMessageBox::No);
		if(QMessageBox::Yes != reply)
			return;
		autoSaveActive = false;
		camera->stopRecording();
		delayms(100);
	}
	autoSaveActive = false;
	camera->stopRecording();

	whiteBalanceDialog *whiteBalWindow = new whiteBalanceDialog(NULL, camera);
	whiteBalWindow->setAttribute(Qt::WA_DeleteOnClose);
	whiteBalWindow->show();
	whiteBalWindow->setModal(true);
	//whiteBalWindow->move(camera->ButtonsOnLeft? 0:600, 0);
}

void CamMainWindow::on_cmdIOSettings_clicked()
{
	if(camera->getIsRecording()) {
		QMessageBox::StandardButton reply;
		reply = QMessageBox::question(this, "Stop recording?", "This action will stop recording and erase the video; is this okay?", QMessageBox::Yes|QMessageBox::No);
		if(QMessageBox::Yes != reply)
			return;
		autoSaveActive = false;
		camera->stopRecording();
	}
	IOSettingsWindow *w = new IOSettingsWindow(NULL, camera);
	//w->camera = camera;
	w->setAttribute(Qt::WA_DeleteOnClose);
	w->show();
}

void CamMainWindow::updateRecordingState(bool recording)
{
	if(recording)
	{
		ui->cmdRec->setText("Stop");
		//ui->cmdDebugWnd->setEnabled(false);
		//ui->cmdRecSettings->setEnabled(false);
		//ui->cmdPlay->setEnabled(false);
		//ui->cmdFPNCal->setEnabled(false);
		//ui->cmdWB->setEnabled(false);
		//ui->cmdIOSettings->setEnabled(false);
		//ui->cmdUtil->setEnabled(false);
		////ui->cmdClose->setEnabled(false);

	}
	else	//Not recording
	{
		ui->cmdRec->setText("Record");
		ui->cmdPlay->setEnabled(true);
		//ui->cmdDebugWnd->setEnabled(true);
		//ui->cmdRecSettings->setEnabled(true);
		//ui->cmdFPNCal->setEnabled(true);
		//ui->cmdWB->setEnabled(true);
		//ui->cmdIOSettings->setEnabled(true);
		//ui->cmdUtil->setEnabled(true);
		////ui->cmdClose->setEnabled(true);

		if(camera->get_autoSave() && autoSaveActive)
		{
			playbackWindow *w = new playbackWindow(NULL, camera, true);
			if(camera->get_autoRecord()) connect(w, SIGNAL(finishedSaving()),this, SLOT(playFinishedSaving()));
			//w->camera = camera;
			w->setAttribute(Qt::WA_DeleteOnClose);
			w->show();
		}
	}
}

void CamMainWindow::on_MainWindowTimer()
{
	bool shutterButton = camera->ui->getShutterButton();
	char buf[300];
	Int32 len;
	QSettings appSettings;

	if(shutterButton && !lastShutterButton)
	{
		if(camera->getIsRecording())
		{
			camera->stopRecording();
		}
		else
		{
			QWidgetList qwl = QApplication::topLevelWidgets();	//Hack to stop you from starting record when another window is open. Need to get modal dialogs working for proper fix
			if(qwl.count() <= windowsAlwaysOpen)				//Now that the numeric keypad has been added, there are four windows: cammainwindow, debug buttons window, and both keyboards
			{
				//If there is unsaved video in RAM, prompt to start record.  unsavedWarnEnabled values: 0=always, 1=if not reviewed, 2=never
				if(false == camera->recordingData.hasBeenSaved && (0 != camera->unsavedWarnEnabled && (2 == camera->unsavedWarnEnabled || !camera->videoHasBeenReviewed)) && false == camera->get_autoSave())	//If there is unsaved video in RAM, prompt to start record

				{
					QMessageBox::StandardButton reply;
					reply = QMessageBox::question(this, "Unsaved video in RAM", "Start recording anyway and discard the unsaved video in RAM?", QMessageBox::Yes|QMessageBox::No);
					if(QMessageBox::Yes == reply)
					{
						camera->setRecSequencerModeNormal();
						camera->startRecording();
					}
				}
				else
				{
					camera->setRecSequencerModeNormal();
					camera->startRecording();
					if (camera->get_autoSave()) autoSaveActive = true;
				}
			}
		}
	}

	bool recording = camera->getIsRecording();

	if(recording != lastRecording)
	{
		updateRecordingState(recording);
		lastRecording = recording;
	}

	lastShutterButton = shutterButton;

	//If new data comes in from the BMS, get the battery SOC and updates the info label
	len = read(bmsFifoFD, buf, 300);

	if(len > 0)
	{
		sscanf(buf, "battCapacityPercent %hhu\nbattSOHPercent %hhu\nbattVoltage %u\nbattCurrent %u\nbattHiResCap %u\nbattHiResSOC %u\n"
			   "battVoltageCam %u\nbattCurrentCam %d\nmbTemperature %d\nflags %hhu\nfanPWM %hhu",
			   &battCapacityPercent,
			   &battSOHPercent,
			   &battVoltage,
			   &battCurrent,
			   &battHiResCap,
			   &battHiResSOC,
			   &battVoltageCam,
			   &battCurrentCam,
			   &mbTemperature,
			   &flags,
			   &fanPWM);
		updateCurrentSettingsLabel();
	}

	if (appSettings.value("debug/hideDebug", true).toBool()) {
		ui->cmdDebugWnd->setVisible(false);
		ui->cmdClose->setVisible(false);
		ui->cmdDPCButton->setVisible(false);
	}
	else {
		ui->cmdDebugWnd->setVisible(true);
		ui->cmdClose->setVisible(true);
		ui->cmdDPCButton->setVisible(true);
	}
}

void CamMainWindow::on_newVideoSegment(VideoStatus *st)
{
	/* Flag that we have unsaved video. */
	qDebug("--- Sequencer --- Total recording size: %u", st->totalFrames);
	if (st->totalFrames) {
		camera->recordingData.is = camera->getImagerSettings();
		camera->recordingData.valid = true;
		camera->recordingData.hasBeenSaved = false;
	}
}

void CamMainWindow::on_chkFocusAid_clicked(bool focusAidEnabled)
{
	camera->setFocusPeakEnable(focusAidEnabled);
}

void CamMainWindow::on_expSlider_valueChanged(int exposure)
{
	camera->setIntegrationTime((double)exposure / camera->sensor->getIntegrationClock(), NULL, 0);
	updateCurrentSettingsLabel();
}

void CamMainWindow::recSettingsClosed()
{
	updateExpSliderLimits();
	updateCurrentSettingsLabel();
}

//Upate the exposure slider limits and step size.
void CamMainWindow::updateExpSliderLimits()
{
	FrameGeometry fSize = camera->getImagerSettings().geometry;
	UInt32 fPeriod = camera->sensor->getFramePeriod();
	UInt32 exposure = camera->sensor->getIntegrationTime();

	ui->expSlider->setMinimum(camera->sensor->getMinIntegrationTime(fPeriod, &fSize));
	ui->expSlider->setMaximum(camera->sensor->getMaxIntegrationTime(fPeriod, &fSize));
	ui->expSlider->setValue(exposure);

	/* Do fine stepping in 1us increments */
	ui->expSlider->setSingleStep(camera->sensor->getIntegrationClock() / 1000000);
	ui->expSlider->setPageStep(camera->sensor->getIntegrationClock() / 1000000);
}

//Update the status textbox with the current settings
void CamMainWindow::updateCurrentSettingsLabel()
{
	ImagerSettings_t is = camera->getImagerSettings();
	double framePeriod = camera->sensor->getCurrentFramePeriodDouble();
	double expPeriod = camera->sensor->getCurrentExposureDouble();
	int shutterAngle = (expPeriod * 360.0) / framePeriod;

	char str[300];
	char battStr[50];
	char fpsString[30];
	char expString[30];

	sprintf(fpsString, QString::number(1 / framePeriod).toAscii());
	getSIText(expString, expPeriod, 4, DEF_SI_OPTS, 10);
	shutterAngle = max(shutterAngle, 1); //to prevent 0 degrees from showing on the label if the current exposure is less than 1/360'th of the frame period.

	double battPercent = (flags & 4) ?	//If battery is charging
						within(((double)battVoltageCam/1000.0 - 10.75) / (12.4 - 10.75) * 80, 0.0, 80.0) +
							20 - 20*within(((double)battCurrentCam/1000.0 - 0.1) / (1.28 - 0.1), 0.0, 1.0) :	//Charging
						within(((double)battVoltageCam/1000.0 - 9.75) / (11.8 - 9.75) * 100, 0.0, 100.0);		//Dicharging
	//charge 10.75 - 12.4 is 0-80%

	if(flags & 1)	//If battery present
	{
		sprintf(battStr, "Batt %d%% %.2fV", (UInt32)battPercent,  (double)battVoltageCam / 1000.0);
	}
	else
	{
		sprintf(battStr, "No Batt");
	}

	sprintf(str, "%s\r\n%ux%u %sfps\r\nExp %ss (%u\xb0)", battStr, is.geometry.hRes, is.geometry.vRes, fpsString, expString, shutterAngle);
	ui->lblCurrent->setText(str);
}

void CamMainWindow::on_cmdUtil_clicked()
{
	if(camera->getIsRecording()) {
		QMessageBox::StandardButton reply;
		reply = QMessageBox::question(this, "Stop recording?", "This action will stop recording and erase the video; is this okay?", QMessageBox::Yes|QMessageBox::No);
		if(QMessageBox::Yes != reply)
			return;
		autoSaveActive = false;
		camera->stopRecording();
	}
	UtilWindow *w = new UtilWindow(NULL, camera);
	//w->camera = camera;
	w->setAttribute(Qt::WA_DeleteOnClose);
	w->show();
	connect(w, SIGNAL(moveCamMainWindow()), this, SLOT(updateCamMainWindowPosition()));
	connect(w, SIGNAL(destroyed()), this, SLOT(UtilWindow_closed()));
}

void CamMainWindow::UtilWindow_closed()
{
	ui->chkFocusAid->setChecked(camera->getFocusPeakEnable());
}

void CamMainWindow::updateCamMainWindowPosition(){
	//qDebug()<<"windowpos old " << this->x();
	move(camera->ButtonsOnLeft? 0:600, 0);
	//qDebug()<<"windowpos new " << this->x();
}

void CamMainWindow::on_cmdBkGndButton_clicked()
{
	ui->chkFocusAid->setVisible(true);
	ui->cmdFPNCal->setVisible(true);
	ui->cmdIOSettings->setVisible(true);
	ui->cmdPlay->setVisible(true);
	ui->cmdRec->setVisible(true);
	ui->cmdRecSettings->setVisible(true);
	ui->cmdUtil->setVisible(true);

	ui->cmdWB->setVisible(true);
	ui->expSlider->setVisible(true);
	ui->lblCurrent->setVisible(true);
	ui->lblExp->setVisible(true);
}


void CamMainWindow::on_cmdDPCButton_clicked()
{
	char text[100];
	Int32 retVal;
	int resultCount, resultMax;
	camera->io->setOutLevel((1 << 1));	//Turn on output drive

	retVal = camera->checkForDeadPixels(&resultCount, &resultMax);
	if (retVal != SUCCESS) {
		QMessageBox msg;
		if (retVal == CAMERA_DEAD_PIXEL_RECORD_ERROR)
			sprintf(text, "Failed dead pixel detection, error %d", retVal);
		else if (retVal == CAMERA_DEAD_PIXEL_FAILED)
			sprintf(text, "Failed dead pixel detection\n%d dead pixels found on sensor\nMax value: %d", resultCount, resultMax);
		msg.setText(text);
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
	}
	else {
		QMessageBox msg;
		sprintf(text, "Dead pixel detection passed!\nMax deviation: %d", resultMax);
		msg.setText(text);
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
	}
}

/*
 * Approximate the logarithmic position by computing
 *   theta = 180 * (3/2) ^ (n / 4)
 *
 * Return the next logarithmic step given by:
 *   180 * (3/2) ^ ((n + delta) / 4)
 */
static int expSliderLog(unsigned int angle, int delta)
{
	/* Precomputed quartic roots of 3/2. */
	const double qRoots[] = {
		1.0, 1.10668192, 1.224744871, 1.355403005
	};

	/* n = delta + round(4 * log2(theta / 180) / log2(3/2)) */
	int n = round(log2((double)angle / 180.0) * (4.0 / 0.5849625007211562)) + delta;

	double x = 180 * qRoots[n & 0x3];
	while (n >= 4) { x = (x * 3) / 2; n -= 4; };
	while (n <  0) { x = (x * 2) / 3; n += 4; };
	return (int)(x + 0.5);
}

void CamMainWindow::keyPressEvent(QKeyEvent *ev)
{
	double framePeriod = camera->sensor->getCurrentFramePeriodDouble();
	UInt32 expPeriod = camera->sensor->getIntegrationTime();
	UInt32 nextPeriod = expPeriod;
	int expAngle = (expPeriod * 360) / (framePeriod * camera->sensor->getIntegrationClock());

	qDebug() << "framePeriod =" << framePeriod << " expPeriod =" << expPeriod;

	switch (ev->key()) {
	/* Up/Down moves the slider logarithmically */
	case Qt::Key_Up:
		if (expAngle > 0) {
			nextPeriod = (framePeriod * expSliderLog(expAngle, 1) * camera->sensor->getIntegrationClock()) / 360;
		}
		if (nextPeriod < (expPeriod + ui->expSlider->singleStep())) {
			nextPeriod = expPeriod + ui->expSlider->singleStep();
		}
		ui->expSlider->setValue(nextPeriod);
		break;

	case Qt::Key_Down:
		if (expAngle > 0) {
			nextPeriod = (framePeriod * expSliderLog(expAngle, -1) * camera->sensor->getIntegrationClock()) / 360;
		}
		if (nextPeriod > (expPeriod - ui->expSlider->singleStep())) {
			nextPeriod = expPeriod - ui->expSlider->singleStep();
		}
		ui->expSlider->setValue(nextPeriod);
		break;

	/* PageUp/PageDown moves the slider linearly by degrees. */
	case Qt::Key_PageUp:
		ui->expSlider->setValue(expPeriod + ui->expSlider->singleStep());
		break;

	case Qt::Key_PageDown:
		ui->expSlider->setValue(expPeriod - ui->expSlider->singleStep());
		break;
	}
}
