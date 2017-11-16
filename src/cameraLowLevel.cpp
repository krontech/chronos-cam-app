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

#include "camera.h"
#include "gpmc.h"
#include "gpmcRegs.h"
#include "util.h"
#include "cameraRegisters.h"
extern "C" {
#include "eeprom.h"
}
#include "defines.h"


//Sets the frame pointer source used for live display
void Camera::setDisplayFrameSource(bool liveDisplaySource)
{
	UInt32 displayCtl = gpmc->read32(DISPLAY_CTL_ADDR);
	if(liveDisplaySource)
		gpmc->write32(DISPLAY_CTL_ADDR, displayCtl & ~DISPLAY_CTL_ADDRESS_SEL_MASK);
	else
		gpmc->write32(DISPLAY_CTL_ADDR, displayCtl | DISPLAY_CTL_ADDRESS_SEL_MASK);
}

void Camera::setDisplayFrameAddress(UInt32 address)
{
    UInt32 displayCtl = gpmc->read32(DISPLAY_CTL_ADDR);
	gpmc->write32(DISPLAY_FRAME_ADDRESS_ADDR, address);
    gpmc->write16(DISPLAY_CTL_ADDR, displayCtl | DISPLAY_CTL_MANUAL_SYNC_MASK);

}

void Camera::setLiveOutputTiming(UInt32 hRes, UInt32 vRes, UInt32 hOutRes, UInt32 vOutRes, UInt32 maxFps)
{
    const UInt32 pxClock = 100000000;
    const UInt32 hSync = 50;
    const UInt32 hPorch = 166;
    const UInt32 vSync = 3;
    const UInt32 vPorch = 38;
    UInt32 hPeriod = hOutRes + (2 * hSync) + hPorch;
    UInt32 vPeriod = (pxClock / (hPeriod * maxFps));

	gpmc->write16(DISPLAY_H_RES_ADDR, hRes);
    gpmc->write16(DISPLAY_H_OUT_RES_ADDR, hOutRes);
	gpmc->write16(DISPLAY_V_RES_ADDR, vRes);
    gpmc->write16(DISPLAY_V_OUT_RES_ADDR, vOutRes);

    /* Setup the the horizontal timing for speed. */
    gpmc->write16(DISPLAY_H_PERIOD_ADDR, hPeriod-1);
    gpmc->write16(DISPLAY_H_SYNC_LEN_ADDR, hSync);
    gpmc->write16(DISPLAY_H_BACK_PORCH_ADDR, hPorch);

    /* Setup the vertical timing to match the desired framerate. */
    if (vPeriod < (vOutRes + 2*vSync + vPorch)) {
        vPeriod = vOutRes + 2*vSync + vPorch;
    }
    gpmc->write16(DISPLAY_V_PERIOD_ADDR, vPeriod - 1);
    gpmc->write16(DISPLAY_V_SYNC_LEN_ADDR, vSync);
    gpmc->write16(DISPLAY_V_BACK_PORCH_ADDR, vPorch);
}

bool Camera::getRecDataFifoIsEmpty(void)
{
	return gpmc->read32(SEQ_STATUS_ADDR) & SEQ_STATUS_MD_FIFO_EMPTY_MASK;
}

UInt32 Camera::readRecDataFifo(void)
{
	return gpmc->read32(SEQ_MD_FIFO_READ_ADDR);
}

bool Camera::getRecording(void)
{
	return gpmc->read32(SEQ_STATUS_ADDR) & SEQ_STATUS_RECORDING_MASK;
}

void Camera::startSequencer(void)
{
	UInt32 reg = gpmc->read32(SEQ_CTL_ADDR);
	gpmc->write32(SEQ_CTL_ADDR, reg | SEQ_CTL_START_REC_MASK);
	gpmc->write32(SEQ_CTL_ADDR, reg & ~SEQ_CTL_START_REC_MASK);
}

void Camera::terminateRecord(void)
{
	UInt32 reg = gpmc->read32(SEQ_CTL_ADDR);
	gpmc->write32(SEQ_CTL_ADDR, reg | SEQ_CTL_STOP_REC_MASK);
	gpmc->write32(SEQ_CTL_ADDR, reg & ~SEQ_CTL_STOP_REC_MASK);
}

