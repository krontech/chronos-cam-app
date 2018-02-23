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
#include "ui_cammainwindow.h"
#include "util.h"

extern "C" {
#include "siText.h"
}

#define DEF_SI_OPTS	SI_DELIM_SPACE | SI_SPACE_BEFORE_PREFIX

GPMC * gpmc;
LUX1310 * sensor;
Camera * camera;
Video * vinst;
UserInterface * userInterface;
bool focusAidEnabled = false;
QTimer * menuTimeoutTimer;


CamMainWindow::CamMainWindow(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::CamMainWindow)
{
	QMessageBox msg;
	QSettings appSettings;
	CameraErrortype retVal;
	ui->setupUi(this);

	gpmc = new GPMC();
	camera = new Camera();
	vinst = new Video();
	sensor = new LUX1310();
	userInterface = new UserInterface();

	battCapacityPercent = 0;

	gpmc->init();

	//vinst->frameCallback = &frameCallback;
	vinst->frameCallbackArg = NULL;
	vinst->init();


	userInterface->init();



	retVal = camera->init(gpmc, vinst, sensor, userInterface, 16*1024/32*1024*1024, true);

	if(retVal != SUCCESS)
	{

		msg.setText(QString("Camera init failed, error") + QString::number((Int32)retVal));
		msg.exec();
	}
	//camera->endOfRecCallback = &endOfRecCallback;
	//camera->endOfRecCallbackArg = this;

	qDebug() << "camera->sensor->getMaxCurrentIntegrationTime() returned" << camera->sensor->getMaxCurrentIntegrationTime();

	ui->expSlider->setMinimum(LUX1310_MIN_INT_TIME * 100000000.0);
	ui->expSlider->setMaximum(camera->sensor->getMaxCurrentIntegrationTime() * 100000000.0);
	ui->expSlider->setValue(camera->sensor->getIntegrationTime() * 100000000.0);
	ui->cmdWB->setEnabled(camera->getIsColor());

	const char * myfifo = "/var/run/bmsFifo";


	/* open, read, and display the message from the FIFO */
	bmsFifoFD = ::open(myfifo, O_RDONLY|O_NONBLOCK);

	sw = new StatusWindow;

	updateCurrentSettingsLabel();

	lastShutterButton = camera->ui->getShutterButton();
	lastRecording = camera->getIsRecording();
	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(on_MainWindowTimer()));
	timer->start(16);

	menuTimeoutTimer = new QTimer(this);
	connect(menuTimeoutTimer, SIGNAL(timeout()), this, SLOT(on_MainWindowTimeoutTimer()));

	if (appSettings.value("debug/hideDebug", true).toBool()) {
		ui->cmdDebugWnd->setVisible(false);
		ui->cmdClose->setVisible(false);
		ui->cmdDPCButton->setVisible(false);
	}
/*	ui->cmdFocusAid->setVisible(false);
	ui->cmdFPNCal->setVisible(false);
	ui->cmdIOSettings->setVisible(false);
	ui->cmdPlay->setVisible(false);
	ui->cmdRec->setVisible(false);
	ui->cmdRecSettings->setVisible(false);
	ui->cmdUtil->setVisible(false);

	ui->cmdWB->setVisible(false);
	ui->expSlider->setVisible(false);
	ui->lblCurrent->setVisible(false);
	ui->lblExp->setVisible(false);
*/

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

}

CamMainWindow::~CamMainWindow()
{
	timer->stop();
	::close(bmsFifoFD);
	delete sw;

	delete ui;
	if(camera->vinst->isRunning())
	{
		camera->vinst->setRunning(false);
	}
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
	}
	playbackWindow *w = new playbackWindow(NULL, camera);
	//w->camera = camera;
	w->setAttribute(Qt::WA_DeleteOnClose);
	w->show();
	//w->setGeometry(0, 0,w->width(), w->height());
}

void CamMainWindow::playFinishedSaving()
{
	qDebug("--- Play Finished ---");
	if (camera->get_autoRecord()) {
		if (camera->get_autoSave()) autoSaveActive = true;
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
	}
	else {
			//If there is unsaved video in RAM, prompt to start record
			QMessageBox::StandardButton reply;
			if(false == camera->recordingData.hasBeenSaved)	reply = QMessageBox::question(this, "Unsaved video in RAM", "Performing black calibration will erase the unsaved video in RAM. Continue?", QMessageBox::Yes|QMessageBox::No);
			else											reply = QMessageBox::question(this, "Start black calibration?", "Will start black calibration. Continue?", QMessageBox::Yes|QMessageBox::No);

			if(QMessageBox::Yes != reply)
				return;
	}
	sw->setText("Performing black calibration. Please wait.\r\nBeta Software: This will be much faster in a future software update");
	sw->show();
	QCoreApplication::processEvents();
	camera->autoFPNCorrection(16, true);
	sw->hide();
}

