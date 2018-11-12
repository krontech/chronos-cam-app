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
#include <time.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/vfs.h>

#include "util.h"
#include "camera.h"

#include "savesettingswindow.h"
#include "playbackwindow.h"
#include "ui_playbackwindow.h"

#include <QTimer>
#include <QMessageBox>
#include <QSettings>
#include <QKeyEvent>

#define USE_AUTONAME_FOR_SAVE ""
#define MIN_FREE_SPACE 20000000

playbackWindow::playbackWindow(QWidget *parent, Camera * cameraInst, bool autosave) :
	QWidget(parent),
	ui(new Ui::playbackWindow)
{
	QSettings appSettings;
	VideoStatus vStatus;
	ui->setupUi(this);
	this->setWindowFlags(Qt::Dialog /*| Qt::WindowStaysOnTopHint*/ | Qt::FramelessWindowHint);

	camera = cameraInst;
	autoSaveFlag = autosave;
	autoRecordFlag = camera->get_autoRecord();
	this->move(camera->ButtonsOnLeft? 0:600, 0);
	saveAborted = false;
	saveAbortedAutomatically = false;
	
	camera->vinst->getStatus(&vStatus);
	playFrame = 0;
	playLoop = false;
	totalFrames = vStatus.totalFrames;

	sw = new StatusWindow;

	connect(ui->cmdClose, SIGNAL(clicked()), this, SLOT(close()));

	ui->verticalSlider->setMinimum(0);
	ui->verticalSlider->setMaximum(totalFrames - 1);
	ui->verticalSlider->setValue(playFrame);
	ui->cmdLoop->setVisible(appSettings.value("camera/demoMode", false).toBool());
	markInFrame = 1;
	markOutFrame = totalFrames;
	ui->verticalSlider->setHighlightRegion(markInFrame, markOutFrame);
	ui->verticalSlider->setFocusProxy(this);

	camera->setPlayMode(true);
	camera->vinst->setPosition(0, 0);
	connect(camera->vinst, SIGNAL(started(VideoState)), this, SLOT(videoStarted(VideoState)));
	connect(camera->vinst, SIGNAL(ended(VideoState, QString)), this, SLOT(videoEnded(VideoState, QString)));

	playbackExponent = 0;
	updatePlayRateLabel();

	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(updatePlayFrame()));
	timer->start(30);

	updateStatusText();

	settingsWindowIsOpen = false;

	if(autoSaveFlag) {
		strcpy(camera->vinst->filename, USE_AUTONAME_FOR_SAVE);

		on_cmdSave_clicked();
	} else {
		strcpy(camera->vinst->filename, appSettings.value("recorder/filename", "").toString().toAscii());
	}
	
	if(camera->vinst->getOverlayStatus())	camera->vinst->setOverlay("%.6h/%.6z Sg=%g/%i T=%.8Ss");
}

playbackWindow::~playbackWindow()
{
	qDebug()<<"playbackwindow deconstructor";
	camera->setPlayMode(false);
	timer->stop();
	emit finishedSaving();
	delete sw;
	delete ui;
}

void playbackWindow::videoStarted(VideoState state)
{
	qDebug()<<"videoStarted. status:" <<camera->vinst->getStatus(NULL);
	ui->cmdSave->setEnabled(true);

	/* When starting a filesave, increase the frame timing for maximum speed */
	if (state == VIDEO_STATE_FILESAVE) {
		qDebug()<<"state == VIDEO_STATE_FILESAVE";
		camera->recordingData.hasBeenSaved = true;
		camera->sensor->seqOnOff(false); /* Disable the sensor to reduce RAM contention */
		camera->setDisplaySettings(true, MAX_RECORD_FRAMERATE);
		ui->cmdSave->setText("Abort\nSave");
		saveDoneTimer = new QTimer(this);
		connect(saveDoneTimer, SIGNAL(timeout()), this, SLOT(checkForSaveDone()));
		saveDoneTimer->start(1000);
		saveAbortedAutomatically = false;

		/* Prevent the user from pressing the abort/save button just after the last frame,
		 * as that can make the camera try to save a 2nd video too soon, crashing the camapp.
		 * It is also disabled in checkForSaveDone(), but if the video is very short,
		 * that might not be called at all before the end of the video, so just disable the button right away.*/
		if(markOutFrame - markInFrame < 25) ui->cmdSave->setEnabled(false);
		else ui->cmdSave->setEnabled(true);
		
	} else {
		qDebug()<<"state != VIDEO_STATE_FILESAVE";
		ui->cmdSave->setText("Save");
		ui->cmdSave->setEnabled(true);
		setControlEnable(true);
		emit enableSaveSettingsButtons(true);
	}
	qDebug()<<"videoStarted() - function finished";
	/* TODO: Other start events might occur on HDMI hotplugs. */
}

