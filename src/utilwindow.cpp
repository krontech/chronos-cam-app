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
#include "utilwindow.h"
#include "ui_utilwindow.h"
#include "statuswindow.h"

#include <QDebug>
#include <QMessageBox>
#include <QTimer>
#include <QSettings>
#include <QDBusInterface>
#include <QProgressDialog>

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/sendfile.h>
#include <mntent.h>

#include "control.h"
#include "util.h"
#include "chronosControlInterface.h"

#define FOCUS_PEAK_THRESH_LOW	35
#define FOCUS_PEAK_THRESH_MED	25
#define FOCUS_PEAK_THRESH_HIGH	15

#define USER_EXIT -2

extern const char* git_version_str;

bool copyFile(const char * fromfile, const char * tofile);

static char *readReleaseString(char *buf, size_t len)
{
	FILE * fp = fopen("filesystemRevision", "r");
	if (!fp) {
		return strcpy(buf, CAMERA_APP_VERSION);
	}
	if (fgets(buf, len, fp)) {
		/* strip off any newlines */
		buf[strcspn(buf, "\r\n")] = '\0';
	}
	else {
		strcpy(buf, CAMERA_APP_VERSION);
	}
	fclose(fp);
	return buf;
}

UtilWindow::UtilWindow(QWidget *parent, Camera * cameraInst) :
	QWidget(parent),
	ui(new Ui::UtilWindow)
{
	openingWindow = true;
	QSettings appSettings;
	QString aboutText;

	ui->setupUi(this);
	this->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
	this->move(0,0);

	camera = cameraInst;

	settingClock = false;
	ui->dateTimeEdit->setDateTime(QDateTime::currentDateTime());
	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(onUtilWindowTimer()));
	timer->start(500);

	ui->tabWidget->setCurrentIndex(0);

	ui->chkFPEnable->setChecked(camera->getFocusPeakEnable());
	//ui->comboFPColor->setEnabled(false);
	ui->comboFPColor->addItem("Blue");
	ui->comboFPColor->addItem("Green");
	ui->comboFPColor->addItem("Cyan");
	ui->comboFPColor->addItem("Red");
	ui->comboFPColor->addItem("Magenta");
	ui->comboFPColor->addItem("Yellow");
	ui->comboFPColor->addItem("White");
	ui->comboFPColor->setCurrentIndex(camera->getFocusPeakColorLL() - 1);
	ui->chkZebraEnable->setChecked(camera->getZebraEnable());

	if(camera->getFocusPeakThresholdLL() == FOCUS_PEAK_THRESH_HIGH)
		ui->radioFPSensHigh->setChecked(true);
	else if(camera->getFocusPeakThresholdLL() == FOCUS_PEAK_THRESH_MED)
		ui->radioFPSensMed->setChecked(true);
	else if(camera->getFocusPeakThresholdLL() == FOCUS_PEAK_THRESH_LOW)
		ui->radioFPSensLow->setChecked(true);


	ui->lineSerialNumber->setText(camera->getSerialNumber());

	//Fill about label with camera info
	UInt32 ramSizeSlot1, ramSizeSlot2;
	char serialNumber[33];
	char release[128];
	camera->getRamSizeGB(&ramSizeSlot1, &ramSizeSlot2);
	camera->readSerialNumber(serialNumber);

	aboutText.sprintf("Camera Model: Chronos 1.4, %s, %dGB\r\n", (camera->getIsColor() ? "Color" : "Monochrome"), ramSizeSlot1 + ramSizeSlot2);
	aboutText.append(QString("Serial Number: %1\r\n").arg(serialNumber));
	aboutText.append(QString("\r\n"));
	aboutText.append(QString("Release Version: %1\r\n").arg(readReleaseString(release, sizeof(release))));
	aboutText.append(QString("Build: %1 (%2)\r\n").arg(CAMERA_APP_VERSION, git_version_str));
	aboutText.append(QString("FPGA Revision: %1.%2").arg(QString::number(camera->getFPGAVersion()), QString::number(camera->getFPGASubVersion())));
	ui->lblAbout->setText(aboutText);
	
	ui->cmdAutoCal->setVisible(false);
	//ui->cmdBlackCalAll->setVisible(false);
	ui->cmdCloseApp->setVisible(false);
	ui->cmdColumnGain->setVisible(false);
	ui->cmdWhiteRef->setVisible(false);
	ui->cmdSetSN->setVisible(false);
	ui->lineSerialNumber->setVisible(false);
	//ui->chkShowDebugControls->setVisible(false);

	ui->chkAutoSave->setChecked(camera->get_autoSave());
	ui->chkAutoRecord->setChecked(camera->get_autoRecord());
	ui->chkDemoMode->setChecked(camera->get_demoMode());
	ui->chkUiOnLeft->setChecked(camera->getButtonsOnLeft());
	ui->comboDisableUnsavedWarning->setCurrentIndex(camera->getUnsavedWarnEnable());
	ui->comboAutoPowerMode->setCurrentIndex(camera->getAutoPowerMode());
	ui->spinAutoSavePercent->setValue(camera->getAutoSavePercent());

	ui->lineNetAddress->setEnabled(false);
	ui->lineNetUser->setEnabled(false);
	ui->lineNetPassword->setEnabled(false);
	ui->cmdNetTest->setEnabled(false);
	ui->lblNetStatus->setText("Placeholder - Work in Progress");

	if(camera->RotationArgumentIsSet())
		ui->chkUpsideDownDisplay->setChecked(camera->getUpsideDownDisplay());
	else //If the argument was not added, set the control to invisible because it would be useless anyway
		ui->chkUpsideDownDisplay->setVisible(false);

	ui->chkLiveLoop->setChecked(camera->liveLoopActive);
	ui->spinLiveLoopTime->setValue(camera->liveLoopTime);
	openingWindow = false;

	//ui->chkShowDebugControls->setChecked(!(appSettings.value("debug/hideDebug", true).toBool()));
}

