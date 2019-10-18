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
#include "cammainwindow.h"
#include "whitebalancedialog.h"
#include "ui_cammainwindow.h"
#include "util.h"
#include "whitebalancedialog.h"

extern "C" {
#include "siText.h"
}

#define DEF_SI_OPTS	SI_DELIM_SPACE | SI_SPACE_BEFORE_PREFIX

Camera * camera;
Video * vinst;
Control * cinst;
UserInterface * userInterface;
bool focusAidEnabled = false;
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

	userInterface = new UserInterface();
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

	userInterface->init();
	retVal = camera->init(vinst, cinst, userInterface, true);

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

	cinst->listen("videoState", this, SLOT(on_videoState_valueChanged(const QVariant &)));
	cinst->listen("exposurePeriod", this, SLOT(on_exposurePeriod_valueChanged(const QVariant &)));

	cinst->getArray("wbMatrix", 3, (double *)&camera->whiteBalMatrix);
	cinst->getArray("colorMatrix", 9, (double *)&camera->colorCalMatrix);
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
	/*
	 * TODO: We don't really need to update the limits if it was just the
	 * the exposure that changed. We should, however, update the limits
	 * if either exposureMin, exposureMax or framePeriod were changed.
	 *
	 * Maybe do ui->expSlider->setValue(value.toInt()); instead?
	 */
	updateExpSliderLimits();
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
	//temporary: API softReset call instead
	camera->cinst->doReset();
	return;
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
		if (camera->get_autoSave()) autoSaveActive = true;

	}

}

void CamMainWindow::on_cmdPlay_clicked()
{
	if(camera->getIsRecording()) {
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


void CamMainWindow::on_cmdRecSettings_clicked()
{
	if(camera->getIsRecording()) {
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
	if(camera->getIsRecording()) {
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

	bool calWaiting;
	do {
		QString state;
		cinst->getString("state", &state);
		calWaiting = state != "idle";

		struct timespec t = {0, 50000000};
		nanosleep(&t, NULL);

	} while (calWaiting);

	sw->hide();
}

void CamMainWindow::on_cmdWB_clicked()
{
	if(camera->getIsRecording()) {
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
	if(camera->getIsRecording()) {
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

void CamMainWindow::updateRecordingState(bool recording)
{
	if(recording)
	{
		ui->cmdRec->setText("Stop");

	}
	else	//Not recording
	{
		ui->cmdRec->setText("Record");
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
extern bool debugDbus;

void CamMainWindow::on_MainWindowTimer()
{
	bool shutterButton = camera->ui->getShutterButton();
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
				if(false == camera->recordingData.hasBeenSaved && (0 != camera->unsavedWarnEnabled && (2 == camera->unsavedWarnEnabled || !camera->recordingData.hasBeenViewed)) && false == camera->get_autoSave())	//If there is unsaved video in RAM, prompt to start record

				{
					if(QMessageBox::Yes == question("Unsaved video in RAM", "Start recording anyway and discard the unsaved video in RAM?"))
					{
						camera->startRecording();
					}
				}
				else
				{
					camera->startRecording();
					if (camera->get_autoSave()) autoSaveActive = true;
				}
			}
		}
	}

	bool recording = camera->getIsRecording();

	if(recording != lastRecording)
	{
		if(recording)
		{
			qDebug() << "### RECORDING:";
		}
		else
		{
			qDebug() << "### NOT RECORDING:";
		}
		updateRecordingState(recording);
		lastRecording = recording;
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
}

void CamMainWindow::on_newVideoSegment(VideoStatus *st)
{
	/* Flag that we have unsaved video. */
	qDebug("--- Sequencer --- Total recording size: %u", st->totalFrames);
	if (camera->recordingData.ignoreSegments) {
		camera->recordingData.ignoreSegments--;
	}
	else if (st->totalFrames) {
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
	UInt32 expClock = camera->getSensorInfo().timingClock;
	camera->setIntegrationTime((double)exposure / expClock, NULL, 0);
	updateCurrentSettingsLabel();
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
	ImagerSettings_t is = camera->getImagerSettings();
	UInt32 clock = camera->getSensorInfo().timingClock;
	UInt32 fPeriod;
	UInt32 ePeriod;

	camera->cinst->getInt("framePeriod", &fPeriod);
	camera->cinst->getInt("exposurePeriod", &ePeriod);

	double framePeriod = (double)fPeriod / clock;
	double expPeriod = (double)ePeriod / clock;
	int shutterAngle = (expPeriod * 360.0) / framePeriod;

	char str[300];
	char battStr[50];
	char fpsString[30];
	char expString[30];

	sprintf(fpsString, QString::number(1 / framePeriod).toAscii());
	getSIText(expString, expPeriod, 4, DEF_SI_OPTS, 10);
	shutterAngle = max(shutterAngle, 1); //to prevent 0 degrees from showing on the label if the current exposure is less than 1/360'th of the frame period.

	if(batteryPresent)	//If battery present
	{
		sprintf(battStr, "Batt %d%% %.2fV", (UInt32)batteryPercent, batteryVoltage);
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
