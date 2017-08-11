
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

//Defining this here because something in the preprocessor undefines this somewhere up above
#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
	   __typeof__ (b) _b = (b); \
	 _a < _b ? _a : _b; })

saveSettingsWindow::saveSettingsWindow(QWidget *parent, Camera * camInst) :
	QWidget(parent),
	ui(new Ui::saveSettingsWindow)
{
	ui->setupUi(this);
	this->setWindowFlags(Qt::Dialog /*| Qt::WindowStaysOnTopHint*/ | Qt::FramelessWindowHint);
	camera = camInst;

	ui->spinBitrate->setValue(camera->recorder->bitsPerPixel);
	ui->spinMaxBitrate->setValue(camera->recorder->maxBitrate);
	ui->spinFramerate->setValue(camera->recorder->framerate);
	ui->lineFilename->setText(camera->recorder->filename);

	refreshDriveList();

	//Select the entry corresponding to the last selected path
	Int32 index = ui->comboDrive->findText(camera->recorder->fileDirectory);
	if ( index != -1 ) { // -1 for not found
		ui->comboDrive->setCurrentIndex(index);
	}

	ui->comboProfile->clear();
	ui->comboProfile->addItem("Base");
	ui->comboProfile->addItem("Main");
	ui->comboProfile->addItem("Extended");
	ui->comboProfile->addItem("High");

	UInt32 val = camera->recorder->profile;	//Compute base 2 logarithm to get index
	index = 0;
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

	val = camera->recorder->level;	//Compute base 2 logarithm to get index
	index = 0;
	while (val >>= 1) ++index;
	ui->comboLevel->setCurrentIndex(index);

	driveCount = 0;
	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(updateDrives()));
	timer->start(1000);

}

saveSettingsWindow::~saveSettingsWindow()
{
	timer->stop();
	delete timer;
	delete ui;
}

void saveSettingsWindow::on_cmdClose_clicked()
{
	camera->recorder->bitsPerPixel = ui->spinBitrate->value();
	camera->recorder->maxBitrate = ui->spinMaxBitrate->value();
	camera->recorder->framerate = ui->spinFramerate->value();
	camera->recorder->profile = 1 << ui->comboProfile->currentIndex();
	camera->recorder->level = 1 << ui->comboLevel->currentIndex();

	strcpy(camera->recorder->filename, ui->lineFilename->text().toStdString().c_str());

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
	close();
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

}

void saveSettingsWindow::on_cmdRefresh_clicked()
{
	refreshDriveList();
}

void saveSettingsWindow::on_spinBitrate_valueChanged(double arg1)
{
	UInt32 frameRate = ui->spinFramerate->value();
	UInt32 bitrate = min(ui->spinBitrate->value() * camera->recordingData.is.hRes * camera->recordingData.is.vRes * frameRate, min(60000000, (UInt32)(ui->spinMaxBitrate->value() * 1000000.0)) * frameRate / 60);	//Max of 60Mbps

	char str[100];

	sprintf(str, "%4.2fMbps @\n%dx%d %dfps", (double)bitrate / 1000000.0, camera->recordingData.is.hRes, camera->recordingData.is.vRes, frameRate);
	ui->lblBitrate->setText(str);
}

void saveSettingsWindow::on_spinFramerate_valueChanged(int arg1)
{
	on_spinBitrate_valueChanged(arg1);
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



void saveSettingsWindow::on_spinMaxBitrate_valueChanged(int arg1)
{
	on_spinBitrate_valueChanged(arg1);
}
