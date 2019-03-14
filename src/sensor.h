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
#ifndef IMAGE_SENSOR_H
#define IMAGE_SENSOR_H
#include "frameGeometry.h"
#include "errorCodes.h"
#include "gpmc.h"
#include <string>

class ImageSensor
{
public:
	virtual CameraErrortype init(GPMC * gpmc_inst) = 0;
	virtual CameraErrortype initSensor() = 0;

	/* Frame Geometry Functions. */
	virtual void setResolution(FrameGeometry *frameSize) = 0;
	virtual bool isValidResolution(FrameGeometry *frameSize) = 0;
	virtual FrameGeometry getMaxGeometry(void) = 0;
	virtual UInt8 getFilterColor(UInt32 h, UInt32 v) = 0;
	/* These APIs seem a bit sloppy. */
	virtual UInt32 getHResIncrement() = 0;
	virtual UInt32 getVResIncrement() = 0;
	virtual UInt32 getMinHRes() = 0;
	virtual UInt32 getMinVRes() = 0;

	/* Frame Timing Functions. */
	/* These are in need of some deduplication. */
	virtual Int32 seqOnOff(bool on) = 0;
	virtual UInt32 getFramePeriodClock(void) = 0;
	virtual UInt32 getMinFramePeriod(FrameGeometry *frameSize) = 0;
	virtual UInt32 getActualFramePeriod(double target, FrameGeometry *frameSize) = 0;
	virtual UInt32 setFramePeriod(UInt32 period, FrameGeometry *frameSize) = 0;

	/* Frame Exposure Functions. */
	virtual UInt32 getIntegrationClock(void) = 0;
	virtual UInt32 getMaxIntegrationTime(UInt32 period, FrameGeometry *frameSize) = 0;

	virtual double getMaxCurrentIntegrationTime(void) = 0;
	virtual double setIntegrationTime(double intTime, FrameGeometry *frameSize) = 0;
	virtual double getIntegrationTime(void) = 0;
	virtual double getActualIntegrationTime(double intTime, UInt32 period, FrameGeometry *frameSize) = 0;

	/* Analog Calibration APIs. */
	/* TODO: The analog calibration algorithm ought to be private to the
	 * image sensor, so these really should be replaced with a single
	 * function that does all the analog cal at once.
	 */
	virtual unsigned int enableAnalogTestMode(void) { return 0; } /* Return number of voltage steps, or 0 if not supported. */
	virtual void disableAnalogTestMode(void) {}
	virtual void setAnalogTestVoltage(unsigned int) {}

	/* The junk APIs that need to go away. */
	virtual UInt32 getMaxExposure(UInt32 period) = 0;
	virtual double getCurrentFramePeriodDouble(void) = 0;
	virtual double getCurrentExposureDouble(void) = 0;

	virtual void setSlaveExposure(UInt32 exposure) = 0;
	virtual void setClkPhase(UInt8 phase) = 0;
	virtual void SCIWrite(UInt8 address, UInt16 data) = 0;

	virtual void setADCOffset(UInt8 channel, Int16 offset) = 0;
	virtual std::string getFilename(const char * filename, const char * extension) = 0;
	virtual Int32 setGain(UInt32 gainSetting) = 0;
};

#endif // IMAGE_SENSOR_H
