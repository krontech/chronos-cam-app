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

#include <fcntl.h>
#include <sys/mman.h>
#include "gpmc.h"
#include "defines.h"
#include "cameraRegisters.h"

GPMC::GPMC()
{

}

Int32 GPMC::init()
{
	int fd_reg, fd_gpmc, fd_ram;

	fd_reg = open("/dev/mem", O_RDWR | O_SYNC);
	if(fd_reg < 0)
		return GPMCERR_FOPEN;

	fd_gpmc = open("/dev/mem", O_RDWR | O_SYNC);
	if(fd_gpmc < 0)
		return GPMCERR_FOPEN;

	fd_ram = open("/dev/mem", O_RDWR | O_SYNC);
	if(fd_ram < 0)
		return GPMCERR_FOPEN;

	map_base =(unsigned int) mmap(0, 0x1000000/*16MB*/, PROT_READ | PROT_WRITE, MAP_SHARED, fd_reg, GPMC_BASE);

	map_registers =(unsigned int) mmap(0, 0x1000000/*16MB*/, PROT_READ | PROT_WRITE, MAP_SHARED, fd_gpmc, GPMC_RANGE_BASE + GPMC_REGISTER_OFFSET);

	map_ram =(unsigned int) mmap(0, 0x1000000/*16MB*/, PROT_READ | PROT_WRITE, MAP_SHARED, fd_ram, GPMC_RANGE_BASE + GPMC_RAM_OFFSET);

	if(map_base == -1)
		return GPMCERR_MAP;

	if(map_registers == -1)
		return GPMCERR_MAP;

	if(map_ram == -1)
		return GPMCERR_MAP;

	//Reset GPMC
	GPMC_SYSCONFIG = GPMC_SYSCONFIG | 0x2;	//Put module in reset
	while(!(GPMC_STATUS & 1)) {};	//Wait for reset to complete

	//Set wait pins to active high
	GPMC_CONFIG = GPMC_CONFIG | (3 << 8);

	GPMC_TIMEOUT_CONTROL =	0x1FF << 4 |	/*TIMEOUTSTARTVALUE*/
        0;				/*TIMEOUTENABLE*/      // <<<<<<<<<<<<<<<<<<<<<<<<<<< TIMEOUT ENABLE

	//Disable CS0
	GPMC_CONFIG7_i(0) = 0;

	//Setup CS config
	GPMC_CONFIG1_i(0) =	0 << 31 |	/*WRAPBURST*/
						0 << 30 |	/*READMULTIPLE*/
						1 << 29 |	/*READTYPE*/
						0 << 28 |	/*WRITEMULTIPLE*/
						1 << 27 |	/*WRITETYPE*/
						0 << 25 |	/*CLKACTIVATIONTIME*/
						0 << 23 |	/*ATTACHEDDEVICEPAGELENGTH*/
						0 << 22 |	/*WAITREADMONITORING*/
						0 << 21 |	/*WAITWRITEMONITORING*/
						0 << 18 |	/*WAITWRITEMONITORINGTIME*/
						0 << 16 |	/*WAITPINSELECT*/
						1 << 12 |	/*DEVICESIZE*/
						0 << 10 |	/*DEVICETYPE*/
						0x2 << 8 |	/*MUXADDDATA*/
						0 << 4 |	/*TIMEPARAGRANULARITY*/
						0;			/*GPMCFCLKDIVIDER*/

	GPMC_CONFIG7_i(0) = 0xF << 8 |	/*MASKADDRESS*/
						0 << 6 |	/*CSVALID*/
						((GPMC_RANGE_BASE + GPMC_REGISTER_OFFSET) >> 24);			/*BASEADDRESS*/

	//Setup timings
	GPMC_CONFIG2_i(0) =	10 << 16 |	/*CSWROFFTIME          11*/
						10 << 8 |	/*CSRDOFFTIME          12*/
						0 << 7 |	/*CSEXTRADELAY*/
						0;			/*CSONTIME             1*/

	GPMC_CONFIG3_i(0) =	0 << 28 |	/*ADVAADMUXWROFFTIME*/
						0 << 24 |	/*ADVAADMUXRDOFFTIME*/
						4 << 16 |	/*ADVWROFFTIME         6*/
						4 << 8 |	/*ADVRDOFFTIME         4*/
						0 << 7 |	/*ADVEXTRADELAY*/
						0 << 4 |	/*ADVAADMUXONTIME*/
						2;			/*ADVONTIME*/

	GPMC_CONFIG4_i(0) =	8 << 24 |	/*WEOFFTIME            10*/
						0 << 23 |	/*WEEXTRADELAY*/
						4 << 16 |	/*WEONTIME             7*/
						0 << 13 |	/*OEAADMUXOFFTIME*/
						10 << 8 |	/*OEOFFTIME            11*/
						0 << 7 |	/*OEEXTRADELAY*/
						0 << 4 |	/*OEAADMUXONTIME*/
						4;			/*OEONTIME             7*/

	GPMC_CONFIG5_i(0) =	0 << 24 |	/*PAGEBURSTACCESSTIME*/
						9 << 16 |	/*RDACCESSTIME         10*/
						12 << 8 |	/*WRCYCLETIME          12*/
						12;			/*RDCYCLETIME          12*/



	GPMC_CONFIG6_i(0) =	4 << 24 |	/*WRACCESSTIME          6*/
						4 << 16 |	/*WRDATAONADMUXBUS      7*/
						4 << 8 |	/*CYCLE2CYCLEDELAY*/
						1 << 7 |	/*CYCLE2CYCLESAMECSEN*/
						1 << 6 |	/*CYCLE2CYCLEDIFFCSEN*/
						3;			/*BUSTURNAROUND*/

	//Setup wait pin

	//enable CS
	GPMC_CONFIG7_i(0) = GPMC_CONFIG7_i(0) | (1 << 6) /*CSVALID*/;

	//---------------------------------------------------------------
	//Disable CS1
	GPMC_CONFIG7_i(1) = 0;

	//Setup CS config
	GPMC_CONFIG1_i(1) =	0 << 31 |	/*WRAPBURST*/
						0 << 30 |	/*READMULTIPLE*/
						0 << 29 |	/*READTYPE*/
						0 << 28 |	/*WRITEMULTIPLE*/
						0 << 27 |	/*WRITETYPE*/
						0 << 25 |	/*CLKACTIVATIONTIME*/
						0 << 23 |	/*ATTACHEDDEVICEPAGELENGTH*/
						1 << 22 |	/*WAITREADMONITORING*/
						1 << 21 |	/*WAITWRITEMONITORING*/
						0 << 18 |	/*WAITWRITEMONITORINGTIME*/
						0 << 16 |	/*WAITPINSELECT*/
						1 << 12 |	/*DEVICESIZE*/
						0 << 10 |	/*DEVICETYPE*/
						0x1 << 8 |	/*MUXADDDATA*/
						0 << 4 |	/*TIMEPARAGRANULARITY*/
						0;			/*GPMCFCLKDIVIDER*/

	GPMC_CONFIG7_i(1) = 0xF << 8 |	/*MASKADDRESS*/
						0 << 6 |	/*CSVALID*/
						((GPMC_RANGE_BASE + GPMC_RAM_OFFSET) >> 24);			/*BASEADDRESS*/

	//Setup timings
	GPMC_CONFIG2_i(1) =	18 << 16 |	/*CSWROFFTIME*/
						18 << 8 |	/*CSRDOFFTIME*/
						0 << 7 |	/*CSEXTRADELAY*/
						1;			/*CSONTIME*/

	GPMC_CONFIG3_i(1) =	2 << 28 |	/*ADVAADMUXWROFFTIME*/
						2 << 24 |	/*ADVAADMUXRDOFFTIME*/
						6 << 16 |	/*ADVWROFFTIME*/
						6 << 8 |	/*ADVRDOFFTIME*/
						0 << 7 |	/*ADVEXTRADELAY*/
						1 << 4 |	/*ADVAADMUXONTIME*/
						4;			/*ADVONTIME*/

	GPMC_CONFIG4_i(1) =	8 << 24 |	/*WEOFFTIME*/
						0 << 23 |	/*WEEXTRADELAY*/
						6 << 16 |	/*WEONTIME*/
						3 << 13 |	/*OEAADMUXOFFTIME*/
						18 << 8 |	/*OEOFFTIME*/
						0 << 7 |	/*OEEXTRADELAY*/
						0 << 4 |	/*OEAADMUXONTIME*/
						7;			/*OEONTIME*/

	GPMC_CONFIG5_i(1) =	0 << 24 |	/*PAGEBURSTACCESSTIME*/
						16 << 16 |	/*RDACCESSTIME*/
						19 << 8 |	/*WRCYCLETIME*/
						19;			/*RDCYCLETIME*/



	GPMC_CONFIG6_i(1) =	12 << 24 |	/*WRACCESSTIME*/
						7 << 16 |	/*WRDATAONADMUXBUS*/
						1 << 8 |	/*CYCLE2CYCLEDELAY*/
						1 << 7 |	/*CYCLE2CYCLESAMECSEN*/
						1 << 6 |	/*CYCLE2CYCLEDIFFCSEN*/
						1;			/*BUSTURNAROUND*/

	//Setup wait pin

	//enable CS
	GPMC_CONFIG7_i(1) = GPMC_CONFIG7_i(1) | (1 << 6) /*CSVALID*/;

	return SUCCESS;
}