UtilWindow::~UtilWindow()
{
	timer->stop();
	delete timer;
	delete ui;
}

void UtilWindow::on_cmdSWUpdate_clicked()
{
	int itr, retval;
	char location[100];
	
	for(itr = 1; itr <= 4; itr++)
	{
		//Look for the update on sda
		sprintf(location, "/media/sda%d/camUpdate/update.sh", itr);
		if((retval = updateSoftware(location)) != CAMERA_FILE_NOT_FOUND) return;
		
		//Also look for the update on sdb, as the usb is sometimes mounted there instead of sda
		sprintf(location, "/media/sdb%d/camUpdate/update.sh", itr);
		if((retval = updateSoftware(location)) != CAMERA_FILE_NOT_FOUND) return;
		
		//Look for the update on the SD card
		sprintf(location, "/media/mmcblk1p%d/camUpdate/update.sh", itr);
		if((retval = updateSoftware(location)) != CAMERA_FILE_NOT_FOUND) return;
	}

	QMessageBox msg;
	msg.setText("No software update found");
	msg.setWindowFlags(Qt::WindowStaysOnTopHint);
	//msg.setInformativeText("Update");
	msg.exec();
}

int UtilWindow::updateSoftware(char * updateLocation){
	struct stat buffer;
	char mesg[100];

	if(stat (updateLocation, &buffer) == 0)	//If file exists
	{
		QMessageBox::StandardButton reply;
		reply = QMessageBox::question(this, "Software update", "Found software update, do you want to install it now?\r\nThe display may go blank and the camera may restart during this process.\r\nWARNING: Any unsaved video in RAM will be lost.", QMessageBox::Yes|QMessageBox::No);
		if(QMessageBox::Yes != reply)
			return USER_EXIT;

		UInt32 retVal = system(updateLocation);
		QMessageBox msg;
		sprintf(mesg, "Update complete! Please restart camera to complete update.");
		msg.setText(mesg);
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
		return SUCCESS;
	}
	return CAMERA_FILE_NOT_FOUND;
}
bool copyFile(const char * fromfile, const char * tofile)
{

	int fromfd, tofd;
	off_t off = 0;
	int rv, len;

	struct stat stat_buf;
	rv = stat(fromfile, &stat_buf);
	len = (rv == 0) ? stat_buf.st_size : 0;

	if(0 == len)
		return false;

	if (unlink(tofile) < 0 && errno != ENOENT) {
		perror("unlink");
		return false;
	}

	errno = 0;
	if ((fromfd = open(fromfile, O_RDONLY)) < 0 ||
		(tofd = open(tofile, O_WRONLY | O_CREAT)) < 0) {
		perror("open");
		return false;
	}

	if ((rv = sendfile(tofd, fromfd, &off, len)) < 0) {
		qDebug() << "Error: sendfile returned" << rv << "errno:" << errno;
	}
	chmod(tofile, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH);
	qDebug() << "Copy completed, " << len / 1024 << "kiB transferred.";

	close(fromfd);
	close(tofd);
	return true;
}

void UtilWindow::onUtilWindowTimer()
{
	if(!settingClock)
	{
		if(ui->dateTimeEdit->hasFocus())
			settingClock = true;
		else
			ui->dateTimeEdit->setDateTime(QDateTime::currentDateTime());
	}

	/* If on the storage tab, update the drive and network status. */
	if (ui->tabWidget->currentWidget() == ui->tabStorage) {
		char line[128];
		QString mText = "";
		FILE *fp = popen("df -h /media/*", "r");
		if (!fp) return;
		while (fgets(line, sizeof(line), fp) != NULL) {
			mText.append(line);
		}
		pclose(fp);
		ui->lblMountedDevices->setText(mText);
	}
}