void CamMainWindow::on_cmdWB_clicked()
{

		QMessageBox::StandardButton reply;
		if(camera->getIsRecording()) reply = QMessageBox::question(this, "Stop recording?", "This action will stop recording and erase the video; is this okay?", QMessageBox::Yes|QMessageBox::No);
		else						 reply = QMessageBox::question(this, "Set white balance?", "Will set white balance. Continue?", QMessageBox::Yes|QMessageBox::No);

		if(QMessageBox::Yes != reply)
			return;
		autoSaveActive = false;
		camera->stopRecording();

	Int32 ret = camera->setWhiteBalance(camera->getImagerSettings().hRes / 2 & 0xFFFFFFFE,
							camera->getImagerSettings().vRes / 2 & 0xFFFFFFFE);	//Sample from middle but make sure position is a multiple of 2
	if(ret == CAMERA_CLIPPED_ERROR)
	{
		sw->setText("Clipping. Reduce exposure and try white balance again");
		sw->setTimeout(3000);
		sw->show();
	}
	else if(ret == CAMERA_LOW_SIGNAL_ERROR)
	{
		sw->setText("Too dark. Increase exposure and try white balance again");
		sw->setTimeout(3000);
		sw->show();
	}
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
			connect(w, SIGNAL(finishedSaving()),this, SLOT(playFinishedSaving()));
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
			if(qwl.count() <= 4)
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


void CamMainWindow::on_MainWindowTimeoutTimer()
{
	menuTimeoutTimer->stop();
/*	ui->cmdFocusAid->setVisible(false);
	ui->cmdFPNCal->setVisible(false);
	ui->cmdIOSettings->setVisible(false);
	ui->cmdPlay->setVisible(false);
	ui->cmdRec->setVisible(false);
	ui->cmdRecSettings->setVisible(false);
	ui->cmdUtil->setVisible(false);

	ui->cmdWB->setVisible(false);
	ui->expSlider->setVisible(false);
	ui->lblCurrent->setVisible(false);
	ui->lblExp->setVisible(false);*/
}

void CamMainWindow::on_cmdFocusAid_clicked()
{
	camera->setFocusPeakEnable(!focusAidEnabled);
	focusAidEnabled = !focusAidEnabled;
	if(focusAidEnabled)
	{
		ui->cmdFocusAid->setText("Focus\nAid (On)");
	}
	else
	{
		ui->cmdFocusAid->setText("Focus\nAid");
	}
}

void CamMainWindow::on_expSlider_sliderMoved(int position)
{
	camera->setIntegrationTime((double)position / 100000000.0, 0, 0, 0);
	updateCurrentSettingsLabel();
}



void CamMainWindow::recSettingsClosed()
{
	ui->expSlider->setMinimum(LUX1310_MIN_INT_TIME * 100000000.0);
	ui->expSlider->setMaximum(camera->sensor->getMaxCurrentIntegrationTime() * 100000000.0);
	ui->expSlider->setValue(camera->sensor->getIntegrationTime() * 100000000.0);
	updateCurrentSettingsLabel();
}

//Update the status textbox with the current settings
void CamMainWindow::updateCurrentSettingsLabel()
{
	char str[300];
	char battStr[50];
	char fpsString[30];
	char expString[30];
	getSIText(fpsString, 1.0 / camera->sensor->getCurrentFramePeriodDouble(), 4, DEF_SI_OPTS, 10);
	getSIText(expString, camera->sensor->getCurrentExposureDouble(), 4, DEF_SI_OPTS, 10);
	UInt32 expPercent = camera->sensor->getCurrentExposureDouble() * 100 / camera->sensor->getCurrentFramePeriodDouble();

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

	sprintf(str, "%s\r\n%ux%u %sfps\r\nExp %ss (%u%%)", battStr, camera->sensor->currentHRes, camera->sensor->currentVRes, fpsString, expString, expPercent);
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
}

void CamMainWindow::updateCamMainWindowPosition(){
	//qDebug()<<"windowpos old " << this->x();
	move(camera->ButtonsOnLeft? 0:600, 0);
	//qDebug()<<"windowpos new " << this->x();
}

void CamMainWindow::on_cmdBkGndButton_clicked()
{
	ui->cmdFocusAid->setVisible(true);
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
	menuTimeoutTimer->start(8000);
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

