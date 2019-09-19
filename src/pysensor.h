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
#include "errorCodes.h"
#include "types.h"
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

//class PySensor : public ImageSensor
class PySensor : QObject
{
	Q_OBJECT

public:
	PySensor(Control *control);
	~PySensor();
	CameraErrortype init(void);
	CameraErrortype setControl(Control *control_inst);

	Control *cinst;
	/* Frame Geometry Functions. */
	void setResolution(FrameGeometry *frameSize);
	FrameGeometry getMaxGeometry(void);
	UInt32 getHResIncrement() { return PYSENSOR_HRES_INCREMENT; }
	UInt32 getVResIncrement()  { return PYSENSOR_VRES_INCREMENT; }
	UInt32 getMinHRes() { return PYSENSOR_MIN_HRES; }
	UInt32 getMinVRes() { return PYSENSOR_MIN_VRES; }

	/* Frame Timing Functions. */
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

	UInt32 currentPeriod;		//moved to public for PyChronos
	UInt32 currentExposure;		//moved to public for PyChronos
	void setCurrentPeriod(UInt32 period);
	void setCurrentExposure(UInt32 period);

	double getCurrentFramePeriodDouble(void);
	double getCurrentExposureDouble();
	double getCurrentExposureAngle();

	bool isValidResolution(FrameGeometry *frameSize);
	UInt32 getActualIntegrationTime(double target, UInt32 period, FrameGeometry *frameSize);

private:
	FrameGeometry currentRes;
};

#endif // PYSENSOR_H