void GPMC::setTimeoutEnable(bool timeoutEnable)
{
	GPMC_TIMEOUT_CONTROL =	0x1FF << 4 |	/*TIMEOUTSTARTVALUE*/
							(timeoutEnable ? 1 : 0);				/*TIMEOUTENABLE*/
}

UInt32 GPMC::read32(UInt32 offset)
{
	return *((volatile UInt32 *)(map_registers + offset));
}

void GPMC::write32(UInt32 offset, UInt32 data)
{
	*((volatile UInt32 *)(map_registers + offset)) = data;

}

UInt16 GPMC::read16(UInt32 offset)
{
	return *((volatile UInt16 *)(map_registers + offset));
}

void GPMC::write16(UInt32 offset, UInt16 data)
{
	*((volatile UInt16 *)(map_registers + offset)) = data;
}

UInt32 GPMC::readRam32(UInt32 offset)
{
	return *((volatile UInt32 *)(map_ram + offset));
}

void GPMC::writeRam32(UInt32 offset, UInt32 data)
{
	*((volatile UInt32 *)(map_ram + offset)) = data;
}

UInt16 GPMC::readRam16(UInt32 offset)
{
	return *((volatile UInt16 *)(map_ram + offset));
}

void GPMC::writeRam16(UInt32 offset, UInt16 data)
{
	*((volatile UInt16 *)(map_ram + offset)) = data;
}

