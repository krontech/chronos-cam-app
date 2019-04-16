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

/* readPixel12
 *
 * Reads a 12-bit pixel out of a local buffer containing 12-bit packed data
 *
 * buf:		Pointer to image buffer
 * pixel:	Number of pixels into the buffer to read from
 *
 * returns: Pixel value
 **/
UInt16 readPixelBuf12(const void *buf, UInt32 pixel)
{
	const UInt8 *u8buf = (const UInt8 *)buf;
	UInt32 address = pixel * 12 / 8;
	UInt8 shift = (pixel & 0x1) * 4;
	return ((u8buf[address] >> shift) | (((UInt16)u8buf[address+1]) << (8 - shift))) & ((1 << 12) - 1);
}

/* writePixelBuf12
 *
 * Writes a 12-bit pixel into a local buffer containing 12-bit packed data
 *
 * buf:		Pointer to image buffer
 * pixel:	Number of pixels into the image to write to
 * value:	Pixel value to write
 *
 * returns: nothing
 **/
void writePixelBuf12(void * buf, UInt32 pixel, UInt16 value)
{
	UInt8 *u8buf = (UInt8 *)buf;
	UInt32 address = pixel * 12 / 8;
	UInt8 shift = (pixel & 0x1) * 4;
	UInt8 dataL = u8buf[address], dataH = u8buf[address+1];

	UInt8 maskL = ~(0xFF << shift);
	UInt8 maskH = ~(0xFFF >> (8 - shift));
	dataL &= maskL;
	dataH &= maskH;

	dataL |= (value << shift);
	dataH |= (value >> (8 - shift));
	u8buf[address] = dataL;
	u8buf[address+1] = dataH;
}
