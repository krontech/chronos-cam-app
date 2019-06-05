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

extern bool pych;

UInt32 	ioShadow[16];
double pychIo1Threshold;
double pychIo2Threshold;

ioElement io1;
ioElement io2;
ioElement io3;
ioElement combOr1;
ioElement combOr2;
ioElement combOr3;
ioElement combXor;
ioElement combAnd;
ioElement out1;
ioElement out2;

enum {
	io_none = 0,
	io_end,
	io_exposure,
	io_shuttergating,
	io_framesync
};

UInt32 io1Mode;
UInt32 io2Mode;
UInt32 io3Mode;

void resetElement(ioElement *e)
{
	e->debounce = false;
	e->invert = false;
	e->source = SRC_NONE;
}

QString ioSources[] = {
	"NONE",
	"IO1",
	"IO2",
	"IO3",
	"COMB",
	"DELAY",
	"TOGGLE",
	"SHUTTER",
	"RECORDING",
	"DISPFRAME",
	"STARTREC",
	"ENDREC",
	"NEXTSEG",
	"TIMINGIO",
	"ALWAYSHIGH"
};

void printIoElement(QString name, ioElement *e)
{
	QString debounceStr = e->debounce ? "debounce" : "-";
	QString invertStr = e->invert ? "invert" : "-";
	qDebug() << name << ": "  << ioSources[e->source] << debounceStr << invertStr;
}

void printIoElements(void)
{
	printIoElement("io1", &io1);
	printIoElement("io2", &io2);
	printIoElement("io3", &io3);
	printIoElement("combOr1", &combOr1);
	printIoElement("combOr2", &combOr2);
	printIoElement("combOr3", &combOr3);
	printIoElement("combXor", &combXor);
	printIoElement("combAnd", &combAnd);
	printIoElement("out1", &out2);
	printIoElement("out2", &out2);
}


