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
#include "lux1310.h"

#include <QSettings>

/* Some Timing Constants */
#define LUX1310_SOF_DELAY	0x0f	/* Delay from TXN rising edge to start of frame. */
#define LUX1310_LV_DELAY	0x07	/* Linevalid delay to match ADC latency. */
#define LUX1310_MIN_HBLANK	0x02	/* Minimum horiontal blanking period. */

#define round(x) (floor(x + 0.5))

LUX1310::LUX1310()
{
	spi = new SPI();
	startDelaySensorClocks = LUX1310_MAGIC_ABN_DELAY;
	//Zero out offsets
	for(int i = 0; i < LUX1310_HRES_INCREMENT; i++)
		offsetsA[i] = 0;

}

LUX1310::~LUX1310()
{
	delete spi;
}

CameraErrortype LUX1310::init(GPMC * gpmc_inst)
{
	CameraErrortype retVal;
	retVal = spi->Init(IMAGE_SENSOR_SPI, 16, 1000000, false, true);	//Invert clock phase
	if(SUCCESS != retVal)
		return retVal;

	dacCSFD = open("/sys/class/gpio/gpio33/value", O_WRONLY);
	if (-1 == dacCSFD)
		return LUX1310_FILE_ERROR;

	gpmc = gpmc_inst;

	retVal = initSensor();
	//mem problem before this
	if(SUCCESS != retVal)
		return retVal;

	return SUCCESS;
}

