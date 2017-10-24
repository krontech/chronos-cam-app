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
#include <stdint.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <unistd.h>

#include "defines.h"
#include "spi.h"



SPI::SPI()
{

}

CameraErrortype SPI::Init(const char * dev, uint8_t bits_val, uint32_t speed_val, bool cpol, bool cpha)
{
	delay = 0;
	bits = bits_val;
	speed = speed_val;
	mode =	(cpol ? 0x2 : 0) |
			(cpha ? 0x1 : 0);

	isOpen = false;

	return Open(dev);
}

CameraErrortype SPI::Open(const char * dev)
{

	int ret;
	if(isOpen)
		return SPI_ALREAY_OPEN;

	fd = open(dev, O_RDWR);
	if (fd < 0)
		return SPI_OPEN_FAIL;

	/*
	 * spi mode
	 */
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		return SPI_IOCTL_FAIL;

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		return SPI_IOCTL_FAIL;

	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		return SPI_IOCTL_FAIL;

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		return SPI_IOCTL_FAIL;

	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		return SPI_IOCTL_FAIL;

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		return SPI_IOCTL_FAIL;

	isOpen = true;
	return SUCCESS;

}

void SPI::Close()
{
	if(isOpen)
		close(fd);
	isOpen = false;
}

Int32 SPI::Transfer(uint64_t txBuf, uint64_t rxBuf, uint32_t len)
{
	int ret;
	struct spi_ioc_transfer tr;
	if(!isOpen)
		return SPI_NOT_OPEN;

	tr.tx_buf = txBuf;
	tr.rx_buf = rxBuf;
	tr.len = len;
	tr.delay_usecs = delay;
	tr.speed_hz = speed;
	tr.bits_per_word = bits;
	tr.cs_change = 0;

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		return SPI_IOCTL_FAIL;
	else
		return SUCCESS;
}
