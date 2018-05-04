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
#include <mntent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/mount.h>
#include <unistd.h>


#include "savesettingswindow.h"
#include "ui_savesettingswindow.h"

#include <cstring>

#include <QMessageBox>
#include <QTimer>
#include <QDebug>
#include <QSettings>

//Defining this here because something in the preprocessor undefines this somewhere up above
#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
	   __typeof__ (b) _b = (b); \
	 _a < _b ? _a : _b; })

saveSettingsWindow::saveSettingsWindow(QWidget *parent, Camera * camInst) :
	QWidget(parent),
	ui(new Ui::saveSettingsWindow)
{
	windowInitComplete = false;

	QSettings settings;
	ui->setupUi(this);
	this->setWindowFlags(Qt::Dialog /*| Qt::WindowStaysOnTopHint*/ | Qt::FramelessWindowHint);
	camera = camInst;

	
	ui->spinBitrate->setValue(settings.value("recorder/bitsPerPixel", camera->recorder->bitsPerPixel).toDouble());
	ui->spinMaxBitrate->setValue(settings.value("recorder/maxBitrate", camera->recorder->maxBitrate).toDouble());
	ui->spinFramerate->setValue(settings.value("recorder/framerate", camera->recorder->framerate).toDouble());
	ui->lineFilename->setText(settings.value("recorder/filename", camera->recorder->filename).toString());

	refreshDriveList();

	ui->comboProfile->clear();
	ui->comboProfile->addItem("Base");
	ui->comboProfile->addItem("Main");
	ui->comboProfile->addItem("Extended");
	ui->comboProfile->addItem("High");

	UInt32 val = settings.value("recorder/profile", camera->recorder->profile).toUInt();
	//Compute base 2 logarithm to get index
	Int32 index = 0;
	while (val >>= 1) ++index;
	ui->comboProfile->setCurrentIndex(index);

	ui->comboLevel->clear();
	ui->comboLevel->addItem("Level 1");
	ui->comboLevel->addItem("Level 1b");
	ui->comboLevel->addItem("Level 11");
	ui->comboLevel->addItem("Level 12");
	ui->comboLevel->addItem("Level 13");
	ui->comboLevel->addItem("Level 2");
	ui->comboLevel->addItem("Level 21");
	ui->comboLevel->addItem("Level 22");
	ui->comboLevel->addItem("Level 3");
	ui->comboLevel->addItem("Level 31");
	ui->comboLevel->addItem("Level 32");
	ui->comboLevel->addItem("Level 4");
	ui->comboLevel->addItem("Level 41");
	ui->comboLevel->addItem("Level 42");
	ui->comboLevel->addItem("Level 5");
	ui->comboLevel->addItem("Level 51");

	val = settings.value("recorder/level", camera->recorder->level).toUInt();
	//Compute base 2 logarithm to get index
	index = 0;
	while (val >>= 1) ++index;
	ui->comboLevel->setCurrentIndex(index);

	ui->comboSaveFormat->setEnabled(false);
	ui->comboSaveFormat->clear();
	// these must line up with the enum in videoRecord.h
	ui->comboSaveFormat->addItem("H.264");            // SAVE_MODE_H264
	ui->comboSaveFormat->addItem("Raw 16bit");        // SAVE_MODE_RAW16
	ui->comboSaveFormat->addItem("Raw 16RJ");         // SAVE_MODE_RAW16RJ
	ui->comboSaveFormat->addItem("Raw 12bit packed"); // SAVE_MODE_RAW12
	
	ui->comboSaveFormat->setCurrentIndex(settings.value("recorder/saveFormat", 0).toUInt());

	ui->comboSaveFormat->setEnabled(true);

	if(ui->comboSaveFormat->currentIndex() == 0) {
		ui->spinBitrate->setEnabled(true);
		ui->spinFramerate->setEnabled(true);
		ui->spinMaxBitrate->setEnabled(true);
	}
	else {
		ui->spinBitrate->setEnabled(false);
		ui->spinFramerate->setEnabled(false);
		ui->spinMaxBitrate->setEnabled(false);
	}

	driveCount = 0;
	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(updateDrives()));
	timer->start(1000);

	windowInitComplete = true;
	updateBitrate();
}

