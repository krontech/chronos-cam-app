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
#include "gpmc.h"
#include "gpmcRegs.h"
#include "util.h"
#include "cameraRegisters.h"
extern "C" {
#include "eeprom.h"
}
#include "defines.h"

bool Camera::getRecDataFifoIsEmpty(void)
{
	return 0;
}

UInt32 Camera::readRecDataFifo(void)
{
	return 0;
}

bool Camera::getRecording(void)
{
	QString state;
	cinst->getString("state", &state);
	return (state == "recording");
}

void Camera::startSequencer(void)
{
	return;
}

void Camera::terminateRecord(void)
{
	raise(SIGSEGV);
}

void Camera::writeSeqPgmMem(SeqPgmMemWord pgmWord, UInt32 address)
{
}

void Camera::setRecRegion(UInt32 start, UInt32 count, FrameGeometry *geometry)
{
	raise(SIGSEGV);
}

/* Camera::readAcqMem
 *
 * Reads data from acquisition memory into a buffer
 *
 * buf:			Pointer to image buffer
 * offsetWords:	Number words into acquisition memory to start read
 * length:		number of bytes to read (must be a multiple of 4)
 *
 * returns: nothing
 **/
void Camera::readAcqMem(UInt32 * buf, UInt32 offsetWords, UInt32 length)
{
	raise(SIGSEGV);
}

/* Camera::writeAcqMem
 *
 * Writes data from a buffer to acquisition memory
 *
 * buf:			Pointer to image buffer
 * offsetWords:	Number words into aqcuisition memory to start write
 * length:		number of bytes to write (must be a multiple of 4)
 *
 * returns: nothing
 **/
void Camera::writeAcqMem(UInt32 * buf, UInt32 offsetWords, UInt32 length)
{
	raise(SIGSEGV);
}

/* Camera::writeDGCMem
 *
 * Writes data to the display column gain buffer
 *
 * gain:		double value representing gain to apply to column
 * column:		selects which column the gain is applied to
 *
 * returns: nothing
 **/

void Camera::writeDGCMem(double gain, UInt32 column)
{
	raise(SIGSEGV);
}

/* Camera::readIsColor
 *
 * Returns weather the camera is color or mono, read from the hardware jumper
 *
 * returns: true for color, false for mono
 **/

bool Camera::readIsColor(void)
{
	return 0;
}


bool Camera::getFocusPeakEnableLL(void)
{
	//TODO - when implemented in API

	return false;
}



void Camera::setFocusPeakEnableLL(bool en)
{
	//TODO - when implemented in API
	raise(SIGSEGV);
}


UInt8 Camera::getFocusPeakColorLL(void)
{
	//TODO - when implemented in API
	return 4;
}


void Camera::setFocusPeakColorLL(UInt8 color)
{

		//TODO - when implemented in API
}


bool Camera::getZebraEnableLL(void)
{
	return 0;
}



void Camera::setZebraEnableLL(bool en)
{
	//TODO - when implemented in API
}

void Camera::setFocusPeakThresholdLL(UInt32 thresh)
{
	//TODO - when implemented in API
}

UInt32 Camera::getFocusPeakThresholdLL(void)
{
	//TODO - when implemented in API
	return 4;
}



Int32 Camera::getRamSizeGB(UInt32 * stick0SizeGB, UInt32 * stick1SizeGB)
{
	int retVal;
	retVal = cinst->getInt("cameraMemoryGB", stick0SizeGB);
	*stick1SizeGB = 0;
	return retVal;
}

//dest must be a char array that can handle SERIAL_NUMBER_MAX_LEN + 1 bytes
Int32 Camera::readSerialNumber(char * dest)
{
	int retVal;
	QString serial;
	cinst->getString("cameraSerial", &serial);
	std::copy(serial.toStdString().begin(),serial.toStdString().end(), dest);
	return retVal;
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
