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
#include "mainwindow.h"
#include "ramwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <qthread.h>
#include <QApplication>
#include <QDebug>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include "gpmcRegs.h"
#include <sys/mman.h>

#include "spi.h"
#include "gpmc.h"
#include "defines.h"
#include "cameraRegisters.h"
#include "camera.h"

#include "video.h"
#include "types.h"
extern "C" {
#include "eeprom.h"
}

void endOfRecCallback(void * arg);

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

}

/*
SPI * spi;
GPMC * gpmc;
LUPA1300 * sensor;
Camera * camera;
Video * vinst;
*/
MainWindow::~MainWindow()
{
	delete ui;
}

void pabort(const char * msg)
{
	QMessageBox Msgbox;

	Msgbox.setText(msg);
	Msgbox.exec();

}


#define GPMC_MAPPED_BASE	map_base
#define GPMC_RANGE_BASE		0x1000000

unsigned int map_base, map_gpmc;


void MainWindow::on_cmdWrite_clicked()
{
	char msg [1000];
	//camera->autoColGainCorrection();
	UInt64 wl =	(UInt64)camera->gpmc->read16(WL_DYNDLY_3_ADDR) << 48 |
				(UInt64)camera->gpmc->read16(WL_DYNDLY_2_ADDR) << 32 |
				(UInt64)camera->gpmc->read16(WL_DYNDLY_1_ADDR) << 16 |
				(UInt64)camera->gpmc->read16(WL_DYNDLY_0_ADDR);

	qDebug() << "wl_dyndly ="	<< ((wl >> 56) & 0xFF)
								<< ((wl >> 48) & 0xFF)
								<< ((wl >> 40) & 0xFF)
								<< ((wl >> 32) & 0xFF)
								<< ((wl >> 24) & 0xFF)
								<< ((wl >> 16) & 0xFF)
								<< ((wl >> 8) & 0xFF)
								<< (wl & 0xFF);
	sprintf(msg, "wl_dyndly = %d, %d, %d, %d, %d, %d, %d, %d",
			(UInt32)((wl >> 56) & 0xFF),
			(UInt32)((wl >> 48) & 0xFF),
			(UInt32)((wl >> 40) & 0xFF),
			(UInt32)((wl >> 32) & 0xFF),
			(UInt32)((wl >> 24) & 0xFF),
			(UInt32)((wl >> 16) & 0xFF),
			(UInt32)((wl >> 8) & 0xFF),
			(UInt32)(wl & 0xFF));

	QMessageBox Msgbox;

	Msgbox.setText(msg);
	Msgbox.exec();

}


void endOfRecCallback(void * arg)
{
	char st[100];
	MainWindow * mw = (MainWindow*)arg;
	sprintf(st, "Recording Ended!\n");
	mw->ui->lblStatus->setText(st);
	qApp->processEvents();
}

void MainWindow::on_cmdAdvPhase_clicked()
{
	char st[100];
	camera->sensor->setClkPhase(++camera->sensor->clkPhase);

	sprintf(st, "Set phase to %d\n", camera->sensor->clkPhase);
	ui->lblStatus->setText(st);
}

void MainWindow::on_cmdDecPhase_clicked()
{
	char st[100];
	camera->sensor->setClkPhase(--camera->sensor->clkPhase);

	sprintf(st, "Set phase to %d\n", camera->sensor->clkPhase);
	ui->lblStatus->setText(st);
}

void MainWindow::on_cmdSeqOn_clicked()
{
	static bool on = false;

	camera->sensor->seqOnOff(on);
	on = !on;
}

void MainWindow::on_cmdSeqOff_clicked()
{
	camera->sensor->seqOnOff(false);
}