saveSettingsWindow::~saveSettingsWindow()
{
	timer->stop();
	delete timer;
	delete ui;
}

void saveSettingsWindow::on_cmdClose_clicked()
{
	QSettings settings;

	camera->recorder->profile = 1 << 3;
	camera->recorder->level = 1 << 15;

	saveFileDirectory();

	settings.setValue("recorder/profile", camera->recorder->profile);
	settings.setValue("recorder/level", camera->recorder->level);

	close();
}

void saveSettingsWindow::saveFileDirectory(){
	//Keep the beginning of the combo box text (the path)
	char str[100];
	const char * path;

	if(ui->comboDrive->isEnabled())
	{
		strcpy(str, ui->comboDrive->currentText().toStdString().c_str());

		//Keep only the part before the first space (the actual path)
		path = strtok(str, " ");
	}
	else	//No valid paths available
		path = "";

	strcpy(camera->recorder->fileDirectory, path);
	QSettings settings;
	settings.setValue("recorder/fileDirectory", camera->recorder->fileDirectory);
}

void saveSettingsWindow::on_cmdUMount_clicked()
{
	//Keep the beginning of the combo box text (the path)
	char str[100];
	char * path;
	strcpy(str, ui->comboDrive->currentText().toStdString().c_str());
	path = strtok(str, " ");

	if(umount2(path, 0))
	{ //Failed, show error
		QMessageBox msg;
		msg.setText("Unmount failed!");
		msg.setInformativeText(ui->comboDrive->currentText());
		msg.exec();
	}

	refreshDriveList();
}