void playbackWindow::videoEnded(VideoState state, QString err)
{
	qDebug()<<"videoEnded. status:" <<camera->vinst->getStatus(NULL);
	if (state == VIDEO_STATE_FILESAVE) { //Filesave has just ended
		QMessageBox msg;

		/* When ending a filesave, restart the sensor and return to live display timing. */
		camera->sensor->seqOnOff(true);
		camera->setDisplaySettings(false, MAX_LIVE_FRAMERATE);

		/* If recording failed from an error. Tell the user about it. */
		if (!err.isNull()) {
			msg.setText("Recording Failed: " + err);
			msg.exec();
			return;
		}
		
		sw->close();
		ui->cmdSave->setText("Save");
		saveAborted = false;
		autoSaveFlag = false;
		updatePlayRateLabel();
		ui->verticalSlider->setHighlightRegion(markInFrame, markOutFrame);
		
		if(saveDoneTimer){
			saveDoneTimer->stop();
			delete saveDoneTimer;
			qDebug()<<"saveDoneTimer deleted";
		} else qDebug("cannot delete saveDoneTimer because it is null");
		
		if(autoRecordFlag) {
			qDebug()<<"autoRecordFlag is true.  closing";
			emit finishedSaving();
			delete this;
		}
	}
}

void playbackWindow::on_verticalSlider_sliderMoved(int position)
{
	/* Note that a rate of zero will also pause playback. */
	stopPlayLoop();
	camera->vinst->setPosition(position, 0);
}

void playbackWindow::on_verticalSlider_valueChanged(int value)
{

}

void playbackWindow::on_cmdPlayForward_pressed()
{
	int fps = (playbackExponent >= 0) ? (60 << playbackExponent) : 60 / (1 - playbackExponent);
	stopPlayLoop();
	camera->vinst->setPlayback(fps);
}

void playbackWindow::on_cmdPlayForward_released()
{
	stopPlayLoop();
	camera->vinst->setPlayback(0);
}

void playbackWindow::on_cmdPlayReverse_pressed()
{
	int fps = (playbackExponent >= 0) ? (60 << playbackExponent) : 60 / (1 - playbackExponent);
	stopPlayLoop();
	camera->vinst->setPlayback(-fps);
}

void playbackWindow::on_cmdPlayReverse_released()
{
	stopPlayLoop();
	camera->vinst->setPlayback(0);
}

