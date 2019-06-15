
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
#include <QObject>

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
	return (double)getFramePeriod() / getFramePeriodClock();
}

double ImageSensor::getCurrentExposureDouble()
{
	return (double)getIntegrationTime() / getIntegrationClock();
}

double ImageSensor::getCurrentExposureAngle()
{
	return getCurrentExposureDouble() * 360.0 / getCurrentFramePeriodDouble();
}
/*
void ImageSensor::apiDoSetInt(QString param, UInt32 value) {}
void ImageSensor::apiDoSetFramePeriod2(UInt32 period) {}
void ImageSensor::apiDoSetFramePeriod3(UInt32 period) {}
void ImageSensor::apiDoSetCurrentIso(UInt32 iso) {}
void ImageSensor::apiDoSetCurrentGain(UInt32 gain) {}
void ImageSensor::apiDoSetPlaybackPosition(UInt32 frame) {}
void ImageSensor::apiDoSetPlaybackStart(UInt32 frame) {}
void ImageSensor::apiDoSetPlaybackLength(UInt32 frames) {}
void ImageSensor::apiDoSetWbTemperature(UInt32 temp) {}
void ImageSensor::apiDoSetRecMaxFrames(UInt32 frames) {}
void ImageSensor::apiDoSetRecSegments(UInt32 seg) {}
void ImageSensor::apiDoSetRecPreBurst(UInt32 frames) {}
void ImageSensor::apiDoSetExposurePeriod(UInt32 period) {}

void ImageSensor::apiDoSetExposurePercent(double percent) {}
void ImageSensor::apiDoSetExposureNormalized(double norm) {}
void ImageSensor::apiDoSetIoDelayTime(double delay) {}
void ImageSensor::apiDoSetFrameRate(double rate) {}
void ImageSensor::apiDoSetShutterAngle(double angle) {}

void ImageSensor::apiDoSetExposureMode(QString mode) {}
void ImageSensor::apiDoSetCameraTallyMode(QString mode) {}
void ImageSensor::apiDoSetCameraDescription(QString desc) {}
void ImageSensor::apiDoSetNetworkHostname(QString name) {}

void ImageSensor::apiDoSetWbMatrix(QVariant wb) {}
void ImageSensor::apiDoSetResolution(QVariant wb) {}
*/
