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
	#include "eeprom.h"
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
	int retVal;
	int file;

	/* if we are reading, *WRITE* to file */
	if ((file = open(RAM_SPD_I2C_BUS_FILE, O_WRONLY|O_CREAT,0666)) < 0) {
		/* ERROR HANDLING: you can check errno to see what went wrong */
		qDebug() << "Failed to open i2c bus" << RAM_SPD_I2C_BUS_FILE;
		return CAMERA_FILE_ERROR;
	}

	retVal = eeprom_read_large(file, CAMERA_EEPROM_I2C_ADDR/*Address*/, SERIAL_NUMBER_OFFSET/*Offset*/, (unsigned char *)dest/*buffer*/, SERIAL_NUMBER_MAX_LEN/*Length*/);
	close(file);

	if(retVal < 0)
	{
		return CAMERA_ERROR_IO;
	}

	qDebug() << "Read in serial number" << dest;
	dest[SERIAL_NUMBER_MAX_LEN] = '\0';

	return SUCCESS;
}

Int32 Camera::writeSerialNumber(char * src)
{
	int retVal;
	int file;
	char serialNumber[SERIAL_NUMBER_MAX_LEN];

	memset(serialNumber, 0x00, SERIAL_NUMBER_MAX_LEN);

	if (strlen(src) > SERIAL_NUMBER_MAX_LEN) {
		// forcefully null terminate string
		src[SERIAL_NUMBER_MAX_LEN] = 0;
	}

	strcpy(serialNumber, src);

	const char *filename = RAM_SPD_I2C_BUS_FILE;

	/* if we are writing to eeprom, *READ* from file */
	if ((file = open(filename, O_RDONLY)) < 0) {
		/* ERROR HANDLING: you can check errno to see what went wrong */
		qDebug() << "Failed to open the i2c bus";
		return CAMERA_FILE_ERROR;
	}

	retVal = eeprom_write_large(file, CAMERA_EEPROM_I2C_ADDR, SERIAL_NUMBER_OFFSET, (unsigned char *) serialNumber, SERIAL_NUMBER_MAX_LEN);
	qDebug("eeprom_write_large returned: %d", retVal);
	::close(file);

	delayms(250);

	return SUCCESS;
}
