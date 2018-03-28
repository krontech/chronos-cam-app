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

#include "videoRecord.h"
#include "util.h"
#include "camera.h"

#include "savesettingswindow.h"
#include "playbackwindow.h"
#include "ui_playbackwindow.h"

#include <QTimer>
#include <QMessageBox>
#include <QSettings>
#include <QKeyEvent>


playbackWindow::playbackWindow(QWidget *parent, Camera * cameraInst, bool autosave) :
	QWidget(parent),
	ui(new Ui::playbackWindow)
{
	ui->setupUi(this);
	this->setWindowFlags(Qt::Dialog /*| Qt::WindowStaysOnTopHint*/ | Qt::FramelessWindowHint);

	camera = cameraInst;
	autoSaveFlag = autosave;
	this->move(camera->ButtonsOnLeft? 0:600, 0);

	sw = new StatusWindow;

	connect(ui->cmdClose, SIGNAL(clicked()), this, SLOT(close()));

	ui->verticalSlider->setMinimum(0);
	ui->verticalSlider->setMaximum(camera->recordingData.totalFrames - 1);
	ui->verticalSlider->setValue(camera->playFrame);
	markInFrame = 1;
	markOutFrame = camera->recordingData.totalFrames;
	ui->verticalSlider->setHighlightRegion(markInFrame, markOutFrame);

	camera->setPlayMode(true);
	camera->vinst->setPosition(0, 0);

	playbackExponent = 0;
	updatePlayRateLabel(playbackExponent);

	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(updatePlayFrame()));
	timer->start(30);

	updateStatusText();

	setFocusPolicy(Qt::StrongFocus);
	
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
	/* Note that a rate of zero will also pause playback. */
	camera->vinst->setPosition(position, 0);
}

void playbackWindow::on_verticalSlider_valueChanged(int value)
{

}

void playbackWindow::on_cmdPlayForward_pressed()
{
	int fps = (playbackExponent >= 0) ? (60 << playbackExponent) : 60 / (1 - playbackExponent);
	camera->vinst->setPlayback(fps);
}

void playbackWindow::on_cmdPlayForward_released()
{
	camera->vinst->setPlayback(0);
}

void playbackWindow::on_cmdPlayReverse_pressed()
{
	int fps = (playbackExponent >= 0) ? (60 << playbackExponent) : 60 / (1 - playbackExponent);
	camera->vinst->setPlayback(-fps);
}

void playbackWindow::on_cmdPlayReverse_released()
{
	camera->vinst->setPlayback(0);
}

void playbackWindow::on_cmdSave_clicked()
{
	UInt32 ret;
	QMessageBox msg;
	char parentPath[1000];
	struct statvfs statvfsBuf;
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
				estimatedSize += estimatedSize + (4096<<8);
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

			if (estimatedSize > (statvfsBuf.f_bsize * (uint64_t)statvfsBuf.f_bfree) || estimatedSize > 4294967296) {
				QMessageBox::StandardButton reply;
				reply = QMessageBox::question(this, "Estimated file size too large", "Estimated file size is larger than room on media/4GB. Attempt to save?", QMessageBox::Yes|QMessageBox::No);
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
			ui->verticalSlider->appendRegionToList();
			ui->verticalSlider->setHighlightRegion(markOutFrame, markOutFrame);
			//both arguments should be markout because a new rectangle will be drawn, and it should not overlap the one that was just appended
			markInFrameOld = markInFrame;
			markInFrame = markOutFrame;
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
		markInFrame = markInFrameOld;
		ui->verticalSlider->setHighlightRegion(markInFrame, markOutFrame);
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

void playbackWindow::keyPressEvent(QKeyEvent *ev)
{
	unsigned int skip = 1;
	switch (ev->key()) {
	case Qt::Key_PageUp:
		skip = 10;
		if (playbackExponent > 0) {
			skip <<= playbackExponent;
		}
	case Qt::Key_Up:
		camera->vinst->setPosition((camera->playFrame + skip) % camera->recordingData.totalFrames, 0);
		break;

	case Qt::Key_PageDown:
		skip = 10;
		if (playbackExponent > 0) {
			skip <<= playbackExponent;
		}
	case Qt::Key_Down:
		if (camera->playFrame >= skip) {
			camera->vinst->setPosition(camera->playFrame - skip, 0);
		} else {
			camera->vinst->setPosition(camera->playFrame + camera->recordingData.totalFrames - skip, 0);
		}
		break;
	}
}

void playbackWindow::updateStatusText()
{
	char text[100];
	sprintf(text, "Frame %d/%d\r\nMark in %d\r\nMark out %d", camera->playFrame + 1, camera->recordingData.totalFrames, markInFrame, markOutFrame);
	ui->lblInfo->setText(text);
}

//Periodically check if the play frame is updated
void playbackWindow::updatePlayFrame()
{
	camera->playFrame = camera->vinst->getPosition();
	ui->verticalSlider->setValue(camera->playFrame);
	updateStatusText();
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
		updatePlayRateLabel(playbackExponent);

		if(autoSaveFlag) {
			close();
		}
	}
	else {
		char tmp[64];
		sprintf(tmp, "%.1ffps", camera->recorder->getFramerate());
		ui->lblFrameRate->setText(tmp);
		setControlEnable(false);
	}
}

void playbackWindow::on_cmdRateUp_clicked()
{
	if(playbackExponent < 5)
		playbackExponent++;

	updatePlayRateLabel(playbackExponent);
}

void playbackWindow::on_cmdRateDn_clicked()
{
	if(playbackExponent > -5)
		playbackExponent--;

	updatePlayRateLabel(playbackExponent);
}

void playbackWindow::updatePlayRateLabel(Int32 playbackRate)
{
	char playRateStr[100];
	double playRate;

	playRate = (playbackExponent >= 0) ? (60 << playbackExponent) : 60.0 / (1 - playbackExponent);
	sprintf(playRateStr, "%.1ffps", playRate);

	ui->lblFrameRate->setText(playRateStr);
}

void playbackWindow::setControlEnable(bool en)
{
	if(!settingsWindowIsOpen){//While settings window is open, don't let the user close the playback window or open another settings window.
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