void MainWindow::on_cmdSetOffset_clicked()
{
	FILE * fp;
	UInt32 pixelsPerFrame = camera->recordingData.is.geometry.hRes * camera->recordingData.is.geometry.vRes;

	UInt16 * frameBuffer = new UInt16[pixelsPerFrame];
	camera->getRawCorrectedFramesAveraged(0, 16, frameBuffer);

	fp = fopen("img.raw", "wb");
	fwrite(frameBuffer, sizeof(frameBuffer[0]), pixelsPerFrame, fp);
	fclose(fp);

	return;

	int file;
	const char *filename = "/dev/i2c-2";
	if ((file = open(filename, O_RDWR)) < 0) {
		/* ERROR HANDLING: you can check errno to see what went wrong */
		perror("Failed to open the i2c bus");
		exit(1);
	}
}







void MainWindow::on_cmdTrigger_clicked()
{
	UInt32 reg = camera->gpmc->read32(SEQ_CTL_ADDR);
	camera->gpmc->write32(SEQ_CTL_ADDR, reg | SEQ_CTL_SW_TRIG_MASK);
	camera->gpmc->write32(SEQ_CTL_ADDR, reg & ~SEQ_CTL_SW_TRIG_MASK);
}

void MainWindow::on_cmdRam_clicked()
{
	int FRAME_SIZE = 1280*1024*12/8;

	qDebug() << "Setting display addresses";
	camera->gpmc->write16(DISPLAY_CTL_ADDR, camera->gpmc->read16(DISPLAY_CTL_ADDR) | 1);	//Set to display from display frame address register
	camera->gpmc->write32(DISPLAY_FRAME_ADDRESS_ADDR, 0x40000);	//Set display address

	qDebug() << "Zero FPN area";
	//Zero FPN area
	camera->gpmc->write32(GPMC_PAGE_OFFSET_ADDR, 0);	//Set GPMC offset
	for(int i = 0; i < FRAME_SIZE; i = i + 2)
	{
		camera->gpmc->writeRam16(i, 0);
	}

/*
	srand(1);
	for(int i = 0; i < FRAME_SIZE; i = i + 2)
	{
		camera->gpmc->writeRam16(i, rand());
	}

	srand(1);
	for(int i = 0; i < FRAME_SIZE; i = i + 2)
	{
		if(rand() != camera->gpmc->readRam16(i))
		{
			qDebug() << "Error at address" << i;
			i = FRAME_SIZE;
		}

	}
	*/

	qDebug() << "Zero image area";
	//Zero image area
	camera->gpmc->write32(GPMC_PAGE_OFFSET_ADDR, 0x40000);	//Set GPMC offset
	for(int i = 0; i < FRAME_SIZE; i = i + 2)
	{
		camera->gpmc->writeRam16(i, 0);
	}

	qDebug() << "Draw a rectangular box with diagonal";
	//Draw a rectangular box around the outside of the screen and a diagonal lines starting from the top left
	for(int i = 0; i < 1; i++)
	{
		for(int y = 0; y < 1024; y++)
		{
			for(int x = 0; x < 1280; x++)
			{
				writePixel12(x+y*1280, 0, 2048);//(x == 0 || y == 0 || x == 1279 || y == 1023 || x == y) ? 0xFFF : 0);
			}
//			qDebug() << "line" << y;
		}

	}
	qDebug() << "Set GPMC offset";
	camera->gpmc->write32(GPMC_PAGE_OFFSET_ADDR, 0);	//Set GPMC offset
	qDebug() << "done";
}

void MainWindow::on_cmdReset_clicked()
{
	camera->sensor->initSensor();
	//camera->gpmc->write16(SYSTEM_RESET_ADDR, 1);
}

UInt16 MainWindow::readPixel(UInt32 pixel, UInt32 offset)
{
	UInt32 address = pixel * 10 / 8 + offset;
	UInt8 shift = (pixel & 0x3) * 2;
	return (camera->gpmc->readRam8(address) >> shift) | (((UInt16)camera->gpmc->readRam8(address+1)) << (8 - shift));
}

