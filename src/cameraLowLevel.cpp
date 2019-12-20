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
#include <stdio.h>
#include <fcntl.h>
#include <QDebug>
#include <unistd.h>
#include <signal.h>

#include "camera.h"
#include "util.h"
extern "C" {
}
#include "defines.h"


UInt8 Camera::getFocusPeakColorLL(void)
{
	//static QString fpColors[] = {"blue", "green", "cyan", "red", "magenta", "yellow", "white"};

	QString col;

	cinst->getString("focusPeakingColor", &col);
	int index = 0;
	while (col != fpColors[index])
	{
		index++;
		if (index >= 7)
		{
			qDebug() << "Invalid color!";
			return 0;
		}
	}
	return index + 1;

	}

void Camera::setFocusPeakColorLL(UInt8 color)
{
		cinst->setString("focusPeakingColor", fpColors[color]);
}

void Camera::setFocusPeakThresholdLL(UInt32 thresh)
{
	double level;
	level = convertFocusPeakThreshold(thresh);
	cinst->setFloat("focusPeakingLevel", level);
	focusPeakLevel = level;
}

double Camera::convertFocusPeakThreshold(UInt32 thresh)
{
	double level;
	if (thresh <= FOCUS_PEAK_THRESH_HIGH ) level = FOCUS_PEAK_API_HIGH;
	else if (thresh <= FOCUS_PEAK_THRESH_MED) level = FOCUS_PEAK_API_MED;
	else level = FOCUS_PEAK_API_LOW;
	return level;
}

UInt32 Camera::getFocusPeakThresholdLL(void)
{
	double thresh;
	cinst->getFloat("focusPeakingLevel", &thresh);
	if (thresh >= FOCUS_PEAK_API_HIGH) return FOCUS_PEAK_THRESH_HIGH;
	if (thresh >= FOCUS_PEAK_API_MED) return FOCUS_PEAK_THRESH_MED;
	return FOCUS_PEAK_THRESH_LOW;
}

//dest must be a char array that can handle SERIAL_NUMBER_MAX_LEN + 1 bytes
Int32 Camera::readSerialNumber(char * dest)
{
	QString serial;
	cinst->getString("cameraSerial", &serial);
	strncpy(dest, serial.toAscii(), SERIAL_NUMBER_MAX_LEN + 1);
	dest[SERIAL_NUMBER_MAX_LEN] = '\0';
	return 0;
}

Int32 Camera::writeSerialNumber(char * src)
{
	//TODO - when implemented in API

	return 4;
}

UInt16 Camera::getFPGAVersion(void)
{
	bool ok;
	QString compoundVersion;
	cinst->getString("cameraFpgaVersion", &compoundVersion);
	double dVersion = compoundVersion.toDouble(&ok);
	return (int)dVersion;
}

UInt16 Camera::getFPGASubVersion(void)
{
	bool ok;
	QString compoundVersion;
	cinst->getString("cameraFpgaVersion", &compoundVersion);
	double dVersion = compoundVersion.toDouble(&ok);
	return round(100 * (dVersion - (int)dVersion));
}