void UtilWindow::on_cmdSetClock_clicked()
{
	time_t newTime;
	int retval;
	newTime = ui->dateTimeEdit->dateTime().toTime_t();
	retval = stime(&newTime);
	settingClock = false;

	if(retval == -1)
		qDebug() << "Couldn't set time, errorno =" << errno;
}

void UtilWindow::on_cmdColumnGain_clicked()
{
	StatusWindow sw;
	Int32 retVal;
	char text[100];

	sw.setText("Performing column gain calibration. Please wait...");
	sw.show();
	QCoreApplication::processEvents();

	//Turn on calibration light
	camera->setBncDriveLevel((1 << 1));	//Turn on output drive

	//retVal = camera->autoColGainCorrection();

	if(SUCCESS != retVal)
	{
		sw.hide();
		QMessageBox msg;
		sprintf(text, "Error during gain calibration, error %d", retVal);
		msg.setText(text);
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
	}
	else
	{
		sw.hide();
		QMessageBox msg;
		sprintf(text, "Column gain calibration was successful");
		msg.setText(text);
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
	}
}

void UtilWindow::on_cmdBlackCalAll_clicked()
{
	StatusWindow sw;
	Int32 retVal;
	char text[100];

	sw.setText("Performing black cal on all standard resolutions. Please wait...");
	sw.show();
	QCoreApplication::processEvents();

	//Turn off calibration light
	camera->setBncDriveLevel(0);	//Turn off output drive

	//Black cal all standard resolutions
	retVal = blackCalAllStdRes();

	if(SUCCESS != retVal)
	{
		sw.hide();
		QMessageBox msg;
		sprintf(text, "Error during black calibration, error %d", retVal);
		msg.setText(text);
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
	}
	else
	{
		sw.hide();
		QMessageBox msg;
		sprintf(text, "Black cal of all standard resolutions was successful");
		msg.setText(text);
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
	}
}

void UtilWindow::on_cmdCloseApp_clicked()
{
	qApp->exit();
}

void UtilWindow::on_chkFPEnable_stateChanged(int arg1)
{
	camera->setFocusPeakEnable(ui->chkFPEnable->checkState());
}

void UtilWindow::on_chkZebraEnable_stateChanged(int arg1)
{
	camera->setZebraEnable(ui->chkZebraEnable->checkState());
}

void UtilWindow::on_comboFPColor_currentIndexChanged(int index)
{
	static QString fpColors[] = {"blue", "green", "cyan", "red", "magenta", "yellow", "white"};

	if(ui->comboFPColor->count() < 7)	//Hack so the incorrect value doesn't get set during population of values
		return;
	if (index < 0) return;		//avoids errors during population
	camera->cinst->setString("focusPeakingColor", fpColors[index]);
}

void UtilWindow::on_radioFPSensLow_toggled(bool checked)
{
	if(checked)
	{
		camera->setFocusPeakThresholdLL(FOCUS_PEAK_THRESH_LOW);
		QSettings appSettings;
		appSettings.setValue("camera/focusPeakThreshold", FOCUS_PEAK_THRESH_LOW);
	}

}

void UtilWindow::on_radioFPSensMed_toggled(bool checked)
{
	if(checked)
	{
		camera->setFocusPeakThresholdLL(FOCUS_PEAK_THRESH_MED);
		QSettings appSettings;
		appSettings.setValue("camera/focusPeakThreshold", FOCUS_PEAK_THRESH_MED);
	}
}

void UtilWindow::on_radioFPSensHigh_toggled(bool checked)
{
	if(checked)
	{
		camera->setFocusPeakThresholdLL(FOCUS_PEAK_THRESH_HIGH);
		QSettings appSettings;
		appSettings.setValue("camera/focusPeakThreshold", FOCUS_PEAK_THRESH_HIGH);
	}
}

void UtilWindow::on_cmdAutoCal_clicked()
{
	StatusWindow sw;
	Int32 retVal;
	char text[100];

	sw.setText("Performing factory calibration. Please wait...");
	sw.show();
	QCoreApplication::processEvents();

	//Turn off calibration light
	qDebug("cmdAutoCal: turn off cal light");
	camera->setBncDriveLevel(0);	//Turn off output drive

	//Black cal all standard resolutions
	qDebug("cmdAutoCal: blackCalAllStdRes");
	retVal = camera->blackCalAllStdRes(true);

	if(SUCCESS != retVal)
	{
		sw.hide();
		QMessageBox msg;
		sprintf(text, "Error during black calibration, error %d", retVal);
		msg.setText(text);
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
		return;
	}

	//Turn on calibration light
	qDebug("cmdAutoCal: turn on cal light");
	camera->setBncDriveLevel((1 << 1));	//Turn on output drive

	qDebug("cmdAutoCal: autoColGainCorrection");
	//retVal = camera->autoColGainCorrection();

	if(SUCCESS != retVal) {
		sw.hide();
		QMessageBox msg;
		sprintf(text, "Error during gain calibration, error %d", retVal);
		msg.setText(text);
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
		return;
	}
	else {
		sw.hide();
		QMessageBox msg;
		msg.setText("Done!");
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
	}
}



