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
#include "cameraRegisters.h"
#include "defines.h"

#include "types.h"

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


IO::IO(void)
{
}

CameraErrortype IO::init()
{
	return SUCCESS;
}


void IO::resetToDefaults(Int32 flags) {
	setThreshold(1, 2.5, flags);
	setThreshold(2, 2.5, flags);
	setTriggerEnable(0, flags);
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
		pychIo1Threshold = thresholdVolts;
		break;

	case 2:
		if (flags & FLAG_USESAVED)
			thresholdVolts = appSettings.value("io/thresh2", 2.5).toDouble();
		io2Thresh = within(thresholdVolts, 0, IO_DAC_FS);
		if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
			appSettings.setValue("io/thresh2", io2Thresh);
		pychIo2Threshold = thresholdVolts;
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
	ioShadowWrite(PYCH_TRIG_ENABLE, source);
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/triggerEnable", source);
}

void IO::setTriggerInvert(UInt32 invert, Int32 flags)
{
	QSettings appSettings;
	if (flags & FLAG_USESAVED)
		invert = appSettings.value("io/triggerInvert", 1).toUInt();
	ioShadowWrite(PYCH_TRIG_INVERT, invert);
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/triggerInvert", invert);
}

void IO::setTriggerDebounceEn(UInt32 dbEn, Int32 flags)
{
	QSettings appSettings;
	if (flags & FLAG_USESAVED)
		dbEn = appSettings.value("io/triggerDebounce", 0).toUInt();
	ioShadowWrite(PYCH_TRIG_DEBOUNCE, dbEn);
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/triggerDebounce", dbEn);
}

void IO::setTriggerDelayFrames(UInt32 delayFrames, Int32 flags)
{
	QSettings appSettings;
	if (flags & FLAG_USESAVED)
		delayFrames = appSettings.value("io/triggerDelayFrames", 0).toUInt();
	ioShadowWrite(PYCH_SEQ_TRIG_DELAY, delayFrames);
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/triggerDelayFrames", delayFrames);
}

UInt32 IO::getTriggerEnable()
{
	return ioShadowRead(PYCH_TRIG_ENABLE);
}

UInt32 IO::getTriggerInvert()
{
	return ioShadowRead(PYCH_TRIG_INVERT);
}

UInt32 IO::getTriggerDebounceEn()
{
	return ioShadowRead(PYCH_TRIG_DEBOUNCE);
}

UInt32 IO::getTriggerDelayFrames()
{
	return ioShadowRead(PYCH_SEQ_TRIG_DELAY);
}

void IO::setOutLevel(UInt32 level, Int32 flags)
{
	QSettings appSettings;
	if (flags & FLAG_USESAVED)
		level = appSettings.value("io/outLevel", 2).toUInt();
	ioShadowWrite(PYCH_IO_OUT_LEVEL, level);
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/outLevel", level);
}

void IO::setOutSource(UInt32 source, Int32 flags)
{
	QSettings appSettings;
	if (flags & FLAG_USESAVED)
		source = appSettings.value("io/outSource", 0).toUInt();
	ioShadowWrite(PYCH_IO_OUT_SOURCE, source);
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/outSource", source);
}

void IO::setOutInvert(UInt32 invert, Int32 flags)
{
	QSettings appSettings;
	if (flags & FLAG_USESAVED)
		invert = appSettings.value("io/outInvert", 0).toUInt();
	ioShadowWrite(PYCH_IO_OUT_INVERT, invert);
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/outInvert", invert);
}

UInt32 IO::getOutLevel()
{
	return ioShadowRead(PYCH_IO_OUT_LEVEL);
}

UInt32 IO::getOutSource()
{
	return ioShadowRead(PYCH_IO_OUT_SOURCE);
}

UInt32 IO::getOutInvert()
{
	return ioShadowRead(PYCH_IO_OUT_INVERT);
}

UInt32 IO::getIn()
{
	return ioShadowRead(PYCH_IO_IN);
}

bool IO::getTriggeredExpEnable()
{
	return ioShadowRead(PYCH_EXT_SHUTTER_EXP);
}

UInt32 IO::getExtShutterSrcEn()
{
	return ioShadowRead(PYCH_EXT_SHUTTER_SRC_EN);
}

bool IO::getShutterGatingEnable()
{
	return ioShadowRead(PYCH_SHUTTER_GATING_ENABLE);
}

void IO::setTriggeredExpEnable(bool en, Int32 flags)
{
	QSettings appSettings;
	if (flags & FLAG_USESAVED)
		en = appSettings.value("io/triggeredExpEnable", false).toBool();
	ioShadowWrite(PYCH_EXT_SHUTTER_EXP, en);
}

void IO::setExtShutterSrcEn(UInt32 extShutterSrcEn, Int32 flags)
{
	QSettings appSettings;
	if (flags & FLAG_USESAVED)
		extShutterSrcEn = appSettings.value("io/extShutterSrcEn", 0).toUInt();
	ioShadowWrite(PYCH_EXT_SHUTTER_SRC_EN, extShutterSrcEn);
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/extShutterSrcEn", extShutterSrcEn);
}

void IO::setShutterGatingEnable(bool en, Int32 flags)
{
	QSettings appSettings;
	if (flags & FLAG_USESAVED) 
		en = appSettings.value("io/shutterGatingEnable", false).toBool();
	ioShadowWrite(PYCH_SHUTTER_GATING_ENABLE, en);
	if (!(flags & FLAG_TEMPORARY) && !(flags & FLAG_USESAVED))
		appSettings.setValue("io/shutterGatingEnable", en);
}
