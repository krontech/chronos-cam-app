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
#include <sys/statvfs.h>

#include "control.h"
#include "util.h"
#include "chronosControlInterface.h"
#include "networkconfig.h"

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
	ui->comboFPColor->setCurrentIndex(camera->getFocusPeakColor() - 1);
	ui->chkZebraEnable->setChecked(camera->getZebraEnable());
	ui->chkShippingMode->setChecked(camera->cinst->getProperty("shippingMode", false).toBool());

	if (camera->focusPeakEnabled)
	{
		UInt32 thresh = camera->getFocusPeakThresholdLL();
		if(thresh == FOCUS_PEAK_THRESH_HIGH)
			ui->radioFPSensHigh->setChecked(true);
		else if(thresh == FOCUS_PEAK_THRESH_MED)
			ui->radioFPSensMed->setChecked(true);
		else if(thresh == FOCUS_PEAK_THRESH_LOW)
			ui->radioFPSensLow->setChecked(true);
	}
	else
	{
		if(camera->focusPeakLevel >= FOCUS_PEAK_API_HIGH)
			ui->radioFPSensHigh->setChecked(true);
		else if(camera->focusPeakLevel >= FOCUS_PEAK_API_MED)
			ui->radioFPSensMed->setChecked(true);
		else ui->radioFPSensLow->setChecked(true);
	}


	ui->lineSerialNumber->setText(camera->getSerialNumber());

	//Fill about label with camera info
	UInt32 ramSizeGB;
	QString modelName;
	QString serialNumber;
	QString fpgaVersion;
	const char *modelFullName = "Chronos 1.4";
	char release[128];

	camera->cinst->getInt("cameraMemoryGB", &ramSizeGB);
	camera->cinst->getString("cameraModel", &modelName);
	camera->cinst->getString("cameraSerial", &serialNumber);
	camera->cinst->getString("cameraFpgaVersion", &fpgaVersion);

	// Chop the version digits off the end of the camera model.
	if (modelName.startsWith("CR14")) modelFullName = "Chronos 1.4";
	else if (modelName.startsWith("CR21")) modelFullName = "Chronos 2.1";

	aboutText.sprintf("Camera Model: %s, %s, %dGB\r\n", modelFullName, (camera->getIsColor() ? "Color" : "Monochrome"), ramSizeGB);
	aboutText.append(QString("Serial Number: %1\r\n").arg(serialNumber));
	aboutText.append(QString("\r\n"));
	aboutText.append(QString("Release Version: %1\r\n").arg(readReleaseString(release, sizeof(release))));
#ifdef DEBIAN
	QFile releaseFile("/etc/apt/sources.list.d/krontech-debian.list");
	if (releaseFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QTextStream fileStream(&releaseFile);
		QRegExp whitespace = QRegExp("\\s+");
		while (!fileStream.atEnd()) {
			QStringList slist = fileStream.readLine().split(whitespace);
			if (slist.count() < 4) continue;
			if (slist[0] != "deb") continue;
			aboutText.append(QString("Release Codename: %1\r\n").arg(slist[2]));
		}
	}