void UtilWindow::on_cmdClose_clicked()
{
	bool ButtonsOnLeftChanged = (camera->ButtonsOnLeft !=	 ui->chkUiOnLeft->isChecked());
	bool UpsideDownDisplayChanged = (camera->UpsideDownDisplay != ui->chkUpsideDownDisplay->isChecked());

	if(ButtonsOnLeftChanged) {
		camera->setButtonsOnLeft(ui->chkUiOnLeft->isChecked());
		emit moveCamMainWindow();
	}

	if(UpsideDownDisplayChanged){
		camera->setUpsideDownDisplay(ui->chkUpsideDownDisplay->isChecked());
		camera->upsideDownTransform(camera->UpsideDownDisplay ? 2 : 0);//2 for upside down, 0 for normal
	}

	if(UpsideDownDisplayChanged ^ ButtonsOnLeftChanged) camera->updateVideoPosition();

	camera->liveLoopActive = ui->chkLiveLoop->isChecked();
	if (camera->liveLoopTime != ui->spinLiveLoopTime->value())
	{
		camera->on_spinLiveLoopTime_valueChanged(ui->spinLiveLoopTime->value());
	}
	camera->liveLoopTime = ui->spinLiveLoopTime->value();

	close();
/*
	if((camera->ButtonsOnLeft !=	 ui->chkUiOnLeft->isChecked()) ||
		camera->UpsideDownDisplay != ui->chkUpsideDownDisplay->isChecked())
			{
				camera->updateVideoPosition();
	*/
}

void UtilWindow::on_cmdClose_2_clicked()
{
	qDebug()<<"on_cmdClose_2_clicked";
	on_cmdClose_clicked();
}

void UtilWindow::on_cmdClose_3_clicked()
{
	qDebug()<<"on_cmdClose_3_clicked";
	on_cmdClose_clicked();
}

void UtilWindow::on_cmdClose_4_clicked()
{
	qDebug()<<"on_cmdClose_4_clicked";
	on_cmdClose_clicked();
}

void UtilWindow::on_cmdWhiteRef_clicked()
{
	char text[100];
	Int32 retVal;
	//Turn on calibration light
	camera->setBncDriveLevel((1 << 1));	//Turn on output drive

	retVal = camera->takeWhiteReferences();

	if(SUCCESS != retVal)
	{
		QMessageBox msg;
		sprintf(text, "Error during white reference calibration, error %d", retVal);
		msg.setText(text);
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
	}
	else
	{
		QMessageBox msg;
		msg.setText("Done!");
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
	}
}

void UtilWindow::on_cmdSetSN_clicked()
{
	if(ui->lineSerialNumber->text().length() > SERIAL_NUMBER_MAX_LEN)
	{
		QMessageBox msg;
		msg.setText("Serial number too long");
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
		return;
	}
	camera->setSerialNumber(ui->lineSerialNumber->text().toStdString().c_str());
	camera->writeSerialNumber(camera->getSerialNumber());
}

void UtilWindow::statErrorMessage(){
	QMessageBox msg;
	msg.setText("Error: USB device not detected"); // /media/sda1
	msg.setWindowFlags(Qt::WindowStaysOnTopHint);
	msg.exec();
}

void UtilWindow::on_cmdSaveCal_clicked()
{
	StatusWindow sw;
	QMessageBox msg;
	Int32 retVal;
	char str[500];
	char path[250];
	struct stat st;

	retVal = stat("/media/sda1",&st);
	if(retVal != 0)
	{
		statErrorMessage();
		return;
	}

	if(S_ISDIR(st.st_mode) == false)
	{
		msg.setText("Error: /media/sda1 not present");
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
		return;
	}

	//Check that the directory is writable
	if(access("/media/sda1", W_OK) != 0)
	{	//Not writable
		msg.setText("Error: /media/sda1 is not present or not writable");
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
		return;
	}

	struct stat mp, mp_parent;
	stat("/media/sda1",&mp);
	stat("/media/sda1/..",&mp_parent);

	if(mp.st_dev == mp_parent.st_dev)
	{
		msg.setText("Error: no device is mounted to /media/sda1");
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
		return;
	}

	sw.setText("Saving calibration to /media/sda1. Please wait...");
	sw.show();
	QCoreApplication::processEvents();

	sprintf(path, "/media/sda1/cal_%s", camera->getSerialNumber());

	sprintf(str, "tar -cf %s.tar cal", path);

	retVal = system(str);	//tar cal files

	if(0 != retVal)
	{
		sw.hide();
		msg.setText("Error: tar command failed");
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
		return;
	}

	sw.hide();
	msg.setText("Calibration backup successful!");
	msg.setWindowFlags(Qt::WindowStaysOnTopHint);
	msg.exec();
}