CameraErrortype LUX1310::initSensor()
{
	CameraErrortype err;
	UInt16 rev;

	wavetableSize = lux1310wt[0]->clocks;
	gain = LUX1310_GAIN_1;

	gpmc->write32(IMAGER_FRAME_PERIOD_ADDR, 100*4000);	//Disable integration
	gpmc->write32(IMAGER_INT_TIME_ADDR, 100*4100);

	initDAC();
	writeDACVoltage(VABL_VOLTAGE, 0.3);
	writeDACVoltage(VRSTB_VOLTAGE, 2.7);
	writeDACVoltage(VRST_VOLTAGE, 3.3);
	writeDACVoltage(VRSTL_VOLTAGE, 0.7);
	writeDACVoltage(VRSTH_VOLTAGE, 3.6);
	writeDACVoltage(VDR1_VOLTAGE, 2.5);
	writeDACVoltage(VDR2_VOLTAGE, 2);
	writeDACVoltage(VDR3_VOLTAGE, 1.5);

	delayms(10);		//Settling time

	setReset(true);

	setReset(false);

	delayms(1);		//Seems to be required for SCI clock to get running

	/* Reset the sensor and check its chip ID. */
	SCIWrite(0x7E, 0);	//reset all registers
	rev = SCIRead(0x00);
	if ((rev >> 8) != LUX1310_CHIP_ID) {
		fprintf(stderr, "LUX1310 rev_chip returned invalid ID (0x02x)\n", rev>>8);
		return CAMERA_ERROR_SENSOR;
	}
	sensorVersion = rev & 0xFF;
	fprintf(stderr, "Initializing LUX1310: silicon revision %d\n", sensorVersion);

	//delayms(1);
	SCIWrite(0x57, 0x0FC0); //custom pattern for ADC training pattern
	//delayms(1);
	qDebug() << "SCI Read returned" << SCIRead(0x57);
	SCIWrite(0x56, 0x0002); //Enable test pattern
	SCIWrite(0x4C, 0x0FC0);	//pclk channel output during vertical blanking
	SCIWrite(0x5A, 0xE1);	//Invert dclk output

	//Sensor is now outputting 0xFC0 on all ports. Receiver will automatically sync when the clock phase is right

	autoPhaseCal();

	/* Break out in case of an error. */
	err = LUX1310_SERIAL_READBACK_ERROR;
	do {
		//Return to normal data mode
		if (!SCIWrite(0x4C, 0x0F00, true)) break;	//pclk channel output during vertical blanking
		if (!SCIWrite(0x4E, 0x0FC0, true)) break;	//pclk channel output during dark pixel readout
		if (!SCIWrite(0x56, 0x0000, true)) break;	//Disable test pattern

		//Set for the longest wavetable
		if (!SCIWrite(0x37, lux1310wt[0]->clocks, true)) break;	//non-overlapping readout delay
		if (!SCIWrite(0x7A, lux1310wt[0]->clocks, true)) break;	//wavetable size

		//Set internal control registers to fine tune the performance of the sensor
		if (!SCIWrite(0x71, LUX1310_LV_DELAY, true)) break;		//line valid delay to match internal ADC latency
		if (!SCIWrite(0x03, LUX1310_MIN_HBLANK, true)) break;	//set horizontal blanking period.
		if (!SCIWrite(0x2D, 0xE08E, true)) break; //state for idle controls
		if (!SCIWrite(0x2E, 0xFC1F, true)) break; //state for idle controls
		if (!SCIWrite(0x2F, 0x0003, true)) break; //state for idle controls

		if (!SCIWrite(0x5C, 0x2202, true)) break; //ADC clock controls
		if (!SCIWrite(0x62, 0x5A76, true)) break; //ADC range to match pixel saturation level
		if (!SCIWrite(0x74, 0x041F, true)) break; //internal clock timing
		if (!SCIWrite(0x66, 0x0845, true)) break; //internal current control

		//Load the appropriate values based on the silicon revision
		if(LUX1310_VERSION_2 == sensorVersion) {
			SCIWrite(0x5B, 0x307F); //internal control register
			SCIWrite(0x7B, 0x3007); //internal control register
		}
		else if(LUX1310_VERSION_1 == sensorVersion) {
			SCIWrite(0x5B, 0x301F); //internal control register
			SCIWrite(0x7B, 0x3001); //internal control register
		}
		else
		{	//Unknown version, default to version 1 values
			SCIWrite(0x5B, 0x301F); //internal control register
			SCIWrite(0x7B, 0x3001); //internal control register
			qDebug() << "Found LUX1310 sensor, unexpected Silicon revision found:" << sensorVersion;
		}

		err = SUCCESS;
	} while (0);
	if (err != SUCCESS) {
		return err;
	}

	/* Load and enable ADC offsets. */
	for (int i = 0; i < LUX1310_HRES_INCREMENT; i++) {
		setADCOffset(i, 0);
	}
	SCIWrite(0x39, 0x1); //ADC offset enable??

	//Set Gain
	SCIWrite(0x51, 0x007F);	//gain selection sampling cap (11)	12 bit
	SCIWrite(0x52, 0x007F);	//gain selection feedback cap (8) 7 bit
	SCIWrite(0x53, 0x03);	//Serial gain

	//Load the default wavetable
	SCIWriteBuf(0x7F, lux1310wt[0]->wavetab, lux1310wt[0]->length);

	//Enable internal timing engine
	SCIWrite(0x01, 0x0001);

	delayms(10);

	gpmc->write32(IMAGER_FRAME_PERIOD_ADDR, 100*4000);
	gpmc->write32(IMAGER_INT_TIME_ADDR, 100*3900);
	delayms(50);

	currentRes.hRes = LUX1310_MAX_H_RES;
	currentRes.vRes = LUX1310_MAX_V_RES;
	currentRes.hOffset = 0;
	currentRes.vOffset = 0;
	currentRes.vDarkRows = 0;
	currentRes.bitDepth = LUX1310_BITS_PER_PIXEL;

	setFramePeriod(getMinFramePeriod(&currentRes)/100000000.0, &currentRes);
	//mem problem before this
	setIntegrationTime((double)getMaxExposure(currentPeriod) / 100000000.0, &currentRes);

	return SUCCESS;
}

bool LUX1310::SCIWrite(UInt8 address, UInt16 data, bool readback)
{
	int i = 0;

	//Clear RW and reset FIFO
	gpmc->write16(SENSOR_SCI_CONTROL_ADDR, 0x8000);

	//Set up address, transfer length and put data into FIFO
	gpmc->write16(SENSOR_SCI_ADDRESS_ADDR, address);
	gpmc->write16(SENSOR_SCI_DATALEN_ADDR, 2);
	gpmc->write16(SENSOR_SCI_FIFO_WR_ADDR_ADDR, data >> 8);
	gpmc->write16(SENSOR_SCI_FIFO_WR_ADDR_ADDR, data & 0xFF);

	// Start the write and wait for completion.
	gpmc->write16(SENSOR_SCI_CONTROL_ADDR, SENSOR_SCI_CONTROL_RUN_MASK);
	while(gpmc->read16(SENSOR_SCI_CONTROL_ADDR) & SENSOR_SCI_CONTROL_RUN_MASK) i++;

	if (readback) {
		UInt16 readback;

		if (i == 0) {
			qDebug() << "No busy detected, something is probably very wrong, address:" << address;
		}
		readback = SCIRead(address);
		if (data != readback) {
			qDebug() << "SCI readback wrong, address: " << address << " expected: " << data << " got: " << readback;
			return false;
		}
	}
	return true;
}