void translateToComb(void)
{
	io1Mode = io_none;
	io2Mode = io_none;
	io3Mode = io_none;

	resetElement(&io1);
	resetElement(&io2);
	resetElement(&io3);
	resetElement(&combOr1);
	resetElement(&combOr2);
	resetElement(&combOr3);
	resetElement(&combXor);
	resetElement(&combAnd);
	resetElement(&out1);
	resetElement(&out2);

	bool io1Pullup1mA = false;
	bool io1Pullup20mA = false;
	bool io2Pullup20mA = false;

	qDebug() << "-----------------------------";
	qDebug() << "PYCH_TRIG_ENABLE: " << ioShadow[PYCH_TRIG_ENABLE];
	qDebug() << "PYCH_TRIG_INVERT: " << ioShadow[PYCH_TRIG_INVERT];
	qDebug() << "PYCH_TRIG_DEBOUNCE: " << ioShadow[PYCH_TRIG_DEBOUNCE];
	qDebug() << "PYCH_SEQ_TRIG_DELAY: " << ioShadow[PYCH_SEQ_TRIG_DELAY];
	qDebug() << "PYCH_IO_OUT_LEVEL: " << ioShadow[PYCH_IO_OUT_LEVEL];
	qDebug() << "PYCH_IO_OUT_SOURCE: " << ioShadow[PYCH_IO_OUT_SOURCE];
	qDebug() << "PYCH_IO_OUT_INVERT: " << ioShadow[PYCH_IO_OUT_INVERT];
	qDebug() << "PYCH_IO_IN: " << ioShadow[PYCH_IO_IN];
	qDebug() << "PYCH_EXT_SHUTTER_EXP: " << ioShadow[PYCH_EXT_SHUTTER_EXP];
	qDebug() << "PYCH_EXT_SHUTTER_SRC_EN: " << ioShadow[PYCH_EXT_SHUTTER_SRC_EN];
	qDebug() << "PYCH_SHUTTER_GATING_ENABLE: " << ioShadow[PYCH_SHUTTER_GATING_ENABLE];

	// check for exposure trigger and shutter gating
	if (ioShadow[PYCH_EXT_SHUTTER_EXP])
	{
		qDebug() << "Exposure Trigger";
		if (ioShadow[PYCH_EXT_SHUTTER_SRC_EN] == 1)
		{
			qDebug() << "  io1";
			io1Mode = io_exposure;
		}
		else if (ioShadow[PYCH_EXT_SHUTTER_SRC_EN] == 2)
		{
			qDebug() << "  io2";
			io2Mode = io_exposure;
		}
		else
		{
			qDebug ("ERROR!");
		}
	}
	else if (ioShadow[PYCH_SHUTTER_GATING_ENABLE])
	{
		qDebug() << "Shutter Gating";
		if (ioShadow[PYCH_EXT_SHUTTER_SRC_EN] == 1)
		{
			qDebug() << "  io1";
			io1Mode = io_shuttergating;
		}
		else if (ioShadow[PYCH_EXT_SHUTTER_SRC_EN] == 2)
		{
			qDebug() << "  io2";
			io2Mode = io_shuttergating;
		}
		else
		{
			qDebug ("ERROR!");
		}
	}

	// Frame Sync
	if ((ioShadow[PYCH_IO_IN] & 1) && ioShadow[PYCH_IO_OUT_SOURCE] & 2)
	{
		qDebug() << "Frame Sync Output io1";
		io1Mode = io_framesync;
	}
	if ((ioShadow[PYCH_IO_IN] & 2) && ioShadow[PYCH_IO_OUT_SOURCE] & 4)
	{
		qDebug() << "Frame Sync Output io2";
		io2Mode = io_framesync;
	}

	// Record End Trigger
	if (ioShadow[PYCH_TRIG_ENABLE] & 1)
	{
		qDebug() << "Record End Trigger io1";
		io1Mode = io_end;
	}
	if (ioShadow[PYCH_TRIG_ENABLE] & 2)
	{
		qDebug() << "Record End Trigger io2";
		io2Mode = io_end;
	}
	if (ioShadow[PYCH_TRIG_ENABLE] & 4)
	{
		qDebug() << "Record End Trigger io3";
		io3Mode = io_end;
	}

	// pullups
	if (ioShadow[PYCH_IO_OUT_LEVEL] & 1)
	{
		qDebug() << "   1 mA Pullup io1 ";
		io1Pullup1mA = true;
	}
	if (ioShadow[PYCH_IO_OUT_LEVEL] & 2)
	{
		qDebug() << "   20 mA Pullup io1 ";
		io1Pullup20mA = true;
	}

	if (ioShadow[PYCH_IO_OUT_LEVEL] & 4)
	{
		qDebug() << "   20 mA Pullup io2 ";
		io2Pullup20mA = true;
	}

	// invert
	if (ioShadow[PYCH_TRIG_INVERT] & 1)
	{
		qDebug() << "Invert io1";
		io1.invert = true;
	}
	if (ioShadow[PYCH_TRIG_INVERT] & 2)
	{
		qDebug() << "Invert io2";
		io2.invert = true;
	}
	if (ioShadow[PYCH_TRIG_INVERT] & 4)
	{
		qDebug() << "Invert io3";
		io3.invert = true;
	}

	if (ioShadow[PYCH_TRIG_DEBOUNCE] & 1)
	{
		qDebug() << "Debounce io1";
		io1.debounce = true;
	}
	if (ioShadow[PYCH_TRIG_DEBOUNCE] & 2)
	{
		qDebug() << "Debounce io2";
		io2.debounce = true;
	}
	if (ioShadow[PYCH_TRIG_DEBOUNCE] & 4)
	{
		qDebug() << "Debounce io3";
		io3.debounce = true;
	}

	// invert outputs
	if ((ioShadow[PYCH_IO_OUT_INVERT] & 3) == 3)
	{
		qDebug() << "Invert output 1";
		out1.invert = true;
	}
	if (ioShadow[PYCH_IO_OUT_INVERT] & 4)
	{
		qDebug() << "Invert output 2";
		out2.invert = true;
	}
	printIoElements();
}



void ioShadowWrite(UInt32 reg, UInt32 value)
{
	ioShadow[reg] = value;
}

UInt32 ioShadowRead(UInt32 reg)
{
	return ioShadow[reg];
}




IO::IO(GPMC* gpmcInst)
{
	gpmc = gpmcInst;
}