void UtilWindow::on_cmdRestoreCal_clicked()
{
	StatusWindow sw;
	QMessageBox msg;
	Int32 retVal;
	char str[500];
	char path[250];
	struct stat st;

	retVal = stat("/media/sda1",&st);
	if(retVal != 0)
	{
		statErrorMessage();
		return;
	}

	if(S_ISDIR(st.st_mode) == false)
	{
		msg.setText("Error: /media/sda1 not present");
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
		return;
	}

	//Check that the directory is writable
	if(access("/media/sda1", R_OK) != 0)
	{	//Not readable
		msg.setText("Error: /media/sda1 is not present or not readable");
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
		return;
	}

	struct stat mp, mp_parent;
	stat("/media/sda1",&mp);
	stat("/media/sda1/..",&mp_parent);

	if(mp.st_dev == mp_parent.st_dev)
	{
		msg.setText("Error: no device is mounted to /media/sda1");
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
		return;
	}

	sw.setText("Restoring calibration from /media/sda1. Please wait...");
	sw.show();
	QCoreApplication::processEvents();

	sprintf(path, "/media/sda1/cal_%s", camera->getSerialNumber());

	sprintf(str, "tar -xf %s.tar", path);

	retVal = system(str);	//tar cal files

	if(0 != retVal)
	{
		sw.hide();
		msg.setText("Error: tar command failed");
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
		return;
	}

	sw.hide();
	msg.setText("Calibration restore successful!");
	msg.setWindowFlags(Qt::WindowStaysOnTopHint);
	msg.exec();
}

void UtilWindow::on_linePassword_textEdited(const QString &arg1)
{
	if(0 == QString::compare(arg1, "4242"))
	{
		ui->cmdAutoCal->setVisible(true);
		//ui->cmdBlackCalAll->setVisible(true);
		ui->cmdCloseApp->setVisible(true);
		ui->cmdColumnGain->setVisible(true);
		ui->cmdWhiteRef->setVisible(true);
		ui->cmdSetSN->setVisible(true);
		ui->lineSerialNumber->setVisible(true);
		//ui->chkShowDebugControls->setVisible(true);
	}
}

void UtilWindow::on_cmdEjectSD_clicked()
{
	if(umount2("/media/mmcblk1p1", 0))
	{ //Failed, show error
		QMessageBox msg;
		msg.setText("Eject failed! This may mean that the card is not installed, or was already ejected.");
		msg.setInformativeText("/media/mmcblk1p1");
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
	}
	else
	{
		QMessageBox msg;
		msg.setText("SD card is now safe to remove");
		msg.setInformativeText("/media/mmcblk1p1");
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
	}
}