void LUX1310::SCIWriteBuf(UInt8 address, const UInt8 * data, UInt32 dataLen)
{
	//Clear RW and reset FIFO
	gpmc->write16(SENSOR_SCI_CONTROL_ADDR, 0x8000 | (gpmc->read16(SENSOR_SCI_CONTROL_ADDR) & ~SENSOR_SCI_CONTROL_RW_MASK));

	//Set up address, transfer length and put data into FIFO
	gpmc->write16(SENSOR_SCI_ADDRESS_ADDR, address);
	gpmc->write16(SENSOR_SCI_DATALEN_ADDR, dataLen);
	for(UInt32 i = 0; i < dataLen; i++) {
		gpmc->write16(SENSOR_SCI_FIFO_WR_ADDR_ADDR, data[i]);
	}

	//Start transfer
	gpmc->write16(SENSOR_SCI_CONTROL_ADDR, gpmc->read16(SENSOR_SCI_CONTROL_ADDR) | SENSOR_SCI_CONTROL_RUN_MASK);

	//Wait for completion
	while(gpmc->read16(SENSOR_SCI_CONTROL_ADDR) & SENSOR_SCI_CONTROL_RUN_MASK);
}

UInt16 LUX1310::SCIRead(UInt8 address)
{
	int i = 0;

	//Set RW
	gpmc->write16(SENSOR_SCI_CONTROL_ADDR, gpmc->read16(SENSOR_SCI_CONTROL_ADDR) | SENSOR_SCI_CONTROL_RW_MASK);

	//Set up address and transfer length
	gpmc->write16(SENSOR_SCI_ADDRESS_ADDR, address);
	gpmc->write16(SENSOR_SCI_DATALEN_ADDR, 2);

	//Start transfer
	gpmc->write16(SENSOR_SCI_CONTROL_ADDR, gpmc->read16(SENSOR_SCI_CONTROL_ADDR) | SENSOR_SCI_CONTROL_RUN_MASK);

	int first = gpmc->read16(SENSOR_SCI_CONTROL_ADDR) & SENSOR_SCI_CONTROL_RUN_MASK;
	//Wait for completion
	while(gpmc->read16(SENSOR_SCI_CONTROL_ADDR) & SENSOR_SCI_CONTROL_RUN_MASK)
		i++;

	if(!first && i != 0)
		qDebug() << "Read First busy was missed, address:" << address;
	else if(i == 0)
		qDebug() << "Read No busy detected, something is probably very wrong, address:" << address;


	//Wait for completion
//	while(gpmc->read16(SENSOR_SCI_CONTROL_ADDR) & SENSOR_SCI_CONTROL_RUN_MASK)
//		count++;
	delayms(1);
	return gpmc->read16(SENSOR_SCI_READ_DATA_ADDR);
}

Int32 LUX1310::setOffset(UInt16 * offsets)
{
	return 0;
	for(int i = 0; i < 16; i++)
		SCIWrite(0x3a+i, offsets[i]); //internal control register
}

CameraErrortype LUX1310::autoPhaseCal(void)
{
	UInt16 valid = 0;
	UInt32 valid32;

	setClkPhase(0);
	setClkPhase(1);

	setClkPhase(0);


	int dataCorrect = getDataCorrect();
	qDebug() << "datacorrect" << dataCorrect;
	return SUCCESS;

//	setSyncToken(0x32A);	//Set sync token to lock to the training pattern (0x32A or 1100101010)

	for(int i = 0; i < 16; i++)
	{
		valid = (valid >> 1) | ((0x1FFFF == getDataCorrect()) ? 0x8000 : 0);

		qDebug() << "datacorrect" << clkPhase << " " << getDataCorrect();
		//advance clock phase
		clkPhase = getClkPhase() + 1;
		if(clkPhase >= 16)
			clkPhase = 0;
		setClkPhase(clkPhase);
	}

	if(0 == valid)
	{
		setClkPhase(4);
		return LUPA1300_NO_DATA_VALID_WINDOW;
	}

	qDebug() << "Valid: " << valid;

	valid32 = valid | (valid << 16);

	//Determine the start and length of the window of clock phase values that produce valid outputs
	UInt32 bestMargin = 0;
	UInt32 bestStart = 0;

	for(int i = 0; i < 16; i++)
	{
		UInt32 margin = 0;
		if(valid32 & (1 << i))
		{
			//Scan starting at this valid point
			for(int j = 0; j < 16; j++)
			{
				//Track the margin until we hit a non-valid point
				if(valid32 & (1 << (i + j)))
					margin++;
				else
				{
					//Track the best
					if(margin > bestMargin)
					{
						bestMargin = margin;
						bestStart = i;
					}
				}
			}
		}
	}

	if(bestMargin <= 3)
		return LUPA1300_INSUFFICIENT_DATA_VALID_WINDOW;

	//Set clock phase to the best
	clkPhase = (bestStart + bestMargin / 2) % 16;
	setClkPhase(clkPhase);
	qDebug() << "Valid Window start: " << bestStart << "Valid Window Length: " << bestMargin << "clkPhase: " << clkPhase;
}

