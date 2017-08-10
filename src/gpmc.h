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

	UInt32 map_base, map_registers, map_ram;
};
/*
enum {
	GPMC_SUCCESS = 0,
	GPMCERR_FOPEN,
	GPMCERR_MAP
};
*/
#endif // GPMC_H
