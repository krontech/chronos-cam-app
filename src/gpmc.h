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
#ifndef GPMC_H
#define GPMC_H

#include "errorCodes.h"
#include "types.h"
#include "gpmcRegs.h"

#define GPMC_MAPPED_BASE	map_base
#define	GPMC_RANGE_BASE		0x1000000

class GPMC
{
public:
	GPMC();
	Int32 init();
	void setTimeoutEnable(bool timeoutEnable);
	UInt32 read32(UInt32 offset);
	void write32(UInt32 offset, UInt32 data);
	UInt16 read16(UInt32 offset);
	void write16(UInt32 offset, UInt16 data);
	UInt32 readRam32(UInt32 offset);
	void writeRam32(UInt32 offset, UInt32 data);
	UInt16 readRam16(UInt32 offset);
	void writeRam16(UInt32 offset, UInt16 data);
	UInt8 readRam8(UInt32 offset);
	void writeRam8(UInt32 offset, UInt8 data);

	UInt16 readPixel12(UInt32 pixel, UInt32 offset);
	void readAcqMem(UInt32 * buf, UInt32 offsetWords, UInt32 length);
	void writeAcqMem(UInt32 * buf, UInt32 offsetWords, UInt32 length);

private:
	UInt32 map_base, map_registers, map_ram;
};

#endif // GPMC_H