Int32 LUX1310::seqOnOff(bool on)
{
	if (on) {
		gpmc->write32(IMAGER_INT_TIME_ADDR, currentExposure);
	} else {
		gpmc->write32(IMAGER_INT_TIME_ADDR, 0);	//Disable integration
	}
	return SUCCESS;
}

void LUX1310::setReset(bool reset)
{
		gpmc->write16(IMAGE_SENSOR_CONTROL_ADDR, (gpmc->read16(IMAGE_SENSOR_CONTROL_ADDR) & ~IMAGE_SENSOR_RESET_MASK) | (reset ? IMAGE_SENSOR_RESET_MASK : 0));
}

void LUX1310::setClkPhase(UInt8 phase)
{
		gpmc->write16(IMAGE_SENSOR_CLK_PHASE_ADDR, phase);
}

UInt8 LUX1310::getClkPhase(void)
{
		return gpmc->read16(IMAGE_SENSOR_CLK_PHASE_ADDR);
}

UInt32 LUX1310::getDataCorrect(void)
{
	return gpmc->read32(IMAGE_SENSOR_DATA_CORRECT_ADDR);
}

void LUX1310::setSyncToken(UInt16 token)
{
	gpmc->write16(IMAGE_SENSOR_SYNC_TOKEN_ADDR, token);
}

FrameGeometry LUX1310::getMaxGeometry(void)
{
	FrameGeometry size = {
		.hRes = LUX1310_MAX_H_RES,
		.vRes = LUX1310_MAX_V_RES,
		.hOffset = 0,
		.vOffset = 0,
		.vDarkRows = LUX1310_MAX_V_DARK,
		.bitDepth = LUX1310_BITS_PER_PIXEL,
	};
	return size;
}

void LUX1310::setResolution(FrameGeometry *size)
{
	UInt32 hStartBlocks = size->hOffset / LUX1310_HRES_INCREMENT;
	UInt32 hWidthblocks = size->hRes / LUX1310_HRES_INCREMENT;
	SCIWrite(0x05, 0x20 + hStartBlocks * LUX1310_HRES_INCREMENT);//X Start
	SCIWrite(0x06, 0x20 + (hStartBlocks + hWidthblocks) * LUX1310_HRES_INCREMENT - 1);//X End
	SCIWrite(0x07, size->vOffset);							//Y Start
	SCIWrite(0x08, size->vOffset + size->vRes - 1);	//Y End
	SCIWrite(0x29, (size->vDarkRows << 12) + (LUX1310_MAX_V_RES + LUX1310_MAX_V_DARK - size->vDarkRows + 4));

	memcpy(&currentRes, size, sizeof(currentRes));
}

bool LUX1310::isValidResolution(FrameGeometry *size)
{
	/* Enforce resolution limits. */
	if ((size->hRes < LUX1310_MIN_HRES) || (size->hRes + size->hOffset > LUX1310_MAX_H_RES)) {
		return false;
	}
	if ((size->vRes + size->vDarkRows < LUX1310_MIN_VRES) || (size->vRes + size->vOffset > LUX1310_MAX_V_RES)) {
		return false;
	}
	if (size->vDarkRows > LUX1310_MAX_V_DARK) {
		return false;
	}
	if (size->bitDepth != LUX1310_BITS_PER_PIXEL) {
		return false;
	}
	/* Enforce minimum pixel increments. */
	if ((size->hRes % LUX1310_HRES_INCREMENT) || (size->hOffset % LUX1310_HRES_INCREMENT)) {
		return false;
	}
	if ((size->vRes % LUX1310_VRES_INCREMENT) || (size->vOffset % LUX1310_VRES_INCREMENT)) {
		return false;
	}
	if (size->vDarkRows % LUX1310_VRES_INCREMENT) {
		return false;
	}
	/* Otherwise, the resultion and offset are valid. */
	return true;
}

