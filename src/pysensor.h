#ifndef PYSENSOR_H
#define PYSENSOR_H

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

#include "frameGeometry.h"
#include "control.h"
#include "sensor.h"
#include "errorCodes.h"
#include "types.h"
#include "spi.h"
#include "gpmc.h"
#include <string>
#include <QtDBus/QtDBus>


extern UInt32 sensorHIncrement;
extern UInt32 sensorVIncrement;
extern UInt32 sensorHMax;
extern UInt32 sensorVMax;
extern UInt32 sensorHMin;
extern UInt32 sensorVMin;
extern UInt32 sensorVDark;
extern UInt32 sensorBitDepth;
extern double sensorMinFrameTime;



#define PYSENSOR_HRES_INCREMENT		sensorHIncrement
#define PYSENSOR_VRES_INCREMENT		sensorVIncrement
#define PYSENSOR_MAX_H_RES			sensorHMax
#define PYSENSOR_MAX_V_RES			sensorVMax
#define PYSENSOR_MIN_HRES			sensorHMin
#define PYSENSOR_MIN_VRES			sensorVMin
#define PYSENSOR_MAGIC_ABN_DELAY	(1/0)			//26
#define PYSENSOR_MAX_V_DARK			sensorVDark
#define PYSENSOR_BITS_PER_PIXEL		sensorBitDepth
#define PYSENSOR_MINFRAMETIME		sensorMinFrameTime

#define PYSENSOR_TIMING_CLOCK		1000000000.0	//this clock is set to 1 ns for math purposes
//#define PYSENSOR_SENSOR_CLOCK		90000000.0	//pych - do properly

//#define PYSENSOR_ROT			(9+4)		//Granularity clock cycles (63MHz periods by default)
//#define PYSENSOR_FOT			315		//Granularity clock cycles (63MHz periods by default)

#define ROT_TIMER				9
#define FOT_TIMER				315

#define _ROT						(ROT_TIMER + 4)
#define _FOT						(FOT_TIMER + 29)
#define MAX_TS					54

#define PYSENSOR_MAX_SLAVE_PERIOD ((double)0xFFFFFFFF)

#define PYSENSOR_GAIN_CORRECTION_MIN 0.999
#define PYSENSOR_GAIN_CORRECTION_MAX 1.2


#define PYSENSOR_GAIN_1				0
#define PYSENSOR_GAIN_2				1
#define PYSENSOR_GAIN_4				2
#define PYSENSOR_GAIN_8				3
#define PYSENSOR_GAIN_16				4

//Strings to build FPN filenames
#define PYSENSOR_GAIN_1_FN			"G1"
#define PYSENSOR_GAIN_2_FN			"G2"
#define PYSENSOR_GAIN_4_FN			"G4"
#define PYSENSOR_GAIN_8_FN			"G8"
#define PYSENSOR_GAIN_16_FN			"G16"

#define PYSENSOR_CHIP_ID				0xDA
#define PYSENSOR_VERSION_1			1
#define PYSENSOR_VERSION_2			2

#define PYSENSOR_CLOCK_PERIOD	(1.0/90000000.0)

#define FILTER_COLOR_RED	0
#define FILTER_COLOR_GREEN	1
#define FILTER_COLOR_BLUE	2

/* Array of wavetables, sorted longest first, and terminated with a NULL */
//extern const lux1310wavetab_t *lux1310wt[];

extern UInt32 recShadowExposure;

class PySensor : public ImageSensor
{
public:
	PySensor(Control *control);
	~PySensor();
	CameraErrortype init(GPMC * gpmc_inst);
	CameraErrortype setControl(Control *control_inst);

	Control *cinst;
	/* Frame Geometry Functions. */
	void setResolution(FrameGeometry *frameSize);
	FrameGeometry getMaxGeometry(void);
	UInt8 getFilterColor(UInt32 h, UInt32 v);
	UInt32 getHResIncrement() { return PYSENSOR_HRES_INCREMENT; }
	UInt32 getVResIncrement()  { return PYSENSOR_VRES_INCREMENT; }
	UInt32 getMinHRes() { return PYSENSOR_MIN_HRES; }
	UInt32 getMinVRes() { return PYSENSOR_MIN_VRES; }

