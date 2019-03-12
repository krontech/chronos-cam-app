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
#include "QDebug"
#include <sys/stat.h>
#include <QCoreApplication>
#include <QTime>

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