//Used by init functions only
UInt32 LUX1310::getMinFramePeriod(FrameGeometry *frameSize, UInt32 wtSize)
{
	if(!isValidResolution(frameSize))
		return 0;

	/*
	 * Select the longest wavetable that fits within the line readout time,
	 * or fall back to the shortest wavetable for extremely small resolutions.
	 */
	if (wtSize == 0) {
		unsigned int wtIdeal = (frameSize->hRes / LUX1310_HRES_INCREMENT) + LUX1310_MIN_HBLANK - 3;
		/* Search for the longest matching wavetable */
		for (int i = 0; lux1310wt[i] != NULL; i++) {
			wtSize = lux1310wt[i]->clocks;
			if (wtSize <= wtIdeal) break;
		}
	}

	/* Updated to v3.0 datasheet and computed directly in LUX1310 clocks. */
	unsigned int tRead = frameSize->hRes / LUX1310_HRES_INCREMENT;
	unsigned int tTx = 25; /* Hard-coded to 25 clocks in the FPGA, should actually be at least 350ns. */
	unsigned int tRow = max(tRead + LUX1310_MIN_HBLANK, wtSize + 3);
	unsigned int tFovf = LUX1310_SOF_DELAY + wtSize + LUX1310_LV_DELAY + 10;
	unsigned int tFovfb = 41; /* Duration between PRSTN falling and TXN falling (I think) */
	unsigned int tFrame = tRow * (frameSize->vRes + frameSize->vDarkRows) + tTx + tFovf + tFovfb - LUX1310_MIN_HBLANK;

	/* Convert from 90MHz sensor clocks to 100MHz FPGA clocks. */
	unsigned int value = ceil(tFrame * 100 / 90);
	qDebug("getMinFramePeriod(%u) = %u", wtSize, value);
	return value;
}

double LUX1310::getMinMasterFramePeriod(FrameGeometry *frameSize)
{
	if(!isValidResolution(frameSize))
		return 0.0;

	return (double)getMinFramePeriod(frameSize) / 100000000.0;
}

UInt32 LUX1310::getMaxExposure(UInt32 period)
{
	return period - 500;
}

//Returns the period the sensor is set to in seconds
double LUX1310::getCurrentFramePeriodDouble(void)
{
	return (double)currentPeriod / 100000000.0;
}

//Returns the exposure time the sensor is set to in seconds
double LUX1310::getCurrentExposureDouble(void)
{
	return (double)currentExposure / 100000000.0;
}

double LUX1310::getActualFramePeriod(double targetPeriod, FrameGeometry *size)
{
	//Round to nearest 10ns period
	targetPeriod = round(targetPeriod * (100000000.0)) / 100000000.0;

	double minPeriod = getMinMasterFramePeriod(size);
	double maxPeriod = LUX1310_MAX_SLAVE_PERIOD;

	return within(targetPeriod, minPeriod, maxPeriod);
}

double LUX1310::setFramePeriod(double period, FrameGeometry *size)
{
	//Round to nearest 10ns period
	period = round(period * (100000000.0)) / 100000000.0;
	qDebug() << "Requested period" << period;
	double minPeriod = getMinMasterFramePeriod(size);
	double maxPeriod = LUX1310_MAX_SLAVE_PERIOD / 100000000.0;

	period = within(period, minPeriod, maxPeriod);
	currentPeriod = floor((period * 100000000.0) + 0.5);

	setSlavePeriod(currentPeriod);
	return period;
}

/* getMaxIntegrationTime
 *
 * Gets the actual maximum integration time the sensor can be set to for the given settings
 *
 * intTime:	Desired integration time in seconds
 *
 * returns: Maximum integration time
 */
double LUX1310::getMaxIntegrationTime(double period, FrameGeometry *size)
{
	//Round to nearest 10ns period
	period = round(period * (100000000.0)) / 100000000.0;
	return (double)getMaxExposure(period * 100000000.0) / 100000000.0;

}

/* getMaxCurrentIntegrationTime
 *
 * Gets the actual maximum integration for the current settings
 *
 * returns: Maximum integration time
 */
double LUX1310::getMaxCurrentIntegrationTime(void)
{
	return (double)getMaxExposure(currentPeriod) / 100000000.0;
}

/* getActualIntegrationTime
 *
 * Gets the actual integration time that the sensor can be set to which is as close as possible to desired
 *
 * intTime:	Desired integration time in seconds
 *
 * returns: Actual closest integration time
 */
