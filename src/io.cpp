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
#include <QDebug>
#include <QSettings>
#include <fcntl.h>
#include <unistd.h>
#include "io.h"
#include "dm8148PWM.h"
#include "cameraRegisters.h"
#include "gpmc.h"
#include "defines.h"

#include "types.h"


IO::IO(GPMC* gpmcInst)
{
	gpmc = gpmcInst;
}

CameraErrortype IO::init()
{
	QSettings appSettings;
	IO2InFD = open(IO2IN_PATH, O_RDONLY);

	if (-1 == IO2InFD)
		return IO_FILE_ERROR;

	if(io1DAC.init(IO1DAC_TIMER_BASE) != DM8148PWM_SUCCESS)
		return IO_ERROR_OPEN;

	if(io2DAC.init(IO2DAC_TIMER_BASE) != DM8148PWM_SUCCESS)
		return IO_ERROR_OPEN;

	io1DAC.setPeriod(6600);
	io2DAC.setPeriod(6600);
	io1Thresh = appSettings.value("io/thresh1", 2.5).toDouble();
	io2Thresh = appSettings.value("io/thresh2", 2.5).toDouble();
	io1DAC.setDuty(io1Thresh / IO_DAC_FS);
	io2DAC.setDuty(io2Thresh / IO_DAC_FS);

	setTriggerEnable(0, FLAG_USESAVED);
	setTriggerInvert(0, FLAG_USESAVED);
	setTriggerDebounceEn(0, FLAG_USESAVED);
	setTriggerDelayFrames(0, FLAG_USESAVED);
	
	setOutInvert(0, FLAG_USESAVED);
	setOutSource(0, FLAG_USESAVED);
	setOutLevel(0, FLAG_USESAVED);
	
	setExtShutterSrcEn(0, FLAG_USESAVED);
	setTriggeredExpEnable(0, FLAG_USESAVED);
	setShutterGatingEnable(0, FLAG_USESAVED);

	return SUCCESS;
}


void IO::resetToDefaults(Int32 flags) {
	setThreshold(1, 2.5, flags);
	setThreshold(2, 2.5, flags);
	setTriggerEnable(0, flags);
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


void IO::setThreshold(UInt32 io, double thresholdVolts, Int32 flags)
{
	QSettings appSettings;
	switch(io)
	{
	case 1:
		if (flags & FLAG_USESAVED)
			thresholdVolts = appSettings.value("io/thresh1", 2.5).toDouble();
		io1Thresh = clamp(thresholdVolts, 0, IO_DAC_FS);
		if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
			appSettings.setValue("io/thresh1", io1Thresh);
		io1DAC.setDuty(io1Thresh / IO_DAC_FS);
		break;

	case 2:
		if (flags & FLAG_USESAVED)
			thresholdVolts = appSettings.value("io/thresh2", 2.5).toDouble();
		io2Thresh = clamp(thresholdVolts, 0, IO_DAC_FS);
		if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
			appSettings.setValue("io/thresh2", io2Thresh);
		io2DAC.setDuty(io2Thresh / IO_DAC_FS);
		break;
	}
}

double IO::getThreshold(UInt32 io)
{
	QSettings appSettings;
	switch(io)
	{
	case 1:
		return appSettings.value("io/thresh1", io1Thresh).toDouble();

	case 2:
		return appSettings.value("io/thresh2", io2Thresh).toDouble();

	default:
		return 0.0;
	}
}

void IO::setTriggerEnable(UInt32 source, Int32 flags)
{
	QSettings appSettings;
	if (flags & FLAG_USESAVED)
		source = appSettings.value("io/triggerEnable", 1).toUInt();
	gpmc->write16(TRIG_ENABLE_ADDR, source);
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/triggerEnable", source);
}

void IO::setTriggerInvert(UInt32 invert, Int32 flags)
{
	QSettings appSettings;
	if (flags & FLAG_USESAVED)
		invert = appSettings.value("io/triggerInvert", 1).toUInt();
	gpmc->write16(TRIG_INVERT_ADDR, invert);
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/triggerInvert", invert);
}

void IO::setTriggerDebounceEn(UInt32 dbEn, Int32 flags)
{
	QSettings appSettings;
	if (flags & FLAG_USESAVED)
		dbEn = appSettings.value("io/triggerDebounce", 0).toUInt();
	gpmc->write16(TRIG_DEBOUNCE_ADDR, dbEn);
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/triggerDebounce", dbEn);
}

void IO::setTriggerDelayFrames(UInt32 delayFrames, Int32 flags)
{
	QSettings appSettings;
	if (flags & FLAG_USESAVED)
		delayFrames = appSettings.value("io/triggerDelayFrames", 0).toUInt();
	gpmc->write32(SEQ_TRIG_DELAY_ADDR, delayFrames);
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/triggerDelayFrames", delayFrames);
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

void IO::setOutLevel(UInt32 level, Int32 flags)
{
	QSettings appSettings;
	if (flags & FLAG_USESAVED)
		level = appSettings.value("io/outLevel", 2).toUInt();
	gpmc->write16(IO_OUT_LEVEL_ADDR, level);
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/outLevel", level);
}

void IO::setOutSource(UInt32 source, Int32 flags)
{
	QSettings appSettings;
	if (flags & FLAG_USESAVED)
		source = appSettings.value("io/outSource", 0).toUInt();
	gpmc->write16(IO_OUT_SOURCE_ADDR, source);
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/outSource", source);
}

void IO::setOutInvert(UInt32 invert, Int32 flags)
{
	QSettings appSettings;
	if (flags & FLAG_USESAVED)
		invert = appSettings.value("io/outInvert", 0).toUInt();
	gpmc->write16(IO_OUT_INVERT_ADDR, invert);
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/outInvert", invert);
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

void IO::setTriggeredExpEnable(bool en, Int32 flags)
{
	QSettings appSettings;
	if (flags & FLAG_USESAVED)
		en = appSettings.value("io/triggeredExpEnable", false).toBool();
	gpmc->write16(EXT_SHUTTER_CTL_ADDR, (gpmc->read16(EXT_SHUTTER_CTL_ADDR) & ~EXT_SH_TRIGD_EXP_EN_MASK) | ((en ? 1 : 0) << EXT_SH_TRIGD_EXP_EN_OFFSET));
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/triggeredExpEnable", en);
}

void IO::setExtShutterSrcEn(UInt32 extShutterSrcEn, Int32 flags)
{
	QSettings appSettings;
	if (flags & FLAG_USESAVED)
		extShutterSrcEn = appSettings.value("io/extShutterSrcEn", 0).toUInt();
	gpmc->write16(EXT_SHUTTER_CTL_ADDR, (gpmc->read16(EXT_SHUTTER_CTL_ADDR) & ~EXT_SH_SRC_EN_MASK) | ((extShutterSrcEn ? 1 : 0) << EXT_SH_SRC_EN_OFFSET));
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/extShutterSrcEn", extShutterSrcEn);
}

void IO::setShutterGatingEnable(bool en, Int32 flags)
{
	QSettings appSettings;
	if (flags & FLAG_USESAVED) 
		en = appSettings.value("io/shutterGatingEnable", false).toBool();
	gpmc->write16(EXT_SHUTTER_CTL_ADDR, (gpmc->read16(EXT_SHUTTER_CTL_ADDR) & ~EXT_SH_GATING_EN_MASK) | ((en ? 1 : 0) << EXT_SH_GATING_EN_OFFSET));
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/shutterGatingEnable", en);
}
