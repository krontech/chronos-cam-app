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
	void resetToDefaults();
	bool readIO(UInt32 io);
	void writeIO(UInt32 io, UInt32 val);

	void setThreshold(UInt32 io, double thresholdVolts);
	double getThreshold(UInt32 io);
	void setTriggerEnable(UInt32 source);
	void setTriggerInvert(UInt32 invert);
	void setTriggerDebounceEn(UInt32 dbEn);
	void setTriggerDelayFrames(UInt32 delayFrames);
	UInt32 getTriggerEnable();
	UInt32 getTriggerInvert();
	UInt32 getTriggerDebounceEn();
	UInt32 getTriggerDelayFrames();
	void setOutLevel(UInt32 level);
	void setOutSource(UInt32 source);
	void setOutInvert(UInt32 invert);
	UInt32 getOutLevel();
	UInt32 getOutSource();
	UInt32 getOutInvert();
	UInt32 getIn();

	bool getTriggeredExpEnable();
	UInt32 getExtShutterSrcEn();
	bool getShutterGatingEnable();
	void setTriggeredExpEnable(bool en);
	void setExtShutterSrcEn(UInt32 extShutterSrcEn);
	void setShutterGatingEnable(bool en);

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



