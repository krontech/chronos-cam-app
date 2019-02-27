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
#ifndef SPI_H
#define SPI_H

#include <stdint.h>
#include "errorCodes.h"
#include "types.h"


class SPI
{
public:

	SPI();
	CameraErrortype Init(const char * dev, uint8_t bits_val, uint32_t speed_val, bool cpol = false, bool cpha = false);
	CameraErrortype Open(const char * dev);
	void Close(void);
    CameraErrortype setMode(bool cpol, bool cpha);
    Int32 Transfer(uint64_t txBuf, uint64_t rxBuf, uint32_t len, bool cpol = false, bool cpha = true);

	int fd;

	bool isOpen;
	uint8_t mode;
	uint8_t bits;
	uint32_t speed;
	uint16_t delay;
};
/*
enum {
	SPI_SUCCESS = 0,
	SPI_NOT_OPEN,
	SPI_ALREAY_OPEN,
	SPI_OPEN_FAIL,
	SPI_IOCTL_FAIL
};
*/
#endif // SPI_H