	/* Frame Timing Functions. */
	Int32 seqOnOff(bool on);
	UInt32 getFramePeriodClock(void) { return PYSENSOR_TIMING_CLOCK; }
	UInt32 getMinFramePeriod(FrameGeometry *frameSize);
	UInt32 getActualFramePeriod(double target, FrameGeometry *frameSize);
	UInt32 getFramePeriod(void);
	UInt32 setFramePeriod(UInt32 period, FrameGeometry *frameSize);

	/* Exposure Timing Functions */
	UInt32 getIntegrationClock(void) { return PYSENSOR_TIMING_CLOCK; }
	UInt32 getMaxIntegrationTime(UInt32 period, FrameGeometry *frameSize);
	UInt32 getMinIntegrationTime(UInt32 period, FrameGeometry *frameSize) { return PYSENSOR_TIMING_CLOCK / 1000000; }  /* 1us */
	UInt32 getIntegrationTime(void);
	UInt32 setIntegrationTime(UInt32 intTime, FrameGeometry *frameSize);

	/* Analog calibration APIs. */
	unsigned int enableAnalogTestMode(void);
	void disableAnalogTestMode(void);
	void setAnalogTestVoltage(unsigned int);
	void setADCOffset(UInt8 channel, Int16 offset);
	std::string getFilename(const char * filename, const char * extension);

	Int32 setGain(UInt32 gainSetting);
	UInt32 currentPeriod;		//moved to public for PyChronos
	UInt32 currentExposure;		//moved to public for PyChronos


	void slotConnect(void);

	double getCurrentFramePeriodDouble(void);
	double getCurrentExposureDouble();
	double getCurrentExposureAngle();




private:
	CameraErrortype initSensor();
	CameraErrortype autoPhaseCal(void);
	UInt32 getDataCorrect(void);
	void setSyncToken(UInt16 token);

	UInt32 getMinWavetablePeriod(FrameGeometry *frameSize, UInt32 wtSize);

	void setSlaveExposure(UInt32 exposure);
	void setReset(bool reset);
	void setClkPhase(UInt8 phase);
	UInt8 getClkPhase(void);
	void initDAC();
	void writeDAC(UInt16 data, UInt8 channel);
	void writeDACVoltage(UInt8 channel, float voltage);
	int writeDACSPI(UInt16 data);
	void setDACCS(bool on);
	bool SCIWrite(UInt8 address, UInt16 data, bool readback = false);
	void SCIWriteBuf(UInt8 address, const UInt8 * data, UInt32 dataLen);
	UInt16 SCIRead(UInt8 address);
	void updateWavetableSetting(bool gainCalMode);

	FrameGeometry currentRes;
	Int32 dacCSFD;
	UInt32 wavetableSize;
	UInt32 gain;
	UInt32 startDelaySensorClocks;
	UInt32 sensorVersion;

	SPI * spi;
	GPMC * gpmc;
protected slots:
	//void apiDoSetFramePeriod(UInt32 period);
	void apiDoSetFramePeriod2(UInt32 period);
	void apiDoSetFramePeriod3(UInt32 period);
	void apiDoSetCurrentIso(UInt32 iso);
	void apiDoSetCurrentGain(UInt32 gain );
	void apiDoSetPlaybackPosition(UInt32 frame);
	void apiDoSetPlaybackStart(UInt32 frame);
	void apiDoSetPlaybackLength(UInt32 frames);
	void apiDoSetWbTemperature(UInt32 temp);
	void apiDoSetRecMaxFrames(UInt32 frames);
	void apiDoSetRecSegments(UInt32 seg);
	void apiDoSetRecPreBurst(UInt32 frames);
	void apiDoSetExposurePeriod(UInt32 period);

	void apiDoSetExposurePercent(double percent);
	void apiDoSetExposureNormalized(double norm);
	void apiDoSetIoDelayTime(double delay);
	void apiDoSetFrameRate(double rate);
	void apiDoSetShutterAngle(double angle);

	void apiDoSetExposureMode(QString mode);
	void apiDoSetCameraTallyMode(QString mode);
	void apiDoSetCameraDescription(QString desc);
	void apiDoSetNetworkHostname(QString name);

	void apiDoSetWbMatrix(QVariant wb);

	//void apiDoSetInt(QString param, UInt32 value);

};

#endif // PYSENSOR_H

