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
#include "errorCodes.h"
#include "types.h"
#endif // IO_H


class IO {
public:
	IO(void);
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
	UInt32 IO2InFD;
	double io1Thresh, io2Thresh;
};

extern UInt32 	ioShadow[16];
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

enum {
	SRC_NONE = 0,
	SRC_IO1,
	SRC_IO2,
	SRC_IO3,
	SRC_COMB,
	SRC_DELAY,
	SRC_TOGGLE,
	SRC_SHUTTER,
	SRC_RECORDING,
	SRC_DISPFRAME,
	SRC_STARTREC,
	SRC_ENDREC,
	SRC_NEXTSEG,
	SRC_TIMINGIO,
	SRC_ALWAYSHIGH
};

enum {
	EXP_NORMAL = 0,
	EXP_FRAMETRIGGER,
	EXP_SHUTTERGATING,
	EXP_HDR2SLOPE,
	EXP_HDR3SLOPE
};

class ioElement
{
public:
	bool debounce;
	bool invert;
	unsigned int source;

};

extern ioElement io1;
extern ioElement io2;
extern ioElement combOr1;
extern ioElement combOr2;
extern ioElement combOr3;
extern ioElement combXor;
extern ioElement combAnd;
extern ioElement start;
extern ioElement stop;
extern ioElement shutter;
extern ioElement gate;


void translateToComb(void);