void UtilWindow::formatStorageDevice(const char *blkdev)
{
	QMessageBox::StandardButton reply;
	QProgressDialog *progress;
	FILE * mtab = setmntent("/etc/mtab", "r");
	struct mntent mnt;
	char tempbuf[4096];		//Temp buffer used by mntent
	char command[128];
	char filepath[128];
	char partpath[128];
	char diskname[128];
	int filepathlen;
	FILE *fp;
	int ret;

	/* Read the disk name from sysfs. */
	sprintf(filepath, "/sys/block/%s/device/model", blkdev);
	if ((fp = fopen(filepath, "r")) != NULL) {
		int len = fread(diskname, 1, sizeof(diskname)-1, fp);
		diskname[len] = '\0';
		fclose(fp);
	}
	else {
		sprintf(filepath, "/sys/block/%s/device/name", blkdev);
		if ((fp = fopen(filepath, "r")) != NULL) {
			int len = fread(diskname, 1, sizeof(diskname)-1, fp);
			diskname[len] = '\0';
			fclose(fp);
		}
		else {
			strcpy(diskname, blkdev);
		}
	}

	/* Prompt the user for confirmation */
	reply = QMessageBox::question(this, QString("Format Device: %1").arg(diskname),
								  "This will erase all data on the device, are you sure you want to continue?",
								  QMessageBox::Yes|QMessageBox::No);
	if(QMessageBox::Yes != reply)
		return;

	progress = new QProgressDialog(this);
	progress->setWindowTitle(QString("Format Device: %1").arg(diskname));
	progress->setMaximum(4);
	progress->setMinimumDuration(0);
	progress->setWindowModality(Qt::WindowModal);

	/* Unmount the block device and any of its partitions */
	progress->setLabelText("Unmounting devices");
	progress->setValue(0);
	filepathlen = sprintf(filepath, "/dev/%s", blkdev);
	sprintf(partpath, "/dev/%s%s", blkdev, isdigit(blkdev[filepathlen-1]) ? "p1" : "1");
	while (getmntent_r(mtab, &mnt, tempbuf, sizeof(tempbuf)) != NULL) {
		if (strncmp(filepath, mnt.mnt_fsname, filepathlen) != 0) continue;
		qDebug("Unmounting %s", mnt.mnt_dir);
		umount2(mnt.mnt_dir, 0);
	}
	endmntent(mtab);

	/* Overwrite the first 4kB with zeros and then rebuild the partition table. */
	progress->setLabelText("Wiping partition table");
	if (progress->wasCanceled()) {
		delete progress;
		return;
	}
	progress->setValue(1);
	fp = fopen(filepath, "wb");
	if (fp) {
		fwrite(memset(tempbuf, 0, sizeof(tempbuf)), sizeof(tempbuf), 1, fp);
		fflush(fp);
		fclose(fp);
	}

	progress->setLabelText("Writing partition table");
	if (progress->wasCanceled()) {
		delete progress;
		return;
	}
	progress->setValue(2);
	sprintf(command, "echo \"0 - c -\" | sfdisk -Dq %s && sleep 1", filepath);
	system(command);

	/* Delay for a second to give the kernel some time to reload the partitons, and then format the disk. */
	progress->setLabelText("Formatting partition");
	if (progress->wasCanceled()) {
		delete progress;
		return;
	}
	progress->setValue(3);
	sprintf(command, "mkfs.vfat %s && sleep 1 && blockdev --rereadpt %s", partpath, filepath);
	system(command);

	/* Turn the 'cancel' button into a 'done' button */
	progress->setLabelText("Formatting complete");
	progress->setValue(4);
	delete progress;
}

void UtilWindow::on_cmdFormatSD_clicked()
{
	formatStorageDevice("mmcblk1");
}

void UtilWindow::on_cmdEjectDisk_clicked()
{
	if(umount2("/media/sda1", 0))
	{ //Failed, show error
		QMessageBox msg;
		msg.setText("Eject failed! This may mean that the drive is not installed, or was already ejected.");
		msg.setInformativeText("/media/sda1");
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
	}
	else
	{
		QMessageBox msg;
		msg.setText("USB drive is now safe to remove");
		msg.setInformativeText("/media/sda1");
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
	}
}

void UtilWindow::on_cmdFormatDisk_clicked()
{
	formatStorageDevice("sda");
}

void UtilWindow::on_chkAutoSave_stateChanged(int arg1)
{
	camera->set_autoSave(ui->chkAutoSave->isChecked());
}

void UtilWindow::on_chkAutoRecord_stateChanged(int arg1)
{
	camera->set_autoRecord(ui->chkAutoRecord->isChecked());
}

void UtilWindow::on_chkDemoMode_stateChanged(int arg1)
{
	camera->set_demoMode(ui->chkDemoMode->isChecked());
}

void UtilWindow::on_cmdDefaults_clicked()
{
	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(this, "Revert settings", "Are you sure you want to delete all settings and return to defaults?", QMessageBox::Yes|QMessageBox::No);
	if(QMessageBox::Yes != reply)
		return;

	QSettings appSettings;
	appSettings.clear();
	appSettings.sync();

	QMessageBox::StandardButton reply2;
	reply2 = QMessageBox::question(this, "Restart app?", "Current settings are cleared. Is it okay to restart the app so defaults can be selected?", QMessageBox::Yes|QMessageBox::No);
	if(QMessageBox::Yes != reply2)
		return;

	system("killall camApp && /etc/init.d/camera restart");
}

void UtilWindow::on_cmdBackupSettings_clicked()
{
	QSettings appSettings;
	StatusWindow sw;
	QMessageBox msg;
	Int32 retVal;
	char str[500];
	struct stat st;

	appSettings.sync();

	retVal = stat("/media/sda1",&st);
	if(retVal != 0)
	{
		statErrorMessage();
		return;
	}

	if(S_ISDIR(st.st_mode) == false)
	{
		msg.setText("Error: /media/sda1 not present");
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
		return;
	}

	//Check that the directory is writable
	if(access("/media/sda1", W_OK) != 0)
	{	//Not writable
		msg.setText("Error: /media/sda1 is not present or not writable");
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
		return;
	}

	struct stat mp, mp_parent;
	stat("/media/sda1",&mp);
	stat("/media/sda1/..",&mp_parent);

	if(mp.st_dev == mp_parent.st_dev)
	{
		msg.setText("Error: no device is mounted to /media/sda1");
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
		return;
	}

	sw.setText("Saving user settings to /media/sda1. Please wait...");
	sw.show();
	QCoreApplication::processEvents();

	sprintf(str, "tar -cf /media/sda1/user_settings.tar /Settings/KronTech");

	retVal = system(str);	//tar cal files

	if(0 != retVal)
	{
		sw.hide();
		msg.setText("Error: tar command failed");
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
		return;
	}

	sw.hide();
	msg.setText("User settings backup successful!");
	msg.setWindowFlags(Qt::WindowStaysOnTopHint);
	msg.exec();
}