//Find all mounted drives and populate the drive comboBox
void saveSettingsWindow::refreshDriveList()
{
	FILE * fp;
	FILE * mtab = setmntent("/etc/mtab", "r");
	struct mntent* m;
	struct mntent mnt;
	char strings[4096];		//Temp buffer used by mntent
	char model[256];		//Stores model name of sd* device
	char vendor[256];		//Stores vendor of sd* device
	char drive[1024];		//Stores string to be placed in combo box
	UInt32 len;

	okToSaveLocation = false;//prevent saving a new value while drive list is being updated
	ui->comboDrive->clear();

	//ui->comboDrive->addItem("/");

	while ((m = getmntent_r(mtab, &mnt, strings, sizeof(strings))))
	{
		struct statfs fs;
		if ((mnt.mnt_dir != NULL) && (statfs(mnt.mnt_dir, &fs) == 0))
		{
			ui->comboDrive->setEnabled(true);

			//Find only drives that are SD card or USB drives
			if(strstr(mnt.mnt_dir, "/media/mmcblk1") ||
					strstr(mnt.mnt_dir, "/media/sd"))
			{
				if(strstr(mnt.mnt_dir, "/media/sd"))
				{	//If this is not an SD card (ie a "sd*") type, get the
					char * mountPoint = mnt.mnt_dir + 7;	//Get the mounted name eg "sda1"
					Int32 part;
					char device[10];
					strncpy(device, mountPoint, 3);		//keep only the "sda" (discard any numbers such as "sda1"
					device[3] = '\0';					//Add null terminator
					char modelPath[256];
					char vendorPath[256];

					//If there's a number following the block name, that's our partition number
					if(*(mountPoint+3))
						part = atoi(mountPoint + 3);
					else
						part = 1;

					//Produce the paths to read the model and vendor strings
					sprintf(modelPath, "/sys/block/%s/device/model", device);
					sprintf(vendorPath, "/sys/block/%s/device/vendor", device);

					//Read the model and vendor strings for this block device
					fp = fopen(modelPath, "r");
					if(fp)
					{
						len = fread(model, 1, 255, fp);
						model[len] = '\0';
						fclose(fp);
					}
					else
						strcpy(model, "???");

					fp = fopen(vendorPath, "r");
					if(fp)
					{
						len = fread(vendor, 1, 255, fp);
						vendor[len] = '\0';
						fclose(fp);
					}
					else
						strcpy(vendor, "???");

					//remove all trailing whitespace and carrage returns
					int i;
					for(i = strlen(vendor) - 1; ' ' == vendor[i] || '\n' == vendor[i]; i--) {};	//Search back from the end and put a null at the first trailing space
					vendor[i + 1] = '\0';

					for(i = strlen(model) - 1; ' ' == model[i] || '\n' == model[i]; i--) {};	//Search back from the end and put a null at the first trailing space
					model[i + 1] = '\0';

					sprintf(drive, "%s (%s %s Partition %d)", mnt.mnt_dir, vendor, model, part);
				}
				else
				{
					Int32 part;

					//If there's a number following the block name, that's our partition number
					if(*(mnt.mnt_dir + 15))		//Find the first character after "/media/mmcblk1p"
						part = atoi(mnt.mnt_dir + 15);
					else
						part = 1;
					sprintf(drive, "%s (SD Card Partition %d)", mnt.mnt_dir, part);

				}

				ui->comboDrive->addItem(drive);

				unsigned long long int size = fs.f_blocks * fs.f_bsize;
				unsigned long long int free = fs.f_bfree * fs.f_bsize;
				unsigned long long int avail = fs.f_bavail * fs.f_bsize;
				printf("%s %s size=%lld free=%lld avail=%lld\n",
				mnt.mnt_fsname, mnt.mnt_dir, size, free, avail);
			}
		}
	}

	endmntent(mtab);

	if(ui->comboDrive->count() == 0)
	{
		ui->comboDrive->addItem("No storage devices detected");
		ui->comboDrive->setEnabled(false);

	}
	okToSaveLocation = true;

	//Select the entry corresponding to the last selected path
	QSettings settings;
	Int32 index = ui->comboDrive->findText(settings.value("recorder/fileDirectory", camera->recorder->fileDirectory).toString());
	if ( index != -1 ) { // -1 for not found
		ui->comboDrive->setCurrentIndex(index);
	}
	//Only save the location if it has changed
	if(strcmp(camera->recorder->fileDirectory, ui->comboDrive->currentText().toAscii()))
		saveFileDirectory();
}

void saveSettingsWindow::on_cmdRefresh_clicked()
{
	refreshDriveList();
}

void saveSettingsWindow::updateBitrate()
{
	if(!windowInitComplete) return;
	int saveFormat = ui->comboSaveFormat->currentIndex();
	UInt32 frameRate = ui->spinFramerate->value();
	double bitsPerPixel;
	char str[100];
	
	if (saveFormat == 0) {
		UInt32 bitrate = min(ui->spinBitrate->value() * camera->recordingData.is.hRes * camera->recordingData.is.vRes * frameRate, min(60000000, (UInt32)(ui->spinMaxBitrate->value() * 1000000.0)) * frameRate / 60);	//Max of 60Mbps
		
		sprintf(str, "%4.2fMbps @\n%dx%d %dfps", (double)bitrate / 1000000.0, camera->recordingData.is.hRes, camera->recordingData.is.vRes, frameRate);
		ui->lblBitrate->setText(str);
	}
	else {
		switch(saveFormat) {
			// these must line up with the enum in videoRecord.h
			case 1: // SAVE_MODE_RAW16
			case 2: // SAVE_MODE_RAW16RJ
				bitsPerPixel = 16.0;
				break;
			case 3: // SAVE_MODE_RAW12
				bitsPerPixel = 12.0;
				break;
		}
		sprintf(str, "%4.2fMbps @\n%dx%d %dfps", ((double)camera->recordingData.is.hRes * (double)camera->recordingData.is.vRes * (double)frameRate * bitsPerPixel) / 1000000.0, camera->recordingData.is.hRes, camera->recordingData.is.vRes, frameRate);
		ui->lblBitrate->setText(str);
	}
}

