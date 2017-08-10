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
