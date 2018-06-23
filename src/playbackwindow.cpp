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
#include <sys/statvfs.h>
#include <sys/vfs.h>

#include "videoRecord.h"
#include "util.h"
#include "camera.h"

#include "savesettingswindow.h"
#include "playbackwindow.h"
#include "ui_playbackwindow.h"

#include <QTimer>
#include <QMessageBox>
#include <QSettings>

#define MIN_FREE_SPACE 10000000

playbackWindow::playbackWindow(QWidget *parent, Camera * cameraInst, bool autosave) :
	QWidget(parent),
	ui(new Ui::playbackWindow)
{
	ui->setupUi(this);
	this->setWindowFlags(Qt::Dialog /*| Qt::WindowStaysOnTopHint*/ | Qt::FramelessWindowHint);

	camera = cameraInst;
	autoSaveFlag = autosave;
	this->move(camera->ButtonsOnLeft? 0:600, 0);
	saveAborted = false;
	

	sw = new StatusWindow;

	connect(ui->cmdClose, SIGNAL(clicked()), this, SLOT(close()));

	ui->verticalSlider->setMinimum(0);
	ui->verticalSlider->setMaximum(camera->recordingData.totalFrames - 1);
	ui->verticalSlider->setValue(camera->playFrame);
	markInFrame = 1;
	markOutFrame = camera->recordingData.totalFrames;
	ui->verticalSlider->setHighlightRegion(markInFrame, markOutFrame);

	camera->setPlayMode(true);

	lastPlayframe = camera->playFrame;

	playbackRate = 1;
	updatePlayRateLabel(playbackRate);

	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(updatePlayFrame()));
	timer->start(30);

	updateStatusText();

	settingsWindowIsOpen = false;

	if(autoSaveFlag) {
		on_cmdSave_clicked();
	}
}

playbackWindow::~playbackWindow()
{
	camera->setPlayMode(false);
	timer->stop();
	emit finishedSaving();
	delete sw;
	delete ui;
}

void playbackWindow::on_verticalSlider_sliderMoved(int position)
{
	camera->playFrame = position;
}

void playbackWindow::on_verticalSlider_valueChanged(int value)
{
	camera->playFrame = value;
}

void playbackWindow::on_cmdPlayForward_pressed()
{
	camera->setPlaybackRate(playbackRate, true);
}

void playbackWindow::on_cmdPlayForward_released()
{
	camera->setPlaybackRate(0, true);
}

void playbackWindow::on_cmdPlayReverse_pressed()
{
	camera->setPlaybackRate(playbackRate, false);
}

void playbackWindow::on_cmdPlayReverse_released()
{
	camera->setPlaybackRate(0, false);
}