void Camera::writeSeqPgmMem(SeqPgmMemWord pgmWord, UInt32 address)
{
	gpmc->write32((SEQ_PGM_MEM_START_ADDR + address * 16)+4, pgmWord.data.high);
	gpmc->write32((SEQ_PGM_MEM_START_ADDR + address * 16)+0, pgmWord.data.low/*0x00282084*/);
}

void Camera::setFrameSizeWords(UInt16 frameSize)
{
	gpmc->write16(SEQ_FRAME_SIZE_ADDR, frameSize);
}

void Camera::setRecRegionStartWords(UInt32 start)
{
	gpmc->write32(SEQ_REC_REGION_START_ADDR, start);
}

void Camera::setRecRegionEndWords(UInt32 end)
{
	gpmc->write32(SEQ_REC_REGION_END_ADDR, end);
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
	gpmc->write32(GPMC_PAGE_OFFSET_ADDR, offsetWords);

	for(int i = 0; i < length/4; i++)
	{
		buf[i] = gpmc->readRam32(4*i);
	}

	gpmc->write32(GPMC_PAGE_OFFSET_ADDR, 0);
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

	gpmc->write32(GPMC_PAGE_OFFSET_ADDR, offsetWords);

	for(int i = 0; i < length/4; i++)
	{
		gpmc->writeRam32(4*i, buf[i]);
	}

	gpmc->write32(GPMC_PAGE_OFFSET_ADDR, 0);
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
	gpmc->write16(DCG_MEM_START_ADDR+2*column, gain*4096.0);
}

/* Camera::readIsColor
 *
 * Returns weather the camera is color or mono, read from the hardware jumper
 *
 * returns: true for color, false for mono
 **/

bool Camera::readIsColor(void)
{
	Int32 colorSelFD;

	colorSelFD = open("/sys/class/gpio/gpio34/value", O_RDONLY);

	if (-1 == colorSelFD)
		return CAMERA_FILE_ERROR;

	char buf[2];

	lseek(colorSelFD, 0, SEEK_SET);
	read(colorSelFD, buf, sizeof(buf));

	return ('1' == buf[0]) ? true : false;

	close(colorSelFD);
}




bool Camera::getFocusPeakEnableLL(void)
{
	return gpmc->read32(DISPLAY_CTL_ADDR) & DISPLAY_CTL_FOCUS_PEAK_EN_MASK;
}



void Camera::setFocusPeakEnableLL(bool en)
{
	UInt32 reg = gpmc->read32(DISPLAY_CTL_ADDR);
	gpmc->write32(DISPLAY_CTL_ADDR, (reg & ~DISPLAY_CTL_FOCUS_PEAK_EN_MASK) | (en ? DISPLAY_CTL_FOCUS_PEAK_EN_MASK : 0));
}


UInt8 Camera::getFocusPeakColor(void)
{
	return (gpmc->read32(DISPLAY_CTL_ADDR) & DISPLAY_CTL_FOCUS_PEAK_COLOR_MASK) >> DISPLAY_CTL_FOCUS_PEAK_COLOR_OFFSET;
}



void Camera::setFocusPeakColor(UInt8 color)
{
	UInt32 reg = gpmc->read32(DISPLAY_CTL_ADDR);
	gpmc->write32(DISPLAY_CTL_ADDR, (reg & ~DISPLAY_CTL_FOCUS_PEAK_COLOR_MASK) | ((color & 7) << DISPLAY_CTL_FOCUS_PEAK_COLOR_OFFSET));
}


bool Camera::getZebraEnableLL(void)
{
	return gpmc->read32(DISPLAY_CTL_ADDR) & DISPLAY_CTL_ZEBRA_EN_MASK;
}



void Camera::setZebraEnableLL(bool en)
{
	UInt32 reg = gpmc->read32(DISPLAY_CTL_ADDR);
	gpmc->write32(DISPLAY_CTL_ADDR, (reg & ~DISPLAY_CTL_ZEBRA_EN_MASK) | (en ? DISPLAY_CTL_ZEBRA_EN_MASK : 0));
}