#endif
	aboutText.append(QString("Build: %1 (%2)\r\n").arg(CAMERA_APP_VERSION, git_version_str));
	aboutText.append(QString("FPGA Revision: %1").arg(fpgaVersion));
	ui->lblAbout->setText(aboutText);
	
	ui->cmdCloseApp->setVisible(false);
	ui->cmdColumnGain->setVisible(false);
	ui->cmdSetSN->setVisible(false);
	ui->cmdExportCalData->setVisible(false);
	ui->cmdImportCalData->setVisible(false);
	ui->lineSerialNumber->setVisible(false);
	ui->chkShowDebugControls->setVisible(false);
	ui->chkFanDisable->setVisible(false);

	/* fan override is either [1.0,0.0] to set a PWM, or < 0 for auto. */
	double fanOverride = camera->cinst->getProperty("fanOverride", -1).toDouble();
	ui->chkFanDisable->setChecked(fanOverride >= 0);

	ui->chkAutoSave->setChecked(camera->get_autoSave());
	ui->chkAutoRecord->setChecked(camera->get_autoRecord());
	ui->chkDemoMode->setChecked(camera->get_demoMode());
	ui->chkUiOnLeft->setChecked(camera->getButtonsOnLeft());
	ui->comboDisableUnsavedWarning->setCurrentIndex(camera->getUnsavedWarnEnable());
	ui->comboAutoPowerMode->setCurrentIndex(camera->getAutoPowerMode());


	/* Load the Samba network settings. */
	ui->lineSmbUser->setText(appSettings.value("network/smbUser", "").toString());
	ui->lineSmbShare->setText(appSettings.value("network/smbShare", "").toString());
	/* Place a dummy password until set by the user. */
	//ui->lineSmbPassword->setText("********");
	ui->lineSmbPassword->setText(appSettings.value("network/smbPassword", "").toString());
	ui->lineSmbPassword->setEchoMode(QLineEdit::PasswordEchoOnEdit);
	lineSmbPassword_wasEdited = false;

	/* Load the NFS network settings. */
	ui->lineNfsAddress->setText(appSettings.value("network/nfsAddress", "").toString());
	ui->lineNfsMount->setText(appSettings.value("network/nfsMount", "").toString());

	if(camera->RotationArgumentIsSet())
		ui->chkUpsideDownDisplay->setChecked(camera->getUpsideDownDisplay());
	else //If the argument was not added, set the control to invisible because it would be useless anyway
		ui->chkUpsideDownDisplay->setVisible(false);

	/* Load the configuration from the network manager */
	NetworkConfig netcfg;
	QStringList list;
	ui->chkDhcpEnable->setChecked(netcfg.dhcp);
	list = netcfg.ipAddress.split('.');
	if (list.count() == 4) {
		ui->lineStaticIpPart1->setText(list[0]);
		ui->lineStaticIpPart2->setText(list[1]);
		ui->lineStaticIpPart3->setText(list[2]);
		ui->lineStaticIpPart4->setText(list[3]);
	}
	list = netcfg.gateway.split('.');
	if (list.count() == 4) {
		ui->lineStaticGwPart1->setText(list[0]);
		ui->lineStaticGwPart2->setText(list[1]);
		ui->lineStaticGwPart3->setText(list[2]);
		ui->lineStaticGwPart4->setText(list[3]);
	}
	unsigned long mask = 0xffffffff ^ (0xffffffff >> netcfg.prefixLen);
	ui->lineStaticMaskPart1->setText(QString::number((mask >> 24) & 0xff));
	ui->lineStaticMaskPart2->setText(QString::number((mask >> 16) & 0xff));
	ui->lineStaticMaskPart3->setText(QString::number((mask >> 8) & 0xff));
	ui->lineStaticMaskPart4->setText(QString::number((mask >> 0) & 0xff));

	/* Install input validators for IP addresses. */
	ipValidator.setRange(0, 255);
	ui->lineStaticIpPart1->setValidator(&ipValidator);
	ui->lineStaticIpPart2->setValidator(&ipValidator);
	ui->lineStaticIpPart3->setValidator(&ipValidator);
	ui->lineStaticIpPart4->setValidator(&ipValidator);
	ui->lineStaticMaskPart1->setValidator(&ipValidator);
	ui->lineStaticMaskPart2->setValidator(&ipValidator);
	ui->lineStaticMaskPart3->setValidator(&ipValidator);
	ui->lineStaticMaskPart4->setValidator(&ipValidator);
	ui->lineStaticGwPart1->setValidator(&ipValidator);
	ui->lineStaticGwPart2->setValidator(&ipValidator);
	ui->lineStaticGwPart3->setValidator(&ipValidator);
	ui->lineStaticGwPart4->setValidator(&ipValidator);

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
#ifdef DEBIAN
	system("service chronos-update start");
	QApplication::quit();
#else
	int itr, retval;
	char location[100];

	for(itr = 1; itr <= 4; itr++)
	{
		//Look for the update on sda
		sprintf(location, "/media/sda%d/camUpdate/update.sh", itr, script);
		if((retval = updateSoftware(location)) != CAMERA_FILE_NOT_FOUND) return;
		
		//Also look for the update on sdb, as the usb is sometimes mounted there instead of sda
		sprintf(location, "/media/sdb%d/camUpdate/update.sh", itr, script);
		if((retval = updateSoftware(location)) != CAMERA_FILE_NOT_FOUND) return;
		
		//Look for the update on the SD card
		sprintf(location, "/media/mmcblk1p%d/camUpdate/update.sh", itr, script);
		if((retval = updateSoftware(location)) != CAMERA_FILE_NOT_FOUND) return;
	}

	QMessageBox msg;
	msg.setText("No software update found");
	msg.setWindowFlags(Qt::WindowStaysOnTopHint);
	//msg.setInformativeText("Update");
	msg.exec();