CameraErrortype IO::init()
{
	if (pych)
	{
		return SUCCESS;
	}

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
	// pych note: unused
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
	//pych note: unused
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
		io1Thresh = within(thresholdVolts, 0, IO_DAC_FS);
		if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
			appSettings.setValue("io/thresh1", io1Thresh);
		if (pych)
		{
			pychIo1Threshold = thresholdVolts;
		}
		else
		{
			io1DAC.setDuty(io1Thresh / IO_DAC_FS);
		}
		break;

	case 2:
		if (flags & FLAG_USESAVED)
			thresholdVolts = appSettings.value("io/thresh2", 2.5).toDouble();
		io2Thresh = within(thresholdVolts, 0, IO_DAC_FS);
		if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
			appSettings.setValue("io/thresh2", io2Thresh);
		if (pych)
		{
			pychIo2Threshold = thresholdVolts;
		}
		else
		{
			io2DAC.setDuty(io2Thresh / IO_DAC_FS);
		}
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
	if (pych)
	{
		ioShadowWrite(PYCH_TRIG_ENABLE, source);
	}
	else
	{
		gpmc->write16(TRIG_ENABLE_ADDR, source);
	}
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/triggerEnable", source);
}

void IO::setTriggerInvert(UInt32 invert, Int32 flags)
{
	QSettings appSettings;
	if (flags & FLAG_USESAVED)
		invert = appSettings.value("io/triggerInvert", 1).toUInt();
	if (pych)
	{
		ioShadowWrite(PYCH_TRIG_INVERT, invert);
	}
	else
	{
		gpmc->write16(TRIG_INVERT_ADDR, invert);
	}
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/triggerInvert", invert);
}

void IO::setTriggerDebounceEn(UInt32 dbEn, Int32 flags)
{
	QSettings appSettings;
	if (flags & FLAG_USESAVED)
		dbEn = appSettings.value("io/triggerDebounce", 0).toUInt();
	if (pych)
	{
		ioShadowWrite(PYCH_TRIG_DEBOUNCE, dbEn);
	}
	else
	{
		gpmc->write16(TRIG_DEBOUNCE_ADDR, dbEn);
	}
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/triggerDebounce", dbEn);
}

void IO::setTriggerDelayFrames(UInt32 delayFrames, Int32 flags)
{
	QSettings appSettings;
	if (flags & FLAG_USESAVED)
		delayFrames = appSettings.value("io/triggerDelayFrames", 0).toUInt();
	if (pych)
	{
		ioShadowWrite(PYCH_SEQ_TRIG_DELAY, delayFrames);
	}
	else
	{
		gpmc->write32(SEQ_TRIG_DELAY_ADDR, delayFrames);
	}
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/triggerDelayFrames", delayFrames);
}

UInt32 IO::getTriggerEnable()
{
	if (pych)
	{
		return ioShadowRead(PYCH_TRIG_ENABLE);
	}
	else
	{
		return gpmc->read16(TRIG_ENABLE_ADDR);
	}
}

UInt32 IO::getTriggerInvert()
{
	if (pych)
	{
		return ioShadowRead(PYCH_TRIG_INVERT);
	}
	else
	{
		return gpmc->read16(TRIG_INVERT_ADDR);
	}
}

UInt32 IO::getTriggerDebounceEn()
{
	if (pych)
	{
		return ioShadowRead(PYCH_TRIG_DEBOUNCE);
	}
	else
	{
		return gpmc->read16(TRIG_DEBOUNCE_ADDR);
	}
}

UInt32 IO::getTriggerDelayFrames()
{
	if (pych)
	{
		return ioShadowRead(PYCH_SEQ_TRIG_DELAY);
	}
	else
	{
		return gpmc->read32(SEQ_TRIG_DELAY_ADDR);
	}
}

void IO::setOutLevel(UInt32 level, Int32 flags)
{
	QSettings appSettings;
	if (flags & FLAG_USESAVED)
		level = appSettings.value("io/outLevel", 2).toUInt();
	if (pych)
	{
		ioShadowWrite(PYCH_IO_OUT_LEVEL, level);
	}
	else
	{
		gpmc->write16(IO_OUT_LEVEL_ADDR, level);
	}
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/outLevel", level);
}

void IO::setOutSource(UInt32 source, Int32 flags)
{
	QSettings appSettings;
	if (flags & FLAG_USESAVED)
		source = appSettings.value("io/outSource", 0).toUInt();
	if (pych)
	{
		ioShadowWrite(PYCH_IO_OUT_SOURCE, source);
	}
	else
	{
		gpmc->write16(IO_OUT_SOURCE_ADDR, source);
	}
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/outSource", source);
}

