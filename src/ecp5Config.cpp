/*

Pin		dir(c)
PROGRAMn	Out	V1 : GP1[9]	41
INITn		In	W2 : GP1[8]	40
DONE		In	V2 : GP1[10]	42

SN		Out	AH6 : GP1[0]	32
HOLDn		Out	AG6 : GP1[1]	33




*/

#include <stdint.h>
#include <cstdio>
#include <QDebug>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <unistd.h>

#include "util.h"

#include "defines.h"

#include "ecp5Config.h"
#include "types.h"


Ecp5Config::Ecp5Config()
{
	isOpen = false;
}


Ecp5Config::~Ecp5Config()
{

}


Int32 Ecp5Config::init(const char * spiDev, const char * pgmnGPIO, const char * initnGPIO, const char * doneGPIO, const char * snGPIO, const char * holdnGPIO, uint32_t speed_val)
{
	int ret;

	if(isOpen)
		return ECP5_ALREAY_OPEN;
	//FDs for GPIOs
	pgmnFD = open(pgmnGPIO, O_WRONLY);
	initnFD = open(initnGPIO, O_RDONLY);
	doneFD = open(doneGPIO, O_RDONLY);
	snFD = open(snGPIO, O_WRONLY);
	holdnFD = open(holdnGPIO, O_WRONLY);

	if(!pgmnFD || !initnFD || !doneFD || !snFD || !holdnFD)
		return ECP5_GPIO_ERROR;

	delay = 0;
	bits = 8;
	speed = speed_val;
	mode = 0;

	spiFD = open(spiDev, O_RDWR);
	if (spiFD < 0)
		return ECP5_SPI_OPEN_FAIL;

	/*
	 * spi mode
	 */
	ret = ioctl(spiFD, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		return  ECP5_IOCTL_FAIL;

	ret = ioctl(spiFD, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		return  ECP5_IOCTL_FAIL;

	/*
	 * bits per word
	 */
	ret = ioctl(spiFD, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		return  ECP5_IOCTL_FAIL;

	ret = ioctl(spiFD, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		return  ECP5_IOCTL_FAIL;

	/*
	 * max speed hz
	 */
	ret = ioctl(spiFD, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		return  ECP5_IOCTL_FAIL;

	ret = ioctl(spiFD, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		return  ECP5_IOCTL_FAIL;

	isOpen = true;

	return SUCCESS;
}

void Ecp5Config::deinit(void)
{
	if(isOpen)
	{
		close(pgmnFD);
		close(initnFD);
		close(doneFD);
		close(snFD);
		close(holdnFD);
		close(spiFD);
		isOpen = false;
	}
}

Int32 Ecp5Config::configure(const char * configFile)
{
	UInt8 cmd[4] = {0, 0, 0, 0};
	UInt8 data[4];
	UInt8 * bitstream;

	struct stat stat_buf;
	FILE * cfd;

	//Open bitstream file
	cfd = fopen(configFile, "rb");
	if(!cfd)
		return ECP5_FILE_IO_ERROR;

	//Get file size
	int rc = fstat(fileno(cfd), &stat_buf);
	if(rc)
		return ECP5_FILE_IO_ERROR;

	UInt32 cfSize = stat_buf.st_size;

	//Load bistream file
	bitstream = new UInt8[cfSize];

	if(!bitstream)
		return ECP5_MEMORY_ERROR;

	if(cfSize != fread(bitstream, sizeof(bitstream[0]), cfSize, cfd))
		return ECP5_FILE_IO_ERROR;

	// See MachXO3 programming and configuration user guide for correct info. ECP5 SysConfig manual procedure is wrong
	//Initialize signals
	writeGPIO(pgmnFD, true);
	writeGPIO(snFD, true);
	writeGPIO(holdnFD, true);
	delayms(50);

	//Create a falling edge on PROGRAMn to put FPGA in program mode
	writeGPIO(pgmnFD, false);
	delayms(1);
	writeGPIO(pgmnFD, true);
	delayms(50);		//50ms delay as specified by mfg

	//INITn should guaranteed be high by now

	//Send READ_ID command and read ID response
	writeGPIO(snFD, false);
	cmd[0] = ECP5_READ_ID;
	spiWrite(cmd, 4);
	spiRead(data, 4);
	writeGPIO(snFD, true);
	delayms(1);	//Just in case

	//Check returend device ID. ID of LFE5U-85 is 0x41113043 (from device programmer).
	if(	0x41 != data[0] ||
		0x11 != data[1] ||
		0x30 != data[2] ||
		0x43 != data[3])
		return ECP5_WRONG_DEVICE_ID;

	//Send refresh command
	writeGPIO(snFD, false);
	cmd[0] = ECP5_REFRESH;
	spiWrite(cmd, 4);
	writeGPIO(snFD, true);
	delayms(100);	//The configuration manual says this isn't required, but some delay here is. Not sure exactly how much.

	//Send write enable command
	writeGPIO(snFD, false);
	cmd[0] = ECP5_LSC_ENABLE;
	spiWrite(cmd, 4);
	writeGPIO(snFD, true);
	delayms(1);	//Just in case

	//Send write inc command
	writeGPIO(snFD, false);
	cmd[0] = ECP5_LSC_BITSTREAM_BURST;
	spiWrite(cmd, 4);	//command
	//Now send the entire bitstream in one go
	//The whole block? You can't handle the whole block!
	//Write in 1k blocks because the SPI driver can't handle the whole thing
	UInt32 pos = 0;
	UInt32 len;
	while(pos < cfSize)
	{
		len = min(1024, cfSize - pos);
		spiWrite(bitstream + pos, len);
		pos += len;

	}
	writeGPIO(snFD, true);
	delayms(1);	//Just in case

	//Read the status to check config success (done bit)
	writeGPIO(snFD, false);
	cmd[0] = ECP5_LSC_READ_STATUS;
	spiWrite(cmd, 4);
	spiRead(data, 4);
	writeGPIO(snFD, true);
	delayms(1);	//Just in case
	qDebug() << "Status readback:" << data[0] << data[1] << data[2] << data[3];

	//Check DONE bit
	if((data[2] & 0x1) != 1)
		return ECP5_DONE_NOT_ASSERTED;

	//Send write disable command
	writeGPIO(snFD, false);
	cmd[0] = ECP5_LSC_DISABLE;
	spiWrite(cmd, 4);
	writeGPIO(snFD, true);
	delayms(1);	//Just in case

	//Send nop command
	writeGPIO(snFD, false);
	cmd[0] = 0xFF;
	cmd[1] = 0xFF;
	cmd[2] = 0xFF;
	cmd[3] = 0xFF;
	spiWrite(cmd, 4);
	writeGPIO(snFD, true);
	delayms(1);	//Just in case

	return SUCCESS;
}

void Ecp5Config::readStatus()
{
	UInt8 cmd[4] = {0, 0, 0, 0};
	UInt8 data[4];

	//Send READ_ID command and read ID response
	writeGPIO(snFD, false);
	cmd[0] = ECP5_LSC_READ_STATUS;
	spiWrite(cmd, 4);
	spiRead(data, 4);
	writeGPIO(snFD, true);
	delayms(1);	//Just in case

	qDebug() << "Status readback:" << data[0] << data[1] << data[2] << data[3];
}

Int32 Ecp5Config::spiWrite(UInt8 * data, UInt32 len)
{
	int ret;
	spi_ioc_transfer tr;

	if(!isOpen)
		return ECP5_NOT_OPEN;

	tr.tx_buf = (uint64_t)data;
	tr.rx_buf = (uint64_t)NULL;
	tr.len = len;
	tr.delay_usecs = delay;
	tr.speed_hz = speed;
	tr.bits_per_word = bits;

	ret = ioctl(spiFD, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		return ECP5_IOCTL_FAIL;
	else
		return SUCCESS;
}

//FPGA sends data in the opposite bit order it receives it in.
Int32 Ecp5Config::spiRead(UInt8 * data, UInt32 len)
{
	int ret;
	spi_ioc_transfer tr;

	if(!isOpen)
		return ECP5_NOT_OPEN;

	tr.tx_buf = (uint64_t)NULL;
	tr.rx_buf = (uint64_t)data;
	tr.len = len;
	tr.delay_usecs = delay;
	tr.speed_hz = speed;
	tr.bits_per_word = bits;

	ret = ioctl(spiFD, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		return ECP5_IOCTL_FAIL;

	return SUCCESS;

}

bool Ecp5Config::readGPIO(Int32 fd)
{
	UInt8 value;
	read(fd, &value, 1);
	return value != 0;
}

void Ecp5Config::writeGPIO(Int32 fd, bool value)
{
	write(fd, value ? "1" : "0", 1);
}