double LUX1310::getActualIntegrationTime(double intTime, double period, FrameGeometry *size)
{

	//Round to nearest 10ns period
	period = round(period * (100000000.0)) / 100000000.0;
	intTime = round(intTime * (100000000.0)) / 100000000.0;

	double maxIntTime = (double)getMaxExposure(period * 100000000.0) / 100000000.0;
	double minIntTime = LUX1310_MIN_INT_TIME;
	return within(intTime, minIntTime, maxIntTime);

}

/* setIntegrationTime
 *
 * Sets the integration time of the image sensor to a value as close as possible to requested
 *
 * intTime:	Desired integration time in seconds
 *
 * returns: Actual integration time that was set
 */
double LUX1310::setIntegrationTime(double intTime, FrameGeometry *size)
{
	//Round to nearest 10ns period
	intTime = round(intTime * (100000000.0)) / 100000000.0;

	//Set integration time to within limits
	double maxIntTime = (double)getMaxExposure(currentPeriod) / 100000000.0;
	double minIntTime = LUX1310_MIN_INT_TIME;
	intTime = within(intTime, minIntTime, maxIntTime);
	currentExposure = intTime * 100000000.0;
	setSlaveExposure(currentExposure);
	return intTime;
}

/* getIntegrationTime
 *
 * Gets the integration time of the image sensor
 *
 * returns: Integration tim
 */
double LUX1310::getIntegrationTime(void)
{

	return (double)currentExposure / 100000000.0;
}


void LUX1310::setSlavePeriod(UInt32 period)
{
	gpmc->write32(IMAGER_FRAME_PERIOD_ADDR, period);
}

void LUX1310::setSlaveExposure(UInt32 exposure)
{
	//hack to fix line issue. Not perfect, need to properly register this on the sensor clock.
	double linePeriod = max((currentRes.hRes / LUX1310_HRES_INCREMENT)+2, (wavetableSize + 3)) * 1.0/LUX1310_SENSOR_CLOCK;	//Line period in seconds
	UInt32 startDelay = (double)startDelaySensorClocks * TIMING_CLOCK_FREQ / LUX1310_SENSOR_CLOCK;
	double targetExp = (double)exposure / 100000000.0;
	UInt32 expLines = round(targetExp / linePeriod);

	if(exposure > 500) exposure = startDelay + (linePeriod * expLines * 100000000.0);
	//qDebug() << "linePeriod" << linePeriod << "startDelaySensorClocks" << startDelaySensorClocks << "startDelay" << startDelay
	//		 << "targetExp" << targetExp << "expLines" << expLines << "exposure" << exposure;
	gpmc->write32(IMAGER_INT_TIME_ADDR, exposure);
	currentExposure = exposure;
}

void LUX1310::dumpRegisters(void)
{
	return;
}

//Set up DAC
void LUX1310::initDAC()
{
	//Put DAC in Write through mode (DAC channels update immediately on register write)
	writeDACSPI(0x9000);
}

//Write the data into the selected channel
void LUX1310::writeDAC(UInt16 data, UInt8 channel)
{
	writeDACSPI(((channel & 0x7) << 12) | (data & 0x0FFF));
}

void LUX1310::writeDACVoltage(UInt8 channel, float voltage)
{
	switch(channel)
	{
		case VDR3_VOLTAGE:
			writeDAC((UInt16)(voltage * VDR3_SCALE), VDR3_VOLTAGE);
			break;
		case VABL_VOLTAGE:
			writeDAC((UInt16)(voltage * VABL_SCALE), VABL_VOLTAGE);
			break;
		case VDR1_VOLTAGE:
			writeDAC((UInt16)(voltage * VDR1_SCALE), VDR1_VOLTAGE);
			break;
		case VDR2_VOLTAGE:
			writeDAC((UInt16)(voltage * VDR2_SCALE), VDR2_VOLTAGE);
			break;
		case VRSTB_VOLTAGE:
			writeDAC((UInt16)(voltage * VRSTB_SCALE), VRSTB_VOLTAGE);
			break;
		case VRSTH_VOLTAGE:
			writeDAC((UInt16)(voltage * VRSTH_SCALE), VRSTH_VOLTAGE);
			break;
		case VRSTL_VOLTAGE:
			writeDAC((UInt16)(voltage * VRSTL_SCALE), VRSTL_VOLTAGE);
			break;
		case VRST_VOLTAGE:
			writeDAC((UInt16)(voltage * VRST_SCALE), VRST_VOLTAGE);
			break;
		default:
			return;
			break;
	}
}

