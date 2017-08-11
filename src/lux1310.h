#ifndef LUX1310_H
#define LUX1310_H
#include "errorCodes.h"
#include "types.h"
#include "spi.h"
#include "gpmc.h"
#include <string>

#define LUX1310_HRES_INCREMENT 16
#define LUX1310_VRES_INCREMENT	2
#define LUX1310_MAX_STRIDE		1280
#define LUX1310_MAX_H_RES		1280
#define LUX1310_MAX_V_RES		1024
#define LUX1310_MIN_HRES		192		//Limited by video encoder minimum
#define LUX1310_MIN_VRES		96
#define LUX1310_MAGIC_ABN_DELAY	26

#define TIMING_CLOCK_FREQ		100000000.0	//Hz
#define LUX1310_SENSOR_CLOCK	90000000.0	//Hz

#define LUX1310_ROT			(9+4)		//Granularity clock cycles (63MHz periods by default)
#define LUX1310_FOT			315		//Granularity clock cycles (63MHz periods by default)

#define ROT_TIMER				9
#define FOT_TIMER				315

#define ROT						(ROT_TIMER + 4)
#define FOT						(FOT_TIMER + 29)
#define MAX_TS					54

#define LUX1310_MIN_INT_TIME	0.000001	//1us
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


#define LUX1310_WAVETABLE_80		0
#define LUX1310_WAVETABLE_39		1
#define LUX1310_WAVETABLE_30		2
#define LUX1310_WAVETABLE_25		3
#define LUX1310_WAVETABLE_20		4
#define LUX1310_WAVETABLE_AUTO		0x7FFFFFFF

#define LUX1310_WAVETABLE_80_FN		"WT80"
#define LUX1310_WAVETABLE_39_FN		"WT39"
#define LUX1310_WAVETABLE_30_FN		"WT30"
#define LUX1310_WAVETABLE_25_FN		"WT25"
#define LUX1310_WAVETABLE_20_FN		"WT20"

#define LUX1310_GAIN_1				0
#define LUX1310_GAIN_2				1
#define LUX1310_GAIN_4				2
#define LUX1310_GAIN_8				3
#define LUX1310_GAIN_16				4

//Strings to build FPN filenames
#define LUX1310_GAIN_1_FN			"G1"
#define LUX1310_GAIN_2_FN			"G2"
#define LUX1310_GAIN_4_FN			"G4"
#define LUX1310_GAIN_8_FN			"G8"
#define LUX1310_GAIN_16_FN			"G16"

#define ABN_DELAY_WT80				25
#define ABN_DELAY_WT39				30
#define ABN_DELAY_WT30				29
#define ABN_DELAY_WT25				27
#define ABN_DELAY_WT20				25

#define LUX1310_VERSION_1			1
#define LUX1310_VERSION_2			2

#define LUX1310_CLOCK_PERIOD	(1.0/90000000.0)
#define LUX1310_MIN_WAVETABLE_SIZE	20

#define FILTER_COLOR_RED	0
#define FILTER_COLOR_GREEN	1
#define FILTER_COLOR_BLUE	2

class LUX1310
{
public:
	LUX1310();
	~LUX1310();
	CameraErrortype init(GPMC * gpmc_inst);
	CameraErrortype initSensor();
	Int32 setOffset(UInt16 * offsets);
	CameraErrortype autoPhaseCal(void);
	UInt32 getDataCorrect(void);
	bool isValidResolution(UInt32 hRes, UInt32 vRes, UInt32 hOffset, UInt32 vOffset);
	void setSyncToken(UInt16 token);
	void setResolution(UInt32 hStart, UInt32 hWidth, UInt32 vStart, UInt32 vEnd);
	UInt32 getMinFramePeriod(UInt32 hRes, UInt32 vRes, UInt32 wtSize = LUX1310_MIN_WAVETABLE_SIZE);
	double getMinMasterFramePeriod(UInt32 hRes, UInt32 vRes);
	double getActualFramePeriod(double targetPeriod, UInt32 hRes, UInt32 vRes);
	double setFramePeriod(double period, UInt32 hRes, UInt32 vRes);
	double getMaxIntegrationTime(double period, UInt32 hRes, UInt32 vRes);
	double getMaxCurrentIntegrationTime(void);
	double getActualIntegrationTime(double intTime, double period, UInt32 hRes, UInt32 vRes);
	double setIntegrationTime(double intTime, UInt32 hRes, UInt32 vRes);
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
	inline UInt32 getHResIncrement() { return LUX1310_HRES_INCREMENT; }
	inline UInt32 getVResIncrement()  { return LUX1310_VRES_INCREMENT; }
	inline UInt32 getMaxHRes() { return LUX1310_MAX_H_RES; }
	inline UInt32 getMaxHStride() { return LUX1310_MAX_STRIDE; }
	inline UInt32 getMaxVRes()  { return LUX1310_MAX_V_RES; }
	inline UInt32 getMinHRes() { return LUX1310_MIN_HRES; }
	inline UInt32 getMinVRes() { return LUX1310_MIN_VRES; }
	void initDAC();
	void writeDAC(UInt16 data, UInt8 channel);
	void writeDACVoltage(UInt8 channel, float voltage);
	int writeDACSPI(UInt16 data);
	void setDACCS(bool on);
	void SCIWrite(UInt8 address, UInt16 data);
	void SCIWriteBuf(UInt8 address, UInt8 * data, UInt32 dataLen);
	UInt16 SCIRead(UInt8 address);
	void setWavetable(UInt8 mode);
	void updateWavetableSetting();
	void setADCOffset(UInt8 channel, Int16 offset);
	Int16 getADCOffset(UInt8 channel);
	Int32 loadADCOffsetsFromFile(const char * filename);
	Int32 saveADCOffsetsToFile(const char * filename);
	std::string getFilename(const char * filename, const char * extension);
	Int32 setGain(UInt32 gainSetting);
	UInt8 getFilterColor(UInt32 h, UInt32 v);
	Int32 setABNDelayClocks(UInt32 ABNOffset);


	bool masterMode;
	UInt32 masterModeTotalLines;
	UInt32 currentHRes;
	UInt32 currentVRes;
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
	Int16 offsetsA[16];
};
/*
enum {
	LUX1310_SUCCESS = 0,
	LUX1310_SPI_OPEN_FAIL
};
*/
#endif // LUX1310_H
