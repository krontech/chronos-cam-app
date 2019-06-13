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

#include "spi.h"
#include "util.h"

#include "defines.h"
#include "cameraRegisters.h"

#include "types.h"
#include "pysensor.h"

#include <QSettings>

/* Some Timing Constants */
//#define PYSENSOR_SOF_DELAY	0x0f	/* Delay from TXN rising edge to start of frame. */
//#define PYSENSOR_LV_DELAY	0x07	/* Linevalid delay to match ADC latency. */
//#define PYSENSOR_MIN_HBLANK	0x02	/* Minimum horiontal blanking period. */

#define round(x) (floor(x + 0.5))

UInt32 recShadowExposure;


PySensor::PySensor(Control *control)
{
	//spi = new SPI();
	//startDelaySensorClocks = PYSENSOR_MAGIC_ABN_DELAY;
	qDebug() << "PySensor";
	cinst = control;

//	if (!QObject::connect(cinst, SIGNAL(apiSetFramePeriod(UInt32 period)), this, SLOT(apiDoSetFramePeriod(UInt32 period))))
	{
		qDebug() << "Connect failed";
	}

}

void PySensor::slotConnect(void)
{
	//if (!QObject::connect(cinst, SIGNAL(apiSetFramePeriod(UInt32 period)), this, SLOT(apiDoSetFramePeriod(UInt32 period))))
	{
		qDebug() << "Connect failed";
	}

}

PySensor::~PySensor()
{
	//delete spi;
}

CameraErrortype PySensor::init(GPMC * gpmc_inst)
{

	return SUCCESS;
}

CameraErrortype PySensor::setControl(Control *control_inst)
{
	cinst = control_inst;
}


CameraErrortype PySensor::initSensor()
{
	//pych softReset()

	return SUCCESS;
}

bool PySensor::SCIWrite(UInt8 address, UInt16 data, bool readback)
{
	return true;
}

void PySensor::SCIWriteBuf(UInt8 address, const UInt8 * data, UInt32 dataLen)
{
}

UInt16 PySensor::SCIRead(UInt8 address)
{
	return 0;
}

CameraErrortype PySensor::autoPhaseCal(void)
{
	//pych phase cal
	return SUCCESS;
}

Int32 PySensor::seqOnOff(bool on)
{
	//pych do we need anything here?
	return SUCCESS;
}

void PySensor::setReset(bool reset)
{
}

void PySensor::setClkPhase(UInt8 phase)
{
}

UInt8 PySensor::getClkPhase(void)
{
		return 0;
}

UInt32 PySensor::getDataCorrect(void)
{
	return 0;
}

void PySensor::setSyncToken(UInt16 token)
{

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
	//pych setResolution
	cinst->setResolution(size);
}

UInt32 PySensor::getMinWavetablePeriod(FrameGeometry *frameSize, UInt32 wtSize)
{
	//pych stuff
	return 1000/0;
}

UInt32 PySensor::getMinFramePeriod(FrameGeometry *frameSize)
{
	FrameTiming timing;

	cinst->getTiming(frameSize, &timing);
	//return timing.minFramePeriod / 10;		//in tens of nanoseconds, as wanted by camApp!
	return timing.minFramePeriod;		//now in nanoseconds, as wanted by camApp!
}

UInt32 PySensor::getFramePeriod(void)
{
	return currentPeriod;
	//return pyCurrentPeriod;

}

/*
UInt32 PySensor::getExposure(void)
{
	return currentExposure;
}
*/


/*
double PySensor::getCurrentFramePeriodDouble(void)
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
*/

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
	//pych
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
	//pych?
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

	setSlaveExposure(currentExposure);

	return currentExposure;
}

/* getIntegrationTime
 *
 * Gets the integration time of the image sensor
 *
 * returns: Integration tim
 */
UInt32 PySensor::getIntegrationTime(void)
{
	UInt32 exp;
	cinst->getInt("exposurePeriod", &exp);
	return exp;
}

void PySensor::setSlaveExposure(UInt32 exposure)
{
	//pych
	recShadowExposure = exposure;
	cinst->setIntegrationTime(exposure);
}

//Set up DAC
void PySensor::initDAC()
{
}

//Write the data into the selected channel
void PySensor::writeDAC(UInt16 data, UInt8 channel)
{
}

void PySensor::writeDACVoltage(UInt8 channel, float voltage)
{
}

//Performs an SPI write to the DAC
int PySensor::writeDACSPI(UInt16 data)
{
	return 0;
}

void PySensor::setDACCS(bool on)
{
}

void PySensor::updateWavetableSetting(bool gainCalMode)
{
}

unsigned int PySensor::enableAnalogTestMode(void)
{
}

void PySensor::disableAnalogTestMode(void)
{
}