void playbackWindow::on_cmdSave_clicked()
{
	UInt32 ret;
	QMessageBox msg;
	char parentPath[1000];
	struct statvfs statvfsBuf;
	struct statfs fileSystemInfoBuf;
	uint64_t estimatedSize;
	QSettings appSettings;

	//Build the parent path of the save directory, to determine if it's a mount point
	strcpy(parentPath, camera->recorder->fileDirectory);
	strcat(parentPath, "/..");

	if(!camera->recorder->getRunning())
	{
		//If no directory set, complain to the user
		if(strlen(camera->recorder->fileDirectory) == 0)
		{
			msg.setText("No save location set! Set save location in Settings");
			msg.exec();
			return;
		}

		if (!statvfs(camera->recorder->fileDirectory, &statvfsBuf)) {
			
			qDebug("===================================");
			
			// calculated estimated size
			estimatedSize = (markOutFrame - markInFrame + 1);
			qDebug("Number of frames: %llu", estimatedSize);
			estimatedSize *= appSettings.value("camera/hRes", MAX_FRAME_SIZE_H).toInt();
			estimatedSize *= appSettings.value("camera/vRes", MAX_FRAME_SIZE_V).toInt();
			qDebug("Resolution: %d x %d", appSettings.value("camera/hRes", MAX_FRAME_SIZE_H).toInt(), appSettings.value("camera/vRes", MAX_FRAME_SIZE_V).toInt());
			// multiply by bits per pixel
			switch(appSettings.value("recorder/saveFormat", 0).toUInt()) {
			case SAVE_MODE_H264:
				// the *1.2 part is fudge factor
				estimatedSize = (uint64_t) ((double)estimatedSize * appSettings.value("recorder/bitsPerPixel", camera->recorder->bitsPerPixel).toDouble() * 1.2);
				qDebug("Bits/pixel: %0.3f", appSettings.value("recorder/bitsPerPixel", camera->recorder->bitsPerPixel).toDouble());
				break;
			case SAVE_MODE_RAW16:
			case SAVE_MODE_RAW16RJ:
				qDebug("Bits/pixel: %d", 16);
				estimatedSize *= 16;
				estimatedSize += (4096<<8);
				break;
			case SAVE_MODE_RAW12:
				qDebug("Bits/pixel: %d", 12);
				estimatedSize *= 12;
				estimatedSize += (4096<<8);
				break;
			default:
				// unknown format
				qDebug("Bits/pixel: unknown - default: %d", 16);
				estimatedSize *= 16;
			}
			// convert to bytes
			estimatedSize /= 8;

			qDebug("Free space: %llu  (%lu * %lu)", statvfsBuf.f_bsize * (uint64_t)statvfsBuf.f_bfree, statvfsBuf.f_bsize, statvfsBuf.f_bfree);
			qDebug("Estimated file size: %llu", estimatedSize);
			
			qDebug("===================================");

			statfs(camera->recorder->fileDirectory, &fileSystemInfoBuf);
			bool fileOverMaxSize = (estimatedSize > 4294967296 && fileSystemInfoBuf.f_type == 0x4d44);//If file size is over 4GB and file system is FAT32
			insufficientFreeSpace_estimate = (estimatedSize > (statvfsBuf.f_bsize * (uint64_t)statvfsBuf.f_bfree));
			
			//If amount of free space is below both 10MB and below the estimated size of the video, do not allow the save to start
			if(insufficientFreeSpace_estimate && MIN_FREE_SPACE > (statvfsBuf.f_bsize * (uint64_t)statvfsBuf.f_bfree)){
				QMessageBox::warning(this, "Warning - Insufficient free space", "Cannot save a video because of insufficient free space", QMessageBox::Ok);
				return;
			}
			
			if (fileOverMaxSize && !insufficientFreeSpace_estimate) {//If file size is over 4GB and file system is FAT32
				QMessageBox::StandardButton reply;
				reply = QMessageBox::warning(this, "Warning - File size over limit", "Estimated file size is larger than the 4GB limit for the the filesystem.\nAttempt to save anyway?", QMessageBox::Yes|QMessageBox::No);
				if(QMessageBox::Yes != reply)
					return;
			}
			
			if (insufficientFreeSpace_estimate && !fileOverMaxSize) {
				QMessageBox::StandardButton reply;
				reply = QMessageBox::warning(this, "Warning - Insufficient free space", "Estimated file size is larger than free space on drive.\nAttempt to save anyway?", QMessageBox::Yes|QMessageBox::No);
				if(QMessageBox::Yes != reply)
					return;
			}
			
			if (fileOverMaxSize && insufficientFreeSpace_estimate){
				QMessageBox::StandardButton reply;
				reply = QMessageBox::warning(this, "Warning - File size over limits", "Estimated file size is larger than free space on drive.\nEstimated file size is larger than the 4GB limit for the the filesystem.\nAttempt to save anyway?", QMessageBox::Yes|QMessageBox::No);
				if(QMessageBox::Yes != reply)
					return;
			}
		}

		//Check that the path exists
		struct stat sb;
		struct stat sbP;
		
		if (stat(camera->recorder->fileDirectory, &sb) == 0 && S_ISDIR(sb.st_mode) &&
				stat(parentPath, &sbP) == 0 && sb.st_dev != sbP.st_dev)		//If location is directory and is a mount point (device ID of parent is different from device ID of path)
		{
			ret = camera->startSave(markInFrame - 1, markOutFrame - markInFrame + 1);
			if(RECORD_FILE_EXISTS == ret)
			{
				if(camera->recorder->errorCallback)
					(*camera->recorder->errorCallback)(camera->recorder->errorCallbackArg, "file already exists");
				msg.setText("File already exists. Rename then try saving again.");
				msg.exec();
				return;
			}
			else if(RECORD_DIRECTORY_NOT_WRITABLE == ret)
			{
				if(camera->recorder->errorCallback)
					(*camera->recorder->errorCallback)(camera->recorder->errorCallbackArg, "save directory is not writable");
				msg.setText("Save directory is not writable.");
				msg.exec();
				return;
			}
			else if(RECORD_INSUFFICIENT_SPACE == ret)
			{
				if(camera->recorder->errorCallback)
					(*camera->recorder->errorCallback)(camera->recorder->errorCallbackArg, "insufficient free space");
				msg.setText("Selected device does not have sufficient free space.");
				msg.exec();
				return;
			}

			ui->cmdSave->setText("Abort\nSave");
			setControlEnable(false);
			sw->setText("Saving...");
			sw->show();

			saveDoneTimer = new QTimer(this);
			connect(saveDoneTimer, SIGNAL(timeout()), this, SLOT(checkForSaveDone()));
			saveDoneTimer->start(100);

			/* Prevent the user from pressing the abort/save button just after the last frame,
			 * as that can make the camera try to save a 2nd video too soon, crashing the camapp.
			 * It is also disabled in checkForSaveDone(), but if the video is very short,
			 * that might not be called at all before the end of the video, so just disable the button right away.*/
			if(markOutFrame - markInFrame < 25) ui->cmdSave->setEnabled(false);

			ui->verticalSlider->appendRegionToList();
			ui->verticalSlider->setHighlightRegion(markOutFrame, markOutFrame);
			//both arguments should be markout because a new rectangle will be drawn,
			//and it should not overlap the one that was just appended
			emit enableSaveSettingsButtons(false);
		}
		else
		{
			if(camera->recorder->errorCallback)
				(*camera->recorder->errorCallback)(camera->recorder->errorCallbackArg, "location not found");
			msg.setText(QString("Save location ") + QString(camera->recorder->fileDirectory) + " not found, set save location in Settings");
			msg.exec();
			return;
		}
	}
	else
	{
		//This block is executed when Abort is clicked
		camera->recorder->stop2();
		ui->verticalSlider->removeLastRegionFromList();
		ui->verticalSlider->setHighlightRegion(markInFrame, markOutFrame);
		saveAborted = true;
		sw->setText("Aborting...");
		//qDebug()<<"Aborting...";
	}

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
	if(camera->ButtonsOnLeft) w->move(230, 0);
	ui->cmdSaveSettings->setEnabled(false);
	ui->cmdClose->setEnabled(false);
	connect(w, SIGNAL(destroyed()), this, SLOT(saveSettingsClosed()));
	connect(this, SIGNAL(enableSaveSettingsButtons(bool)), w, SLOT(setControlEnable(bool)));
}

