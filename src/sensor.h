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

/* Bitmask of quirky sensor behavior for the UI to deal with. */
#define SENSOR_QUIRK_UPSIDE_DOWN	0x00000001

class ImageSensor
{
public:
	virtual CameraErrortype init(GPMC * gpmc_inst) = 0;

	/* Report strange sensor issues. */
	virtual UInt32 getSensorQuirks() { return 0; }

	/* Frame Geometry Functions. */
	virtual void setResolution(FrameGeometry *frameSize) = 0;
	virtual FrameGeometry getMaxGeometry(void) = 0;
	virtual bool isValidResolution(FrameGeometry *frameSize);
	virtual UInt8 getFilterColor(UInt32 h, UInt32 v) = 0;
	/* These APIs seem a bit sloppy. */
	virtual UInt32 getHResIncrement() = 0;
	virtual UInt32 getVResIncrement() = 0;
	virtual UInt32 getMinHRes() = 0;
	virtual UInt32 getMinVRes() = 0;

	/* Frame Timing Functions. */
	virtual Int32 seqOnOff(bool on) = 0;
	virtual UInt32 getFramePeriodClock(void) = 0;
	virtual UInt32 getMinFramePeriod(FrameGeometry *frameSize) = 0;
	virtual UInt32 getActualFramePeriod(double target, FrameGeometry *frameSize) = 0;
	virtual UInt32 getFramePeriod(void) = 0;
	virtual UInt32 setFramePeriod(UInt32 period, FrameGeometry *frameSize) = 0;

	/* Frame Exposure Functions. */
	virtual UInt32 getIntegrationClock(void) = 0;
	virtual UInt32 getMaxIntegrationTime(UInt32 period, FrameGeometry *frameSize) = 0;
	virtual UInt32 getMinIntegrationTime(UInt32 period, FrameGeometry *frameSize) = 0;
	virtual UInt32 getActualIntegrationTime(double target, UInt32 period, FrameGeometry *frameSize);
	virtual UInt32 getIntegrationTime(void) = 0;
	virtual UInt32 setIntegrationTime(UInt32 intTime, FrameGeometry *frameSize) = 0;

	/* Analog Gain Functions. */
	/*
	 * Min and max gain are reported as the linear values (eg: 1=0dB, 2=6dB and so on).
	 * All sensors are expected to support a gain of 1 (eg: normal operation).
	 * For now, the range of gains are only accessed in powers-of-two steps.
	 */
	virtual UInt32 getMinGain() { return 1; }
	virtual UInt32 getMaxGain() { return 1; }
	virtual Int32 setGain(UInt32 gain) { return (gain == 1) ? SUCCESS : CAMERA_INVALID_SETTINGS; }

	/* Analog Calibration APIs. */
	/* TODO: The analog calibration algorithm ought to be private to the
	 * image sensor, so these really should be replaced with a single
	 * function that does all the analog cal at once.
	 */
	virtual unsigned int enableAnalogTestMode(void) { return 0; } /* Return number of voltage steps, or 0 if not supported. */
	virtual void disableAnalogTestMode(void) {}
	virtual void setAnalogTestVoltage(unsigned int) {}
	virtual void setADCOffset(UInt8 channel, Int16 offset) = 0;
	virtual std::string getFilename(const char * filename, const char * extension) = 0;

	/* Helper Functions */
	inline double getCurrentFramePeriodDouble() { return (double)getFramePeriod() / getFramePeriodClock(); }
	inline double getCurrentExposureDouble() { return (double)getIntegrationTime() / getIntegrationClock(); }
	inline double getCurrentExposureAngle() { return getCurrentExposureDouble() * 360.0 / getCurrentFramePeriodDouble(); }
};

#endif // IMAGE_SENSOR_H