//Performs an SPI write to the DAC
int LUX1310::writeDACSPI(UInt16 data)
{
	UInt8 tx[2];
	UInt8 rx[sizeof(tx)];
	int retval;

	tx[1] = data >> 8;
	tx[0] = data & 0xFF;

	setDACCS(false);
	//delayms(1);
	retval = spi->Transfer((uint64_t) tx, (uint64_t) rx, sizeof(tx));
	//delayms(1);
	setDACCS(true);
	return retval;
}

void LUX1310::setDACCS(bool on)
{
	lseek(dacCSFD, 0, SEEK_SET);
	write(dacCSFD, on ? "1" : "0", 1);
}

void LUX1310::updateWavetableSetting(bool gainCalMode)
{
	/* Search for the longest wavetable that it shorter than the current frame period. */
	const lux1310wavetab_t *wt = NULL;
	int i;

	qDebug() << "Selecting wavetable for period of" << currentPeriod;

	for (i = 0; lux1310wt[i] != NULL; i++) {
		wt = lux1310wt[i];
		if (currentPeriod >= getMinFramePeriod(&currentRes, wt->clocks)) break;
	}

	/* Update the wavetable. */
	if (wt) {
		SCIWrite(0x01, 0x0000);         //Disable internal timing engine
		SCIWrite(0x37, wt->clocks);     //non-overlapping readout delay
		SCIWrite(0x7A, wt->clocks);     //wavetable size
		SCIWriteBuf(0x7F, gainCalMode ? wt->gaincal : wt->wavetab, wt->length);
		SCIWrite(0x01, 0x0001);         //Enable internal timing engine
		wavetableSize = wt->clocks;
		setABNDelayClocks(wt->abnDelay);

		qDebug() << "Wavetable size set to" << wavetableSize;
	}
}

//Sets ADC offset for one channel
//Converts the input 2s complement value to the sensors's weird sign bit plus value format (sort of like float, with +0 and -0)
void LUX1310::setADCOffset(UInt8 channel, Int16 offset)
{
	offsetsA[channel] = offset;

	if(offset < 0)
		SCIWrite(0x3a+channel, 0x400 | (-offset & 0x3FF));
	else
		SCIWrite(0x3a+channel, offset);

}

//Gets ADC offset for one channel
//Don't use this, the values seem shifted right depending on the gain setting. Higher gain results in more shift. Seems to work fine at lower gains
Int16 LUX1310::getADCOffset(UInt8 channel)
{
	Int16 val = SCIRead(0x3a+channel);

	if(val & 0x400)
		return -(val & 0x3FF);
	else
		return val & 0x3FF;
}

Int32 LUX1310::loadADCOffsets(const char *filename)
{
	Int16 offsets[LUX1310_HRES_INCREMENT];
	Int32 ret = SUCCESS;
	
	do {
		qDebug("Attempting to load ADC offsets from %s", filename);

		//If the offsets file exists, read it in
		if( access( filename, R_OK ) == -1 ) {
			ret = CAMERA_FILE_NOT_FOUND;
			break;
		}

		FILE * fp;
		fp = fopen(filename, "rb");
		if(!fp) {
			ret = CAMERA_FILE_ERROR;
			break;
		}

		fread(offsets, sizeof(offsets[0]), LUX1310_HRES_INCREMENT, fp);
		fclose(fp);

		//Write the values into the sensor
		for(int i = 0; i < LUX1310_HRES_INCREMENT; i++) {
			setADCOffset(i, offsets[i]);
		}
		return SUCCESS;
	} while (0);

	/* Otherwise, if there is no cal, clear the offsets to zero. */
	for (int i = 0; i < LUX1310_HRES_INCREMENT; i++) {
		setADCOffset(i, 0);
	}
	return ret;
}

Int32 LUX1310::saveADCOffsets(const char *filename)
{
	Int16 offsets[LUX1310_HRES_INCREMENT];
	FILE *fp;

	qDebug("writing ADC offsets to %s", filename);
	fp = fopen(filename, "wb");
	if(!fp) {
		return CAMERA_FILE_ERROR;
	}

	for(int i = 0; i < LUX1310_HRES_INCREMENT; i++) {
		offsets[i] = offsetsA[i];
	}

	//Ugly print to keep it all on one line
	qDebug() << "Saving offsets to file" << filename << "Offsets:"
			 << offsets[0] << offsets[1] << offsets[2] << offsets[3]
			 << offsets[4] << offsets[5] << offsets[6] << offsets[7]
			 << offsets[8] << offsets[9] << offsets[10] << offsets[11]
			 << offsets[12] << offsets[13] << offsets[14] << offsets[15];

	//Store to file
	fwrite(offsets, sizeof(offsets[0]), LUX1310_HRES_INCREMENT, fp);
	fclose(fp);

	return SUCCESS;
}