void Camera::setFocusPeakThreshold(UInt32 thresh)
{
	gpmc->write32(DISPLAY_PEAKING_THRESH_ADDR, thresh);
}

UInt32 Camera::getFocusPeakThreshold(void)
{
	return gpmc->read32(DISPLAY_PEAKING_THRESH_ADDR);
}




Int32 Camera::getRamSizeGB(UInt32 * stick0SizeGB, UInt32 * stick1SizeGB)
{
	int retVal;
	int file;
    unsigned char ram0_buf, ram1_buf;

	/* if we are reading, *WRITE* to file */
	if ((file = open(RAM_SPD_I2C_BUS_FILE, O_WRONLY|O_CREAT,0666)) < 0) {
		/* ERROR HANDLING: you can check errno to see what went wrong */
		qDebug() << "Failed to open i2c bus" << RAM_SPD_I2C_BUS_FILE;
		return CAMERA_FILE_ERROR;
	}

//	Print out all the SPD data
//	for(int i = 0; i < 256; i++)
//	{
//		retVal = eeprom_read(file, RAM_SPD_I2C_ADDRESS_STICK_0/*Address*/, i/*Offset*/, &buf/*buffer*/, sizeof(buf)/*Length*/);
//		qDebug() << buf;
//	}

    // Read ram slot 0 size
    retVal = eeprom_read(file, RAM_SPD_I2C_ADDRESS_STICK_0/*Address*/, 5/*Offset*/, &ram0_buf/*buffer*/, sizeof(ram0_buf)/*Length*/);

    if(retVal < 0)
    {
        *stick0SizeGB = 0;
    }


    switch(ram0_buf & 0x07)	//Column size is in bits 2:0, and is row address width - 9
    {
        case 1:	//8GB, column address width is 10
            *stick0SizeGB = 8;

            break;
        case 2:	//16GB, column address width is 11
            *stick0SizeGB = 16;
            break;
        default:
        *stick0SizeGB = 0;
            break;
    }

    qDebug() << "Found" << *stick0SizeGB << "GB memory stick in slot 0";


    // Read ram slot 1 size
    retVal = eeprom_read(file, RAM_SPD_I2C_ADDRESS_STICK_1/*Address*/, 5/*Offset*/, &ram1_buf/*buffer*/, sizeof(ram1_buf)/*Length*/);

    if(retVal < 0)
    {
        *stick1SizeGB = 0;
    }
    else {
        switch(ram1_buf & 0x07)	//Column size is in bits 2:0, and is row address width - 9
        {
            case 1:	//8GB, column address width is 10
                *stick1SizeGB = 8;

                break;
            case 2:	//16GB, column address width is 11
                *stick1SizeGB = 16;
                break;
            default:
                *stick1SizeGB = 0;
                break;
        }
    }

    qDebug() << "Found" << *stick1SizeGB << "GB memory stick in slot 1";

    close(file);

    if (*stick0SizeGB == 0 && *stick1SizeGB == 0) {
        return CAMERA_ERROR_IO;
    }

    return SUCCESS;
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
	int len = strlen(src);

	if(len > SERIAL_NUMBER_MAX_LEN) {
		len = SERIAL_NUMBER_MAX_LEN;
	}

	const char *filename = RAM_SPD_I2C_BUS_FILE;

	/* if we are writing to eeprom, *READ* from file */
	if ((file = open(filename, O_RDONLY)) < 0) {
		/* ERROR HANDLING: you can check errno to see what went wrong */
		qDebug() << "Failed to open the i2c bus";
		return CAMERA_FILE_ERROR;
	}

	retVal = eeprom_write_large(file, CAMERA_EEPROM_I2C_ADDR, SERIAL_NUMBER_OFFSET, (unsigned char *)src, len);
	qDebug() <<"eeprom_write_large returned" << retVal;
	::close(file);

	delayms(250);

	return SUCCESS;
}

UInt16 Camera::getFPGAVersion(void)
{
	return gpmc->read16(FPGA_VERSION_ADDR);
}

UInt16 Camera::getFPGASubVersion(void)
{
	return gpmc->read16(FPGA_SUBVERSION_ADDR);
}
