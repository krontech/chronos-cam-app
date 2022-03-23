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
#include <QFile>
#include <QTextStream>

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

#include <sys/mount.h>
#include <sys/vfs.h>
#include <sys/sendfile.h>
#include <mntent.h>
#include <sys/statvfs.h>
#include "defines.h"

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
    ui(new Ui::CamMainWindow),
    iface("ca.krontech.chronos.control", "/ca/krontech/chronos/control", QDBusConnection::systemBus())
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

    ui->battery->setVisible(false);
    ui->battLabel->setVisible(false);

    QPixmap pixmap(":/qss/assets/images/record.png");
    QIcon ButtonIcon(pixmap);
    ui->cmdRec->setIcon(ButtonIcon);

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
	ui->chkFocusAid->setChecked(camera->getFocusPeakEnable());

	/* Get the initial white balance */
	if (camera->getIsColor()) {
		ui->cmdWB->setEnabled(true);
		on_wbTemperature_valueChanged(camera->cinst->getProperty("wbTemperature"));
	} else {
		ui->cmdWB->setEnabled(false);
	}

	sw = new StatusWindow;

    inw = new IndicateWindow;
    inw->setRGInfoText("");
    inw->setTriggerText("");
    updateIndicateWindow();

	updateExpSliderLimits();
	updateCurrentSettingsLabel();

	lastShutterButton = interface->getShutterButton();
	recording = (camera->cinst->getProperty("state", "unknown").toString() == "recording");

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(on_MainWindowTimer()));
    timer->start(16);

    connect(camera->vinst, SIGNAL(newSegment(VideoStatus *)), this, SLOT(on_newVideoSegment(VideoStatus *)));

    // Call stopRecordingFromBtn function to set flag when finishedRecording signal is emitted
    connect(camera, SIGNAL(finishedRecording()), this, SLOT(stopRecordingFromBtn()));

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
	cinst->listen("wbTemperature", this, SLOT(on_wbTemperature_valueChanged(const QVariant &)));
    cinst->listen("wbTemperature", this, SLOT(on_wbTemperature_valueChanged(const QVariant &)));

	cinst->getArray("colorMatrix", 9, (double *)&camera->colorCalMatrix);

	/* Go into live display after initialization */
	camera->setPlayMode(false);

    mountTimer = new QTimer(this);
    connect(mountTimer, SIGNAL(timeout()), this, SLOT(checkForWebMount()));
    mountTimer->start(5000);

    QTimer::singleShot(500, this, SLOT(checkForCalibration())); // wait a moment, then check for calibration at startup
    QTimer::singleShot(15000, this, SLOT(checkForNfsStorage()));

    QDBusConnection conn = iface.connection();
    conn.connect("ca.krontech.chronos.control", "/ca/krontech/chronos/control", "ca.krontech.chronos.control",
                 "notify", this, SLOT(runTimer()));

    // Call saveNextSegment function to handle next step when video clip saving ends
    connect(camera->vinst, SIGNAL(ended(VideoState,QString)), this, SLOT(saveNextSegment(VideoState)));
}

CamMainWindow::~CamMainWindow()
{
	timer->stop();
	delete sw;
    delete inw;

	delete ui;
	delete camera;
}

void CamMainWindow::runTimer()
{
    VideoStatus st;
    vinst->getStatus(&st);

    if (st.state == VIDEO_STATE_LIVEDISPLAY)
    {
        timer->start();
    }
}

void CamMainWindow::stopTimer()
{
    timer->stop();
}