UInt8 GPMC::readRam8(UInt32 offset)
{
	return *((volatile UInt8 *)(map_ram + offset));
}

void GPMC::writeRam8(UInt32 offset, UInt8 data)
{
	*((volatile UInt8 *)(map_ram + offset)) = data;
}

UInt16 GPMC::readPixel12(UInt32 pixel, UInt32 offset)
{
	UInt32 address = pixel * 12 / 8 + offset;
	UInt8 shift = (pixel & 0x1) * 4;
	return ((readRam8(address) >> shift) | (((UInt16)readRam8(address+1)) << (8 - shift))) & ((1 << 12) - 1);
}

/* GPMC::readAcqMem
 *
 * Reads data from acquisition memory into a buffer
 *
 * buf:			Pointer to image buffer
 * offsetWords:	Number words into acquisition memory to start read
 * length:		number of bytes to read (must be a multiple of 4)
 *
 * returns: nothing
 **/
void GPMC::readAcqMem(UInt32 * buf, UInt32 offsetWords, UInt32 length)
{
	int i;
	if (read16(RAM_IDENTIFIER_REG) == RAM_IDENTIFIER) {
		UInt32 bytesLeft = length;
		UInt32 pageOffset = 0;

		while (bytesLeft) {
			//---- read a page
			// set address (in words or 256-bit blocks)
			write32(RAM_ADDRESS, offsetWords + (pageOffset >> 3));
			// trigger a read
			write16(RAM_CONTROL, RAM_CONTROL_TRIGGER_READ);
			// wait for read to complete
			for(i = 0; i < 1000 && read16(RAM_CONTROL); i++);

			// loop through reading out the data up to the full page
			// size or until there's no data left
			for (i = 0; i < 512 && bytesLeft > 0; ) {
				buf[i+pageOffset] = read32(RAM_BUFFER_START + (i<<2));
				i++;
				bytesLeft -= 4;
			}

			pageOffset += 512;
		}
	}
	else {
		write32(GPMC_PAGE_OFFSET_ADDR, offsetWords);

		for(i = 0; i < length/4; i++)
		{
			buf[i] = readRam32(4*i);
		}

		write32(GPMC_PAGE_OFFSET_ADDR, 0);
	}
}

/* GPMC::writeAcqMem
 *
 * Writes data from a buffer to acquisition memory
 *
 * buf:			Pointer to image buffer
 * offsetWords:	Number words into aqcuisition memory to start write
 * length:		number of bytes to write (must be a multiple of 4)
 *
 * returns: nothing
 **/
void GPMC::writeAcqMem(UInt32 * buf, UInt32 offsetWords, UInt32 length)
{
	int i;
	if (read16(RAM_IDENTIFIER_REG) == RAM_IDENTIFIER) {
		UInt32 bytesLeft = length;
		UInt32 pageOffset = 0;

		while (bytesLeft) {
			//---- write a page

			// loop through reading out the data up to the full page
			// size or until there's no data left
			for (i = 0; i < 512 && bytesLeft > 0; ) {
				write32(RAM_BUFFER_START + (i<<2), buf[i+pageOffset]);
				i++;
				bytesLeft -= 4;
			}

			// set address (in words or 256-bit blocks)
			write32(RAM_ADDRESS, offsetWords + (pageOffset >> 3));

			// trigger a read
			write16(RAM_CONTROL, RAM_CONTROL_TRIGGER_WRITE);
			// wait for read to complete
			for(i = 0; i < 1000 && read16(RAM_CONTROL); i++);

			pageOffset += 512;
		}
	}
	else {
		write32(GPMC_PAGE_OFFSET_ADDR, offsetWords);

		for(i = 0; i < length/4; i++)
		{
			writeRam32(4*i, buf[i]);
		}

		write32(GPMC_PAGE_OFFSET_ADDR, 0);
	}
}
