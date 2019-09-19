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
#include "math.h"
#include <QDebug>
#include <fcntl.h>
#include <unistd.h>
#include <QResource>
#include <QDir>
#include <QIODevice>

#include "util.h"

#include "defines.h"
#include "cameraRegisters.h"

#include "types.h"
#include "pysensor.h"

#include <QSettings>

#define round(x) (floor(x + 0.5))

PySensor::PySensor(Control *control)
{
	cinst = control;
}

PySensor::~PySensor()
{
}

CameraErrortype PySensor::init(void)
{
	return SUCCESS;
}

CameraErrortype PySensor::setControl(Control *control_inst)
{
	cinst = control_inst;
}

FrameGeometry PySensor::getMaxGeometry(void)
{
	FrameGeometry size = {
		.hRes = PYSENSOR_MAX_H_RES,
		.vRes = PYSENSOR_MAX_V_RES,
		.hOffset = 0,
		.vOffset = 0,
		.vDarkRows = PYSENSOR_MAX_V_DARK,
		.bitDepth = PYSENSOR_BITS_PER_PIXEL,
		.minFrameTime = PYSENSOR_MINFRAMETIME
	};
	return size;
}

void PySensor::setResolution(FrameGeometry *size)
{
	cinst->setResolution(size);
}

UInt32 PySensor::getMinFramePeriod(FrameGeometry *frameSize)
{
	FrameTiming timing;

	cinst->getTiming(frameSize, &timing);
	return timing.minFramePeriod;		//now in nanoseconds, as wanted by camApp
}

UInt32 PySensor::getFramePeriod(void)
{
	return currentPeriod;
}

double PySensor::getCurrentExposureDouble(void)
{
	return currentExposure;
}

double PySensor::getCurrentExposureAngle(void)
{
	return 360.0 * currentExposure / currentPeriod;
}


UInt32 PySensor::getActualFramePeriod(double target, FrameGeometry *size)
{
	UInt32 clocks = round(target * PYSENSOR_TIMING_CLOCK);
	UInt32 minPeriod = getMinFramePeriod(size);
	UInt32 maxPeriod = PYSENSOR_MAX_SLAVE_PERIOD;

	return within(clocks, minPeriod, maxPeriod);
}

extern UInt32 pyCurrentPeriod;

UInt32 PySensor::setFramePeriod(UInt32 period, FrameGeometry *size)
{
	qDebug() << "Requested period" << period;
	UInt32 minPeriod = getMinFramePeriod(size);
	UInt32 maxPeriod = PYSENSOR_MAX_SLAVE_PERIOD;

	currentPeriod = within(period, minPeriod, maxPeriod);
	//pyCurrentPeriod = currentPeriod;

	cinst->setInt("framePeriod", currentPeriod);

	return currentPeriod;
}

/* getMaxIntegrationTime
 *
 * Gets the actual maximum integration time the sensor can be set to for the given settings
 *
 * intTime:	Desired integration time in seconds
 *
 * returns: Maximum integration time
 */
UInt32 PySensor::getMaxIntegrationTime(UInt32 period, FrameGeometry *size)
{
	return period - 500;
}

/* setIntegrationTime
 *
 * Sets the integration time of the image sensor to a value as close as possible to requested
 *
 * intTime:	Desired integration time in clocks
 *
 * returns: Actual integration time that was set
 */
UInt32 PySensor::setIntegrationTime(UInt32 intTime, FrameGeometry *size)
{
	//Set integration time to within limits
	UInt32 maxIntTime = getMaxIntegrationTime(currentPeriod, size);
	UInt32 minIntTime = getMinIntegrationTime(currentPeriod, size);
	currentExposure = within(intTime, minIntTime, maxIntTime);

	cinst->setInt("exposurePeriod", currentExposure);

	return currentExposure;
}

/* getIntegrationTime
 *
 * Gets the integration time of the image sensor
 *
 * returns: Integration time
 */
UInt32 PySensor::getIntegrationTime(void)
{
	UInt32 exp;
	//cinst->getInt("exposurePeriod", &exp);
	//return exp;
	return currentExposure;
}

void PySensor::setCurrentPeriod(UInt32 period)
{
	currentPeriod = period;
}

void PySensor::setCurrentExposure(UInt32 period)
{
	currentExposure = period;
}



