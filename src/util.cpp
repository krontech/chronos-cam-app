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

int path_is_mounted(const char *path)
{
	char tmp[PATH_MAX];
	struct stat st;
	struct stat parent;

	/* Get the stats for the given path and check that it's a directory. */
	if ((stat(path, &st) != 0) || !S_ISDIR(st.st_mode)) {
		return FALSE;
	}

	/* Ensure that the parent directly is mounted on a different device. */
	snprintf(tmp, sizeof(tmp), "%s/..", path);
	return (stat(tmp, &parent) == 0) && (parent.st_dev != st.st_dev);
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

	mountString.append("user=" + appSettings.value("network/smbUser", "").toString());
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