void saveSettingsWindow::on_spinBitrate_valueChanged(double arg1)
{
	updateBitrate();
	QSettings settings;
	camera->recorder->bitsPerPixel = ui->spinBitrate->value();
	settings.setValue("recorder/bitsPerPixel", camera->recorder->bitsPerPixel);
}

void saveSettingsWindow::on_spinFramerate_valueChanged(int arg1)
{
	updateBitrate();
	QSettings settings;
	camera->recorder->framerate = ui->spinFramerate->value();
	settings.setValue("recorder/framerate", camera->recorder->framerate);
}

//When the number of drives connected changes, refresh the drive list
//Called by timer
void saveSettingsWindow::updateDrives(void)
{
	FILE* mtab = setmntent("/etc/mtab", "r");
	struct mntent* m;
	struct mntent mnt;
	char strings[4096];
	UInt32 newDriveCount = 0;

	while ((m = getmntent_r(mtab, &mnt, strings, sizeof(strings))))
	{
		struct statfs fs;
		if ((mnt.mnt_dir != NULL) && (statfs(mnt.mnt_dir, &fs) == 0))
		{
			//Find only drives that are SD card or USB drives
			if(strstr(mnt.mnt_dir, "/media/mmcblk1") ||
					strstr(mnt.mnt_dir, "/media/sd"))
			{
				newDriveCount++;
			}
		}
	}

	endmntent(mtab);

	if(newDriveCount != driveCount)
		refreshDriveList();

	driveCount = newDriveCount;


}

void saveSettingsWindow::on_lineFilename_textEdited(const QString &arg1)
{
	QSettings settings;
	strcpy(camera->recorder->filename, ui->lineFilename->text().toStdString().c_str());
	settings.setValue("recorder/filename", camera->recorder->filename);
}

void saveSettingsWindow::on_spinMaxBitrate_valueChanged(int arg1)
{
	updateBitrate();
	QSettings settings;
	camera->recorder->maxBitrate = ui->spinMaxBitrate->value();
	settings.setValue("recorder/maxBitrate", camera->recorder->maxBitrate);
}

void saveSettingsWindow::on_comboSaveFormat_currentIndexChanged(int index)
{
	if(!ui->comboSaveFormat->isEnabled()) return;
	if(index == 0) {
		ui->spinBitrate->setEnabled(true);
		ui->spinFramerate->setEnabled(true);
		ui->spinMaxBitrate->setEnabled(true);
	}
	else {
		ui->spinBitrate->setEnabled(false);
		ui->spinFramerate->setEnabled(false);
		ui->spinMaxBitrate->setEnabled(false);
	}
	updateBitrate();
	QSettings settings;
	settings.setValue("recorder/saveFormat", ui->comboSaveFormat->currentIndex());
}

void saveSettingsWindow::setControlEnable(bool en){
	//Only re-enable these 3 spinboxes if the save mode is not raw
	bool H264SettingsEnabled = (en && (ui->comboSaveFormat->currentIndex() == SAVE_MODE_H264));
	ui->spinBitrate->setEnabled(H264SettingsEnabled);
	ui->spinFramerate->setEnabled(H264SettingsEnabled);
	ui->spinMaxBitrate->setEnabled(H264SettingsEnabled);
	ui->lineFilename->setEnabled(en);
	ui->comboSaveFormat->setEnabled(en);
	ui->comboDrive->setEnabled(en);
	ui->cmdRefresh->setEnabled(en);
	ui->cmdUMount->setEnabled(en);
	ui->cmdClose->setEnabled(en);
}

void saveSettingsWindow::on_comboDrive_currentIndexChanged(const QString &arg1)
{
	if(okToSaveLocation) saveFileDirectory();
}