void MainWindow::writePixel(UInt32 pixel, UInt32 offset, UInt16 value)
{
	UInt32 address = pixel * 10 / 8 + offset;
	UInt8 shift = (pixel & 0x3) * 2;
	UInt8 dataL = camera->gpmc->readRam8(address), dataH = camera->gpmc->readRam8(address+1);
	//uint8 dataL = 0, dataH = 0;

	dataL &= ~(0xFF << shift);
	dataH &= ~(0xFF >> (8 - shift));

	dataL |= value << shift;
	dataH |= value >> (8 - shift);
	camera->gpmc->writeRam8(address, dataL);
	camera->gpmc->writeRam8(address+1, dataH);
}

void MainWindow::writePixel12(UInt32 pixel, UInt32 offset, UInt16 value)
{
	UInt32 address = pixel * 12 / 8 + offset;
	UInt8 shift = (pixel & 0x1) * 4;
	UInt8 dataL = camera->gpmc->readRam8(address), dataH = camera->gpmc->readRam8(address+1);
	//uint8 dataL = 0, dataH = 0;

	UInt8 maskL = ~(0xFF << shift);
	UInt8 maskH = ~(0xFFF >> (8 - shift));
	dataL &= maskL;
	dataH &= maskH;

	dataL |= (value << shift);
	dataH |= (value >> (8 - shift));
	camera->gpmc->writeRam8(address, dataL);
	camera->gpmc->writeRam8(address+1, dataH);
}


void MainWindow::on_cmdFPN_clicked()
{	
	char msg [1000];
	int retVal;
	int file;
	unsigned char writeBuf[8] = {'A', 'B', 'C', '1', '2', '3', 'X', 'D'};
	unsigned char buf[8];

	const char *filename = "/dev/i2c-1";

	/* if we are writing to eeprom, *READ* from file */
	if ((file = open(filename, O_RDONLY)) < 0) {
		/* ERROR HANDLING: you can check errno to see what went wrong */
		qDebug() << "Failed to open the i2c bus";
		return;
	}

	retVal = eeprom_write_large(file, CAMERA_EEPROM_I2C_ADDR, 0, writeBuf, 8);
	qDebug() <<"eeprom_write_large returned" << retVal;
	::close(file);

	delayms(250);

	/* if we are reading, *WRITE* to file */
	if ((file = open(filename, O_WRONLY|O_CREAT,0666)) < 0) {
		/* ERROR HANDLING: you can check errno to see what went wrong */
		qDebug() << "Failed to open the i2c bus";
		return;
	}

	retVal = eeprom_read_large(file, CAMERA_EEPROM_I2C_ADDR/*Address*/, 0/*Offset*/, buf/*buffer*/, sizeof(buf)/*Length*/);
	qDebug() <<"eeprom_read_large returned" << retVal;

	::close(file);
	sprintf(msg, "Read = %x, %x, %x, %x, %x, %x, %x, %x",
			(UInt32)buf[0],
			(UInt32)buf[1],
			(UInt32)buf[2],
			(UInt32)buf[3],
			(UInt32)buf[4],
			(UInt32)buf[5],
			(UInt32)buf[6],
			(UInt32)buf[7]);

	QMessageBox Msgbox;

	Msgbox.setText(msg);
	Msgbox.exec();
}

void MainWindow::on_cmdFrameNumbers_clicked()
{
    // nothing to see here.
}

void MainWindow::on_cmdGC_clicked()
{
	camera->computeColGainCorrection(1, true);
}

void MainWindow::on_cmdOffsetCorrection_clicked()
{
	camera->adcOffsetCorrection(32, false);
}

void MainWindow::on_cmdSaveOC_clicked()
{
	camera->sensor->saveADCOffsetsToFile();
}

void MainWindow::on_cmdAutoBlack_clicked()
{
	camera->autoAdcOffsetCorrection();
}

void MainWindow::on_cmdSaveFrame_clicked()
{
    // nothing to see here.
    // TODO: Do the same thing with the following shell command.
    //      cat /tmp/cam-screencap.jpg > /media/sda1/somefilename.jpg
}