void playbackWindow::on_cmdSave_clicked()
{
	UInt32 ret;
	QMessageBox msg;
	char parentPath[1000];
	struct statfs statfsBuf;
	uint64_t estimatedSize;
	QSettings appSettings;
	
	autoRecordFlag = camera->autoRecord = camera->get_autoRecord();

	//Build the parent path of the save directory, to determine if it's a mount point
	strcpy(parentPath, camera->vinst->fileDirectory);
	strcat(parentPath, "/..");

	if(camera->vinst->getStatus(NULL) != VIDEO_STATE_FILESAVE)
	{
		save_mode_type format = getSaveFormat();
		UInt32 hRes = appSettings.value("camera/hRes", MAX_FRAME_SIZE_H).toInt();
		UInt32 vRes = appSettings.value("camera/vRes", MAX_FRAME_SIZE_V).toInt();

		//If no directory set, complain to the user
		if(strlen(camera->vinst->fileDirectory) == 0)
		{
			msg.setText("No save location set! Set save location in Settings");
			msg.exec();
			return;
		}

		if (!statfs(camera->vinst->fileDirectory, &statfsBuf)) {
			uint64_t freeSpace = statfsBuf.f_bsize * (uint64_t)statfsBuf.f_bfree;
			bool fileOverMaxSize = (statfsBuf.f_type == 0x4d44); // Check for file size limits for FAT32 only.

			// calculate estimated file size
			estimatedSize = (markOutFrame - markInFrame + 3) * (hRes * vRes);
			switch(format) {
			case SAVE_MODE_H264:
				// the *1.2 part is fudge factor
				estimatedSize = (uint64_t) ((double)estimatedSize * appSettings.value("recorder/bitsPerPixel", camera->vinst->bitsPerPixel).toDouble() * 1.2);
				qDebug("Bits/pixel: %0.3f", appSettings.value("recorder/bitsPerPixel", camera->vinst->bitsPerPixel).toDouble());
				break;
			case SAVE_MODE_DNG:
				fileOverMaxSize = false;
			case SAVE_MODE_RAW16:
				qDebug("Bits/pixel: %d", 16);
				estimatedSize *= 16;
				estimatedSize += (4096<<8);
				break;
			case SAVE_MODE_RAW12:
				qDebug("Bits/pixel: %d", 12);
				estimatedSize *= 12;
				estimatedSize += (4096<<8);
				break;
			case SAVE_MODE_TIFF:
				fileOverMaxSize = false;
				estimatedSize *= 24;
				estimatedSize += (4096<<8);
				break;

			default:
				// unknown format
				qDebug("Bits/pixel: unknown - default: %d", 16);
				estimatedSize *= 16;
			}
			// convert to bytes
			estimatedSize /= 8;

			qDebug("===================================");
			qDebug("Resolution: %d x %d", hRes, vRes);
			qDebug("Frames: %d", markOutFrame - markInFrame + 1);
			qDebug("Free space: %llu", freeSpace);
			qDebug("Estimated file size: %llu", estimatedSize);
			qDebug("===================================");

			fileOverMaxSize = fileOverMaxSize && (estimatedSize > 4294967296);
			insufficientFreeSpaceEstimate = (estimatedSize > freeSpace);

			//If amount of free space is below both 10MB and below the estimated size of the video, do not allow the save to start
			if(insufficientFreeSpaceEstimate && (MIN_FREE_SPACE > freeSpace)) {
				QMessageBox::warning(this, "Warning - Insufficient free space", "Cannot save a video because of insufficient free space", QMessageBox::Ok);
				return;
			}

			if(!autoSaveFlag){
				if (fileOverMaxSize && !insufficientFreeSpaceEstimate) {
					QMessageBox::StandardButton reply;
					reply = QMessageBox::warning(this, "Warning - File size over limit", "Estimated file size is larger than the 4GB limit for the the filesystem.\nAttempt to save anyway?", QMessageBox::Yes|QMessageBox::No);
					if(QMessageBox::Yes != reply)
						return;
				}
				
				if (insufficientFreeSpaceEstimate && !fileOverMaxSize) {
					QMessageBox::StandardButton reply;
					reply = QMessageBox::warning(this, "Warning - Insufficient free space", "Estimated file size is larger than free space on drive.\nAttempt to save anyway?", QMessageBox::Yes|QMessageBox::No);
					if(QMessageBox::Yes != reply)
						return;
				}
				
				if (fileOverMaxSize && insufficientFreeSpaceEstimate){
					QMessageBox::StandardButton reply;
					reply = QMessageBox::warning(this, "Warning - File size over limits", "Estimated file size is larger than free space on drive.\nEstimated file size is larger than the 4GB limit for the the filesystem.\nAttempt to save anyway?", QMessageBox::Yes|QMessageBox::No);
					if(QMessageBox::Yes != reply)
						return;
				}
			}
		}

		//Check that the path exists
		struct stat sb;
		struct stat sbP;
		if (stat(camera->vinst->fileDirectory, &sb) == 0 && S_ISDIR(sb.st_mode) &&
				stat(parentPath, &sbP) == 0 && sb.st_dev != sbP.st_dev)		//If location is directory and is a mount point (device ID of parent is different from device ID of path)
		{
			ret = camera->vinst->startRecording((hRes + 15) & 0xFFFFFFF0, vRes, markInFrame - 1, markOutFrame - markInFrame + 1, format);
			if (RECORD_FILE_EXISTS == ret) {
				msg.setText("File already exists. Rename then try saving again.");
				msg.exec();
				return;
			}
			else if (RECORD_DIRECTORY_NOT_WRITABLE == ret) {
				msg.setText("Save directory is not writable.");
				msg.exec();
				return;
			}
			else if (RECORD_INSUFFICIENT_SPACE == ret) {
				msg.setText("Selected device does not have sufficient free space.");
				msg.exec();
				return;
			}

			ui->cmdSave->setEnabled(false);
			setControlEnable(false);
			sw->setText("Saving...");
			sw->show();

			ui->verticalSlider->appendRegionToList();
			ui->verticalSlider->setHighlightRegion(markOutFrame, markOutFrame);
			//both arguments should be markout because a new rectangle will be drawn,
			//and it should not overlap the one that was just appended
			emit enableSaveSettingsButtons(false);
		}
		else {
			msg.setText(QString("Save location ") + QString(camera->vinst->fileDirectory) + " not found, set save location in Settings");
			msg.exec();
			return;
		}
	}
	else {
		//This block is executed when Abort is clicked
		//or when save is automatically aborted due to full storage
		camera->vinst->stopRecording();
		ui->cmdSave->setEnabled(false);
		ui->verticalSlider->removeLastRegionFromList();
		ui->verticalSlider->setHighlightRegion(markInFrame, markOutFrame);
		saveAborted = true;
		autoRecordFlag = false;
		updateSWText();
	}

}