void UtilWindow::on_cmdRestoreSettings_clicked()
{
	StatusWindow sw;
	QMessageBox msg;
	Int32 retVal;
	char str[500];
	struct stat st;

	retVal = stat("/media/sda1",&st);
	if(retVal != 0)
	{
		statErrorMessage();
		return;
	}

	if(S_ISDIR(st.st_mode) == false)
	{
		msg.setText("Error: /media/sda1 not present");
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
		return;
	}

	//Check that the directory is writable
	if(access("/media/sda1", R_OK) != 0)
	{	//Not readable
		msg.setText("Error: /media/sda1 is not present or not readable");
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
		return;
	}

	struct stat mp, mp_parent;
	stat("/media/sda1",&mp);
	stat("/media/sda1/..",&mp_parent);

	if(mp.st_dev == mp_parent.st_dev)
	{
		msg.setText("Error: no device is mounted to /media/sda1");
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
		return;
	}

	sw.setText("Restoring user settings from /media/sda1. Please wait...");
	sw.show();
	QCoreApplication::processEvents();

	sprintf(str, "tar -xf /media/sda1/user_settings.tar -C /Settings");

	retVal = system(str);	//tar cal files

	if(0 != retVal)
	{
		sw.hide();
		msg.setText("Error: tar command failed");
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
		return;
	}
	QSettings appSettings;
	appSettings.sync();

	sw.hide();
	msg.setText("User settings restore successful!");
	msg.setWindowFlags(Qt::WindowStaysOnTopHint);
	msg.exec();

	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(this, "Restart app?", "To apply the restored settings the app must restart. Is it okay to restart?", QMessageBox::Yes|QMessageBox::No);
	if(QMessageBox::Yes != reply)
		return;

	system("killall camApp && /etc/init.d/camera restart");
}



void UtilWindow::on_chkShowDebugControls_toggled(bool checked)
{
	QSettings appSettings;
	appSettings.setValue("debug/hideDebug", !checked);
}

void UtilWindow::on_cmdRevertCalData_pressed()
{
	Int32 retVal;
	char str[500];

	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(this, "Revert to factory cal?", "Are you sure you want to delete all user calibration data?", QMessageBox::Yes|QMessageBox::No);
	if(QMessageBox::Yes != reply)
		return;

	// delete all files under userFPN/
	sprintf(str, "rm userFPN/*");
	retVal = system(str);

	if(0 != retVal)
	{
		QMessageBox msg;
		msg.setText("Error: removing userFPN failed");
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
		return;
	}

	QMessageBox msg;
	msg.setText("User calibration data deleted");
	msg.setWindowFlags(Qt::WindowStaysOnTopHint);
	msg.exec();
}

void UtilWindow::on_comboDisableUnsavedWarning_currentIndexChanged(int index)
{
	camera->setUnsavedWarnEnable(index);
}

void UtilWindow::on_comboAutoPowerMode_currentIndexChanged(int index)
{
	camera->setAutoPowerMode(index);
}

void UtilWindow::on_tabWidget_currentChanged(int index)
{
#ifdef QT_KEYPAD_NAVIGATION
	if (index == ui->tabWidget->indexOf(ui->tabKickstarter)) {
		ui->textKickstarter->setEditFocus(true);
	}
#endif
}

void UtilWindow::on_chkLiveLoop_stateChanged(int arg1)
{
	if (!openingWindow)
	{
		camera->on_chkLiveLoop_stateChanged(arg1);
	}
}

void UtilWindow::on_spinLiveLoopTime_valueChanged(int arg1)
{
	if (!openingWindow)
	{
		camera->on_spinLiveLoopTime_valueChanged(arg1);
	}
}

void UtilWindow::on_spinAutoSavePercent_valueChanged(int arg1)
{
	camera->setAutoSavePercent(arg1);
}

void UtilWindow::on_chkAutoSavePercent_stateChanged(int arg1)
{
	camera->setAutoSavePercentEnabled(arg1);

}

void UtilWindow::on_chkFanDisable_stateChanged(int arg1)
{
	camera->setFanDisable(arg1);
}

void UtilWindow::on_chkShippingMode_stateChanged(int arg1)
{
	camera->setShippingMode(arg1);
}


// helper function to wait for idle state
void UtilWindow::waitForIdle(void)
{
	bool waiting;
	do
	{
		QString state;
		camera->cinst->getString("state", &state);
		waiting = (state != "idle");

		if (waiting)
		{
			delayms(50);
		}
	} while (waiting);

}