void CamMainWindow::checkForCalibration() // see if the camera has been calibrated or not
{
    QString modelName;

    //Only check for on-camera calibration if the camera is a 2.1
    camera->cinst->getString("cameraModel", &modelName);
    if(modelName.startsWith("CR21")){
        qDebug() << "Checking for onCamera calibration files";
        struct stat tempBuf;
         // check if calibration data is not already on the camera (if it has not been calibrated)
        if(stat ("/var/camera/cal/onCam_colGain_G1_WT66.bin", &tempBuf) != 0)
        {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(this, "Column gain calibration", "This camera has not been calibrated.\r\nWould you like to calibrate it now?\r\nNote: this will take ~5 minutes.", QMessageBox::Yes|QMessageBox::No);
            if(QMessageBox::Yes == reply) // yes
            {
                QMessageBox calibratingBox;
                calibratingBox.setText("\r\nComputing column gain calibration... (this should take ~5 minutes)");
                calibratingBox.setWindowTitle("On-Camera Calibration");
                calibratingBox.setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint); // on top, and no title-bar
                calibratingBox.setStandardButtons(0); // no buttons
                calibratingBox.setModal(true); // should not be able to click somewhere else
                calibratingBox.show(); // display the message (non-blocking)

                qDebug() << "### starting on-camera column gain calibration";
                camera->cinst->exportCalData();
                calibratingBox.close(); // close the message that displayed while running

                 // Notify on completion
                QMessageBox msg;
                msg.setText("Column gain calibration complete.");
                msg.setWindowTitle("On-Camera Calibration");
                msg.setWindowFlags(Qt::WindowStaysOnTopHint);
                msg.exec();
            }
        }
    }

}

void CamMainWindow::checkForNfsStorage()
{
    QSettings appSettings;

    if ((appSettings.value("network/nfsAddress").toString().length()) && (appSettings.value("network/nfsMount").toString().length())) {
        QMessageBox netConnBox;
        netConnBox.setWindowTitle("NFS Connection Status");
        netConnBox.setWindowFlags(Qt::WindowStaysOnTopHint); // on top
        netConnBox.setStandardButtons(QMessageBox::Ok);
        netConnBox.setModal(true); // should not be able to click somewhere else
        netConnBox.hide();

        umount2(NFS_STORAGE_MOUNT, MNT_DETACH);
        checkAndCreateDir(NFS_STORAGE_MOUNT);

        QCoreApplication::processEvents();
        if (!isReachable(appSettings.value("network/nfsAddress").toString())) {
            netConnBox.setText(appSettings.value("network/nfsAddress").toString() + " is not reachable!");
            netConnBox.show();
            netConnBox.exec();
            checkForSmbStorage();
            return;
        }

        QString mountString = buildNfsString();
        mountString.append(" 2>&1");
        QString returnString = runCommand(mountString.toLatin1());
        if (returnString != "") {
            netConnBox.setText("Mount failed: " + returnString);
            netConnBox.show();
            netConnBox.exec();
            checkForSmbStorage();
            return;
        }
        else {
            return;
        }
    }
    else {
        checkForSmbStorage();
    }
}

void CamMainWindow::checkForSmbStorage()
{
    QSettings appSettings;

    if ((appSettings.value("network/smbUser").toString().length()) && (appSettings.value("network/smbShare").toString().length()) && (appSettings.value("network/smbPassword").toString().length())) {
        QMessageBox netConnBox;
        netConnBox.setWindowTitle("SMB Connection Status");
        netConnBox.setWindowFlags(Qt::WindowStaysOnTopHint); // on top
        netConnBox.setStandardButtons(QMessageBox::Ok);
        netConnBox.setModal(true); // should not be able to click somewhere else
        netConnBox.hide();

        umount2(SMB_STORAGE_MOUNT, MNT_DETACH);
        checkAndCreateDir(SMB_STORAGE_MOUNT);

        QCoreApplication::processEvents();
        QString smbServer = parseSambaServer(appSettings.value("network/smbShare").toString());
        if (!isReachable(smbServer)) {
            netConnBox.setText(smbServer + " is not reachable!");
            netConnBox.show();
            netConnBox.exec();
            return;
        }

        QString mountString = buildSambaString();
        mountString.append(" 2>&1");
        QString returnString = runCommand(mountString.toLatin1());
        if (returnString != "") {
            netConnBox.setText("Mount failed: " + returnString);
            netConnBox.show();
            netConnBox.exec();
            return;
        }
    }
}

