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

#include "userInterface.h"
#include "mainwindow.h"
#include "playbackwindow.h"
#include "recsettingswindow.h"
#include "iosettingswindow.h"
#include "utilwindow.h"
#include "cammainwindow.h"
#include "whitebalancedialog.h"
#include "ui_cammainwindow.h"
#include "util.h"
#include "whitebalancedialog.h"

#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

extern "C" {
#include "siText.h"
}

#define DEF_SI_OPTS	SI_DELIM_SPACE | SI_SPACE_BEFORE_PREFIX

Camera * camera;
Video * vinst;
Control * cinst;
uint_fast8_t powerLoopCount = 0;

CamMainWindow::CamMainWindow(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::CamMainWindow)
{
	QSettings appSettings;
	CameraErrortype retVal;
	ui->setupUi(this);

	camera = new Camera();
	vinst = new Video();
	cinst = new Control();

	interface = new UserInterface();
	prompt = NULL;

	//loop until control dBus is ready
	UInt32 mem;
	while (cinst->getInt("cameraMemoryGB", &mem))
	{
		qDebug() << "Control API not present";
		struct timespec t = {1, 0};
		nanosleep(&t, NULL);
	}

	batteryPercent = 0;
	batteryVoltage = 0;
	batteryPresent = false;
	externalPower = false;

	interface->init();
	retVal = camera->init(vinst, cinst);

	if(retVal != SUCCESS)
	{
		QMessageBox msg;
		QString errText;

		errText.sprintf("Camera init failed, error %d: %s", (Int32)retVal, errorCodeString(retVal));
		msg.setText(errText);
		msg.exec();
	}
	ui->cmdWB->setEnabled(camera->getIsColor());
	ui->chkFocusAid->setChecked(camera->getFocusPeakEnable());

	sw = new StatusWindow;

	updateExpSliderLimits();
	updateCurrentSettingsLabel();

	lastShutterButton = interface->getShutterButton();
	recording = (camera->cinst->getProperty("state", "unknown").toString() == "recording");
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
		camera->startRecording();
	}
	if (camera->get_autoSave()) autoSaveActive = true;
	else                        autoSaveActive = false;


	if(camera->UpsideDownDisplay && camera->RotationArgumentIsSet()){
		camera->upsideDownTransform(2);//2 for upside down, 0 for normal
	} else  camera->UpsideDownDisplay = false;//if the rotation argument has not been added, this should be set to false

	//record the number of widgets that are open before any other windows can be opened
	QWidgetList qwl = QApplication::topLevelWidgets();
	windowsAlwaysOpen = qwl.count();

	cinst->listen("state", this, SLOT(on_state_valueChanged(const QVariant &)));
	cinst->listen("videoState", this, SLOT(on_videoState_valueChanged(const QVariant &)));
	cinst->listen("exposurePeriod", this, SLOT(on_exposurePeriod_valueChanged(const QVariant &)));
	cinst->listen("exposureMin", this, SLOT(on_exposureMin_valueChanged(const QVariant &)));
	cinst->listen("exposureMax", this, SLOT(on_exposureMax_valueChanged(const QVariant &)));
	cinst->listen("focusPeakingLevel", this, SLOT(on_focusPeakingLevel_valueChanged(const QVariant &)));

	cinst->getArray("colorMatrix", 9, (double *)&camera->colorCalMatrix);

	/* Go into live display after initialization */
	camera->setPlayMode(false);
}

CamMainWindow::~CamMainWindow()
{
	timer->stop();
	delete sw;

	delete ui;
	delete camera;
}

void CamMainWindow::on_videoState_valueChanged(const QVariant &value)
{
	qDebug() << "Receied new videoState => " << value.toString();
}

void CamMainWindow::on_exposurePeriod_valueChanged(const QVariant &value)
{
	apiUpdate = true;
	ui->expSlider->setValue(value.toInt());
	apiUpdate = false;
}

void CamMainWindow::on_exposureMax_valueChanged(const QVariant &value)
{
	apiUpdate = true;
	ui->expSlider->setMaximum(value.toInt());
	apiUpdate = false;
}