void playbackWindow::saveSettingsClosed(){
	settingsWindowIsOpen = false;
	if(!camera->recorder->getRunning()){//Only enable these buttons if the camera is not saving a video
		ui->cmdSaveSettings->setEnabled(true);
		ui->cmdClose->setEnabled(true);
	}
}

void playbackWindow::on_cmdMarkIn_clicked()
{
	markInFrame = camera->playFrame + 1;
	if(markOutFrame < markInFrame)
		markOutFrame = markInFrame;
	ui->verticalSlider->setHighlightRegion(markInFrame, markOutFrame);
	updateStatusText();
}

void playbackWindow::on_cmdMarkOut_clicked()
{
	markOutFrame = camera->playFrame + 1;
	if(markInFrame > markOutFrame)
		markInFrame = markOutFrame;
	ui->verticalSlider->setHighlightRegion(markInFrame, markOutFrame);
	updateStatusText();
}

void playbackWindow::updateStatusText()
{
	char text[100];
	sprintf(text, "Frame %d/%d\r\nMark start %d\r\nMark end %d", camera->playFrame + 1, camera->recordingData.totalFrames, markInFrame, markOutFrame);
	ui->lblInfo->setText(text);
}

//Periodically check if the play frame is updated
void playbackWindow::updatePlayFrame()
{
	UInt32 playFrame = camera->playFrame;
	if(playFrame != lastPlayframe)
	{
		ui->verticalSlider->setValue(playFrame);
		updateStatusText();
		lastPlayframe = camera->playFrame;
	}
}