void CamMainWindow::checkForWebMount()
{
    QSettings appSettings;

    QFile nfsFile("/var/camera/webNfsMount.txt");
    if (nfsFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
        QTextStream nfsFileStream(&nfsFile);

        while(!nfsFileStream.atEnd()) {
            QString line = nfsFileStream.readLine();
            if (line == "") {
                appSettings.setValue("network/nfsAddress", "");
                appSettings.setValue("network/nfsMount", "");
            }
            QStringList mountInfo = line.split(" ");
            if (mountInfo.length() == 2) {
                if ((mountInfo[0] != appSettings.value("network/nfsAddress").toString()) ||
                    (mountInfo[1] != appSettings.value("network/nfsMount").toString())) {
                    appSettings.setValue("network/nfsAddress", mountInfo[0]);
                    appSettings.setValue("network/nfsMount", mountInfo[1]);
                }
            }
        }
    }
    nfsFile.close();

    QFile smbFile("/var/camera/webSmbMount.txt");
    if (smbFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
        QTextStream smbFileStream(&smbFile);

        while(!smbFileStream.atEnd()) {
            QString line = smbFileStream.readLine();
            if (line == "") {
                appSettings.setValue("network/smbShare", "");
                appSettings.setValue("network/smbUser", "");
                appSettings.setValue("network/smbPassword", "");
            }
            QStringList mountInfo = line.split(" ");
            if (mountInfo.length() == 3) {
                if ((mountInfo[0] != appSettings.value("network/smbShare").toString())  ||
                    (mountInfo[1] != appSettings.value("network/smbUser").toString()) ||
                    (mountInfo[2] != appSettings.value("network/smbPassword").toString())) {
                    appSettings.setValue("network/smbShare", mountInfo[0]);
                    appSettings.setValue("network/smbUser", mountInfo[1]);
                    appSettings.setValue("network/smbPassword", mountInfo[2]);
                }
            }
        }
    }
    smbFile.close();
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
    updateCurrentSettingsLabel();
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

void CamMainWindow::on_wbTemperature_valueChanged(const QVariant &value)
{
	int wbTempK = value.toInt();

	if (!ui->cmdWB->isEnabled()) return; /* Do nothing on monochrome cameras */
	if (wbTempK > 0) {
		ui->cmdWB->setText(QString("White Bal\n%1\xb0K").arg(wbTempK));
	} else {
		ui->cmdWB->setText("White Bal\nCustom");
	}
}

void CamMainWindow::on_rsResolution_valueChanged(const QVariant &value)
{
    int wbResolution = value.toInt();
    ui->cmdRecSettings->setText(QString("Rec Settings\n%1\xb0K").arg(wbResolution));

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
    stopTimer();

	if(recording) {
		if(QMessageBox::Yes != question("Stop recording?", "This action will stop recording; is this okay?"))
			return;
		autoSaveActive = false;
		camera->stopRecording();
		delayms(100);
	}

	createNewPlaybackWindow();
    inw->hide();
}

void CamMainWindow::createNewPlaybackWindow(){
	playbackWindow *w = new playbackWindow(NULL, camera);
	if(camera->get_autoRecord()) connect(w, SIGNAL(finishedSaving()),this, SLOT(playFinishedSaving()));
	//w->camera = camera;
	w->setAttribute(Qt::WA_DeleteOnClose);
	w->show();
    // Send signal when aborting save from Playback to clean all RUn-N-Gun parameters
    connect(w, SIGNAL(abortSave()), this, SLOT(abortRunGunSave()));
    connect(w, SIGNAL(finishedSaving()), this, SLOT(updateIndicateWindow()));
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

void CamMainWindow::updateIndicateWindow()
{
    camera->cinst->getImagerSettings(&is);

    if (camera->liveSlowMotion) {
        is.mode = RECORD_MODE_LIVE;
    }

    switch (is.mode) {
        case RECORD_MODE_NORMAL:
            inw->setRecModeText("Normal");
        break;

        case RECORD_MODE_SEGMENTED:
           inw->setRecModeText("Segmented");
           if(camera->get_runngun()) {
               inw->setRecModeText("Run-N-Gun");
           }
        break;

        case RECORD_MODE_GATED_BURST:
            inw->setRecModeText("Gated Burst");
        break;

        case RECORD_MODE_LIVE:
            inw->setRecModeText("Live Slow Motion");
        break;

        case RECORD_MODE_FPN:
        default:
            /* Internal recording modes only. */
        break;
    }

    inw->show();
    inw->lower();
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

            QPixmap pixmap(":/qss/assets/images/stop.png");
            QIcon ButtonIcon(pixmap);
            ui->cmdRec->setIcon(ButtonIcon);
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
            QPixmap pixmap(":/qss/assets/images/record.png");
            QIcon ButtonIcon(pixmap);
            ui->cmdRec->setIcon(ButtonIcon);
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

        /*------ Run and Gun Mode ------*/
        if (camera->get_runngun()) {
            int segCount = camera->recordingData.is.segments;
            formatForRunGun = getSaveFormatForRunGun();
            realBitrateForRunGun = getBitrateForRunGun(formatForRunGun);
            inw->setTriggerText("trigger: on");

            /* Clear unsaved recordings */
            if (clearFlag) {
                clearFlag = false;
                return;
            }
            else {
                /* ring buffer is not full */
                if (nextSegments.size() < camera->recordingData.is.segments) {
                    nextSegments.insert(startFrame+1, st->totalFrames - startFrame);
                    totalSegCount++;
                    startFrame = st->totalFrames;
                }
                /* ring buffter is full, starting overwriting */
                else {
                    /* When new segment causes overwriting while another segment is being saved */
                    if (camera->vinst->getStatus(NULL) == VIDEO_STATE_FILESAVE) {
                        // Set stopCurrentSeg to TRUE for saveNextSegment function
                        stopCurrentSeg = true;
                        // Mark the end position of this partial clip
                        // TO DO: some delay -3 ?
                        int currentPosition = camera->vinst->getPosition();

                        // stopRecording -> end current segment saving -> ended function -> saveNextSegment function
                        CameraErrortype result = camera->vinst->stopRecording();
                        if (result == SUCCESS) {
                            qDebug() << "End saving of current segment";
                        }

                        /* Calculate new starting position and length for another part of the segment */
                        newStart = currentPosition + 1 - nextSegments.begin().value();
                        int savedSegLength = currentPosition - currentSavingSeg.begin().key() + 1;
                        newSegLength = currentSavingSeg.begin().value() - savedSegLength;

                        qDebug() << "current saved position: " << camera->vinst->getPosition();
                        qDebug() << "current saved length: " << savedSegLength;
                        qDebug() << "new start position: " << newStart;
                        qDebug() << "new length: " << newSegLength;
                    }

                    // Mark the length of the first segment
                    int firstSegLenght = nextSegments.begin().value();
                    // Remove the first segment
                    nextSegments.erase(nextSegments.begin());

                    /* Move forward all rest segments by one length of the first segment */
                    QMap<int, int> temp = {};
                    QMap<int, int>::iterator it;
                    for (it = nextSegments.begin(); it != nextSegments.end(); it++) {
                        int tempKey = it.key() - firstSegLenght;
                        int tempValue = it.value();
                        temp.insert(tempKey, tempValue);
                    }
                    nextSegments = temp;

                    /* Add the new segment */
                    it = nextSegments.end() - 1;
                    startFrame = it.key() + it.value();
                    nextSegments.insert(startFrame, st->totalFrames - startFrame + 1);
                    totalSegCount++;
                }
                qDebug() << "segments in list: " << nextSegments;

                /* Start new saving */
                if (camera->vinst->getStatus(NULL) != VIDEO_STATE_FILESAVE) {
                    /* Start first segment saving in whole recording */
                    if (nextSegments.size() == 1) {
                        qDebug() << "start saving for the first segment";
                        QMap<int, int>::iterator it = nextSegments.begin();
                        cinst->saveRecording(it.key(), it.value(), formatForRunGun, vinst->framerate, realBitrateForRunGun);
                    }
                    /* Re-start saving for the newest segment */
                    /* All exisiting segments finished saving before confirmed (by Record End Trigger) this new one */
                    /* saveNextSegment function cannot start saving for this new segment because it would only be called when the last segment saving is done */
                    else {
                        qDebug() << "new segment is done recording and start saving it";
                        QMap<int, int>::iterator it = nextSegments.end() - 1;
                        cinst->saveRecording(it.key(), it.value(), formatForRunGun, vinst->framerate, realBitrateForRunGun);
                    }
                    savedSegCount++;
                }

                /* New trigger will end current segment and start a new segment */
                /* This new segment will overwrite an old segment in ring buffer once it is ended */
                /* If this old segment is still being saved, then we should: */
                /* Disbale new Record End Trigger from IOs, but should allow stopRecording from buttons (physical or GUI) to finish whole recording */
                /* Stopping whole recording will end current segment without starting a new segment */
                int currentRecordingSeg = totalSegCount + 1;

                /* If the new segment (a new segment is done not started) signal is from buttons */
                if (stopFromBtn) {
                    qDebug() << "End whole recording";
                    inw->setTriggerText("");
                }
                /* If the new segment (a new segment is done not started) signal is from IOs */
                else {
                    /* The old segment that will be overwritten is still being saved */
                    if ((currentRecordingSeg + 1 - segCount) == savedSegCount) {
                        qDebug() << "Ignore trigger from IOs until segment#" << savedSegCount << "saving is done";
                        /* Disbale triggers from IOs */
                        // Save current settings of IO trigger
                        QVariant ioMappingTrigger = camera->cinst->getProperty("ioMappingTrigger");
                        ioMappingTrigger.value<QDBusArgument>() >> triggerConfig;
                        qDebug() << triggerConfig["invert"];

                        // Set IO trigger to 'alwaysHigh' -> disable trigger
                        qDebug() << "Disable Record End Trigger from IOs";
                        QVariantMap temp;

                        if (triggerConfig["invert"] == true) {
                            temp.insert("source", QVariant("none"));
                        }
                        else {
                            temp.insert("source", QVariant("alwaysHigh"));
                        }

                        camera->cinst->setProperty("ioMappingTrigger", temp);

                        inw->setTriggerText("trigger: off");
                    }
                }
            }
            QString runGunInfo = "total: " + QString::number(totalSegCount) + "  saving: seg#" + QString::number(savedSegCount);
            inw->setRGInfoText(runGunInfo);
        }
	}
}

void CamMainWindow::saveNextSegment(VideoState state)
{
    /* Segment is ended manually because new overwritten segment is confirmed while another segment is being saved */
    if (stopCurrentSeg == true) {
        qDebug() << "Stop current saving";
        stopCurrentSeg = false;
        // save the rest of the current segment
        cinst->saveRecording(newStart, newSegLength, formatForRunGun, vinst->framerate, realBitrateForRunGun);
        return;
    }

    /* When the last segment saving just ends -> start saving of the next segment if there is any unsaved segment */
    /*                                           end whole saving and clear all parameters */
    /*                                           wait for new segment to be added */
    if ((state == VIDEO_STATE_FILESAVE) && (nextSegments.size() >= 1))
    {
        qDebug() << "total number of segments: " << totalSegCount;
        qDebug() << "number of saved segments: " << savedSegCount;

        QMap<int, int>::iterator it;
        int start = 0;
        int segLength = 0;

        /* All exisiting segments in segment list finished saving */
        if (savedSegCount == totalSegCount) {
            currentSavingSeg = {};
            /* Whole recording is finished */
            if (camera->cinst->getProperty("state", "unknown").toString() != "recording") {
                startFrame = 0;
                totalSegCount = 0;
                savedSegCount = 0;
                nextSegments = {};
                triggerConfig = {};

                clearFlag = true;
                stopFromBtn = false;
                camera->recordingData.hasBeenSaved = true;

                inw->setRGInfoText("");
                inw->setTriggerText("");
            }
            /* Recording is still running */
            /* Current exisiting segments are all saved, but recording is not stopped*/
            else {
                /* waiting for next trigger to add new segment */
                qDebug() << "waiting for next new segment";
                /* Re-enable Record End Trigger from IOs if it is diabled now */
                if (triggerConfig.size() != 0) {
                    qDebug() << "Record End Trigger from IOs is disabled";
                    qDebug() << "Reset ioMappingTrigger to enable";

                    camera->cinst->setProperty("ioMappingTrigger", triggerConfig);
                    triggerConfig = {};

                    inw->setTriggerText("trigger: on");
                }

                QString runGunInfo = "total: " + QString::number(totalSegCount) + "  saving: none";
                inw->setRGInfoText(runGunInfo);

                return;
            }
        }
        /* Have unsaved segments in segment list */
        else {
            /* Overwriting in Ring Buffer already happened */
            if (totalSegCount > camera->recordingData.is.segments) {
                // Choose the next unsaved segment
                it = nextSegments.begin() + (savedSegCount - (totalSegCount - camera->recordingData.is.segments));
                start = it.key();
                segLength = it.value();

                // Save parameters for interruption by overwritting
                currentSavingSeg.clear();
                currentSavingSeg.insert(start, segLength);

                cinst->saveRecording(start, segLength, formatForRunGun, vinst->framerate, realBitrateForRunGun);
                savedSegCount++;
            }
            /* Overwriting hasn't happened */
            else {
                // Read parameters for next unsaved segment
                it = nextSegments.begin() + savedSegCount;
                start = it.key();
                segLength = it.value();

                // Save parameters for interruption by overwritting
                currentSavingSeg.clear();
                currentSavingSeg.insert(start, segLength);

                cinst->saveRecording(start, segLength, formatForRunGun, vinst->framerate, realBitrateForRunGun);
                savedSegCount++;
            }
            QString runGunInfo = "total: " + QString::number(totalSegCount) + "  saving: seg#" + QString::number(savedSegCount);
            inw->setRGInfoText(runGunInfo);
        }

        /* Re-enable Record End Trigger from IOs if it is disabled now */
        if (triggerConfig.size() != 0) {
            qDebug() << "Record End Trigger from IOs is disabled";
            qDebug() << "Reset ioMappingTrigger to enable";

            camera->cinst->setProperty("ioMappingTrigger", triggerConfig);
            triggerConfig = {};

            if (!stopFromBtn) {
                inw->setTriggerText("trigger: on");
            }
        }
    }
}

/* Get the save format from QSettings <- Playback screen Setting window */
save_mode_type CamMainWindow::getSaveFormatForRunGun()
{
    QSettings appSettings;

    switch (appSettings.value("recorder/saveFormat", SAVE_MODE_H264).toUInt()) {
    case 0:  return SAVE_MODE_H264;
    case 1:  return SAVE_MODE_RAW16;
    case 2:  return SAVE_MODE_RAW12;
    case 3:  return SAVE_MODE_DNG;
    case 4:  return SAVE_MODE_TIFF;
    case 5:  return SAVE_MODE_TIFF_RAW;
    default: return SAVE_MODE_H264;
    }
}

/* Calculate biterate for Run-N-Gun Mode */
UInt32 CamMainWindow::getBitrateForRunGun(save_mode_type format)
{
    QSettings appSettings;
    FrameGeometry frame;
    double bpp = appSettings.value("recorder/bitsPerPixel", camera->vinst->bitsPerPixel).toDouble();

    /* Estimate bits per pixel, by format. */
    switch(format) {
    case SAVE_MODE_H264:
        bpp *= 1.2;	/* Add a fudge factor. */
        break;
    case SAVE_MODE_DNG:
    case SAVE_MODE_TIFF_RAW:
        //fileOverMaxSize = false;
        //fileOverhead *= numFrames;
    case SAVE_MODE_RAW16:
        bpp = 16;
        break;
    case SAVE_MODE_RAW12:
        bpp = 12;
        break;
    case SAVE_MODE_TIFF:
        //fileOverMaxSize = false;
        //fileOverhead *= numFrames;
        if (camera->getIsColor()) {
            bpp = 24;
        } else {
            bpp = 8;
        }
        break;

    default:
        bpp = 16;
        break;
    }

    UInt32 bppBitrate = camera->vinst->bitsPerPixel * frame.pixels() * camera->vinst->framerate;
    UInt32 realBitrate = min(bppBitrate, min(60000000, (UInt32)(camera->vinst->maxBitrate * 1000000.0)));

    return realBitrate;
}

/* End Run-N-Gun saving by 'abort save' from Playback */
/* Clean all parameters */
void CamMainWindow::abortRunGunSave()
{
    qDebug() << "Abort Run-N-Gun Save";

    startFrame = 0;
    totalSegCount = 0;
    savedSegCount = 0;
    nextSegments = {};

    clearFlag = true;
    stopFromBtn = false;
    camera->recordingData.hasBeenSaved = true;

    inw->setRGInfoText("");

    if (triggerConfig.size() != 0) {
        qDebug() << "Record End Trigger from IOs is disabled";
        qDebug() << "Reset ioMappingTrigger to enable";

        camera->cinst->setProperty("ioMappingTrigger", triggerConfig);
        triggerConfig = {};
    }
    return;
}

/* Mark flag when stop whole recording from physical or GUI button */
void CamMainWindow::stopRecordingFromBtn()
{
    qDebug() << "Stop whole recording";
    stopFromBtn = true;
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
    updateIndicateWindow();
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
    UInt32 expMax;

	camera->cinst->getImagerSettings(&is);
    cinst->getInt("exposureMax", &expMax);

	double framePeriod = (double)is.period / clock;
    double expPeriod = (double)is.exposure / clock;

    int expPercent = (double)is.exposure / (double)expMax * 100;
    //qDebug() << "expPercent =" << expPercent << " expPeriod =" << expPeriod;

    char str[300];
    //char maxString[100];
	char battStr[50];
	char fpsString[30];
	char expString[30];
	char shString[30];

    int exp = appSettings.value("camera/expLabel", 1).toInt();

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
        ui->battery->setVisible(true);
        ui->VolLabel->setVisible(true);
        ui->battLabel->setVisible(false);

        sprintf(battStr, "%.2fV", batteryVoltage);
        ui->VolLabel->setText(battStr);

        if (batteryPercent >= 30)
        {
            ui->battery->setStyleSheet("QProgressBar::chunk {background-color: #00ff00;}");
        }
        if (batteryPercent < 30 && batteryPercent > 10)
        {
            ui->battery->setStyleSheet("QProgressBar::chunk {background-color: orange;}");
        }
        if (batteryPercent <= 10)
        {
            ui->battery->setStyleSheet("QProgressBar::chunk {background-color: red;}");
        }
        ui->battery->setValue(batteryPercent);

	}
	else
	{
        ui->battLabel->setVisible(true);
        ui->battery->setVisible(false);
        ui->VolLabel->setVisible(false);

		sprintf(battStr, "No Batt");
        ui->battLabel->setText(battStr);
	}


    if (exp == 0)
    {
        sprintf(str, "%ux%u %sfps\r\nExposure %ss", is.geometry.hRes, is.geometry.vRes, fpsString, expString);
    }
    else if (exp == 1)
    {
        sprintf(str, "%ux%u %sfps\r\nExposure %s", is.geometry.hRes, is.geometry.vRes, fpsString, shString);
    }
    else
    {
        sprintf(str, "%ux%u %sfps\r\nExposure %d%%", is.geometry.hRes, is.geometry.vRes, fpsString, expPercent); //%d%%
    }

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
    ui->VolLabel->setVisible(true);
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
    UInt32 expPeriod = ui->expSlider->value(); //set expPeriod (read from expSlider value)
	UInt32 nextPeriod = expPeriod;
	UInt32 framePeriod = expSliderFramePeriod;
    int expAngle = (expPeriod * 360) / framePeriod; //minimum is 0
    qDebug() << "framePeriod =" << framePeriod << " expPeriod =" << expPeriod;

    //expSlider has two cases (key up / key down -> exposure increase / derease)
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

        updateCurrentSettingsLabel();
		break;

	case Qt::Key_Down:
		if (expAngle > 0) {
			nextPeriod = (framePeriod * expSliderLog(expAngle, -1)) / 360;
		}
		if (nextPeriod > (expPeriod - ui->expSlider->singleStep())) {
			nextPeriod = expPeriod - ui->expSlider->singleStep();
		}
		ui->expSlider->setValue(nextPeriod);

        updateCurrentSettingsLabel();
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