//Generate a filename string used for calibration values that is specific to the current gain and wavetable settings
std::string LUX1310::getFilename(const char * filename, const char * extension)
{
	const char *gName = "";
	char wtName[16];

	switch(gain)
	{
		case LUX1310_GAIN_1:		gName = LUX1310_GAIN_1_FN;		break;
		case LUX1310_GAIN_2:		gName = LUX1310_GAIN_2_FN;		break;
		case LUX1310_GAIN_4:		gName = LUX1310_GAIN_4_FN;		break;
		case LUX1310_GAIN_8:		gName = LUX1310_GAIN_8_FN;		break;
		case LUX1310_GAIN_16:		gName = LUX1310_GAIN_16_FN;		break;
		default:					gName = "";						break;
	}

	snprintf(wtName, sizeof(wtName), "WT%d", wavetableSize);
	return std::string(filename) + "_" + gName + "_" + wtName + extension;
}


Int32 LUX1310::setGain(UInt32 gainSetting)
{
	switch(gainSetting)
	{
	case LUX1310_GAIN_1:	//1
		writeDACVoltage(VRSTB_VOLTAGE, 2.7);
		writeDACVoltage(VRST_VOLTAGE, 3.3);
		writeDACVoltage(VRSTH_VOLTAGE, 3.6);

		//Set Gain
		SCIWrite(0x51, 0x007F);	//gain selection sampling cap (11)	12 bit
		SCIWrite(0x52, 0x007F);	//gain selection feedback cap (8) 7 bit
		SCIWrite(0x53, 0x03);	//Serial gain
	break;

	case LUX1310_GAIN_2:	//2
		writeDACVoltage(VRSTB_VOLTAGE, 2.7);
		writeDACVoltage(VRST_VOLTAGE, 3.3);
		writeDACVoltage(VRSTH_VOLTAGE, 3.6);

		//Set Gain
		SCIWrite(0x51, 0x0FFF);	//gain selection sampling cap (11)	12 bit
		SCIWrite(0x52, 0x007F);	//gain selection feedback cap (8) 7 bit
		SCIWrite(0x53, 0x03);	//Serial gain
	break;

	case LUX1310_GAIN_4:	//4
		writeDACVoltage(VRSTB_VOLTAGE, 2.7);
		writeDACVoltage(VRST_VOLTAGE, 3.3);
		writeDACVoltage(VRSTH_VOLTAGE, 3.6);

		//Set Gain
		SCIWrite(0x51, 0x0FFF);	//gain selection sampling cap (11)	12 bit
		SCIWrite(0x52, 0x007F);	//gain selection feedback cap (8) 7 bit
		SCIWrite(0x53, 0x00);	//Serial gain
	break;

	case LUX1310_GAIN_8:	//8
		writeDACVoltage(VRSTB_VOLTAGE, 1.7);
		writeDACVoltage(VRST_VOLTAGE, 2.3);
		writeDACVoltage(VRSTH_VOLTAGE, 2.6);

		//Set Gain
		SCIWrite(0x51, 0x0FFF);	//gain selection sampling cap (11)	12 bit
		SCIWrite(0x52, 0x0007);	//gain selection feedback cap (8) 7 bit
		SCIWrite(0x53, 0x00);	//Serial gain
	break;

	case LUX1310_GAIN_16:	//16
		writeDACVoltage(VRSTB_VOLTAGE, 1.7);
		writeDACVoltage(VRST_VOLTAGE, 2.3);
		writeDACVoltage(VRSTH_VOLTAGE, 2.6);

		//Set Gain
		SCIWrite(0x51, 0x0FFF);	//gain selection sampling cap (11)	12 bit
		SCIWrite(0x52, 0x0001);	//gain selection feedback cap (8) 7 bit
		SCIWrite(0x53, 0x00);	//Serial gain
	break;
	}

	gain = gainSetting;
	return SUCCESS;
}


// GR
// BG
//

UInt8 LUX1310::getFilterColor(UInt32 h, UInt32 v)
{
	if((v & 1) == 0)	//If on an even line
		return ((h & 1) == 0) ? FILTER_COLOR_GREEN : FILTER_COLOR_RED;
	else	//Odd line
		return ((h & 1) == 0) ? FILTER_COLOR_BLUE : FILTER_COLOR_GREEN;

}

Int32 LUX1310::setABNDelayClocks(UInt32 ABNOffset)
{
	gpmc->write16(SENSOR_MAGIC_START_DELAY_ADDR, ABNOffset);
	return SUCCESS;
}
