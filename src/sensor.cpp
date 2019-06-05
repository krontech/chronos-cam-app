
/****************************************************************************
 *  Copyright (C) 2013-2019 Kron Technologies Inc <http://www.krontech.ca>. *
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
#include "frameGeometry.h"
#include "sensor.h"

extern bool pych;
extern UInt32 pyCurrentPeriod;
extern UInt32 pyCurrentExposure;


/* Default implementations and helper functions for the ImageSensor class. */

/* Frame size validation function. */
bool ImageSensor::isValidResolution(FrameGeometry *frameSize)
{
	FrameGeometry maxFrame = getMaxGeometry();

	/* Enforce resolution limits. */
	if ((frameSize->hRes < getMinHRes()) || (frameSize->hRes + frameSize->hOffset > maxFrame.hRes)) {
		return false;
	}
	if ((frameSize->vRes < getMinVRes()) || (frameSize->vRes + frameSize->vOffset > maxFrame.vRes)) {
		return false;
	}
	if (frameSize->vDarkRows > maxFrame.vDarkRows) {
		return false;
	}
	if (frameSize->bitDepth != maxFrame.bitDepth) {
		return false;
	}

	/* Enforce minimum pixel increments. */
	if ((frameSize->hRes % getHResIncrement()) || (frameSize->hOffset % getHResIncrement())) {
		return false;
	}
	if ((frameSize->vRes % getVResIncrement()) || (frameSize->vOffset % getVResIncrement())) {
		return false;
	}
	if (frameSize->vDarkRows % getVResIncrement()) {
		return false;
	}

	/* Otherwise, the resultion and offset are valid. */
	return true;
}

/*
 * Default implementation: quanitize to the timing clock and then limit to
 * the range given by getMinIntegrationTime() and getMaxIntegrationTime()
 */
UInt32 ImageSensor::getActualIntegrationTime(double target, UInt32 period, FrameGeometry *frameSize)
{
	UInt32 intTime = target * getIntegrationClock() + 0.5;
	UInt32 minIntTime = getMinIntegrationTime(period, frameSize);
	UInt32 maxIntTime = getMaxIntegrationTime(period, frameSize);

	return within(intTime, minIntTime, maxIntTime);
}


double ImageSensor::getCurrentFramePeriodDouble()
{
	if (pych)
	{
		//return this->getPyFramePeriod()
		//return currentPeriod;
		return pyCurrentPeriod / 1000000000.0;
	}
	else
	{
		return (double)getFramePeriod() / getFramePeriodClock();
	}
}

double ImageSensor::getCurrentExposureDouble()
{
	if (pych & 0)
	{
		return pyCurrentExposure;
	}
	else
	{
		return (double)getIntegrationTime() / getIntegrationClock();
	}
}

double ImageSensor::getCurrentExposureAngle()
{
	if (pych)
	{

		return 360.0 * pyCurrentExposure / pyCurrentPeriod;
	}
	else
	{
		return getCurrentExposureDouble() * 360.0 / getCurrentFramePeriodDouble();
	}
}


