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
#ifndef LUX2100_H
#define LUX2100_H
#include "frameGeometry.h"
#include "sensor.h"
#include "errorCodes.h"
#include "types.h"
#include "spi.h"
#include "gpmc.h"
#include <string>

#define LUX2100_HRES_INCREMENT 32
#define LUX2100_VRES_INCREMENT	2
#define LUX2100_MAX_H_RES		1920	//Limited by OMX video pipeline.
#define LUX2100_MAX_V_RES		1080
#define LUX2100_MIN_HRES		192		//Limited by video encoder minimum
#define LUX2100_MIN_VRES		96
#define LUX2100_MAGIC_ABN_DELAY	26
#define LUX2100_MAX_V_DARK		8
#define LUX2100_BITS_PER_PIXEL	12

#define LUX2100_TIMING_CLOCK_FREQ		100000000.0	//Hz
#define LUX2100_SENSOR_CLOCK	75000000.0	//Hz

#define LUX2100_MAX_SLAVE_PERIOD ((double)0xFFFFFFFF)

#define LUX2100_GAIN_CORRECTION_MIN 0.999
#define LUX2100_GAIN_CORRECTION_MAX 1.2

/* Addressable image sensor boundary regions */
#define LUX2100_LOW_BOUNDARY_ROWS	8
#define LUX2100_HIGH_BOUNDARY_ROWS	8
#define LUX2100_HIGH_DARK_ROWS		LUX2100_MAX_V_DARK
#define LUX2100_LEFT_DARK_COLUMNS	32
#define LUX2100_RIGHT_DARK_COLUMNS	32

enum {
    // First DAC
    LUX2100_VABL_VOLTAGE = 0,
    LUX2100_VTX2L_VOLTAGE,
    LUX2100_VTXH_VOLTAGE,
    LUX2100_VDR1_VOLTAGE,
    LUX2100_VRSTH_VOLTAGE,
    //NC
    LUX2100_VDR3_VOLTAGE = 6,
    LUX2100_VRDH_VOLTAGE,
    // Second DAC
    //NC
    LUX2100_VTX2H_VOLTAGE = 9,
    //NC
    LUX2100_VTXL_VOLTAGE = 11,
    LUX2100_VPIX_OP_VOLTAGE,
    LUX2100_VDR2_VOLTAGE,
    LUX2100_VPIX_LDO_VOLTAGE,
    LUX2100_VRSTPIX_VOLTAGE,
};

#define LUX2100_DAC_FS		4095.0
#define LUX2100_DAC_VREF	3.3

#define LUX2100_VABL_SCALE      (LUX2100_DAC_FS / LUX2100_DAC_VREF)
#define LUX2100_VTX2L_SCALE     (LUX2100_DAC_FS / LUX2100_DAC_VREF * (10.0 + 10.0) / 10.0)
#define LUX2100_VTXH_SCALE      (LUX2100_DAC_FS / LUX2100_DAC_VREF * 49.9 / (49.9 + 10.0))
#define LUX2100_VDR1_SCALE      (LUX2100_DAC_FS / LUX2100_DAC_VREF)
#define LUX2100_VRSTH_SCALE     (LUX2100_DAC_FS / LUX2100_DAC_VREF * 49.9 / (49.9 + 10.0))
#define LUX2100_VDR3_SCALE      (LUX2100_DAC_FS / LUX2100_DAC_VREF)
#define LUX2100_VRDH_SCALE      (LUX2100_DAC_FS / LUX2100_DAC_VREF)
#define LUX2100_VTX2H_SCALE     (LUX2100_DAC_FS / LUX2100_DAC_VREF * 49.9 / (49.9 + 10.0))
#define LUX2100_VTXL_SCALE      (LUX2100_DAC_FS / LUX2100_DAC_VREF * 31.6 / 10.0)
#define LUX2100_VTXL_OFFSET     0.5485f      //Offset voltage of VTXL. That is, the opamp output voltage with a DAC value of 0
#define LUX2100_VPIX_OP_SCALE   (LUX2100_DAC_FS / LUX2100_DAC_VREF * 49.9 / (49.9 + 10.0))
#define LUX2100_VDR2_SCALE      (LUX2100_DAC_FS / LUX2100_DAC_VREF)
#define LUX2100_VPIX_LDO_SCALE  (LUX2100_DAC_FS / LUX2100_DAC_VREF * 4.22 / 1.65)
#define LUX2100_VPIX_LDO_OFFSET 3.68f        //Offset voltage of VPIX_LT_LDO. That is, the LDO output voltage with a DAC value of 0
#define LUX2100_VRSTPIX_SCALE   (LUX2100_DAC_FS / LUX2100_DAC_VREF)

#define LUX2100_VERSION_1			1
#define LUX2100_VERSION_2			2