Int32 UtilWindow::blackCalAllStdRes(void)
{
	QProgressDialog *progress;
	QElapsedTimer timer;
	timer.start();

	//save settings
	FrameGeometry saveGeometry;
	UInt32 keepFramePeriod;
	UInt32 keepIntegrationTime;
	UInt32 keepCurrentGain;

	camera->cinst->getInt("exposurePeriod", &keepIntegrationTime);
	camera->cinst->getInt("framePeriod", &keepFramePeriod);
	camera->cinst->getInt("currentGain", &keepCurrentGain);
	camera->cinst->getResolution(&saveGeometry);

	UInt32 maxGain;
	camera->cinst->getInt("sensorMaxGain", &maxGain);

	UInt32 numGains = 0;
	for (int i=maxGain; i > 0; i /= 2)
	{
		numGains++;
	}

	UInt32 hMax;
	UInt32 vMax;

	camera->cinst->getInt("sensorHMax", &hMax);
	camera->cinst->getInt("sensorVMax", &vMax);

	//get the common resolutions from the list of resolutions
	QFile fp;
	QString filename;
	QByteArray line;
	QString lineText;

	filename.append("camApp:resolutions");
	QFileInfo resolutionsFile(filename);
	if (resolutionsFile.exists() && resolutionsFile.isFile()) {
		fp.setFileName(filename);
		fp.open(QIODevice::ReadOnly);
		if(!fp.isOpen()) {
			qDebug("Error: resolutions file couldn't be opened");
		}
	}
	else {
		qDebug("Error: resolutions file isn't present");
	}

	//now count how many resolutions
	UInt32 numResolutions = 0;
	while(true) {
		line = fp.readLine(30);
		if (line.isEmpty() || line.isNull())
			break;
		numResolutions++;
	}

	//close and re-open
	fp.close();
	fp.open(QIODevice::ReadOnly);

	progress = new QProgressDialog(this);
	progress->setWindowTitle(QString("Performing black cal on all standard resolutions."));
	progress->setMaximum(numGains * numResolutions);
	progress->setMinimumDuration(0);
	progress->setWindowModality(Qt::WindowModal);
	progress->resize(progress->size() + QSize(200, 20));

	progress->setLabelText("Preparing for black calibration");
	progress->setValue(0);
	progress->show();

	UInt32 progressCount = 0;

	UInt32 lastHRes = -1;

	bool cancelButton = false;

	while(true) {
		FrameGeometry fSize;

		line = fp.readLine(30);
		if (line.isEmpty() || line.isNull())
			break;

		//Get the resolution
		sscanf(line.constData(), "%dx%d", &fSize.hRes, &fSize.vRes);
		fSize.bitDepth = BITS_PER_PIXEL;
		fSize.vDarkRows = 0;
		fSize.minFrameTime = 0.001;

		//Center
		fSize.hOffset = round((hMax - fSize.hRes) / 2);
		fSize.vOffset = round((vMax - fSize.vRes) / 2);

		bool doAnalogCal = false;

		if (fSize.hRes != lastHRes)
		{
			lastHRes = fSize.hRes;
			doAnalogCal = true;
		}

		waitForIdle();
		camera->cinst->setResolution(&fSize);

		//now do calibration for each gain
		UInt32 gain = 1;


		do
		{
			char str[200];
			sprintf(str, "%ux%u at gain %u", fSize.hRes, fSize.vRes, gain);
			progress->setLabelText(str);
			progress->setValue(progressCount);
			QCoreApplication::processEvents();
			progressCount++;

			if (progress->wasCanceled())
			{
				cancelButton = true;
				break;
			}

			waitForIdle();
			camera->cinst->setInt("currentGain", gain);
			qDebug() << "black cal at gain" << gain;
			waitForIdle();
			camera->cinst->startCalibration("blackCal");

			if (doAnalogCal)
			{
				if (progress->wasCanceled())
				{
					cancelButton = true;
					break;
				}

				waitForIdle();
				camera->cinst->startCalibration("analogCal");
			}

			gain *= 2;

		} while ((gain <= maxGain) && !cancelButton);
	}

	fp.close();
	delete progress;

	//restore settings
	waitForIdle();
	camera->cinst->setResolution(&saveGeometry);
	camera->cinst->setInt("exposurePeriod", keepIntegrationTime);
	camera->cinst->setInt("framePeriod", keepFramePeriod);
	camera->cinst->setInt("currentGain", keepCurrentGain);

	qDebug() << "Black cal all resolutions took" << timer.elapsed()/1000 << "seconds";

	if (cancelButton)
	{
		return CAMERA_ERROR_SENSOR; //some error!
	}
	else
	{
		return SUCCESS;
	}
}