void playbackWindow::addDotsToString(QString* abc)
{
	periodsToAdd++;
	if(periodsToAdd > 2) periodsToAdd = 0;
	if(periodsToAdd == 0) abc->append("  ");
	if(periodsToAdd == 1) abc->append(". ");
	if(periodsToAdd == 2) abc->append("..");
}

void playbackWindow::on_cmdStopSave_clicked()
{

}

void playbackWindow::on_cmdSaveSettings_clicked()
{
	saveSettingsWindow *w = new saveSettingsWindow(NULL, camera);
	w->setAttribute(Qt::WA_DeleteOnClose);
	w->show();
	
	settingsWindowIsOpen = true;
	if(camera->ButtonsOnLeft) w->move(201, 0);
	ui->cmdSaveSettings->setEnabled(false);
	ui->cmdClose->setEnabled(false);
	connect(w, SIGNAL(destroyed()), this, SLOT(saveSettingsClosed()));
	connect(this, SIGNAL(enableSaveSettingsButtons(bool)), w, SLOT(setControlEnable(bool)));
	connect(this, SIGNAL(destroyed(QObject*)), w, SLOT(close()));
}

void playbackWindow::saveSettingsClosed(){
	settingsWindowIsOpen = false;
	if(camera->vinst->getStatus(NULL) != VIDEO_STATE_FILESAVE) {
		/* Only enable these buttons if the camera is not saving a video */
		ui->cmdSaveSettings->setEnabled(true);
		ui->cmdClose->setEnabled(true);
	}
}

void playbackWindow::on_cmdMarkIn_clicked()
{
	markInFrame = playFrame + 1;
	if(markOutFrame < markInFrame)
		markOutFrame = markInFrame;
	ui->verticalSlider->setHighlightRegion(markInFrame, markOutFrame);
	updateStatusText();
}

void playbackWindow::on_cmdMarkOut_clicked()
{
	markOutFrame = playFrame + 1;
	if(markInFrame > markOutFrame)
		markInFrame = markOutFrame;
	ui->verticalSlider->setHighlightRegion(markInFrame, markOutFrame);
	updateStatusText();
}

void playbackWindow::keyPressEvent(QKeyEvent *ev)
{
	unsigned int skip = 10;
	if (playbackExponent > 0) {
		skip <<= playbackExponent;
	}

	switch (ev->key()) {
	case Qt::Key_Up:
		camera->vinst->seekFrame(1);
		break;

	case Qt::Key_Down:
		camera->vinst->seekFrame(-1);
		break;

	case Qt::Key_PageUp:
		camera->vinst->seekFrame(skip);
		break;

	case Qt::Key_PageDown:
		camera->vinst->seekFrame(-skip);
		break;
	}
}

void playbackWindow::updateStatusText()
{
	char text[100];
	sprintf(text, "Frame %d/%d\r\nMark start %d\r\nMark end %d", playFrame + 1, totalFrames, markInFrame, markOutFrame);
	ui->lblInfo->setText(text);
}

void playbackWindow::updateSWText(){
	QString statusWindowText;
	statusWindowText.operator = ("Aborting save.");
	if(saveAbortedAutomatically) statusWindowText.prepend("Storage is now full; ");
	addDotsToString(&statusWindowText);
	sw->setText(statusWindowText);
}

//Periodically check if the play frame is updated
void playbackWindow::updatePlayFrame()
{
	playFrame = camera->vinst->getPosition();
	ui->verticalSlider->setValue(playFrame);
	updateStatusText();
}

