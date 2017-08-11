//#include <stdio.h>
#include <QDebug>
#include <fcntl.h>
#include <unistd.h>
#include "io.h"
#include "dm8148PWM.h"
#include "cameraRegisters.h"
#include "gpmc.h"
#include "defines.h"

#include "types.h"



IO::IO(GPMC * gpmcInst)
{
	gpmc = gpmcInst;
}

CameraErrortype IO::init()
{
	IO2InFD = open(IO2IN_PATH, O_RDONLY);

	if (-1 == IO2InFD)
		return IO_FILE_ERROR;

	if(io1DAC.init(IO1DAC_TIMER_BASE) != DM8148PWM_SUCCESS)
		return IO_ERROR_OPEN;

	if(io2DAC.init(IO2DAC_TIMER_BASE) != DM8148PWM_SUCCESS)
		return IO_ERROR_OPEN;

	io1DAC.setPeriod(6600);
	io2DAC.setPeriod(6600);
	io1Thresh = 2.5;
	io2Thresh = 2.5;
	io1DAC.setDuty(io1Thresh / IO_DAC_FS);
	io2DAC.setDuty(io2Thresh / IO_DAC_FS);

	return CAMERA_SUCCESS;

}

bool IO::readIO(UInt32 io)
{
	char buf[2];

	switch(io)
	{
	case 1:
		return false;

	case 2:
		lseek(IO2InFD, 0, SEEK_SET);
		read(IO2InFD, buf, sizeof(buf));

		return ('1' == buf[0]) ? true : false;

	default:
		return false;

	}
}


void IO::writeIO(UInt32 io, UInt32 val)
{
	switch(io)
	{
	case 1:
		gpmc->write32(IO_OUT_LEVEL_ADDR, (gpmc->read32(IO_OUT_LEVEL_ADDR) & ~1) | (val & 1));
		gpmc->write32(IO_OUT_LEVEL_ADDR, (gpmc->read32(IO_OUT_LEVEL_ADDR) & ~2) | (val & 2));
	break;

	case 2:
		gpmc->write32(IO_OUT_LEVEL_ADDR, (gpmc->read32(IO_OUT_LEVEL_ADDR) & ~4) | (val ? 1 : 0));
	break;
	}
}


void IO::setThreshold(UInt32 io, double thresholdVolts)
{
	switch(io)
	{
	case 1:
		io1Thresh = within(thresholdVolts, 0, IO_DAC_FS);
		io1DAC.setDuty(io1Thresh / IO_DAC_FS);
		break;

	case 2:
		io2Thresh = within(thresholdVolts, 0, IO_DAC_FS);
		io2DAC.setDuty(io2Thresh / IO_DAC_FS);
		break;
	}
}

double IO::getThreshold(UInt32 io)
{
	switch(io)
	{
	case 1:
		return io1Thresh;
		break;

	case 2:
		return io2Thresh;
		break;

	default:
		return 0.0;
	}
}

void IO::setTriggerEnable(UInt32 source)
{
	gpmc->write16(TRIG_ENABLE_ADDR, source);
}

void IO::setTriggerInvert(UInt32 invert)
{
	gpmc->write16(TRIG_INVERT_ADDR, invert);
}

void IO::setTriggerDebounceEn(UInt32 dbEn)
{
	gpmc->write16(TRIG_DEBOUNCE_ADDR, dbEn);
}

void IO::setTriggerDelayFrames(UInt32 delayFrames)
{
	gpmc->write32(SEQ_TRIG_DELAY_ADDR, delayFrames);
}

UInt32 IO::getTriggerEnable()
{
	return gpmc->read16(TRIG_ENABLE_ADDR);
}

UInt32 IO::getTriggerInvert()
{
	return gpmc->read16(TRIG_INVERT_ADDR);
}

UInt32 IO::getTriggerDebounceEn()
{
	return gpmc->read16(TRIG_DEBOUNCE_ADDR);
}

UInt32 IO::getTriggerDelayFrames()
{
	return gpmc->read32(SEQ_TRIG_DELAY_ADDR);
}

void IO::setOutLevel(UInt32 level)
{
	gpmc->write16(IO_OUT_LEVEL_ADDR, level);
}

void IO::setOutSource(UInt32 source)
{
	gpmc->write16(IO_OUT_SOURCE_ADDR, source);
}

void IO::setOutInvert(UInt32 invert)
{
	gpmc->write16(IO_OUT_INVERT_ADDR, invert);
}

UInt32 IO::getOutLevel()
{
	return gpmc->read16(IO_OUT_LEVEL_ADDR);
}

UInt32 IO::getOutSource()
{
	return gpmc->read16(IO_OUT_SOURCE_ADDR);
}

UInt32 IO::getOutInvert()
{
	return gpmc->read16(IO_OUT_INVERT_ADDR);
}

UInt32 IO::getIn()
{
	return gpmc->read16(IO_IN_ADDR);
}

bool IO::getTriggeredExpEnable()
{
	return gpmc->read16(EXT_SHUTTER_CTL_ADDR) & EXT_SH_TRIGD_EXP_EN_MASK;
}

UInt32 IO::getExtShutterSrcEn()
{
	return (gpmc->read16(EXT_SHUTTER_CTL_ADDR) & EXT_SH_SRC_EN_MASK) >> EXT_SH_SRC_EN_OFFSET;
}

bool IO::getShutterGatingEnable()
{
	return gpmc->read16(EXT_SHUTTER_CTL_ADDR) & EXT_SH_GATING_EN_MASK;
}

void IO::setTriggeredExpEnable(bool en)
{
	gpmc->write16(EXT_SHUTTER_CTL_ADDR, (gpmc->read16(EXT_SHUTTER_CTL_ADDR) & ~EXT_SH_TRIGD_EXP_EN_MASK) | ((en ? 1 : 0) << EXT_SH_TRIGD_EXP_EN_OFFSET));
}

void IO::setExtShutterSrcEn(UInt32 extShutterSrcEn)
{
	gpmc->write16(EXT_SHUTTER_CTL_ADDR, (gpmc->read16(EXT_SHUTTER_CTL_ADDR) & ~EXT_SH_SRC_EN_MASK) | ((extShutterSrcEn ? 1 : 0) << EXT_SH_SRC_EN_OFFSET));
}

void IO::setShutterGatingEnable(bool en)
{
	gpmc->write16(EXT_SHUTTER_CTL_ADDR, (gpmc->read16(EXT_SHUTTER_CTL_ADDR) & ~EXT_SH_GATING_EN_MASK) | ((en ? 1 : 0) << EXT_SH_GATING_EN_OFFSET));
}