void CamMainWindow::on_exposureMin_valueChanged(const QVariant &value)
{
	apiUpdate = true;
	ui->expSlider->setMinimum(value.toInt());
	apiUpdate = false;
}

void CamMainWindow::on_focusPeakingLevel_valueChanged(const QVariant &value)
{
	apiUpdate = true;
	ui->chkFocusAid->setChecked(value.toDouble() > 0.0);
	apiUpdate = false;
}


QMessageBox::StandardButton
CamMainWindow::question(const QString &title, const QString &text, QMessageBox::StandardButtons buttons)
{
	QMessageBox::StandardButton reply;

	prompt = new QMessageBox(QMessageBox::Question, title, text, buttons, this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
	reply = (QMessageBox::StandardButton)prompt->exec();
	delete prompt;
	prompt = NULL;

	return reply;
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
	if (camera->liveSlowMotion)
	{
		if (camera->loopTimerEnabled)
		{
			ui->cmdRec->setText("Record");
			camera->stopLiveLoop();
		}
		else
		{
			ui->cmdRec->setText("End Loop");
			camera->startLiveLoop();
		}
	}
	else
	{

		if(recording)
		{
			camera->stopRecording();
		}
		else
		{
			//If there is unsaved video in RAM, prompt to start record.  unsavedWarnEnabled values: 0=always, 1=if not reviewed, 2=never
			if(false == camera->recordingData.hasBeenSaved && (0 != camera->unsavedWarnEnabled && (2 == camera->unsavedWarnEnabled || !camera->recordingData.hasBeenViewed)))
			{
				if(QMessageBox::Yes != question("Unsaved video in RAM", "Start recording anyway and discard the unsaved video in RAM?"))
					return;
			}

			camera->startRecording();
			if (camera->get_liveRecord()) vinst->liveRecord();
			if (camera->get_autoSave()) autoSaveActive = true;
		}
	}
}

void CamMainWindow::on_cmdPlay_clicked()
{
	if(recording) {
		if(QMessageBox::Yes != question("Stop recording?", "This action will stop recording; is this okay?"))
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
}

void CamMainWindow::playFinishedSaving()
{
	qDebug("--- Play Finished ---");
	if (camera->get_autoSave()) autoSaveActive = true;
	if (camera->get_autoRecord() && camera->autoRecord) {
		delayms(100);//delay needed or else the record may not always automatically start
		camera->startRecording();
		qDebug("--- started recording ---");
	}
}

bool CamMainWindow::okToStopLive()
{
	qDebug() << "LTE:" << camera->loopTimerEnabled;
	if(camera->loopTimerEnabled)
	{
		if(QMessageBox::Yes == question("Stop live slow motion?", "This action will stop live slow motion; is this okay?"))
		{
			camera->stopLiveLoop();
			return false;
		}
	}
	return true;
}

void CamMainWindow::on_cmdRecSettings_clicked()
{
	if (!okToStopLive()) return;

	if(recording) {
		if(QMessageBox::Yes != question("Stop recording?", "This action will stop recording; is this okay?"))
			return;
		autoSaveActive = false;
		camera->stopRecording();
	}

	RecSettingsWindow *w = new RecSettingsWindow(NULL, camera);
	connect(w, SIGNAL(settingsChanged()),this, SLOT(recSettingsClosed()));
	w->setAttribute(Qt::WA_DeleteOnClose);
	w->show();
}


void CamMainWindow::on_cmdFPNCal_clicked()//Black cal
{
	if (!okToStopLive()) return;

	if(recording) {
		if(QMessageBox::Yes != question("Stop recording?", "This action will stop recording and erase the video; is this okay?")) {
			return;
		}

		autoSaveActive = false;
		camera->stopRecording();
		delayms(100);
	}
	else if (false == camera->recordingData.hasBeenSaved) {
		/* Check for unsaved video before starting the recording. */
		UInt32 frames = 0;
		camera->cinst->getInt("totalFrames", &frames);
		if (frames > 0) {
			QMessageBox::StandardButton reply;
			if(false == camera->recordingData.hasBeenSaved)	reply = question("Unsaved video in RAM", "Performing black calibration will erase the unsaved video in RAM. Continue?");
			else											reply = question("Start black calibration?", "Will start black calibration. Continue?");

			if(QMessageBox::Yes != reply) {
				return;
			}
		}
	}

	sw->setText("Performing black calibration...");
	sw->show();
	QCoreApplication::processEvents();

	camera->cinst->startCalibration({"analogCal", "blackCal"}, true);

	sw->hide();
}

void CamMainWindow::on_cmdWB_clicked()
{
	if (!okToStopLive()) return;

	if(recording) {
		if(QMessageBox::Yes != question("Stop recording?", "This action will stop recording and erase the video; is this okay?"))
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
	if (!okToStopLive()) return;

	if(recording) {
		if(QMessageBox::Yes != question("Stop recording?", "This action will stop recording and erase the video; is this okay?"))
			return;
		autoSaveActive = false;
		camera->stopRecording();
	}
	IOSettingsWindow *w = new IOSettingsWindow(NULL, camera);
	//w->camera = camera;
	w->setAttribute(Qt::WA_DeleteOnClose);
	w->show();
}

void CamMainWindow::on_state_valueChanged(const QVariant &value)
{
	QString state = value.toString();
	qDebug() << "CamMainWindow state changed to" << state;
	if(!recording) {
		/* Check if recording has started. */
		if (state != "recording") return;
		recording = true;
		if (!camera->loopTimerEnabled)
		{
			ui->cmdRec->setText("Stop");
		}
	}
	else {
		/* Check if recording has ended. */
		if (state != "idle") return;
		recording = false;

		/* Recording has ended */
		if (camera->loopTimerEnabled)
		{
		buttonsEnabled(true);
			ui->cmdRec->setText("End Loop");
		}
		else
		{
			ui->cmdRec->setText("Record");
		}
		ui->cmdPlay->setEnabled(true);

		if(camera->get_autoSave() && autoSaveActive)
		{
			playbackWindow *w = new playbackWindow(NULL, camera, true);
			if(camera->get_autoRecord()) connect(w, SIGNAL(finishedSaving()),this, SLOT(playFinishedSaving()));
			//w->camera = camera;
			w->setAttribute(Qt::WA_DeleteOnClose);
			w->show();
		}

		if (prompt) {
			prompt->done(QMessageBox::Escape);
		}
	}
}


int cnt = 0; //temporary timer until pcUtil broadcasts power

void CamMainWindow::on_MainWindowTimer()
{
	bool shutterButton = interface->getShutterButton();
	QSettings appSettings;


	if(shutterButton && !lastShutterButton)
	{
		QWidgetList qwl = QApplication::topLevelWidgets();	//Hack to stop you from starting record when another window is open. Need to get modal dialogs working for proper fix
		if(qwl.count() <= windowsAlwaysOpen)				//Now that the numeric keypad has been added, there are four windows: cammainwindow, debug buttons window, and both keyboards
		{
			if (camera->liveSlowMotion)
			{
				if (camera->loopTimerEnabled)
				{
					ui->cmdRec->setText("Record");
					camera->stopLiveLoop();
				}
				else
				{
					buttonsEnabled(false);
					ui->cmdRec->setText("End Loop");
					camera->startLiveLoop();

				}
			}
			else
			{
				if(recording)
				{
					camera->stopRecording();
				}
				else
				{
					//If there is unsaved video in RAM, prompt to start record.  unsavedWarnEnabled values: 0=always, 1=if not reviewed, 2=never
					if(false == camera->recordingData.hasBeenSaved && (0 != camera->unsavedWarnEnabled && (2 == camera->unsavedWarnEnabled || !camera->recordingData.hasBeenViewed)) && false == camera->get_autoSave())	//If there is unsaved video in RAM, prompt to start record

					{
						if(QMessageBox::Yes == question("Unsaved video in RAM", "Start recording anyway and discard the unsaved video in RAM?"))
						{
							camera->startRecording();
							if (camera->get_liveRecord()) vinst->liveRecord();
						}
					}
					else
					{
						camera->startRecording();
						if (camera->get_liveRecord()) vinst->liveRecord();
						if (camera->get_autoSave()) autoSaveActive = true;
					}
				}
			}
		}
	}

	lastShutterButton = shutterButton;

	//Request battery information from the PMIC every two seconds (16ms * 125 loops)
	if(powerLoopCount == 125){
		updateBatteryData();
		powerLoopCount = 0;
	}
	powerLoopCount++;
	updateCurrentSettingsLabel();
	updateCurrentSettingsLabel();

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

	// hide Play button if in Live Slow Motion mode, to show the timer label under it
	// also gray out all buttons
	if (camera->liveSlowMotion && camera->loopTimerEnabled)
	{
		ui->cmdPlay->setVisible(false);

		struct timespec now;
		clock_gettime(CLOCK_MONOTONIC, &now);
		double loopCurrentTime = now.tv_sec + now.tv_nsec / 1e9;
		double timeRemaining = camera->liveLoopTime - loopCurrentTime + camera->loopStart - camera->liveLoopRecordTime;
		if (camera->liveOneShot)
		{
			//longer loop display; there is no recording
			timeRemaining += camera->liveLoopRecordTime;
		}
		if ((timeRemaining < 0.1) || camera->liveLoopRecording) timeRemaining = 0.0;  //prevents clock freezing at "0.05"
		ui->lblLiveTimer->setText(QString::number(timeRemaining, 'f', 2));

		buttonsEnabled(false);

		ui->cmdRec->setText("End Loop");
	}
	else
	{
		ui->cmdPlay->setVisible(true);
		buttonsEnabled(true);
	}
}

void CamMainWindow::on_newVideoSegment(VideoStatus *st)
{
	/* Flag that we have unsaved video. */
	qDebug("--- Sequencer --- Total recording size: %u", st->totalFrames);
	if (camera->recordingData.ignoreSegments) {
		camera->recordingData.ignoreSegments--;
	}
	else if (st->totalFrames) {
		camera->cinst->getImagerSettings(&camera->recordingData.is);
		camera->recordingData.valid = true;
		camera->recordingData.hasBeenSaved = false;
	}
}

void CamMainWindow::on_chkFocusAid_clicked(bool focusAidEnabled)
{
	if (apiUpdate) return;
	camera->setFocusPeakEnable(focusAidEnabled);
}

void CamMainWindow::on_expSlider_valueChanged(int exposure)
{
	/* The exposure slider moves too fast for the conventional D-Bus API
	 * to execute without lagging down the GUI significantly, so instead
	 * we will just send-and-forget the exposure change using JSON-RPC.
	 */
	static int sock = -1;

	char msg[256];
	int msglen;
	struct sockaddr_un addr;

	/* Do nothing if the slider moved because of an API update. */
	if (apiUpdate) return;

	/* Attempt to open a socket if not already done */
	if (sock < 0) sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock < 0) {
		qDebug("Failed to open socket: %s", strerror(errno));
		return;
	}

	/* Send the JSON-RPC request. */
	addr.sun_family = AF_UNIX;
	sprintf(addr.sun_path, "/tmp/camControl.sock");
	msglen = sprintf(msg, "{\"jsonrpc\": \"2.0\", \"method\": \"set\", \"params\": {\"exposurePeriod\": %d}}", exposure);
	sendto(sock, msg, msglen, 0, (const struct sockaddr *)&addr, sizeof(addr));
}

void CamMainWindow::recSettingsClosed()
{
	updateExpSliderLimits();
	updateCurrentSettingsLabel();
}

//Update the exposure slider limits and step size.
void CamMainWindow::updateExpSliderLimits()
{
	UInt32 clock = camera->getSensorInfo().timingClock;
	UInt32 expCurrent;
	UInt32 expMin;
	UInt32 expMax;

	cinst->getInt("framePeriod", &expSliderFramePeriod);
	cinst->getInt("exposurePeriod", &expCurrent);
	cinst->getInt("exposureMin", &expMin);
	cinst->getInt("exposureMax", &expMax);

	ui->expSlider->setMinimum(expMin);
	ui->expSlider->setMaximum(expMax);
	ui->expSlider->setValue(expCurrent);

	/* Do fine stepping in 1us increments */
	ui->expSlider->setSingleStep(clock / 1000000);
	ui->expSlider->setPageStep(clock / 1000000);
}

//Update the battery data.
void CamMainWindow::updateBatteryData()
{
	QStringList names = {
		"externalPower",
		"batteryPresent",
		"batteryVoltage",
		"batteryChargePercent"
	};
	QVariantMap battData = cinst->getPropertyGroup(names);

	if (!battData.isEmpty()) {
		externalPower = battData["externalPower"].toBool();
		batteryPresent = battData["batteryPresent"].toBool();
		batteryVoltage = battData["batteryVoltage"].toDouble();
		batteryPercent = battData["batteryChargePercent"].toDouble();
	}
}

//Update the status textbox with the current settings
void CamMainWindow::updateCurrentSettingsLabel()
{
	QSettings appSettings;
	UInt32 clock = camera->getSensorInfo().timingClock;

	camera->cinst->getImagerSettings(&is);

	double framePeriod = (double)is.period / clock;
	double expPeriod = (double)is.exposure / clock;

	char str[300];
	char battStr[50];
	char fpsString[30];
	char expString[30];
	char shString[30];

	sprintf(fpsString, QString::number(1 / framePeriod).toAscii());
	getSIText(expString, expPeriod, 4, DEF_SI_OPTS, 10);
	if (appSettings.value("camera/fractionalExposure", false).toBool()) {
		/* Show secondary exposure period as fractional time. */
		strcpy(shString, "1/");
		getSIText(shString+2, 1/expPeriod, 4, 0 /* no flags */, 0);
	} else {
		/* Show secondary exposure period as shutter angle. */
		int shutterAngle = (expPeriod * 360.0) / framePeriod;
		sprintf(shString, "%u\xb0", max(shutterAngle, 1)); /* Round up if less than zero degrees. */
	}

	if(batteryPresent)	//If battery present
	{
		sprintf(battStr, "Batt %d%% %.2fV", (UInt32)batteryPercent, batteryVoltage);
	}
	else
	{
		sprintf(battStr, "No Batt");
	}

	sprintf(str, "%s\r\n%ux%u %sfps\r\nExp %ss (%s)", battStr, is.geometry.hRes, is.geometry.vRes, fpsString, expString, shString);
	ui->lblCurrent->setText(str);
}

void CamMainWindow::on_cmdUtil_clicked()
{
	if (!okToStopLive()) return;

	if(recording) {
		if(QMessageBox::Yes != question("Stop recording?", "This action will stop recording and erase the video; is this okay?"))
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
	Int32 retVal = SUCCESS;
	int resultCount, resultMax;
	camera->setBncDriveLevel((1 << 1));	//Turn on output drive

	//retVal = camera->checkForDeadPixels(&resultCount, &resultMax);
	//TODO: add this function to Pychronos
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
	UInt32 expPeriod = ui->expSlider->value();
	UInt32 nextPeriod = expPeriod;
	UInt32 framePeriod = expSliderFramePeriod;
	int expAngle = (expPeriod * 360) / framePeriod;

	qDebug() << "framePeriod =" << framePeriod << " expPeriod =" << expPeriod;

	switch (ev->key()) {
	/* Up/Down moves the slider logarithmically */
	case Qt::Key_Up:
		if (expAngle > 0) {
			nextPeriod = (framePeriod * expSliderLog(expAngle, 1)) / 360;
		}
		if (nextPeriod < (expPeriod + ui->expSlider->singleStep())) {
			nextPeriod = expPeriod + ui->expSlider->singleStep();
		}
		ui->expSlider->setValue(nextPeriod);
		break;

	case Qt::Key_Down:
		if (expAngle > 0) {
			nextPeriod = (framePeriod * expSliderLog(expAngle, -1)) / 360;
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

void CamMainWindow::buttonsEnabled(bool en)
{
	ui->cmdUtil->setEnabled(en);
	ui->chkFocusAid->setEnabled(en);
	ui->cmdFPNCal->setEnabled(en);
	ui->cmdWB->setEnabled(en);
	ui->cmdIOSettings->setEnabled(en);
	ui->cmdRecSettings->setEnabled(en);
	QCoreApplication::processEvents();
}