//Once save is done, re-enable the window
void playbackWindow::checkForSaveDone()
{
	VideoStatus st;
	if(camera->vinst->getStatus(&st) != VIDEO_STATE_FILESAVE)
	{
		/* Now done in videoended()
		saveDoneTimer->stop();
		delete saveDoneTimer;
		*/
		
	}
	else {
		char tmp[64];
		sprintf(tmp, "%.1ffps", st.framerate);
		ui->lblFrameRate->setText(tmp);
		setControlEnable(false);

		struct statvfs statvfsBuf;
		statvfs(camera->vinst->fileDirectory, &statvfsBuf);
		qDebug("Free space: %llu  (%lu * %lu)", statvfsBuf.f_bsize * (uint64_t)statvfsBuf.f_bfree, statvfsBuf.f_bsize, statvfsBuf.f_bfree);
		
		/* Prevent the user from pressing the abort/save button just after the last frame,
		 * as that can make the camera try to save a 2nd video too soon, crashing the camapp.*/
		if(playFrame >= markOutFrame - 25)
			ui->cmdSave->setEnabled(false);
		
		/*Abort the save if insufficient free space,
		but not if the save has already been aborted,
		or if the save button is not enabled(unsafe to abort at that time)(except if save mode is RAW)*/
		bool insufficientFreeSpaceCurrent = (MIN_FREE_SPACE > statvfsBuf.f_bsize * (uint64_t)statvfsBuf.f_bfree);
		if(insufficientFreeSpaceCurrent &&
		   insufficientFreeSpaceEstimate &&
		   !saveAborted &&
				(ui->cmdSave->isEnabled() ||
				getSaveFormat() != SAVE_MODE_H264)
		   ) {
			saveAbortedAutomatically = true;
			on_cmdSave_clicked();
		}
		
		if(saveAborted) updateSWText();
	}
}

void playbackWindow::on_cmdRateUp_clicked()
{
	if(playbackExponent < 5)
		playbackExponent++;

	updatePlayRateLabel();
}

void playbackWindow::on_cmdRateDn_clicked()
{
	if(playbackExponent > -5)
		playbackExponent--;

	updatePlayRateLabel();
}

void playbackWindow::updatePlayRateLabel(void)
{
	char playRateStr[100];
	double playRate;

	playRate = (playbackExponent >= 0) ? (60 << playbackExponent) : 60.0 / (1 - playbackExponent);
	sprintf(playRateStr, "%.1ffps", playRate);

	ui->lblFrameRate->setText(playRateStr);
}

void playbackWindow::setControlEnable(bool en)
{
	//While settings window is open, don't let the user
	//close the playback window or open another settings window.
	if(!settingsWindowIsOpen){
		ui->cmdClose->setEnabled(en);
		ui->cmdSaveSettings->setEnabled(en);
	}
	ui->cmdMarkIn->setEnabled(en);
	ui->cmdMarkOut->setEnabled(en);
	ui->cmdPlayForward->setEnabled(en);
	ui->cmdPlayReverse->setEnabled(en);
	ui->cmdRateDn->setEnabled(en);
	ui->cmdRateUp->setEnabled(en);
	ui->cmdLoop->setEnabled(en);
	ui->verticalSlider->setEnabled(en);
}

void playbackWindow::stopPlayLoop(void)
{
	playLoop = false;
	ui->cmdLoop->setText("Play");
}

void playbackWindow::on_cmdClose_clicked()
{
	camera->videoHasBeenReviewed = true;
	camera->autoRecord = false;
}

save_mode_type playbackWindow::getSaveFormat()
{
	QSettings appSettings;

	switch (appSettings.value("recorder/saveFormat", SAVE_MODE_H264).toUInt()) {
	case 0:  return SAVE_MODE_H264;
	case 1:  return SAVE_MODE_RAW16;
	case 2:  return SAVE_MODE_RAW12;
	case 3:  return SAVE_MODE_DNG;
	case 4:  return SAVE_MODE_TIFF;
	default: return SAVE_MODE_H264;
	}
}

void playbackWindow::on_cmdLoop_clicked()
{
	if (playLoop) {
		stopPlayLoop();
		camera->vinst->setPlayback(0);
	}
	else {
		int fps = (playbackExponent >= 0) ? (60 << playbackExponent) : 60 / (1 - playbackExponent);
		unsigned int count = (markOutFrame - markInFrame + 1);
		playLoop = true;
		ui->cmdLoop->setText("Stop");
		camera->vinst->loopPlayback(markInFrame, count, fps);
	}
}
