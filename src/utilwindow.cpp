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

extern "C" {
#include "siText.h"
}

#include "util.h"
#include "chronosControlInterface.h"

#define FOCUS_PEAK_THRESH_LOW	35
#define FOCUS_PEAK_THRESH_MED	25
#define FOCUS_PEAK_THRESH_HIGH	15

#define USER_EXIT -2

extern const char* git_version_str;

bool copyFile(const char * fromfile, const char * tofile);
QString listStorageDevice(const char *blkdev);

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
	ui->comboFPColor->setCurrentIndex(camera->getFocusPeakColor() - 1);
	ui->chkZebraEnable->setChecked(camera->getZebraEnable());
    ui->chkShippingMode->setChecked(camera->get_shippingMode());

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
	ui->cmdCloseApp->setVisible(false);
	ui->cmdColumnGain->setVisible(false);
	ui->cmdWhiteRef->setVisible(false);
	ui->cmdSetSN->setVisible(false);
	ui->lineSerialNumber->setVisible(false);
	ui->chkShowDebugControls->setVisible(false);

	ui->chkAutoSave->setChecked(camera->get_autoSave());
	ui->chkAutoRecord->setChecked(camera->get_autoRecord());
	ui->chkDemoMode->setChecked(camera->get_demoMode());
	ui->chkUiOnLeft->setChecked(camera->getButtonsOnLeft());
	ui->comboDisableUnsavedWarning->setCurrentIndex(camera->getUnsavedWarnEnable());

	if(camera->RotationArgumentIsSet())
		ui->chkUpsideDownDisplay->setChecked(camera->getUpsideDownDisplay());
	else //If the argument was not added, set the control to invisible because it would be useless anyway
		ui->chkUpsideDownDisplay->setVisible(false);

	ui->chkShowDebugControls->setChecked(!(appSettings.value("debug/hideDebug", true).toBool()));
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
		FILE *fp;

		QString mText = "";
		fp = popen("df -h | sed -n \'1p;/\\/media\\/\\(sd\\|mmcblk1\\)/p\'", "r");
		if (fp) {
			while (fgets(line, sizeof(line), fp) != NULL) {
				mText.append(line);
			}
			pclose(fp);
		}
		ui->lblMountedDevices->setText(mText);

		/* Update the disk status text. */
		ui->lblStatusDisk->setText(listStorageDevice("sda"));
		ui->lblStatusSD->setText(listStorageDevice("mmcblk1"));
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
	camera->io->setOutLevel((1 << 1));	//Turn on output drive

	retVal = camera->autoColGainCorrection();

	if(SUCCESS != retVal)
	{
		sw.hide();
		QMessageBox msg;
		sprintf(text, "Error during gain calibration, error %d: %s", retVal, errorCodeString(retVal));
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
	camera->io->setOutLevel(0);	//Turn off output drive

	//Black cal all standard resolutions
	retVal = camera->blackCalAllStdRes(true);

	if(SUCCESS != retVal)
	{
		sw.hide();
		QMessageBox msg;
		sprintf(text, "Error during black calibration, error %d: %s", retVal, errorCodeString(retVal));
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
	if(ui->comboFPColor->count() < 7)	//Hack so the incorrect value doesn't get set during population of values
		return;
	camera->setFocusPeakColor(index + 1);
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
	camera->io->setOutLevel(0);	//Turn off output drive

	//Black cal all standard resolutions
	qDebug("cmdAutoCal: blackCalAllStdRes");
	retVal = camera->blackCalAllStdRes(true);

	if(SUCCESS != retVal)
	{
		sw.hide();
		QMessageBox msg;
		sprintf(text, "Error during black calibration, error %d: %s", retVal, errorCodeString(retVal));
		msg.setText(text);
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
		return;
	}

	//Turn on calibration light
	qDebug("cmdAutoCal: turn on cal light");
	camera->io->setOutLevel((1 << 1));	//Turn on output drive

	qDebug("cmdAutoCal: autoColGainCorrection");
	retVal = camera->autoColGainCorrection();

	if(SUCCESS != retVal) {
		sw.hide();
		QMessageBox msg;
		sprintf(text, "Error during gain calibration, error %d: %s", retVal, errorCodeString(retVal));
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
	camera->io->setOutLevel((1 << 1));	//Turn on output drive

	retVal = camera->takeWhiteReferences();

	if(SUCCESS != retVal)
	{
		QMessageBox msg;
		sprintf(text, "Error during white reference calibration, error %d: %s", retVal, errorCodeString(retVal));
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
		ui->cmdCloseApp->setVisible(true);
		ui->cmdColumnGain->setVisible(true);
		ui->cmdWhiteRef->setVisible(true);
		ui->cmdSetSN->setVisible(true);
		ui->lineSerialNumber->setVisible(true);
		ui->chkShowDebugControls->setVisible(true);
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

/* Quick and dirty wrapper to run a command and get the output. */
char *getCommandOutput(char *dest, size_t maxlen, const char *format, ...)
{
	char command[PATH_MAX+64];
	size_t readsz;
	FILE *fp;

	va_list args;
	va_start(args, format);
	vsnprintf(command, sizeof(command), format, args);
	va_end(args);

	fp = popen(command, "r");
	if (!fp) {
		return strcpy(dest, "");
	}
	readsz = fread(dest, 1, maxlen-1, fp);
	fclose(fp);
	dest[readsz] = '\0';
	return dest;
}

char *stripNewlines(char *s)
{
	char *newline = strpbrk(s, "\r\n");
	if (newline) *newline = '\0';
	return s;
}

QString listStorageDevice(const char *blkdev)
{
	char line[128];
	struct stat st;
	int fd;
	FILE *fp;

	char diskPath[PATH_MAX] = "/dev/";
	char command[PATH_MAX+64];
	QString diskText = "";

	/* Try opening the block device. */
	strcat(diskPath, blkdev);
	fd = open(diskPath, O_RDONLY);
	if (fd < 0) {
		return diskText;
	}
	while (fstat(fd, &st) == 0) {
		if (!S_ISBLK(st.st_mode)) break;
#ifdef DEBIAN
		sprintf(command, "lsblk --output NAME,SIZE,FSTYPE,LABEL,VENDOR,MODEL %s", diskPath);
		fp = popen(command, "r");
		if (!fp) break;
		while (fgets(line, sizeof(line), fp) != NULL) {
			diskText.append(line);
		}
		pclose(fp);
#else
		long blksize = 0;
		char partname[128];
		char fstype[16];
		char label[16];
		char szbuf[16];
		ioctl(fd, BLKGETSIZE, &blksize);

		/* Print the header */
		sprintf(line, "%-10s %7s %-6s %-10s %-8s %s\n", "NAME", "SIZE", "FSTYPE", "LABEL", "VENDOR", "MODEL");
		diskText.append(line);

		/* Print the block device info. */
		getSIText(szbuf, (double)blksize * 512, 4, 0, 4);
		sprintf(line, "%-10s %7s %-6s %-10s %-8s %s\n", blkdev, szbuf, "", "", "", "");
		diskText.append(line);

		/* Get the list of partitions. */
		sprintf(command, "blkid -o device %s?*", diskPath);
		fp = popen(command, "r");
		if (!fp) break;
		while (fgets(partname, sizeof(partname), fp) != NULL) {
			stripNewlines(partname);
			getCommandOutput(fstype, sizeof(fstype), "blkid -o value -s TYPE %s", partname);
			getCommandOutput(label, sizeof(label), "blkid -o value -s LABEL %s", partname);
			getCommandOutput(szbuf, sizeof(szbuf), "blockdev --getsz %s", partname);
			blksize = strtoul(szbuf, NULL, 0);
			getSIText(szbuf, (double)blksize * 512, 4, 0, 4);

			sprintf(line, "%-10s %7s %-6s %-10s %-8s %s\n", partname+5, szbuf, stripNewlines(fstype), stripNewlines(label), "", "");
			diskText.append(line);
		}
		pclose(fp);
#endif
		break;
	}
	close(fd);
	return diskText;
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
	progress->show();

	/* Unmount the block device and any of its partitions */
	progress->setLabelText("Unmounting devices");
	progress->setValue(0);
	filepathlen = sprintf(filepath, "/dev/%s", blkdev);
	sprintf(partpath, "/dev/%s%s", blkdev, isdigit(filepath[filepathlen-1]) ? "p1" : "1");
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
	qDebug("Formatting %s", partpath);
	if (progress->wasCanceled()) {
		delete progress;
		return;
	}
	progress->setValue(3);
	sprintf(command, "mkfs.vfat -n CHRONOS %s && sleep 1 && blockdev --rereadpt %s", partpath, filepath);
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

void UtilWindow::on_chkAutoPowerOn_stateChanged(int arg1)
{

}

void UtilWindow::on_chkAutoPowerOff_stateChanged(int arg1)
{

}

void UtilWindow::on_chkShippingMode_stateChanged()
{
    QMessageBox::information(this, "Shipping Mode","Enabling this will require the AC adapter to be plugged in to turn the camera on next time.", QMessageBox::Ok);
    camera->set_shippingMode(ui->chkShippingMode->isChecked());
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

	sprintf(str, "tar -xf /media/sda1/user_settings.tar -C /");

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

void UtilWindow::on_tabWidget_currentChanged(int index)
{
#ifdef QT_KEYPAD_NAVIGATION
	if (index == ui->tabWidget->indexOf(ui->tabKickstarter)) {
		ui->textKickstarter->setEditFocus(true);
	}
#endif
}