void PySensor::setAnalogTestVoltage(unsigned int voltage)
{
}

//Sets ADC offset for one channel
//Converts the input 2s complement value to the sensors's weird sign bit plus value format (sort of like float, with +0 and -0)
void PySensor::setADCOffset(UInt8 channel, Int16 offset)
{
}

//Generate a filename string used for calibration values that is specific to the current gain and wavetable settings
std::string PySensor::getFilename(const char * filename, const char * extension)
{
	const char *gName = "";
	char wtName[16];

	switch(gain)
	{
		case PYSENSOR_GAIN_1:		gName = PYSENSOR_GAIN_1_FN;		break;
		case PYSENSOR_GAIN_2:		gName = PYSENSOR_GAIN_2_FN;		break;
		case PYSENSOR_GAIN_4:		gName = PYSENSOR_GAIN_4_FN;		break;
		case PYSENSOR_GAIN_8:		gName = PYSENSOR_GAIN_8_FN;		break;
		case PYSENSOR_GAIN_16:		gName = PYSENSOR_GAIN_16_FN;		break;
		default:					gName = "";						break;
	}

	snprintf(wtName, sizeof(wtName), "WT%d", wavetableSize);
	return std::string(filename) + "_" + gName + "_" + wtName + extension;
}


Int32 PySensor::setGain(UInt32 gainSetting)
{
	//do pych things when this is implemented
}


// GR
// BG
//

UInt8 PySensor::getFilterColor(UInt32 h, UInt32 v)
{
	return 0;
}


/*
void PySensor::apiDoSetFramePeriod(UInt32 period)
{
	currentPeriod = period;
}
*/

void PySensor::apiDoSetFramePeriod2(UInt32 period)
{
	currentPeriod = period;
	qDebug() << "apiDoSetFramePeriod2";
}

void PySensor::apiDoSetFramePeriod3(UInt32 period)
{
	currentPeriod = period;
	qDebug() << "apiDoSetFramePeriod3";
}

void PySensor::apiDoSetExposurePeriod(UInt32 period)
{
	//currentPeriod = period;
	qDebug() << "apiDoSetShutterAngle";
}

void PySensor::apiDoSetCurrentIso(UInt32 iso )
{
	qDebug() << "apiDoSetCurrentIso";
}

void PySensor::apiDoSetCurrentGain(UInt32 )
{
	qDebug() << "apiDoSetCurrentGain";
}

void PySensor::apiDoSetPlaybackPosition(UInt32 frame)
{
	qDebug() << "apiDoSetPlaybackPosition";
}

void PySensor::apiDoSetPlaybackStart(UInt32 frame)
{
	qDebug() << "apiDoSetPlaybackStart";
}

void PySensor::apiDoSetPlaybackLength(UInt32 frames)
{
	qDebug() << "apiDoSetPlaybackLength";
}

void PySensor::apiDoSetWbTemperature(UInt32 temp)
{
	qDebug() << "apiDoSetWbTemperature";
}

void PySensor::apiDoSetRecMaxFrames(UInt32 frames)
{
	qDebug() << "apiDoSetRecMaxFrames";
}

void PySensor::apiDoSetRecSegments(UInt32 seg)
{
	qDebug() << "apiDoSetRecSegments";
}

void PySensor::apiDoSetRecPreBurst(UInt32 frames)
{
	qDebug() << "apiDoSetRecPreBurst";
}

void PySensor::apiDoSetExposurePercent(double percent)
{
	qDebug() << "apiDoSetExposurePercent" << percent;
}

void PySensor::apiDoSetExposureNormalized(double norm)
{
	qDebug() << "apiDoSetExposureNormalized";
}

void PySensor::apiDoSetIoDelayTime(double delay)
{
	qDebug() << "apiDoSetIoDelayTime" << delay;
}

void PySensor::apiDoSetFrameRate(double rate)
{
	qDebug() << "apiDoSet";
}

void PySensor::apiDoSetShutterAngle(double angle)
{
	//currentPeriod = period;
	qDebug() << "apiDoSetShutterAngle";
}

void PySensor::apiDoSetExposureMode(QString mode)
{
	qDebug() << "apiDoSetExposureMode";
}

void PySensor::apiDoSetCameraTallyMode(QString mode)
{
	qDebug() << "apiDoSetCameraTallyMode";
}

void PySensor::apiDoSetCameraDescription(QString desc)
{
	qDebug() << "apiDoSetCameraDescription";
}

void PySensor::apiDoSetNetworkHostname(QString )
{
	qDebug() << "apiDoSetNetworkHostname";
}







/*
void PySensor::apiDoSetInt(QString param, UInt32 value)
{
	currentPeriod = value;
}
*/
