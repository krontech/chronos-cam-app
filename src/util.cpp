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
#include "time.h"
#include "util.h"
#include "defines.h"
#include "QDebug"
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/mount.h>
#include <mntent.h>
#include <unistd.h>
#include <signal.h>
#include <QCoreApplication>
#include <QTime>
#include <QSettings>
#include <QString>
#include <QStringList>

void delayms(int ms)
{
	struct timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };
	nanosleep(&ts, NULL);
}

void delayms_events(int ms) {
	QTime dieTime = QTime::currentTime().addMSecs(ms);
	while (QTime::currentTime() < dieTime) {
		QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
	}
}

bool checkAndCreateDir(const char * dir)
{
	struct stat st;

	//Check for and create cal directory
	if(stat(dir, &st) != 0 || !S_ISDIR(st.st_mode))
	{
		qDebug() << "Folder" << dir << "not found, creating";
		const int dir_err = mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if (-1 == dir_err)
		{
			qDebug() <<"Error creating directory!";
			return false;
		}
	}

	return true;
}

bool pathIsMounted(const char *path)
{
	FILE * mtab = setmntent("/etc/mtab", "r");
	struct mntent* m;
	struct mntent mnt;
	char strings[4096];

	//Check for mounted local disks.
	while ((m = getmntent_r(mtab, &mnt, strings, sizeof(strings)))) {
		if (mnt.mnt_dir == NULL) continue;
		if (strcmp(path, mnt.mnt_dir) == 0) {
			endmntent(mtab);
			return true;
		}
	}
	endmntent(mtab);
	return false;
}

QString runCommand(QString command, int *status)
{
	QString output = "";
	int dummy;
	if (status == NULL) status = &dummy;

	FILE *fp = popen(command.toLocal8Bit().constData(), "r");
	char line[200];
	if (!fp) return output;
	while (fgets(line, sizeof(line), fp) != NULL) {
		output.append(line);
	}
	*status = pclose(fp);
	return output;
}

QString removeFirstAndLastSlash(QString string)
{
	if (string.right(1) == "/")
	{
		string.chop(1);
	}
	if (string.left(1) == "/")
	{
		string.remove(0, 1);
	}
	return string;
}

QString parseSambaServer(QString share)
{
	int start = 0;
	int end = -1;

	/* The string may start with an smb: schema, which we will ignore */
	if (share.startsWith("smb://")) {
		start += 6;
	} else if (share.startsWith("//")) {
		start += 2;
	} else {
		/* Otherwise, this is not a valid SMB share */
		return QString();
	}
	/* Find the end of the SMB server name. */
	end = share.indexOf('/', start);
	if (end <= start) return QString();

	/* Return the SMB server name. */
	return share.mid(start, end - start);
}

QString buildSambaString()
{
	QSettings appSettings;
	QString mountString = "mount -t cifs -o ";

	mountString.append("username=" + appSettings.value("network/smbUser", "").toString());
	mountString.append(",password=" + appSettings.value("network/smbPassword", "").toString());
	mountString.append(",noserverino ");
	mountString.append(appSettings.value("network/smbShare", "").toString() + " " + SMB_STORAGE_MOUNT);

	qDebug() << "Preparing samba mount:" << mountString;
	return mountString;
}

QString buildNfsString()
{
	QSettings appSettings;
	QString mountString = "mount ";
	mountString.append(appSettings.value("network/nfsAddress", "").toString());
	mountString.append(":" + appSettings.value("network/nfsMount", "").toString());
	mountString.append(" ");
	mountString.append(NFS_STORAGE_MOUNT);
	return mountString;
}

bool isReachable(QString address)
{
	int status;
	if (address == "") return false;

	runCommand("ping -c1 " + address, &status);
	return (status == 0);
}

