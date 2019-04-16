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
#ifndef LUX2810_H
#define LUX2810_H
#include "frameGeometry.h"
#include "sensor.h"
#include "errorCodes.h"
#include "types.h"
#include "spi.h"
#include "gpmc.h"
#include <string>

#define LUX2810_HRES_INCREMENT 32
#define LUX2810_VRES_INCREMENT	2
#define LUX2810_MAX_H_RES		1920
#define LUX2810_MAX_V_RES		1080
#define LUX2810_MIN_HRES		192		//Limited by video encoder minimum
#define LUX2810_MIN_VRES		96
#define LUX2810_MAGIC_ABN_DELAY	26
#define LUX2810_MAX_V_DARK		40
#define LUX2810_BITS_PER_PIXEL	12

#define LUX2810_TIMING_CLOCK	100000000.0	//Hz
#define LUX2810_SENSOR_CLOCK	75000000.0	//Hz


#define LUX2810_MIN_INT_TIME	0.000001	//1us
#define LUX2810_MAX_SLAVE_PERIOD ((double)0xFFFFFFFF)

#define LUX2810_GAIN_CORRECTION_MIN 0.999
#define LUX2810_GAIN_CORRECTION_MAX 1.2

/* Addressable image sensor boundary regions */
#define LUX2810_BOUNDARY_ROWS	16
#define LUX2810_BOUNDARY_COLUMNS	32
#define LUX2810_LEFT_DARK_COLUMNS	64
#define LUX2810_HIGH_DARK_ROWS	40

enum {
    // First DAC
	LUX2810_VDR1_VOLTAGE = 0,
	LUX2810_VRST_VOLTAGE,
	LUX2810_VAB_VOLTAGE,
	LUX2810_VRDH_VOLTAGE,
	LUX2810_VDR2_VOLTAGE,
	LUX2810_VABL_VOLTAGE,
    //NC
    //NC
    //Second DAC
	LUX2810_VADC1_VOLTAGE = 8,
	LUX2810_VADC2_VOLTAGE,
	LUX2810_VADC3_VOLTAGE,
	LUX2810_VCM_VOLTAGE,
	LUX2810_VADC2_FT_VOLTAGE,
	LUX2810_VADC3_FT_VOLTAGE,
    //NC
    //NC
    //Third DAC
	LUX2810_VREFN_VOLTAGE = 16,
	LUX2810_VREFP_VOLTAGE,
	LUX2810_VTSTH_VOLTAGE,
	LUX2810_VTSTL_VOLTAGE,
	LUX2810_VRSTB_VOLTAGE,
	LUX2810_VRSTH_VOLTAGE,
	LUX2810_VRSTL_VOLTAGE,
    //NC
};

#define PARALLEL(x,y)   (1.0/(1.0/(x)+1.0/(y)))

#define LUX2810_DAC_FS		4095.0
#define LUX2810_DAC_VREF	3.3

#define LUX2810_VDR1_SCALE      (LUX2810_DAC_FS / LUX2810_DAC_VREF)
#define LUX2810_VRST_SCALE      (LUX2810_DAC_FS / LUX2810_DAC_VREF)
#define LUX2810_VAB_SCALE       (LUX2810_DAC_FS / LUX2810_DAC_VREF)
#define LUX2810_VRDH_SCALE      (LUX2810_DAC_FS / LUX2810_DAC_VREF)
#define LUX2810_VDR2_SCALE      (LUX2810_DAC_FS / LUX2810_DAC_VREF)
#define LUX2810_VABL_SCALE      (LUX2810_DAC_FS / LUX2810_DAC_VREF)
//NC
//NC
//Second DAC
#define LUX2810_VADC1_SCALE     (LUX2810_DAC_FS / LUX2810_DAC_VREF)
#define LUX2810_VADC2_SCALE     (LUX2810_DAC_FS / LUX2810_DAC_VREF * (10.0 + PARALLEL(2.0, 100.0)) / PARALLEL(2.0, 100.0))
#define LUX2810_VADC3_SCALE     (LUX2810_DAC_FS / LUX2810_DAC_VREF * (10.0 + PARALLEL(2.0, 100.0)) / PARALLEL(2.0, 100.0))
#define LUX2810_VCM_SCALE       (LUX2810_DAC_FS / LUX2810_DAC_VREF)
#define LUX2810_VADC2_FT_SCALE  (LUX2810_DAC_FS / LUX2810_DAC_VREF * (100.0 + PARALLEL(2.0, 10.0)) / PARALLEL(2.0, 10.0))
#define LUX2810_VADC3_FT_SCALE  (LUX2810_DAC_FS / LUX2810_DAC_VREF * (100.0 + PARALLEL(2.0, 10.0)) / PARALLEL(2.0, 10.0))
//NC
//NC
//Third DAC
#define LUX2810_VREFN_SCALE     (LUX2810_DAC_FS / LUX2810_DAC_VREF)
#define LUX2810_VREFP_SCALE     (LUX2810_DAC_FS / LUX2810_DAC_VREF)
#define LUX2810_VTSTH_SCALE     (LUX2810_DAC_FS / LUX2810_DAC_VREF)
#define LUX2810_VTSTL_SCALE     (LUX2810_DAC_FS / LUX2810_DAC_VREF)
#define LUX2810_VRSTB_SCALE     (LUX2810_DAC_FS / LUX2810_DAC_VREF)
#define LUX2810_VRSTH_SCALE     (LUX2810_DAC_FS / LUX2810_DAC_VREF * 49.9 / (49.9 + 10.0))
#define LUX2810_VRSTL_SCALE     (LUX2810_DAC_FS / LUX2810_DAC_VREF)

