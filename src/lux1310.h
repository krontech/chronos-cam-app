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
#ifndef LUX1310_H
#define LUX1310_H
#include "frameGeometry.h"
#include "sensor.h"
#include "errorCodes.h"
#include "types.h"
#include "spi.h"
#include "gpmc.h"
#include <string>

#define LUX1310_HRES_INCREMENT 16
#define LUX1310_VRES_INCREMENT	2
#define LUX1310_MAX_H_RES		1280
#define LUX1310_MAX_V_RES		1024
#define LUX1310_MIN_HRES		192		//Limited by video encoder minimum
#define LUX1310_MIN_VRES		96
#define LUX1310_MAGIC_ABN_DELAY	26
#define LUX1310_MAX_V_DARK		8
#define LUX1310_BITS_PER_PIXEL	12

#define LUX1310_TIMING_CLOCK	100000000.0	//Hz
#define LUX1310_SENSOR_CLOCK	90000000.0	//Hz

#define LUX1310_MAX_SLAVE_PERIOD ((double)0xFFFFFFFF)

#define LUX1310_GAIN_CORRECTION_MIN 0.999
#define LUX1310_GAIN_CORRECTION_MAX 1.2

enum {
	VDR3_VOLTAGE = 0,
	VABL_VOLTAGE,
	VDR1_VOLTAGE,
	VDR2_VOLTAGE,
	VRSTB_VOLTAGE,
	VRSTH_VOLTAGE,
	VRSTL_VOLTAGE,
	VRST_VOLTAGE
};

#define LUX1310_DAC_FS		4095.0
#define LUX1310_DAC_VREF	3.3
#define VDR3_SCALE			(LUX1310_DAC_FS / LUX1310_DAC_VREF)
#define VABL_SCALE			(LUX1310_DAC_FS / LUX1310_DAC_VREF * (10.0 + 23.2) / 10.0)
#define VDR1_SCALE			(LUX1310_DAC_FS / LUX1310_DAC_VREF)
#define VDR2_SCALE			(LUX1310_DAC_FS / LUX1310_DAC_VREF)
#define VRSTB_SCALE			(LUX1310_DAC_FS / LUX1310_DAC_VREF)
#define VRSTH_SCALE			(LUX1310_DAC_FS / LUX1310_DAC_VREF * 49.9 / (49.9 + 10.0))
#define VRSTL_SCALE			(LUX1310_DAC_FS / LUX1310_DAC_VREF * (10.0 + 23.2) / 10.0)
#define VRST_SCALE			(LUX1310_DAC_FS / LUX1310_DAC_VREF * 49.9 / (49.9 + 10.0))

#define LUX1310_GAIN_1				1
#define LUX1310_GAIN_2				2
#define LUX1310_GAIN_4				4
#define LUX1310_GAIN_8				8
#define LUX1310_GAIN_16				16

#define LUX1310_CHIP_ID				0xDA
#define LUX1310_VERSION_1			1
#define LUX1310_VERSION_2			2

#define LUX1310_CLOCK_PERIOD	(1.0/90000000.0)

#define FILTER_COLOR_RED	0
#define FILTER_COLOR_GREEN	1
#define FILTER_COLOR_BLUE	2

/* LUX 1310 Sensor Wavetables */
typedef struct {
	unsigned int clocks;
	unsigned int length;
	unsigned int abnDelay;
	const UInt8 *wavetab;
	const UInt8 *gaincal;
} lux1310wavetab_t;

/* Array of wavetables, sorted longest first, and terminated with a NULL */
extern const lux1310wavetab_t *lux1310wt[];

class LUX1310 : public ImageSensor
{
public:
	LUX1310();
	~LUX1310();
	CameraErrortype init(GPMC * gpmc_inst);

	/* Frame Geometry Functions. */
	void setResolution(FrameGeometry *frameSize);
	FrameGeometry getMaxGeometry(void);
	UInt8 getFilterColor(UInt32 h, UInt32 v);
	UInt32 getHResIncrement() { return LUX1310_HRES_INCREMENT; }
	UInt32 getVResIncrement()  { return LUX1310_VRES_INCREMENT; }
	UInt32 getMinHRes() { return LUX1310_MIN_HRES; }
	UInt32 getMinVRes() { return LUX1310_MIN_VRES; }

	/* Frame Timing Functions. */
	Int32 seqOnOff(bool on);
	UInt32 getFramePeriodClock(void) { return LUX1310_TIMING_CLOCK; }
	UInt32 getMinFramePeriod(FrameGeometry *frameSize);
	UInt32 getActualFramePeriod(double target, FrameGeometry *frameSize);
	UInt32 getFramePeriod(void);
	UInt32 setFramePeriod(UInt32 period, FrameGeometry *frameSize);

	/* Exposure Timing Functions */
	UInt32 getIntegrationClock(void) { return LUX1310_TIMING_CLOCK; }
	UInt32 getMaxIntegrationTime(UInt32 period, FrameGeometry *frameSize);
	UInt32 getMinIntegrationTime(UInt32 period, FrameGeometry *frameSize) { return LUX1310_TIMING_CLOCK / 1000000; }  /* 1us */
	UInt32 getIntegrationTime(void);
	UInt32 setIntegrationTime(UInt32 intTime, FrameGeometry *frameSize);

	/* Analog calibration APIs. */
	unsigned int enableAnalogTestMode(void);
	void disableAnalogTestMode(void);
	void setAnalogTestVoltage(unsigned int);
	void adcOffsetTraining(FrameGeometry *frameSize, UInt32 address, UInt32 numFrames);
	Int32 loadADCOffsetsFromFile(FrameGeometry *frameSize);
	std::string getFilename(const char * filename, const char * extension);

	UInt32 getMinGain() { return 1; }
	UInt32 getMaxGain() { return 16; }
	Int32 setGain(UInt32 gain);

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
	void setADCOffset(UInt8 channel, Int16 offset);
	void offsetCorrectionIteration(FrameGeometry *geometry, int *offsets, UInt32 address, UInt32 framesToAverage);

	FrameGeometry currentRes;
	UInt32 currentPeriod;
	UInt32 currentExposure;
	Int32 dacCSFD;
	UInt32 wavetableSize;
	UInt32 gain;
	UInt32 startDelaySensorClocks;
	UInt32 sensorVersion;

	SPI * spi;
	GPMC * gpmc;
};

#endif // LUX1310_H
