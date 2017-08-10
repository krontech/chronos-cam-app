#ifndef DM8148PWM_H
#define DM8148PWM_H

#include "types.h"

#define TIDR			0x00
#define TIOCP_CFG		0x10
#define IRQ_EOI			0x20
#define IRQSTATUS_RAW	0x24
#define IRQSTATUS		0x28
#define IRQENABLE_SET	0x2C
#define IRQENABLE_CLR	0x30
#define IRQWAKEEN		0x34
#define TCLR			0x38
#define TCRR			0x3C
#define TLDR			0x40
#define TTGR			0x44
#define TWPS			0x48
#define TMAR			0x4C
#define TCAR1			0x50
#define TSICR			0x54
#define TCAR2			0x58



class dm8148PWM
{
public:
	dm8148PWM();
	Int32 init(UInt32 address);
	void setPeriod(UInt32 period);
	void setDuty(UInt32 duty);
	void setDuty(double pw);

private:
	void writeRegister(UInt32 offset, UInt32 data);

	UInt32 map_base;
	UInt32 period;
};

enum {
	DM8148PWM_SUCCESS = 0,
	DM8148PWM_ERR_FOPEN,
	DM8148PWM_ERR_MAP
};


#endif // DM8148PWM_H