#define LUX2100_CLOCK_PERIOD	(1.0/LUX2100_SENSOR_CLOCK)
#define LUX2100_MIN_WAVETABLE_SIZE	66

#define LUX2100_DAC_NOP_COMMAND     0x9000      //Write through mode is used as a NOP command since the DAC doesn't have NOP

#define FILTER_COLOR_RED	0
#define FILTER_COLOR_GREEN	1
#define FILTER_COLOR_BLUE	2

/* LUX 2100 Sensor Wavetables */
typedef struct {
	unsigned int clocks;
	unsigned int length;
	unsigned int abnDelay;
	unsigned int binning;
	const UInt8 *wavetab;
} lux2100wavetab_t;

/* Array of wavetables, sorted longest first, and terminated with a NULL */
extern const lux2100wavetab_t *lux2100wt[];
extern const lux2100wavetab_t *lux8mwt[];

class LUX2100 : public ImageSensor
{
public:
    LUX2100();
    ~LUX2100();
	CameraErrortype init(GPMC * gpmc_inst);

	//UInt32 getSensorQuirks() { return SENSOR_QUIRK_SLOW_OFFSET_CAL; }

	/* Frame Geometry Functions. */
	void setResolution(FrameGeometry *frameSize);
	FrameGeometry getMaxGeometry(void);
	UInt8 getFilterColor(UInt32 h, UInt32 v);
	UInt32 getHResIncrement() { return LUX2100_HRES_INCREMENT; }
	UInt32 getVResIncrement()  { return LUX2100_VRES_INCREMENT; }
	UInt32 getMinHRes() { return LUX2100_MIN_HRES; }
	UInt32 getMinVRes() { return LUX2100_MIN_VRES; }

	/* Frame Timing Functions. */
	Int32 seqOnOff(bool on);
	UInt32 getFramePeriodClock(void) { return LUX2100_TIMING_CLOCK_FREQ; }
	UInt32 getMinFramePeriod(FrameGeometry *frameSize);
	UInt32 getActualFramePeriod(double target, FrameGeometry *frameSize);
	UInt32 getFramePeriod(void);
	UInt32 setFramePeriod(UInt32 period, FrameGeometry *frameSize);

	/* Exposure Timing Functions */
	UInt32 getIntegrationClock(void) { return LUX2100_TIMING_CLOCK_FREQ; }
	UInt32 getMaxIntegrationTime(UInt32 period, FrameGeometry *frameSize);
	UInt32 getMinIntegrationTime(UInt32 period, FrameGeometry *frameSize) { return LUX2100_TIMING_CLOCK_FREQ / 1000000; } /* 1us */
	UInt32 getIntegrationTime(void);
	UInt32 setIntegrationTime(UInt32 intTime, FrameGeometry *frameSize);

	UInt32 getMinGain() { return 1; }
	UInt32 getMaxGain() { return 16; }
	Int32 setGain(UInt32 gainSetting);

	/* Analog calibration APIs. */
	unsigned int enableAnalogTestMode(void);
	void disableAnalogTestMode(void);
	void setAnalogTestVoltage(unsigned int);
	void adcOffsetTraining(FrameGeometry *frameSize, UInt32 address, UInt32 numFrames);
	Int32 loadADCOffsetsFromFile(FrameGeometry *size);
	std::string getFilename(const char * filename, const char * extension);

protected:
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
    int writeDACSPI(UInt16 data0, unsigned short data1);
	void setDACCS(bool on);
	void SCIWrite(UInt8 address, UInt16 data);
	void SCIWriteBuf(UInt8 address, const UInt8 * data, UInt32 dataLen);
	UInt16 SCIRead(UInt8 address);
	Int32 doAutoADCOffsetCalibration(void);
	Int32 initLUX2100(void);
	void setADCOffset(UInt8 channel, Int16 offset);
	void offsetCorrectionIteration(FrameGeometry *geometry, int *offsets, UInt32 address, UInt32 framesToAverage, int iter);
	void updateWavetableSetting(void);
	UInt32 getMinWavetablePeriod(FrameGeometry *frameSize, UInt32 wtSize);

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
	const lux2100wavetab_t **wtlist;
};

/*
 * The LUX8M Also exists as a 4k variant of the LUX2100
 * It's the same die, just with a different CFA pattern.
 */
class LUX8M : public LUX2100 {
	CameraErrortype init(GPMC * gpmc_inst);
	FrameGeometry getMaxGeometry(void);
	void setResolution(FrameGeometry *frameSize);
	unsigned int enableAnalogTestMode(void);
	void disableAnalogTestMode(void);

private:
	CameraErrortype initSensor();
	Int32 initLUX8M(void);
};

#endif // LUX2100_H