/* check if a remote PC is exporting a folder to this camera */
bool isExportingNfs(QString camIpAddress)
{
	QSettings appSettings;

	QString showString = "showmount -e ";
	showString.append(appSettings.value("network/nfsAddress", "").toString());
	QString mounts = runCommand(showString);
	if (mounts != "")
	{
		QStringList splitString = mounts.split("\n");
		if (splitString.value(0).contains("Export list for"))
		{
			for (int i = 1; i < splitString.length() - 1; i++)
			{
				QStringList shareList = splitString.value(i).simplified().split(" ");
				QString mountString = shareList.value(0);		//should typically be "/mnt/nfs"
				if (mountString == appSettings.value("network/nfsMount", "").toString())
				{
					QStringList splitIps = shareList.value(1).split(",");			//should be splitting something like "192.168.1.215/24,192.168.1.214/24"
					for (int j=0; j<shareList.length(); j++)
					{
						qDebug() << splitIps.value(j);
						if (camIpAddress == splitIps.value(j).split("/").value(0))
						{
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

/* Run a shell command that may need to run in the background */
void
runBackground(const char *command)
{
	int child = fork();
	if (child == 0) {
		char nohupcmd[1024] = "nohup ";

		setsid();
		signal(SIGHUP, SIG_IGN);
		system(strcat(nohupcmd, command));
		exit(0);
	}
}

bool refreshDriveList(QComboBox *combo, QString selection)
{
	FILE * fp;
	FILE * mtab = setmntent("/etc/mtab", "r");
	struct mntent* m;
	struct mntent mnt;
	char strings[4096];		//Temp buffer used by mntent
	char model[256];		//Stores model name of sd* device
	char vendor[256];		//Stores vendor of sd* device
	char drive[1024];		//Stores string to be placed in combo box
	unsigned int len;
	bool changed = true;	//Revert to defaults if the selection was not found.

	combo->clear();

	//Check for mounted local disks.
	while ((m = getmntent_r(mtab, &mnt, strings, sizeof(strings)))) {
		if (mnt.mnt_dir == NULL) continue;

		qDebug("refreshDriveList: %s", mnt.mnt_dir);
		/* Add mount entries for SATA and USB drives. */
		if (strstr(mnt.mnt_dir, "/media/sd") == mnt.mnt_dir) {
			//If this is not an SD card (ie a "sd*") type, get the mounted name eg "sda1"
			char * mountPoint = mnt.mnt_dir + 7;
			int part;
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
		/* Add mount entries for Multimedia/SD cards. */
		else if (strstr(mnt.mnt_dir, "/media/mmcblk1") == mnt.mnt_dir) {
			int part;

			//If there's a number following the block name, that's our partition number
			if(*(mnt.mnt_dir + 15))		//Find the first character after "/media/mmcblk1p"
				part = atoi(mnt.mnt_dir + 15);
			else
				part = 1;
			sprintf(drive, "%s (SD Card Partition %d)", mnt.mnt_dir, part);
		}
		/* It might also be a network share. */
		else if (strcmp(mnt.mnt_dir, SMB_STORAGE_MOUNT) == 0) {
			sprintf(drive, "%s (SMB share)", mnt.mnt_dir);
		}
		else if (strcmp(mnt.mnt_dir, NFS_STORAGE_MOUNT) == 0) {
			sprintf(drive, "%s (NFS share)", mnt.mnt_dir);
		}
		/* Otherwise this is not a drive we care about. */
		else {
			continue;
		}

		/* Add the drive to the QComboBox */
		combo->setEnabled(true);
		combo->addItem(drive, QVariant(mnt.mnt_dir));
		if (strcmp(mnt.mnt_dir, selection.toAscii().data()) == 0) {
			combo->setCurrentIndex(combo->count() - 1);
			changed = false;
		}
	}

	endmntent(mtab);

	if(combo->count() == 0) {
		combo->addItem("No storage devices detected");
		combo->setEnabled(false);
		return false;
	}
	if (changed) {
		combo->setCurrentIndex(0);
	}
	return changed;
}
