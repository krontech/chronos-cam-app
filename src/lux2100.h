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
#include "errorCodes.h"
#include "types.h"
#include "spi.h"
#include "gpmc.h"
#include <string>

#define LUX2100_HRES_INCREMENT 32
#define LUX2100_VRES_INCREMENT	2
#define LUX2100_MAX_H_RES		1952
#define LUX2100_MAX_V_RES		1080
#define LUX2100_MIN_HRES		192		//Limited by video encoder minimum
#define LUX2100_MIN_VRES		96
#define LUX2100_MAGIC_ABN_DELAY	26
#define LUX2100_MAX_V_DARK		8
#define LUX2100_BITS_PER_PIXEL	12

#define LUX2100_TIMING_CLOCK_FREQ		100000000.0	//Hz
#define LUX2100_SENSOR_CLOCK	75000000.0	//Hz


#define LUX2100_MIN_INT_TIME	0.000001	//1us
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

#define LUX2100_WAVETABLE_80		0
#define LUX2100_WAVETABLE_39		1
#define LUX2100_WAVETABLE_30		2
#define LUX2100_WAVETABLE_25		3
#define LUX2100_WAVETABLE_20		4
#define LUX2100_WAVETABLE_AUTO		0x7FFFFFFF

#define LUX2100_WAVETABLE_80_FN		"WT80"
#define LUX2100_WAVETABLE_39_FN		"WT39"
#define LUX2100_WAVETABLE_30_FN		"WT30"
#define LUX2100_WAVETABLE_25_FN		"WT25"
#define LUX2100_WAVETABLE_20_FN		"WT20"

#define LUX2100_GAIN_1				0
#define LUX2100_GAIN_2				1
#define LUX2100_GAIN_4				2
#define LUX2100_GAIN_8				3
#define LUX2100_GAIN_16				4

//Strings to build FPN filenames
#define LUX2100_GAIN_1_FN			"G1"
#define LUX2100_GAIN_2_FN			"G2"
#define LUX2100_GAIN_4_FN			"G4"
#define LUX2100_GAIN_8_FN			"G8"
#define LUX2100_GAIN_16_FN			"G16"

#define ABN_DELAY_WT80				25
#define ABN_DELAY_WT39				30
#define ABN_DELAY_WT30				29
#define ABN_DELAY_WT25				27
#define ABN_DELAY_WT20				25

#define LUX2100_VERSION_1			1
#define LUX2100_VERSION_2			2

#define LUX2100_CLOCK_PERIOD	(1.0/LUX2100_SENSOR_CLOCK)
#define LUX2100_MIN_WAVETABLE_SIZE	66

#define LUX2100_DAC_NOP_COMMAND     0x9000      //Write through mode is used as a NOP command since the DAC doesn't have NOP

#define FILTER_COLOR_RED	0
#define FILTER_COLOR_GREEN	1
#define FILTER_COLOR_BLUE	2

class LUX2100
{
public:
    LUX2100();
    ~LUX2100();
	CameraErrortype init(GPMC * gpmc_inst);
	CameraErrortype initSensor();
	Int32 setOffset(UInt16 * offsets);
	CameraErrortype autoPhaseCal(void);
	UInt32 getDataCorrect(void);
	void setSyncToken(UInt16 token);
	void setResolution(FrameGeometry *frameSize);
	bool isValidResolution(FrameGeometry *frameSize);
	FrameGeometry getMaxGeometry(void);
	UInt32 getMinFramePeriod(FrameGeometry *frameSize, UInt32 wtSize = LUX2100_MIN_WAVETABLE_SIZE);
	double getMinMasterFramePeriod(FrameGeometry *frameSize);
	double getActualFramePeriod(double target, FrameGeometry *frameSize);
	double setFramePeriod(double period, FrameGeometry *frameSize);
	double getMaxIntegrationTime(double period, FrameGeometry *frameSize);
	double getMaxCurrentIntegrationTime(void);
	double getActualIntegrationTime(double intTime, double period, FrameGeometry *frameSize);
	double setIntegrationTime(double intTime, FrameGeometry *frameSize);
	double getIntegrationTime(void);
	UInt32 getMaxExposure(UInt32 period);
	double getCurrentFramePeriodDouble(void);
	double getCurrentExposureDouble(void);
	void setSlavePeriod(UInt32 period);
	void setSlaveExposure(UInt32 exposure);
	void setReset(bool reset);
	void setClkPhase(UInt8 phase);
	UInt8 getClkPhase(void);
	Int32 seqOnOff(bool on);
	void dumpRegisters(void);
    inline UInt32 getHResIncrement() { return LUX2100_HRES_INCREMENT; }
	inline UInt32 getVResIncrement()  { return LUX2100_VRES_INCREMENT; }
    inline UInt32 getMinHRes() { return LUX2100_MIN_HRES; }
    inline UInt32 getMinVRes() { return LUX2100_MIN_VRES; }
	void initDAC();
	void writeDAC(UInt16 data, UInt8 channel);
	void writeDACVoltage(UInt8 channel, float voltage);
    int writeDACSPI(UInt16 data0, unsigned short data1);
	void setDACCS(bool on);
	void SCIWrite(UInt8 address, UInt16 data);
	void SCIWriteBuf(UInt8 address, UInt8 * data, UInt32 dataLen);
	UInt16 SCIRead(UInt8 address);
	void setWavetable(UInt8 mode);
	void updateWavetableSetting();
	void setADCOffset(UInt8 channel, Int16 offset);
	Int16 getADCOffset(UInt8 channel);
    Int32 doAutoADCOffsetCalibration(void);
	Int32 loadADCOffsetsFromFile(void);
	Int32 saveADCOffsetsToFile(void);
	std::string getFilename(const char * filename, const char * extension);
	Int32 setGain(UInt32 gainSetting);
	UInt8 getFilterColor(UInt32 h, UInt32 v);
	Int32 setABNDelayClocks(UInt32 ABNOffset);
    Int32 LUX2100ADCBugCorrection(UInt16 * rawUnpackedFrame, UInt32 hRes, UInt32 vRes);
    Int32 initLUX2100(bool colorBinning = false);
    Int32 initLUX8M(void);
    Int32 initLUX8M_2(void);

	bool masterMode;
	UInt32 masterModeTotalLines;
	FrameGeometry currentRes;
	UInt32 currentPeriod;
	UInt32 currentExposure;
	Int32 dacCSFD;
	UInt32 wavetableSize;
	UInt32 gain;
	UInt32 wavetableSelect;
	UInt32 startDelaySensorClocks;
	UInt32 sensorVersion;

	SPI * spi;
	GPMC * gpmc;
	UInt8 clkPhase;
    Int16 offsetsA[LUX2100_HRES_INCREMENT];
};

#endif // LUX2100_H