#define LUX2810_GAIN_1				0
#define LUX2810_GAIN_2				1
#define LUX2810_GAIN_4				2
#define LUX2810_GAIN_8				3
#define LUX2810_GAIN_16				4

#define LUX2810_VERSION_1			1
#define LUX2810_VERSION_2			2

#define LUX2810_CLOCK_PERIOD	(1.0/LUX2810_SENSOR_CLOCK)
#define LUX2810_MIN_WAVETABLE_SIZE	66

#define LUX2810_DAC_NOP_COMMAND     0x9000      //Write through mode is used as a NOP command since the DAC doesn't have NOP

#define FILTER_COLOR_RED	0
#define FILTER_COLOR_GREEN	1
#define FILTER_COLOR_BLUE	2

class LUX2810 : public ImageSensor
{
public:
	LUX2810();
	~LUX2810();
	CameraErrortype init(GPMC * gpmc_inst);

	UInt32 getSensorQuirks() { return SENSOR_QUIRK_UPSIDE_DOWN; }

	/* Frame Geometry Functions. */
	void setResolution(FrameGeometry *frameSize);
	FrameGeometry getMaxGeometry(void);
	UInt8 getFilterColor(UInt32 h, UInt32 v);
	UInt32 getHResIncrement() { return LUX2810_HRES_INCREMENT; }
	UInt32 getVResIncrement()  { return LUX2810_VRES_INCREMENT; }
	UInt32 getMinHRes() { return LUX2810_MIN_HRES; }
	UInt32 getMinVRes() { return LUX2810_MIN_VRES; }

	/* Frame Timing Functions. */
	Int32 seqOnOff(bool on);
	UInt32 getFramePeriodClock(void) { return LUX2810_TIMING_CLOCK; }
	UInt32 getMinFramePeriod(FrameGeometry *frameSize);
	UInt32 getActualFramePeriod(double target, FrameGeometry *frameSize);
	UInt32 getFramePeriod(void);
	UInt32 setFramePeriod(UInt32 period, FrameGeometry *frameSize);

	/* Exposure Timing Functions */
	UInt32 getIntegrationClock(void) { return LUX2810_TIMING_CLOCK; }
	UInt32 getMaxIntegrationTime(UInt32 period, FrameGeometry *frameSize);
	UInt32 getMinIntegrationTime(UInt32 period, FrameGeometry *frameSize) { return LUX2810_TIMING_CLOCK / 1000000; } /* 1us */
	UInt32 getIntegrationTime(void);
	UInt32 setIntegrationTime(UInt32 intTime, FrameGeometry *frameSize);

	/* Analog calibration APIs. */
	unsigned int enableAnalogTestMode(void);
	void disableAnalogTestMode(void);
	void setAnalogTestVoltage(unsigned int);
	void adcOffsetTraining(FrameGeometry *frameSize, UInt32 address, UInt32 numFrames);
	std::string getFilename(const char * filename, const char * extension);

private:
	CameraErrortype initSensor();
	CameraErrortype autoPhaseCal(void);
	UInt32 getDataCorrect(void);
	void setSyncToken(UInt16 token);
	void setSlaveExposure(UInt32 exposure);
	void setReset(bool reset);
	void setClkPhase(UInt8 phase);
	UInt8 getClkPhase(void);
	void initDAC();
	void writeDAC(UInt16 data, UInt8 channel);
	void writeDACVoltage(UInt8 channel, float voltage);
    int writeDACSPI(UInt16 data0, unsigned short data1, unsigned short data2);
	int writeSensorSPI(UInt16 addr, UInt16 data);
	int readSensorSPI(UInt16 addr, UInt16 & data);
	void setDACCS(bool on);
	void setSensorCS(bool on);
	int LUX2810RegWrite(UInt16 addr, UInt16 data);
	int LUX2810RegRead(UInt16 addr, UInt16 & data);
	int LUX2810LoadWavetable(UInt32 * wavetable, UInt32 length);
	Int32 setABNDelayClocks(UInt32 ABNOffset);
    Int32 initLUX2810();

	FrameGeometry currentRes;
	UInt32 currentPeriod;
	UInt32 currentExposure;
	Int32 dacCSFD;
	Int32 sensorCSFD;
	UInt32 wavetableSize;
	UInt32 gain;
	UInt32 startDelaySensorClocks;
	UInt32 sensorVersion;

	SPI * spi;
	GPMC * gpmc;
};

#endif // LUX2810_H