//Once save is done, re-enable the window
void playbackWindow::checkForSaveDone()
{
	if(camera->recorder->endOfStream())
	{
		saveDoneTimer->stop();
		delete saveDoneTimer;

		sw->close();
		ui->cmdSave->setText("Save");
		setControlEnable(true);
		emit enableSaveSettingsButtons(true);
		ui->cmdSave->setEnabled(true);
		saveAborted = false;
		updatePlayRateLabel(playbackRate);
		ui->verticalSlider->setHighlightRegion(markInFrame, markOutFrame);

		if(autoSaveFlag) {
			close();
		}
	}
	else {
		char tmp[64];
		sprintf(tmp, "%.1ffps", camera->recorder->getFramerate());
		ui->lblFrameRate->setText(tmp);
		setControlEnable(false);

		struct statvfs statvfsBuf;
		statvfs(camera->recorder->fileDirectory, &statvfsBuf);
		qDebug("Free space: %llu  (%lu * %lu)", statvfsBuf.f_bsize * (uint64_t)statvfsBuf.f_bfree, statvfsBuf.f_bsize, statvfsBuf.f_bfree);
		
		/* Prevent the user from pressing the abort/save button just after the last frame,
		 * as that can make the camera try to save a 2nd video too soon, crashing the camapp.*/
		if(camera->playFrame >= markOutFrame - 25)
			ui->cmdSave->setEnabled(false);
		
		/*Abort the save if insufficient free space,
		but not if the save has already been aborted,
		or if the save button is not enabled(unsafe to abort at that time)(except if save mode is RAW)*/
		QSettings appSettings;
		bool insufficientFreeSpace_current = (MIN_FREE_SPACE > statvfsBuf.f_bsize * (uint64_t)statvfsBuf.f_bfree);
		if(insufficientFreeSpace_current &&
		   !saveAborted &&
				(ui->cmdSave->isEnabled() ||
				appSettings.value("recorder/saveFormat", 0).toUInt() != SAVE_MODE_H264)
		   ) {
			on_cmdSave_clicked();
			sw->setText("Storage is now full; Aborting...");			
		}
			
		
		/* Prevent the user from pressing the abort/save button just after the last frame,
		 * as that can make the camera try to save a 2nd video too soon, crashing the camapp.*/
		if(camera->playFrame >= markOutFrame - 25)
			ui->cmdSave->setEnabled(false);
	}
}

void playbackWindow::on_cmdRateUp_clicked()
{
	if(playbackRate < 5)
		playbackRate++;
	if(0 == playbackRate)	//Don't let playback rate be 0 (no playback)
		playbackRate = 1;

	updatePlayRateLabel(playbackRate);
}

void playbackWindow::on_cmdRateDn_clicked()
{
	if(playbackRate > -12)
		playbackRate--;
	if(0 == playbackRate)	//Don't let playback rate be 0 (no playback)
		playbackRate = -1;

	updatePlayRateLabel(playbackRate);
}

void playbackWindow::updatePlayRateLabel(Int32 playbackRate)
{
	char playRateStr[100];
	double playRate;

	playRate = playbackRate > 0 ? 60.0 * (double)(1 << (playbackRate - 1)) : 60.0 / (double)(-playbackRate+1);
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
	ui->verticalSlider->setEnabled(en);
}

void playbackWindow::on_cmdClose_clicked()
{
    camera->videoHasBeenReviewed = true;
}
