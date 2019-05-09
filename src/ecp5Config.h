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
#ifndef ECP5CONFIG_H
#define ECP5CONFIG_H

#include <stdint.h>
#include "errorCodes.h"
#include "types.h"

#define ECP5_READ_ID			0xE0
#define ECP5_LSC_READ_STATUS	0x3C
#define ECP5_LSC_ENABLE			0xC6	//This is listed as 0xC3 in the ECP5 SysConfig documenation, which is WRONG
#define ECP5_LSC_DISABLE		0x26
#define ECP5_REFRESH			0x79
#define ECP5_LSC_PROG_INCR_RTI	0x82
#define ECP5_LSC_BITSTREAM_BURST	0x7A
#define ECP5_LSC_PROGRAM_DONE	0x5E

class Ecp5Config
{
public:

	Ecp5Config();
	~Ecp5Config();
	Int32 init(const char * dev, const char * pgmnGPIO, const char * initnGPIO, const char * doneGPIO, const char * snGPIO, const char * holdnGPIO, uint32_t speed_val);
	void deinit(void);
	Int32 configure(const char * configFile);

private:
	Int32 spiWrite(UInt8 * data, UInt32 len);
	Int32 spiRead(UInt8 * data, UInt32 len);
	bool readGPIO(Int32 fd);
	void writeGPIO(Int32 fd, bool value);
	void readStatus();

	int pgmnFD;
	int initnFD;
	int doneFD;
	int snFD;
	int holdnFD;
	int spiFD;

	bool isOpen;
	uint8_t mode;
	uint8_t bits;
	uint32_t speed;
	uint16_t delay;
};

#endif // ECP5CONFIG_H