void IO::setOutInvert(UInt32 invert, Int32 flags)
{
	QSettings appSettings;
	if (flags & FLAG_USESAVED)
		invert = appSettings.value("io/outInvert", 0).toUInt();
	if (pych)
	{
		ioShadowWrite(PYCH_IO_OUT_INVERT, invert);
	}
	else
	{
		gpmc->write16(IO_OUT_INVERT_ADDR, invert);
	}
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/outInvert", invert);
}

UInt32 IO::getOutLevel()
{
	if (pych)
	{
		return ioShadowRead(PYCH_IO_OUT_LEVEL);
	}
	else
	{
		return gpmc->read16(IO_OUT_LEVEL_ADDR);
	}
}

UInt32 IO::getOutSource()
{
	if (pych)
	{
		return ioShadowRead(PYCH_IO_OUT_SOURCE);
	}
	else
	{
		return gpmc->read16(IO_OUT_SOURCE_ADDR);
	}
}

UInt32 IO::getOutInvert()
{
	if (pych)
	{
		return ioShadowRead(PYCH_IO_OUT_INVERT);
	}
	else
	{
		return gpmc->read16(IO_OUT_INVERT_ADDR);
	}
}

UInt32 IO::getIn()
{
	if (pych)
	{
		return ioShadowRead(PYCH_IO_IN);
	}
	else
	{
		return gpmc->read16(IO_IN_ADDR);
	}
}

bool IO::getTriggeredExpEnable()
{
	if (pych)
	{
		return ioShadowRead(PYCH_EXT_SHUTTER_EXP);
	}
	else
	{
		return gpmc->read16(EXT_SHUTTER_CTL_ADDR) & EXT_SH_TRIGD_EXP_EN_MASK;
	}
}

UInt32 IO::getExtShutterSrcEn()
{
	if (pych)
	{
		return ioShadowRead(PYCH_EXT_SHUTTER_SRC_EN);
	}
	else
	{
		return (gpmc->read16(EXT_SHUTTER_CTL_ADDR) & EXT_SH_SRC_EN_MASK) >> EXT_SH_SRC_EN_OFFSET;
	}
}

bool IO::getShutterGatingEnable()
{
	if (pych)
	{
		return ioShadowRead(PYCH_SHUTTER_GATING_ENABLE);
	}
	else
	{
		return gpmc->read16(EXT_SHUTTER_CTL_ADDR) & EXT_SH_GATING_EN_MASK;\
	}
}

void IO::setTriggeredExpEnable(bool en, Int32 flags)
{
	QSettings appSettings;
	if (flags & FLAG_USESAVED)
		en = appSettings.value("io/triggeredExpEnable", false).toBool();
	if (pych)
	{
		ioShadowWrite(PYCH_EXT_SHUTTER_EXP, en);
	}
	else
	{
		gpmc->write16(EXT_SHUTTER_CTL_ADDR, (gpmc->read16(EXT_SHUTTER_CTL_ADDR) & ~EXT_SH_TRIGD_EXP_EN_MASK) | (en ? EXT_SH_TRIGD_EXP_EN_MASK : 0));
	}
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/triggeredExpEnable", en);
}

void IO::setExtShutterSrcEn(UInt32 extShutterSrcEn, Int32 flags)
{
	QSettings appSettings;
	if (flags & FLAG_USESAVED)
		extShutterSrcEn = appSettings.value("io/extShutterSrcEn", 0).toUInt();
	if (pych)
	{
		ioShadowWrite(PYCH_EXT_SHUTTER_SRC_EN, extShutterSrcEn);
	}
	else
	{
		gpmc->write16(EXT_SHUTTER_CTL_ADDR, (gpmc->read16(EXT_SHUTTER_CTL_ADDR) & ~EXT_SH_SRC_EN_MASK) | (extShutterSrcEn << EXT_SH_SRC_EN_OFFSET));
	}
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/extShutterSrcEn", extShutterSrcEn);
}

void IO::setShutterGatingEnable(bool en, Int32 flags)
{
	QSettings appSettings;
	if (flags & FLAG_USESAVED) 
		en = appSettings.value("io/shutterGatingEnable", false).toBool();
	if (pych)
	{
		ioShadowWrite(PYCH_SHUTTER_GATING_ENABLE, en);
	}
	else
	{
		gpmc->write16(EXT_SHUTTER_CTL_ADDR, (gpmc->read16(EXT_SHUTTER_CTL_ADDR) & ~EXT_SH_GATING_EN_MASK) | (en ? EXT_SH_GATING_EN_MASK : 0));
	}
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/shutterGatingEnable", en);
}