#endif
}

int UtilWindow::updateSoftware(char * updateLocation)
{
#ifndef DEBIAN
	struct stat st;

	/* Check that the update file exists and is executable */
	if (stat(updateLocation, &st) != 0) {
		return CAMERA_FILE_NOT_FOUND;
	}
	if ((st.st_mode & S_IXUSR) == 0) {
		return CAMERA_FILE_NOT_FOUND;
	}

	/* Prompt the user for confirmation. */
	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(this, "Software update", "Found software update, do you want to install it now?\r\nThe display may go blank and the camera may restart during this process.\r\nWARNING: Any unsaved video in RAM will be lost.", QMessageBox::Yes|QMessageBox::No);
	if(QMessageBox::Yes != reply)
		return USER_EXIT;


	/* Start the update in the background and close the GUI in the foreground. */
	runBackground(updateLocation);
	QApplication::quit();

	/* We won't get here... */
	return SUCCESS;
#endif
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
	struct statvfs fsInfoBuf;
	if(!settingClock)
	{
		if(ui->dateTimeEdit->hasFocus())
			settingClock = true;
		else
			ui->dateTimeEdit->setDateTime(QDateTime::currentDateTime());
	}

	/* If on the storage tab, update the drive and network status. */
	if (ui->tabWidget->currentWidget() == ui->tabStorage) {
		struct stat st;

		QString mText = runCommand("df -h | grep \'/media/\'");
		ui->lblMountedDevices->setText(mText);

		/* Update the disk status text. */
		QString diskText = "";
		while (stat("/dev/sda", &st) == 0) {
			if (!S_ISBLK(st.st_mode)) break;
			diskText = runCommand("lsblk -i --output NAME,SIZE,FSTYPE,LABEL,VENDOR,MODEL /dev/sda");
			break;
		}
		ui->lblStatusDisk->setText(diskText);

		/* Update the SD card status text. */
		QString sdText = "";
		while (stat("/dev/mmcblk1", &st) == 0) {
			if (!S_ISBLK(st.st_mode)) break;
			sdText = runCommand("lsblk -i --output NAME,SIZE,FSTYPE,LABEL,VENDOR,MODEL /dev/mmcblk1");
			break;
		}
		ui->lblStatusSD->setText(sdText);
		
		/* enable/disable the format/eject buttons */
		ui->cmdEjectDisk->setEnabled(!diskText.isEmpty());
		ui->cmdFormatDisk->setEnabled(!diskText.isEmpty());
		ui->cmdEjectSD->setEnabled(!sdText.isEmpty());
		ui->cmdFormatSD->setEnabled(!sdText.isEmpty());
	}
	/* If on the network tab, update the interface and network status. */
	if (ui->tabWidget->currentWidget()  == ui->tabNetwork) {
		QString netText = runCommand("ifconfig eth0");
		ui->lblNetStatus->setText(netText);
	}
	/* Of on the about tab, update the temperature and battery voltage. */
	if (ui->tabWidget->currentWidget() == ui->tabAbout) {
		double sensorTemp = camera->cinst->getProperty("sensorTemperature", 0.0).toDouble();
		double systemTemp = camera->cinst->getProperty("systemTemperature", 0.0).toDouble();
		double battVoltage = camera->cinst->getProperty("batteryVoltage", 0.0).toDouble();
		QString lastShutdownReason = camera->cinst->getProperty("lastShutdownReason", "NA").toString();
		QString status;

		status = QString("Sensor Temperature: %1 C\n").arg(sensorTemp);
		status.append(QString("System Temperature: %1 C\n").arg(systemTemp));
		status.append(QString("Battery Voltage: %1 V\n").arg(battVoltage));
		status.append(QString("Last Shutdown Reason: %1\n").arg(lastShutdownReason));
		ui->lblStatus->setText(status);
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
	QString title = QString("Factory Black Calibration");
	QProgressDialog *progress;
	Int32 retVal;
	char text[100];

	progress = new QProgressDialog(this);
	progress->setWindowTitle(title);
	progress->setWindowModality(Qt::WindowModal);
	progress->show();

	QCoreApplication::processEvents();

	//Turn off calibration light
	camera->setBncDriveLevel(0);	//Turn off output drive

	//Black cal all standard resolutions
	retVal = camera->blackCalAllStdRes(true, progress);
	delete progress;

	if(SUCCESS != retVal)
	{
		QMessageBox msg;
		sprintf(text, "Error during black calibration, error %d: %s", retVal, errorCodeString(retVal));
		msg.setText(text);
		msg.setWindowTitle(title);
		msg.setWindowFlags(Qt::WindowStaysOnTopHint);
		msg.exec();
	}
	else
	{
		QMessageBox msg;
		sprintf(text, "Black cal of all standard resolutions was successful");
		msg.setText(text);
		msg.setWindowTitle(title);
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
	if (!openingWindow) camera->setFocusPeakEnable(ui->chkFPEnable->checkState());
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
	if(checked && ui->chkFPEnable->isChecked())
	{
		camera->setFocusPeakThresholdLL(FOCUS_PEAK_THRESH_LOW);
	}
	if (checked)
	{
		camera->focusPeakLevel = FOCUS_PEAK_API_LOW;
	}
}

void UtilWindow::on_radioFPSensMed_toggled(bool checked)
{
	if(checked && ui->chkFPEnable->isChecked())
	{
		camera->setFocusPeakThresholdLL(FOCUS_PEAK_THRESH_MED);
	}
	if (checked)
	{
		camera->focusPeakLevel = FOCUS_PEAK_API_MED;
	}
}

void UtilWindow::on_radioFPSensHigh_toggled(bool checked)
{
	if(checked && ui->chkFPEnable->isChecked())
	{
		camera->setFocusPeakThresholdLL(FOCUS_PEAK_THRESH_HIGH);
	}
	if (checked)
	{
		camera->focusPeakLevel = FOCUS_PEAK_API_HIGH;
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

void UtilWindow::on_cmdClose_5_clicked()
{
	qDebug()<<"on_cmdClose_5_clicked";
	on_cmdClose_clicked();
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

void UtilWindow::on_cmdExportCalData_clicked()
{
	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(this, "Calibration Data Export", "Begin flat field export?\r\nThe display may go blank and the camera will turn off after this process.\r\nWARNING: Any unsaved video in RAM will be lost.", QMessageBox::Yes|QMessageBox::No);
	if(QMessageBox::Yes != reply)
		return;

	qDebug() << "### flat-field export";
	camera->cinst->exportCalData();
}

void UtilWindow::on_cmdImportCalData_clicked()
{
	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(this, "Calibration Data Import", "Begin calibration data import?\r\nAny previous cal data imports will be overwritten.", QMessageBox::Yes|QMessageBox::No);
	if(QMessageBox::Yes != reply)
		return;

	qDebug() << "### flat-field import";
	camera->cinst->importCalData();
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

	//Check that the directory is readable
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
		ui->cmdCloseApp->setVisible(true);
		ui->cmdColumnGain->setVisible(true);
		ui->cmdSetSN->setVisible(true);
		ui->lineSerialNumber->setVisible(true);
		ui->cmdExportCalData->setVisible(true);
		ui->cmdImportCalData->setVisible(true);
		ui->chkShowDebugControls->setVisible(true);
		ui->chkFanDisable->setVisible(true);
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
	char title[128];
	char message[1024];
	struct statvfs fsInfoBuf;
	FILE *fp;

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

	/* Check if storage device exists */
	sprintf(filepath, "/dev/%s", blkdev);
	if(statvfs(filepath, &fsInfoBuf) == -1){
		sprintf(title, "Storage device not found");
		sprintf(message, "\"%s\" not found", diskname);
		QMessageBox::warning(this, title, message);
		return;
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

	/* Overwrite the first 1MB with zeros and then rebuild the partition table. */
	progress->setLabelText("Wiping partition table");
	if (progress->wasCanceled()) {
		delete progress;
		return;
	}
	progress->setValue(1);
	sprintf(command, "dd if=/dev/zero of=%s bs=1M count=1", filepath);
	system(command);

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

void UtilWindow::on_chkShippingMode_clicked()
{
	bool state = ui->chkShippingMode->isChecked();

	if(state == TRUE){
		QMessageBox::information(this, "Shipping Mode Enabled","On the next restart, the AC adapter must be plugged in to turn the camera on.", QMessageBox::Ok);
	}

	camera->cinst->setBool("shippingMode", state);
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

	/* Reboot the conrol and video interfaces */
	camera->vinst->reset();
	camera->cinst->reboot(true);

#ifdef DEBIAN
	/* May not be necessary; systemd will probably reboot us */
	runBackground("service chronos-gui restart");
	QApplication::quit();
#else
	system("killall camApp && /etc/init.d/camera restart");
#endif
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

	/* Generate backups of the control and video API settings. */
	system("cam-json -g config videoConfig > Settings/KronTech/apiBackup.json");

	sprintf(str, "tar -cf /media/sda1/user_settings.tar Settings/KronTech");
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

	sprintf(str, "tar -xf /media/sda1/user_settings.tar");

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

	/* Update the control and video APIs with the restored settings. */
	system("cam-json set Settings/KronTech/apiBackup.json");

	sw.hide();
	msg.setText("User settings restore successful!");
	msg.setWindowFlags(Qt::WindowStaysOnTopHint);
	msg.exec();

	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(this, "Restart app?", "To apply the restored settings the app must restart. Is it okay to restart?", QMessageBox::Yes|QMessageBox::No);
	if(QMessageBox::Yes != reply)
		return;

#ifdef DEBIAN
	/* May not be necessary; systemd will probably reboot us */
	runBackground("service chronos-gui restart");
	QApplication::quit();
#else
	system("killall camApp && /etc/init.d/camera restart");
#endif
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
	if (!openingWindow) camera->setAutoPowerMode(index);
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
//		camera->on_chkLiveLoop_stateChanged(arg1);
	}
}

void UtilWindow::on_spinLiveLoopTime_valueChanged(double arg1)
{
	if (!openingWindow)
	{
		camera->on_spinLiveLoopTime_valueChanged(arg1);
	}
}

void UtilWindow::on_chkFanDisable_stateChanged(int enable)
{
	if (!openingWindow) {
		/* 33% seems to be the slowest we can spin the fan without stopping it */
		camera->cinst->setFloat("fanOverride", enable ? 0.33 : -1);
	}
}

void UtilWindow::on_lineSmbPassword_editingStarted()
{
	ui->lineSmbPassword->setText("");
}

void UtilWindow::on_lineSmbPassword_editingFinished()
{
	lineSmbPassword_wasEdited = true;
}

void UtilWindow::on_cmdSmbListShares_clicked()
{
	/* TODO: Implement Me! */
	//on_cmdNetListConnections_clicked();
}

void UtilWindow::on_cmdSmbApply_clicked()
{
	QSettings appSettings;
	QMessageBox prompt(QMessageBox::Information,
						"SMB Connection Status", "Attempting to connect...",
						QMessageBox::Ok, this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

	/* Store the updated samba settings. */
	appSettings.setValue("network/smbUser", ui->lineSmbUser->text());
	appSettings.setValue("network/smbShare", ui->lineSmbShare->text());
	/* Update the password only if edited... */
	if (lineSmbPassword_wasEdited) {
		appSettings.setValue("network/smbPassword", ui->lineSmbPassword->text());
	}

	/* Disconnect any mounted storage. */
	umount2(SMB_STORAGE_MOUNT, MNT_DETACH);
	checkAndCreateDir(SMB_STORAGE_MOUNT);
	prompt.show();

	/* If any of the fields are blank, disconnect SMB share */
	if (!ui->lineSmbUser->text().length() || !ui->lineSmbShare->text().length())
	{
		prompt.setText("SMB share disconnected.");
		prompt.exec();
		return;
	}

	/* Try to determine server reachability */
	QCoreApplication::processEvents();
	QString smbServer = parseSambaServer(ui->lineSmbShare->text());
	if (!isReachable(smbServer)) {
		prompt.setText(smbServer + " is not reachable!");
		prompt.exec();
		return;
	}

	/* Try to mount the network share */
	QString mountString = buildSambaString();
	mountString.append(" 2>&1");
	QString returnString = runCommand(mountString.toLatin1());
	if (returnString != "")
	{
		prompt.setText("Mount failed: " + returnString);
	}
	else
	{
		prompt.setText("Connection successful");
	}
	prompt.exec();
}

void UtilWindow::on_cmdSmbTest_clicked()
{
	QMessageBox prompt(QMessageBox::Information,
						"SMB Connection Status", "Attempting to connect...",
						QMessageBox::Ok, this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
	prompt.show();

	if (path_is_mounted(SMB_STORAGE_MOUNT))
	{
		QString mountPoint = SMB_STORAGE_MOUNT;
		QString mountFile = mountPoint + "/text.txt";

		QFile file(mountFile);
		if (file.open(QFile::WriteOnly))
		{
			QTextStream out(&file);
			out << "Hello World";
			bool sambaOK = file.flush();

			file.close();
			sambaOK &= file.remove();
			if (sambaOK)
			{
				QString text = "SMB share " + ui->lineSmbUser->text() + " on " + ui->lineSmbShare->text() + " is connected.";
				prompt.setText(text);
				prompt.exec();
			}
			else
			{
				QString text = "SMB share " + ui->lineSmbUser->text() + " on " + ui->lineSmbShare->text() + " write failed.";
				prompt.setText(text);
				prompt.exec();
			}
		}
		else
		{
			QString text = "SMB share " + ui->lineSmbUser->text() + " open failed!";
			prompt.setText(text);
			prompt.exec();
		}
	}
	else
	{
		QString text = "SMB share " + ui->lineSmbUser->text() + " on " + ui->lineSmbShare->text() + " is not connected!";
		prompt.setText(text);
		prompt.exec();
	}
}

void UtilWindow::on_cmdNfsApply_clicked()
{
	QSettings appSettings;
	QMessageBox prompt(QMessageBox::Information,
						"NFS Connection Status", "Attempting to connect...",
						QMessageBox::Ok, this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

	/* Store the updated NFS settings. */
	appSettings.setValue("network/nfsAddress", ui->lineNfsAddress->text());
	appSettings.setValue("network/nfsMount", ui->lineNfsMount->text());

	/* Disconnect any mounted storage. */
	umount2(NFS_STORAGE_MOUNT, MNT_DETACH);
	checkAndCreateDir(NFS_STORAGE_MOUNT);
	prompt.show();

	/* If any of the fields are blank, disconnect NFS share */
	if (!ui->lineNfsAddress->text().length() || !ui->lineNfsMount->text().length())
	{
		prompt.setText("NFS share disconnected.");
		prompt.exec();
		return;
	}

	/* Try to determine server reachability */
	QCoreApplication::processEvents();
	if (!isReachable(ui->lineNfsAddress->text()))
	{
		prompt.setText(ui->lineNfsAddress->text() + " is not reachable!");
		prompt.exec();
		return;
	}

	/* Try to mount the network share */
	QString mountString = buildNfsString();
	mountString.append(" 2>&1");
	QString returnString = runCommand(mountString.toLatin1());
	if (returnString != "")
	{
		prompt.setText("Mount failed: " + returnString);
	}
	else
	{
		prompt.setText("Connection successful");
	}
	prompt.exec();
}

void UtilWindow::on_cmdNfsTest_clicked()
{
	QMessageBox prompt(QMessageBox::Information,
						"NFS Connection Status", "Attempting to connect...",
						QMessageBox::Ok, this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
	prompt.show();

	if (path_is_mounted(NFS_STORAGE_MOUNT)) {
		QString mountFile = QString(NFS_STORAGE_MOUNT) + "/testfile";
		QFile file(mountFile);
		if (file.open(QFile::WriteOnly))
		{
			QTextStream out(&file);
			out << "TESTING";
			bool nfsOK = file.flush();
			file.close();
			nfsOK &= file.remove();
			if (nfsOK)
			{
				QString text = "NFS share " + ui->lineNfsMount->text() + " on " + ui->lineNfsAddress->text() + " is connected.";
				prompt.setText(text);
				prompt.exec();
			}
			else
			{
				QString text = "NFS share " + ui->lineNfsMount->text() + " on " + ui->lineNfsAddress->text() + " write failed.";
				prompt.setText(text);
				prompt.exec();
			}
		}
		else
		{
			QString text = "NFS share " + ui->lineNfsMount->text() + " on " + ui->lineNfsAddress->text() +" open failed!";
			prompt.setText(text);
			prompt.exec();
		}
	}
	else
	{
		QString text = "NFS share " + ui->lineNfsMount->text() + " on " + ui->lineNfsAddress->text() + " is not connected!";
		prompt.setText(text);
		prompt.exec();
	}
}

void UtilWindow::on_chkDhcpEnable_stateChanged(int checked)
{
	ui->lineStaticIpPart1->setEnabled(!checked);
	ui->lineStaticIpPart2->setEnabled(!checked);
	ui->lineStaticIpPart3->setEnabled(!checked);
	ui->lineStaticIpPart4->setEnabled(!checked);
	ui->lineStaticMaskPart1->setEnabled(!checked);
	ui->lineStaticMaskPart2->setEnabled(!checked);
	ui->lineStaticMaskPart3->setEnabled(!checked);
	ui->lineStaticMaskPart4->setEnabled(!checked);
	ui->lineStaticGwPart1->setEnabled(!checked);
	ui->lineStaticGwPart2->setEnabled(!checked);
	ui->lineStaticGwPart3->setEnabled(!checked);
	ui->lineStaticGwPart4->setEnabled(!checked);

	if (!openingWindow) ui->cmdStaticIpApply->setEnabled(true);
}

void UtilWindow::ipChunkChanged(QLineEdit *edit)
{
	if (!openingWindow) ui->cmdStaticIpApply->setEnabled(true);
}

void UtilWindow::on_lineStaticIpPart1_editingFinished() { ipChunkChanged(ui->lineStaticIpPart1); }
void UtilWindow::on_lineStaticIpPart2_editingFinished() { ipChunkChanged(ui->lineStaticIpPart2); }
void UtilWindow::on_lineStaticIpPart3_editingFinished() { ipChunkChanged(ui->lineStaticIpPart3); }
void UtilWindow::on_lineStaticIpPart4_editingFinished() { ipChunkChanged(ui->lineStaticIpPart4); }
void UtilWindow::on_lineStaticMaskPart1_editingFinished() { ipChunkChanged(ui->lineStaticMaskPart1); }
void UtilWindow::on_lineStaticMaskPart2_editingFinished() { ipChunkChanged(ui->lineStaticMaskPart2); }
void UtilWindow::on_lineStaticMaskPart3_editingFinished() { ipChunkChanged(ui->lineStaticMaskPart3); }
void UtilWindow::on_lineStaticMaskPart4_editingFinished() { ipChunkChanged(ui->lineStaticMaskPart4); }
void UtilWindow::on_lineStaticGwPart1_editingFinished() { ipChunkChanged(ui->lineStaticGwPart1); }
void UtilWindow::on_lineStaticGwPart2_editingFinished() { ipChunkChanged(ui->lineStaticGwPart2); }
void UtilWindow::on_lineStaticGwPart3_editingFinished() { ipChunkChanged(ui->lineStaticGwPart3); }
void UtilWindow::on_lineStaticGwPart4_editingFinished() { ipChunkChanged(ui->lineStaticGwPart4); }

static int countbits(int i)
{
	int count;
	for (count = 0; i != 0; i >>= 1) {
		count += (i & 1);
	}
	return count;
}

void UtilWindow::on_cmdStaticIpApply_clicked(void)
{
	QString uuid = NetworkConfig::getUUID();
	QString nmcfg;
	QString nmreload;

	unsigned int ipParts[4] = {
		ui->lineStaticIpPart1->text().toUInt(),
		ui->lineStaticIpPart2->text().toUInt(),
		ui->lineStaticIpPart3->text().toUInt(),
		ui->lineStaticIpPart4->text().toUInt(),
	};
	unsigned int gwParts[4] = {
		ui->lineStaticGwPart1->text().toUInt(),
		ui->lineStaticGwPart2->text().toUInt(),
		ui->lineStaticGwPart3->text().toUInt(),
		ui->lineStaticGwPart4->text().toUInt(),
	};
	unsigned int prefixLen = 0;
	prefixLen += countbits(ui->lineStaticMaskPart1->text().toUInt());
	prefixLen += countbits(ui->lineStaticMaskPart2->text().toUInt());
	prefixLen += countbits(ui->lineStaticMaskPart3->text().toUInt());
	prefixLen += countbits(ui->lineStaticMaskPart4->text().toUInt());

	/* Prepare the nmcli command to set the IPv4 address and gateway. */
	nmcfg.sprintf("nmcli con mod %s ipv4.addresses \"%u.%u.%u.%u/%u %u.%u.%u.%u\"",
				  uuid.toAscii().constData(),
				  ipParts[0], ipParts[1], ipParts[2], ipParts[3], prefixLen,
				  gwParts[0], gwParts[1], gwParts[2], gwParts[3]);

	if (ui->chkDhcpEnable->isChecked()) {
		nmcfg.append(" ipv4.method \"auto\"");
	} else {
		nmcfg.append(" ipv4.method \"manual\"");
	}

	/* Disable the 'Apply' button until the user makes more edits. */
	ui->cmdStaticIpApply->setEnabled(false);

	/* Apply the network configuration */
	qDebug() << "Executing:" << nmcfg;
	system(nmcfg.toAscii().constData());

	/* Restart the network interface */
	nmreload.sprintf("nmcli con down %s && nmcli con up %s", uuid.toAscii().constData(), uuid.toAscii().constData());
	qDebug() << "Executing:" << nmreload;
	runBackground(nmreload.toAscii().constData());
}

#if 0
void UtilWindow::on_cmdNetListConnections_clicked()
{
	QStringList mountedShares;

	ui->lblNetStatus->setText("Attempting to connect...");
	QCoreApplication::processEvents();

	//scan for Samba shares
	QString drives = runCommand("mount -t cifs");
	QStringList splitString = drives.split("\n");

	for (int i=0; i<splitString.length()-1; i++)
	{
		QString mountString = splitString.value(i).split(".").value(3).split("/").value(1).split(" ").value(0);
		QString addrString = splitString.value(i).split("/").value(2);
		mountedShares.append(addrString + ":" + mountString + " (mounted SMB share)");
	}

	//scan for NFS shares
	drives = runCommand("mount -t nfs4");
	splitString = drives.split("\n");

	for (int i=0; i<splitString.length()-1; i++)
	{
		QString mountString = splitString.value(i).split(".").value(3).split(":").value(1).split(" ").value(0);
		QString addrString = splitString.value(i).split(":").value(0);
		if (mountString.right(1) != "/")
		{
			mountString.append("/");	//add "/" if not there
		}
		mountedShares.append(addrString + ":" + mountString + " (mounted NFS share)");
	}

	if (!isReachable(ui->lineSmbAddress->text()))
	{
		ui->lblNetStatus->setText(ui->lineSmbAddress->text() + " is not reachable!");
		return;
	}

	//scan for all available NFS shares not already mounted
	QStringList mountList;
	QString mounts = runCommand("showmount -e " + ui->lineSmbAddress->text());
	if (mounts != "")
	{
		splitString = mounts.split("\n");

		int i=1;
		do
		{
			QStringList shareList = splitString.value(i).simplified().split(" ");
			QString mountString = shareList.value(0);
			if (mountString.right(1) != "/")
			{
				mountString.append("/");	//add "/" if not there
			}
			QString addressString = ui->lineSmbAddress->text();
			QString shareString = addressString + ":" + mountString;
			//now check if it is already in the list
			bool inList = false;
			for (int j = 0; j<mountedShares.length(); j++)
			{
				if (mountedShares[j].contains(shareString + " "))
				{
					inList = true;
				}
			}
			if (!inList)
			{
				mountList.append(shareString + " (available NFS share)");
			}
			i++;
		} while (i < splitString.length()-1);
	}

	QString text; //("Mounted shares:\n");
	for (int i=0; i<mountedShares.length(); i++)
	{
		//text.append
		text.append("--> " + mountedShares.value(i) + "\n");
		//text.append("\n");
	}
	//text.append("Available shares:\n");
	for (int i=0; i<mountList.length(); i++)
	{
		text.append("    " + mountList.value(i) + "\n");
	}

	if (text == "")
	{
		ui->lblNetStatus->setText("No connections.");
	}
	else
	{
		ui->lblNetStatus->setText(text);
	}
}
#endif
