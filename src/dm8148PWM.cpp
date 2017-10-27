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
#include "dm8148PWM.h"
#include "defines.h"
#include "types.h"


dm8148PWM::dm8148PWM()
{

	period = 0;
}

Int32 dm8148PWM::init(UInt32 address)
{
	int fd_reg;

	fd_reg = open("/dev/mem", O_RDWR | O_SYNC);
	if(fd_reg < 0)
		return DM8148PWM_ERR_FOPEN;


	map_base = (unsigned int) mmap(0, 4096/*4k*/, PROT_READ | PROT_WRITE, MAP_SHARED, fd_reg, address);

	if(map_base == -1)
		return DM8148PWM_ERR_MAP;

	writeRegister(TCLR, 0x18C2);
	writeRegister(TLDR, 0xFFFFFFFF - 16384);
	writeRegister(TMAR, 0xFFFFFFFF - 8192);
	writeRegister(TCLR, 0x18C3);

	writeRegister(TTGR, 0);		//Required to start timer running



	return DM8148PWM_SUCCESS;
}

void dm8148PWM::setPeriod(UInt32 per)
{
	writeRegister(TLDR, 0xFFFFFFFF - per + 1);
	period = per;
}

void dm8148PWM::setDuty(UInt32 duty)
{
	if(0 == period)
		return;

	if(duty < 2)
		duty = 2;
	if(duty > (period - 2))
		duty = period - 2;

	writeRegister(TMAR, 0xFFFFFFFF - duty);

}

//Set duty in per unit (1.0 == 100%)
void dm8148PWM::setDuty(double pw)
{
	if(0 == period)
		return;

	UInt32 duty = within(pw, 0.0, 1.0) * period;


	if(duty < 2)
		duty = 2;
	if(duty > (period - 2))
		duty = period - 2;

	writeRegister(TMAR, 0xFFFFFFFF - duty);

}

void dm8148PWM::writeRegister(UInt32 offset, UInt32 data)
{
	*((volatile UInt32 *)(map_base + offset)) = data;

}
