#include <fcntl.h>
#include <sys/mman.h>
#include "gpmc.h"
#include "defines.h"

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
							0;				/*TIMEOUTENABLE*/

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
	GPMC_CONFIG2_i(0) =	11 << 16 |	/*CSWROFFTIME*/
						12 << 8 |	/*CSRDOFFTIME*/
						0 << 7 |	/*CSEXTRADELAY*/
						1;			/*CSONTIME*/

	GPMC_CONFIG3_i(0) =	0 << 28 |	/*ADVAADMUXWROFFTIME*/
						0 << 24 |	/*ADVAADMUXRDOFFTIME*/
						6 << 16 |	/*ADVWROFFTIME*/
						6 << 8 |	/*ADVRDOFFTIME*/
						0 << 7 |	/*ADVEXTRADELAY*/
						0 << 4 |	/*ADVAADMUXONTIME*/
						2;			/*ADVONTIME*/

	GPMC_CONFIG4_i(0) =	10 << 24 |	/*WEOFFTIME*/
						0 << 23 |	/*WEEXTRADELAY*/
						7 << 16 |	/*WEONTIME*/
						0 << 13 |	/*OEAADMUXOFFTIME*/
						11 << 8 |	/*OEOFFTIME*/
						0 << 7 |	/*OEEXTRADELAY*/
						0 << 4 |	/*OEAADMUXONTIME*/
						7;			/*OEONTIME*/

	GPMC_CONFIG5_i(0) =	0 << 24 |	/*PAGEBURSTACCESSTIME*/
						10 << 16 |	/*RDACCESSTIME*/
						12 << 8 |	/*WRCYCLETIME*/
						12;			/*RDCYCLETIME*/



	GPMC_CONFIG6_i(0) =	6 << 24 |	/*WRACCESSTIME*/
						7 << 16 |	/*WRDATAONADMUXBUS*/
						1 << 8 |	/*CYCLE2CYCLEDELAY*/
						1 << 7 |	/*CYCLE2CYCLESAMECSEN*/
						1 << 6 |	/*CYCLE2CYCLEDIFFCSEN*/
						1;			/*BUSTURNAROUND*/

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
						17 << 16 |	/*RDACCESSTIME*/
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

	return CAMERA_SUCCESS;
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
