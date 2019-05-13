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
#ifndef IO_H
#define IO_H
#include "dm8148PWM.h"
#include "errorCodes.h"
#include "types.h"
#include "gpmc.h"
#endif // IO_H


class IO {
public:
	IO(GPMC * gpmcInst);
	CameraErrortype init();
	void resetToDefaults(Int32 flags=0);
	bool readIO(UInt32 io);
	void writeIO(UInt32 io, UInt32 val);

	void setThreshold(UInt32 io, double thresholdVolts, Int32 flags=0);
	double getThreshold(UInt32 io);
	void setTriggerEnable(UInt32 source, Int32 flags=0);
	void setTriggerInvert(UInt32 invert, Int32 flags=0);
	void setTriggerDebounceEn(UInt32 dbEn, Int32 flags=0);
	void setTriggerDelayFrames(UInt32 delayFrames, Int32 flags=0);
	UInt32 getTriggerEnable();
	UInt32 getTriggerInvert();
	UInt32 getTriggerDebounceEn();
	UInt32 getTriggerDelayFrames();
	void setOutLevel(UInt32 level, Int32 flags=0);
	void setOutSource(UInt32 source, Int32 flags=0);
	void setOutInvert(UInt32 invert, Int32 flags=0);
	UInt32 getOutLevel();
	UInt32 getOutSource();
	UInt32 getOutInvert();
	UInt32 getIn();

	bool getTriggeredExpEnable();
	UInt32 getExtShutterSrcEn();
	bool getShutterGatingEnable();
	void setTriggeredExpEnable(bool en, Int32 flags=0);
	void setExtShutterSrcEn(UInt32 extShutterSrcEn, Int32 flags=0);
	void setShutterGatingEnable(bool en, Int32 flags=0);

private:
	dm8148PWM io1DAC, io2DAC;
	GPMC * gpmc;
	UInt32 IO2InFD;
	double io1Thresh, io2Thresh;
};
/*
enum {
	IO_SUCCESS = 0,
	IO_ERROR_OPEN,
	IO_FILE_ERROR
};
*/

extern UInt32 	ioShadowRegister[16];
extern double pychIo1Threshold;
extern double pychIo2Threshold;


enum {
	PYCH_TRIG_ENABLE = 0,
	PYCH_TRIG_INVERT,
	PYCH_TRIG_DEBOUNCE,
	PYCH_SEQ_TRIG_DELAY,
	PYCH_IO_OUT_LEVEL,
	PYCH_IO_OUT_SOURCE,
	PYCH_IO_OUT_INVERT,
	PYCH_IO_IN,
	PYCH_EXT_SHUTTER_EXP,
	PYCH_EXT_SHUTTER_SRC_EN,
	PYCH_SHUTTER_GATING_ENABLE
};


