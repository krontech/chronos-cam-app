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
#include "lux2810.h"

#include <QSettings>

// LUX2810 44 clock wavetable.
// Format is 16-bit address (upper word), 16-bit data (lower word)
UInt32 LUX2810_WT44[] = {0x2000009C, 0x2020201C, 0x2040205C, 0x2060205C, 0x2080205C, 0x20A0205C, 0x20C0205C, 0x20E0205C,
                       0x2100205C, 0x2120205C, 0x2140205C, 0x2160205C, 0x2180205C, 0x21A02054, 0x21C03463, 0x21E0346B,
                       0x2200346B, 0x2220346B, 0x2240346B, 0x2260346B, 0x2280346B, 0x22A0346B, 0x22C0346B, 0x22E0346B,
                       0x2300346B, 0x2320346B, 0x2340346B, 0x2360346B, 0x2380346B, 0x23A0346B, 0x23C0346B, 0x23E0346B,
                       0x2400346B, 0x2420346B, 0x2440246B, 0x2460246B, 0x2480246B, 0x24A0246B, 0x24C0246B, 0x24E0246B,
                       0x2500206B, 0x2520206B, 0x2540206B, 0x25602061, 0x25800000, 0x25A00000, 0x25C00000, 0x25E00000,
                       0x26000000, 0x26200000, 0x26400000, 0x26600000, 0x26800000, 0x26A00000, 0x26C00000, 0x26E00000,
                       0x27000000, 0x27200000, 0x27400000, 0x27600000, 0x27800000, 0x27A00000, 0x27C00000, 0x27E00000,
                       0x28000000, 0x28200000, 0x28400000, 0x28600000, 0x28800000, 0x28A00000, 0x28C00000, 0x28E00000,
                       0x29000000, 0x29200000, 0x29400000, 0x29600000, 0x29800000, 0x29A00000, 0x29C00000, 0x29E00000,
                       0x2A000000, 0x2A200000, 0x2A400000, 0x2A600000, 0x2A800000, 0x2AA00000, 0x2AC00000, 0x2AE00000,
                       0x2B000000, 0x2B200000, 0x2B400000, 0x2B600000, 0x2B800000, 0x2BA00000, 0x2BC00000, 0x2BE00000,
                       0x2C000000, 0x2C200000, 0x2C400000, 0x2C600000, 0x2C800000, 0x2CA00000, 0x2CC00000, 0x2CE00000,
                       0x2D000000, 0x2D200000, 0x2D400000, 0x2D600000, 0x2D800000, 0x2DA00000, 0x2DC00000, 0x2DE00000,
                       0x2E000000, 0x2E200000, 0x2E400000, 0x2E600000, 0x2E800000, 0x2EA00000, 0x2EC00000, 0x2EE00000,
                       0x2F000000, 0x2F200000, 0x2F400000, 0x2F600000, 0x2F800000, 0x2FA00000, 0x2FC00000, 0x2FE00000,
                       0x40100006, 0x40300006, 0x40500002, 0x40700001, 0x40900009, 0x40B00009, 0x40D00009, 0x40F00009,
                       0x41100009, 0x41300009, 0x41500009, 0x41700009, 0x41900009, 0x41B00009, 0x41D00006, 0x41F00006,
                       0x42100006, 0x42300006, 0x42500006, 0x42700006, 0x42900006, 0x42B00006, 0x42D00006, 0x42F00006,
                       0x43100006, 0x43300006, 0x43500006, 0x43700006, 0x43900006, 0x43B00006, 0x43D00006, 0x43F00006,
                       0x44100006, 0x44300006, 0x44500006, 0x44700006, 0x44900006, 0x44B00006, 0x44D00006, 0x44F00006,
                       0x45100006, 0x45300006, 0x45500006, 0x45700006, 0x45900000, 0x45B00000, 0x45D00000, 0x45F00000,
                       0x46100000, 0x46300000, 0x46500000, 0x46700000, 0x46900000, 0x46B00000, 0x46D00000, 0x46F00000,
                       0x47100000, 0x47300000, 0x47500000, 0x47700000, 0x47900000, 0x47B00000, 0x47D00000, 0x47F00000,
                       0x48100000, 0x48300000, 0x48500000, 0x48700000, 0x48900000, 0x48B00000, 0x48D00000, 0x48F00000,
                       0x49100000, 0x49300000, 0x49500000, 0x49700000, 0x49900000, 0x49B00000, 0x49D00000, 0x49F00000,
                       0x4A100000, 0x4A300000, 0x4A500000, 0x4A700000, 0x4A900000, 0x4AB00000, 0x4AD00000, 0x4AF00000,
                       0x4B100000, 0x4B300000, 0x4B500000, 0x4B700000, 0x4B900000, 0x4BB00000, 0x4BD00000, 0x4BF00000,
                       0x4C100000, 0x4C300000, 0x4C500000, 0x4C700000, 0x4C900000, 0x4CB00000, 0x4CD00000, 0x4CF00000,
                       0x4D100000, 0x4D300000, 0x4D500000, 0x4D700000, 0x4D900000, 0x4DB00000, 0x4DD00000, 0x4DF00000,
                       0x4E100000, 0x4E300000, 0x4E500000, 0x4E700000, 0x4E900000, 0x4EB00000, 0x4ED00000, 0x4EF00000,
                       0x4F100000, 0x4F300000, 0x4F500000, 0x4F700000, 0x4F900000, 0x4FB00000, 0x4FD00000, 0x4FF00000,
                       0x40081F00, 0x40281F00, 0x40481C00, 0x40681800, 0x40889000, 0x40A8C000, 0x40C8C000, 0x40E8C000,
                       0x4108C000, 0x4128C000, 0x4148C000, 0x4168C000, 0x4188C000, 0x41A8A000, 0x41C80080, 0x41E80080,
                       0x42082080, 0x422800C0, 0x424800C0, 0x426820C0, 0x428800E0, 0x42A800E0, 0x42C820E0, 0x42E800F0,
                       0x430800F0, 0x432820F0, 0x434800F8, 0x436800F8, 0x438820F8, 0x43A800FC, 0x43C800FC, 0x43E820FC,
                       0x440800FE, 0x442800FE, 0x444820FE, 0x446800FF, 0x448820FF, 0x44A800FF, 0x44C820FF, 0x44E800FF,
                       0x450820FF, 0x452800FF, 0x454820FF, 0x456800FF, 0x45880000, 0x45A80000, 0x45C80000, 0x45E80000,
                       0x46080000, 0x46280000, 0x46480000, 0x46680000, 0x46880000, 0x46A80000, 0x46C80000, 0x46E80000,
                       0x47080000, 0x47280000, 0x47480000, 0x47680000, 0x47880000, 0x47A80000, 0x47C80000, 0x47E80000,
                       0x48080000, 0x48280000, 0x48480000, 0x48680000, 0x48880000, 0x48A80000, 0x48C80000, 0x48E80000,
                       0x49080000, 0x49280000, 0x49480000, 0x49680000, 0x49880000, 0x49A80000, 0x49C80000, 0x49E80000,
                       0x4A080000, 0x4A280000, 0x4A480000, 0x4A680000, 0x4A880000, 0x4AA80000, 0x4AC80000, 0x4AE80000,
                       0x4B080000, 0x4B280000, 0x4B480000, 0x4B680000, 0x4B880000, 0x4BA80000, 0x4BC80000, 0x4BE80000,
                       0x4C080000, 0x4C280000, 0x4C480000, 0x4C680000, 0x4C880000, 0x4CA80000, 0x4CC80000, 0x4CE80000,
                       0x4D080000, 0x4D280000, 0x4D480000, 0x4D680000, 0x4D880000, 0x4DA80000, 0x4DC80000, 0x4DE80000,
                       0x4E080000, 0x4E280000, 0x4E480000, 0x4E680000, 0x4E880000, 0x4EA80000, 0x4EC80000, 0x4EE80000,
                       0x4F080000, 0x4F280000, 0x4F480000, 0x4F680000, 0x4F880000, 0x4FA80000, 0x4FC80000, 0x4FE80000,
                       0x40000000, 0x40200000, 0x40400000, 0x40600000, 0x40800000, 0x40A00000, 0x40C00000, 0x40E00000,
                       0x41000000, 0x41200000, 0x41400000, 0x41600000, 0x41800800, 0x41A00800, 0x41C00800, 0x41E00800,
                       0x42000000, 0x42200400, 0x42400400, 0x42600000, 0x42800200, 0x42A00200, 0x42C00000, 0x42E00100,
                       0x43000100, 0x43200000, 0x43400080, 0x43600080, 0x43800000, 0x43A00040, 0x43C00040, 0x43E00000,
                       0x44000020, 0x44200020, 0x44400000, 0x44600010, 0x44800000, 0x44A08008, 0x44C08000, 0x44E0C004,
                       0x4500C000, 0x4520E002, 0x4540E000, 0x4560F001, 0x45800000, 0x45A00000, 0x45C00000, 0x45E00000,
                       0x46000000, 0x46200000, 0x46400000, 0x46600000, 0x46800000, 0x46A00000, 0x46C00000, 0x46E00000,
                       0x47000000, 0x47200000, 0x47400000, 0x47600000, 0x47800000, 0x47A00000, 0x47C00000, 0x47E00000,
                       0x48000000, 0x48200000, 0x48400000, 0x48600000, 0x48800000, 0x48A00000, 0x48C00000, 0x48E00000,
                       0x49000000, 0x49200000, 0x49400000, 0x49600000, 0x49800000, 0x49A00000, 0x49C00000, 0x49E00000,
                       0x4A000000, 0x4A200000, 0x4A400000, 0x4A600000, 0x4A800000, 0x4AA00000, 0x4AC00000, 0x4AE00000,
                       0x4B000000, 0x4B200000, 0x4B400000, 0x4B600000, 0x4B800000, 0x4BA00000, 0x4BC00000, 0x4BE00000,
                       0x4C000000, 0x4C200000, 0x4C400000, 0x4C600000, 0x4C800000, 0x4CA00000, 0x4CC00000, 0x4CE00000,
                       0x4D000000, 0x4D200000, 0x4D400000, 0x4D600000, 0x4D800000, 0x4DA00000, 0x4DC00000, 0x4DE00000,
                       0x4E000000, 0x4E200000, 0x4E400000, 0x4E600000, 0x4E800000, 0x4EA00000, 0x4EC00000, 0x4EE00000,
                       0x4F000000, 0x4F200000, 0x4F400000, 0x4F600000, 0x4F800000, 0x4FA00000, 0x4FC00000, 0x4FE00000};


#define round(x) (floor(x + 0.5))
//#define SCI_DEBUG_PRINTS

LUX2810::LUX2810()
{
	spi = new SPI();
	startDelaySensorClocks = LUX2810_MAGIC_ABN_DELAY;
}

LUX2810::~LUX2810()
{
	delete spi;
}

CameraErrortype LUX2810::init(GPMC * gpmc_inst)
{
	CameraErrortype retVal;
	retVal = spi->Init(IMAGE_SENSOR_SPI, 16, 1000000, false, true);	//Invert clock phase
	if(SUCCESS != retVal)
		return retVal;

	dacCSFD = open("/sys/class/gpio/gpio33/value", O_WRONLY);
	if (-1 == dacCSFD)
		return LUX1310_FILE_ERROR;

	sensorCSFD = open("/sys/class/gpio/gpio34/value", O_WRONLY);
	if (-1 == dacCSFD)
		return LUX1310_FILE_ERROR;

	gpmc = gpmc_inst;

	retVal = initSensor();
	//mem problem before this
	if(SUCCESS != retVal)
		return retVal;

	return SUCCESS;
}

CameraErrortype LUX2810::initSensor()
{
	wavetableSize = 44;
	gain = LUX2810_GAIN_1;

	gpmc->write32(IMAGER_FRAME_PERIOD_ADDR, 100*4000);	//Disable integration
	gpmc->write32(IMAGER_INT_TIME_ADDR, 100*4100);

	initDAC();

	initLUX2810();

	delayms(10);

    gpmc->write32(IMAGER_FRAME_PERIOD_ADDR, 100000000/100);	//Enable integration
    gpmc->write32(IMAGER_INT_TIME_ADDR, 100000000/200);

	delayms(50);

	currentRes.hRes = LUX2810_MAX_H_RES;
	currentRes.vRes = LUX2810_MAX_V_RES;
	currentRes.hOffset = 0;
	currentRes.vOffset = 0;
	currentRes.vDarkRows = 0;
	currentRes.bitDepth = LUX2810_BITS_PER_PIXEL;
	setFramePeriod(getMinFramePeriod(&currentRes), &currentRes);
	//mem problem before this
	setIntegrationTime(getMaxIntegrationTime(currentPeriod, &currentRes), &currentRes);

	return SUCCESS;
}

void LUX2810::SCIWrite(UInt8 address, UInt16 data)
{
    return;
}

CameraErrortype LUX2810::autoPhaseCal(void)
{
	UInt16 valid = 0;
	UInt32 valid32;

	setClkPhase(0);
	setClkPhase(1);

	setClkPhase(0);


	uint dataCorrect = getDataCorrect();
	qDebug() << "datacorrect" << dataCorrect;
	return SUCCESS;
}

Int32 LUX2810::seqOnOff(bool on)
{
	if (on) {
		gpmc->write32(IMAGER_INT_TIME_ADDR, currentExposure);
	} else {
		gpmc->write32(IMAGER_INT_TIME_ADDR, 0);	//Disable integration
	}
	return SUCCESS;
}

void LUX2810::setReset(bool reset)
{
		gpmc->write16(IMAGE_SENSOR_CONTROL_ADDR, (gpmc->read16(IMAGE_SENSOR_CONTROL_ADDR) & ~IMAGE_SENSOR_RESET_MASK) | (reset ? IMAGE_SENSOR_RESET_MASK : 0));
}

void LUX2810::setClkPhase(UInt8 phase)
{
		gpmc->write16(IMAGE_SENSOR_CLK_PHASE_ADDR, phase);
}

UInt8 LUX2810::getClkPhase(void)
{
		return gpmc->read16(IMAGE_SENSOR_CLK_PHASE_ADDR);
}

UInt32 LUX2810::getDataCorrect(void)
{
	return gpmc->read32(IMAGE_SENSOR_DATA_CORRECT_ADDR);
}

void LUX2810::setSyncToken(UInt16 token)
{
	gpmc->write16(IMAGE_SENSOR_SYNC_TOKEN_ADDR, token);
}

FrameGeometry LUX2810::getMaxGeometry(void)
{
	FrameGeometry size = {
		.hRes = LUX2810_MAX_H_RES,
		.vRes = LUX2810_MAX_V_RES,
		.hOffset = 0,
		.vOffset = 0,
		.vDarkRows = LUX2810_MAX_V_DARK,
		.bitDepth = LUX2810_BITS_PER_PIXEL,
	};
	return size;
}

void LUX2810::setResolution(FrameGeometry *size)
{
	UInt32 hStart = (size->hOffset / LUX2810_HRES_INCREMENT) * LUX2810_HRES_INCREMENT;
	UInt32 hEnd = hStart + size->hRes;
	UInt32 vLastRow = LUX2810_MAX_V_RES + (LUX2810_BOUNDARY_ROWS*2) + LUX2810_HIGH_DARK_ROWS;

	/* Windowed operation - add extra offset to put the readout at the centre. */
	LUX2810RegWrite(0x06, (LUX2810_LEFT_DARK_COLUMNS + LUX2810_BOUNDARY_COLUMNS + 0x40) + hStart);
	LUX2810RegWrite(0x07, (LUX2810_LEFT_DARK_COLUMNS + LUX2810_BOUNDARY_COLUMNS + 0x40) + hEnd - 1);
	LUX2810RegWrite(0x08, (LUX2810_BOUNDARY_ROWS + 0x100) + size->vOffset);
	LUX2810RegWrite(0x09, (LUX2810_BOUNDARY_ROWS + 0x100) + size->vOffset + size->vRes - 1);
	if (size->vDarkRows) {
		LUX2810RegWrite(0x0A, (vLastRow * 2) - size->vDarkRows);
	}
	SCIWrite(0x0B, size->vDarkRows);

	memcpy(&currentRes, size, sizeof(FrameGeometry));
}

bool LUX2810::isValidResolution(FrameGeometry *size)
{
	/* Enforce resolution limits. */
	if ((size->hRes < LUX2810_MIN_HRES) || (size->hRes + size->hOffset > LUX2810_MAX_H_RES)) {
		return false;
	}
	if ((size->vRes < LUX2810_MIN_VRES) || (size->vRes + size->vOffset > LUX2810_MAX_V_RES)) {
		return false;
	}
	if (size->vDarkRows > LUX2810_MAX_V_DARK) {
		return false;
	}
	if (size->bitDepth != LUX2810_BITS_PER_PIXEL) {
		return false;
	}
	/* Enforce minimum pixel increments. */
	if ((size->hRes % LUX2810_HRES_INCREMENT) || (size->hOffset % LUX2810_HRES_INCREMENT)) {
		return false;
	}
	if ((size->vRes % LUX2810_VRES_INCREMENT) || (size->vOffset % LUX2810_VRES_INCREMENT)) {
		return false;
	}
	if (size->vDarkRows % LUX2810_VRES_INCREMENT) {
		return false;
	}
	/* Otherwise, the resultion and offset are valid. */
	return true;
}

//Used by init functions only
UInt32 LUX2810::getMinFramePeriod(FrameGeometry *frameSize)
{
	UInt32 wtSize = 44; /* Only 1080p supported for now. */

	if(!isValidResolution(frameSize))
		return 0;

	double tRead = (double)(frameSize->hRes / LUX2810_HRES_INCREMENT) * LUX2810_CLOCK_PERIOD;
	double tHBlank = 2.0 * LUX2810_CLOCK_PERIOD;
	double tWavetable = wtSize * LUX2810_CLOCK_PERIOD;
	double tRow = max(tRead+tHBlank, tWavetable+3*LUX2810_CLOCK_PERIOD);
	double tTx = 50 * LUX2810_CLOCK_PERIOD;
	double tFovf = 50 * LUX2810_CLOCK_PERIOD;
	double tFovb = (50) * LUX2810_CLOCK_PERIOD;//Duration between PRSTN falling and TXN falling (I think)
	double tFrame = tRow * (frameSize->vRes + frameSize->vDarkRows) + tTx + tFovf + tFovb;
	qDebug() << "getMinFramePeriod:" << tFrame;
	return (UInt64)(ceil(tFrame * 100000000.0));
}

UInt32 LUX2810::getFramePeriod(void)
{
	return currentPeriod;
}

UInt32 LUX2810::getActualFramePeriod(double targetPeriod, FrameGeometry *size)
{
	//Round to nearest timing clock period
	UInt32 clocks = round(targetPeriod * LUX2810_TIMING_CLOCK);
	UInt32 minPeriod = getMinFramePeriod(size);
	UInt32 maxPeriod = LUX2810_MAX_SLAVE_PERIOD;

	return within(clocks, minPeriod, maxPeriod);
}

UInt32 LUX2810::setFramePeriod(UInt32 period, FrameGeometry *size)
{
	qDebug() << "Requested period" << period;
	UInt32 minPeriod = getMinFramePeriod(size);
	UInt32 maxPeriod = LUX2810_MAX_SLAVE_PERIOD;

	currentPeriod = within(period, minPeriod, maxPeriod);

	// Set the timing generator to handle the frame and line period
	gpmc->write16(SENSOR_LINE_PERIOD_ADDR, max((size->hRes / LUX2810_HRES_INCREMENT)+2, (wavetableSize + 3)) - 1);
	gpmc->write32(IMAGER_FRAME_PERIOD_ADDR, period);
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
UInt32 LUX2810::getMaxIntegrationTime(UInt32 period, FrameGeometry *size)
{
	return period - 500;
}

/* getMaxCurrentIntegrationTime
 *
 * Gets the actual maximum integration for the current settings
 *
 * returns: Maximum integration time
 */
double LUX2810::getMaxCurrentIntegrationTime(void)
{
	return (double)getMaxIntegrationTime(currentPeriod, &currentRes) / LUX2810_TIMING_CLOCK;
}

/* getActualIntegrationTime
 *
 * Gets the actual integration time that the sensor can be set to which is as close as possible to desired
 *
 * intTime:	Desired integration time in seconds
 *
 * returns: Actual closest integration time
 */
UInt32 LUX2810::getActualIntegrationTime(double target, UInt32 period, FrameGeometry *size)
{
	//Round to nearest timing clock period
	UInt32 intTime = round(target * LUX2810_TIMING_CLOCK);
	UInt32 minIntTime = LUX2810_MIN_INT_TIME;
	UInt32 maxIntTime = getMaxIntegrationTime(period, size);

	return within(intTime, minIntTime, maxIntTime);
}

/* setIntegrationTime
 *
 * Sets the integration time of the image sensor to a value as close as possible to requested
 *
 * intTime:	Desired integration time in clocks
 *
 * returns: Actual integration time that was set
 */
UInt32 LUX2810::setIntegrationTime(UInt32 intTime, FrameGeometry *size)
{
	//Set integration time to within limits
	UInt32 maxIntTime = getMaxIntegrationTime(currentPeriod, size);
	UInt32 minIntTime = LUX2810_MIN_INT_TIME;
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
UInt32 LUX2810::getIntegrationTime(void)
{
	return currentExposure;
}

void LUX2810::setSlaveExposure(UInt32 exposure)
{
	//hack to fix line issue. Not perfect, need to properly register this on the sensor clock.
	double linePeriod = max((currentRes.hRes / LUX2810_HRES_INCREMENT)+2, (wavetableSize + 3)) * 1.0/LUX2810_SENSOR_CLOCK;	//Line period in seconds
	UInt32 startDelay = (double)startDelaySensorClocks * LUX2810_TIMING_CLOCK / LUX2810_SENSOR_CLOCK;
	double targetExp = (double)exposure / 100000000.0;
	UInt32 expLines = round(targetExp / linePeriod);

	exposure = startDelay + (linePeriod * expLines * 100000000.0);
	//qDebug() << "linePeriod" << linePeriod << "startDelaySensorClocks" << startDelaySensorClocks << "startDelay" << startDelay
	//		 << "targetExp" << targetExp << "expLines" << expLines << "exposure" << exposure;
	gpmc->write32(IMAGER_INT_TIME_ADDR, exposure);
	currentExposure = exposure;
}

//Set up DAC
void LUX2810::initDAC()
{
	//Put DAC in Write through mode (DAC channels update immediately on register write)
	writeDACSPI(0x9000, 0x9000, 0x9000);
}

//Write the data into the selected channel
void LUX2810::writeDAC(UInt16 data, UInt8 channel)
{
    UInt16 dacVal = ((channel & 0x7) << 12) | (data & 0x0FFF);
    UInt8 dacSelect = channel >> 3;

    switch(dacSelect)
    {
		case 0: writeDACSPI(dacVal, LUX2810_DAC_NOP_COMMAND, LUX2810_DAC_NOP_COMMAND); break;
		case 1: writeDACSPI(LUX2810_DAC_NOP_COMMAND, dacVal, LUX2810_DAC_NOP_COMMAND); break;
		case 2: writeDACSPI(LUX2810_DAC_NOP_COMMAND, LUX2810_DAC_NOP_COMMAND, dacVal); break;
    }

}

const float LUX2810_DAC_SCALES[] = {

		LUX2810_VDR1_SCALE,
		LUX2810_VRST_SCALE,
		LUX2810_VAB_SCALE,
		LUX2810_VRDH_SCALE,
		LUX2810_VDR2_SCALE,
		LUX2810_VABL_SCALE,
        0.0, //NC
        0.0,//NC
        //Second DAC
		LUX2810_VADC1_SCALE,
		LUX2810_VADC2_SCALE,
		LUX2810_VADC3_SCALE,
		LUX2810_VCM_SCALE,
		LUX2810_VADC2_FT_SCALE,
		LUX2810_VADC3_FT_SCALE,
        0.0,//NC
        0.0,//NC
        //Third DAC
		LUX2810_VREFN_SCALE,
		LUX2810_VREFP_SCALE,
		LUX2810_VTSTH_SCALE,
		LUX2810_VTSTL_SCALE,
		LUX2810_VRSTB_SCALE,
		LUX2810_VRSTH_SCALE,
		LUX2810_VRSTL_SCALE
};

void LUX2810::writeDACVoltage(UInt8 channel, float voltage)
{
    float dacRaw = (voltage * LUX2810_DAC_SCALES[channel]);

    if(floor(dacRaw) > 4095.0)
    {
        qDebug() << "--- LUX2810 --- WARNING: DAC voltage of" << voltage << "on channel" << channel << "is out of range. Clipped to maximum value.";
        dacRaw = 4095.0;
    }

    writeDAC((UInt16)dacRaw, channel);
}

//Performs an SPI write to the DAC
int LUX2810::writeDACSPI(UInt16 data0, UInt16 data1, UInt16 data2)
{
	UInt8 tx[6];
	UInt8 rx[sizeof(tx)];
	int retval;

	tx[5] = data0 >> 8;
	tx[4] = data0 & 0xFF;
	tx[3] = data1 >> 8;
	tx[2] = data1 & 0xFF;
	tx[1] = data2 >> 8;
	tx[0] = data2 & 0xFF;

	setDACCS(false);
	//delayms(1);
	retval = spi->Transfer((uint64_t) tx, (uint64_t) rx, sizeof(tx));
	//delayms(1);
	setDACCS(true);
	return retval;
}

//Performs an SPI write to the image sensor
int LUX2810::writeSensorSPI(UInt16 addr, UInt16 data)
{
    UInt8 tx[4];
    UInt8 rx[sizeof(tx)];
    int retval;

    tx[3] = data >> 8;
    tx[2] = data & 0xFF;
    tx[1] = (addr & 0x7FFF) >> 8;   //Upper bit of addres is R/!W
    tx[0] = addr & 0xFF;

    setSensorCS(false);
    //delayms(1);
    retval = spi->Transfer((uint64_t) tx, (uint64_t) rx, sizeof(tx), false, false);
    //delayms(1);
    setSensorCS(true);
    return retval;
}

int LUX2810::readSensorSPI(UInt16 addr, UInt16 & data)
{
    UInt8 tx[4];
    UInt8 rx[sizeof(tx)];
    int retval;

    tx[3] = 0;
    tx[2] = 0;
    tx[1] = ((addr & 0x7FFF) | 0x8000) >> 8;   //Upper bit of addres is R/!W
    tx[0] = addr & 0xFF;

    setSensorCS(false);
    //delayms(1);
    retval = spi->Transfer((uint64_t) tx, (uint64_t) rx, sizeof(tx), false, false);
    //delayms(1);
    setSensorCS(true);
    data = rx[3] << 8 | rx[2];
    return retval;
}

void LUX2810::setDACCS(bool on)
{
	lseek(dacCSFD, 0, SEEK_SET);
	write(dacCSFD, on ? "1" : "0", 1);
}

void LUX2810::setSensorCS(bool on)
{
    lseek(sensorCSFD, 0, SEEK_SET);
    write(sensorCSFD, on ? "1" : "0", 1);
}

int LUX2810::LUX2810RegWrite(UInt16 addr, UInt16 data)
{
    return writeSensorSPI((addr & 0xFF) << 4, data);
}

int LUX2810::LUX2810RegRead(UInt16 addr, UInt16 & data)
{
    return readSensorSPI((addr & 0xFF) << 4, data);
}

int LUX2810::LUX2810LoadWavetable(UInt32 * wavetable, UInt32 length)
{
    int retVal;
    for(int i = 0; i < length; i++)
    {
        retVal = writeSensorSPI((wavetable[i] >> 16) & 0x7FFF, wavetable[i] & 0xFFFF);
        if(SUCCESS != retVal)
            return retVal;
    }
    return SUCCESS;
}

unsigned int LUX2810::enableAnalogTestMode(void)
{
	LUX2810RegWrite(0x6B, 0xB111);
	return 64;
}

void LUX2810::disableAnalogTestMode(void)
{
	LUX2810RegWrite(0x6B, 0xB101);
}

void LUX2810::setAnalogTestVoltage(unsigned int voltage)
{
	writeDACVoltage(LUX2810_VTSTH_VOLTAGE, 1.5 + (voltage / 64.0));
}

//Remaps data channels to ADC channel index
const char adcChannelRemap[] = {0, 8, 16, 24, 4, 12,20, 28,  2,10, 18, 26,  6, 14, 22, 30, 1, 9,  17, 25, 5, 13, 21, 29, 3, 11, 19, 27, 7, 15, 23, 31};

//Sets ADC offset for one channel
//Converts the input 2s complement value to the sensors's weird sign bit plus value format (sort of like float, with +0 and -0)
void LUX2810::setADCOffset(UInt8 channel, Int16 offset)
{
	/* TODO: Implement Me! Or Don't... */
}

//This doesn't seem to work. Sensor locks up
Int32 LUX2810::doAutoADCOffsetCalibration(void)
{
    /*
    SCIWrite(0x01, 0x0010); // disable the internal timing engine

    //SCIWrite(0x2A, 0x89A); // Address of first dark row to read out (half way through dark rows)
    //SCIWrite(0x2B, 1); // Readout 5 dark rows
    delayms(10);
    SCIWrite(0x01, 0x0011); // enable the internal timing engine
    delayms(10);
    */
    SCIWrite(0x04, 0x0001); // switch to datapath register space
    SCIWrite(0x0E, 0x0001); // Enable application of ADC offsets during v blank
    SCIWrite(0x0D, 0x0020); // ADC offset target
    SCIWrite(0x0A, 0x0001); // Start ADC Offset calibration
    delayms(2000);
    SCIWrite(0x04, 0x0000); // switch back to sensor register space

    return SUCCESS;
}

//Generate a filename string used for calibration values that is specific to the current gain and wavetable settings
std::string LUX2810::getFilename(const char * filename, const char * extension)
{
	return std::string(filename) + extension;
}


Int32 LUX2810::setGain(UInt32 gainSetting)
{
	return SUCCESS;
}


// GR
// BG
//

UInt8 LUX2810::getFilterColor(UInt32 h, UInt32 v)
{
	if((v & 1) == 0)	//If on an even line
		return ((h & 1) == 0) ? FILTER_COLOR_GREEN : FILTER_COLOR_RED;
	else	//Odd line
		return ((h & 1) == 0) ? FILTER_COLOR_BLUE : FILTER_COLOR_GREEN;

}

Int32 LUX2810::initLUX2810()
{

	writeDACVoltage(LUX2810_VDR1_VOLTAGE, 2.5);
	writeDACVoltage(LUX2810_VRST_VOLTAGE, 3.3);
	writeDACVoltage(LUX2810_VAB_VOLTAGE, 3.3);
	writeDACVoltage(LUX2810_VRDH_VOLTAGE, 3.3);
	writeDACVoltage(LUX2810_VDR2_VOLTAGE, 2.5);
	writeDACVoltage(LUX2810_VABL_VOLTAGE, 0.6);
	writeDACVoltage(LUX2810_VADC1_VOLTAGE, 1.8);
	writeDACVoltage(LUX2810_VADC2_VOLTAGE, 0.1125);
	writeDACVoltage(LUX2810_VADC3_VOLTAGE, 0.0);
	writeDACVoltage(LUX2810_VCM_VOLTAGE, 0.9);
	writeDACVoltage(LUX2810_VADC2_FT_VOLTAGE, 0);
	writeDACVoltage(LUX2810_VADC3_FT_VOLTAGE, 0.014063-0.003);  //Fudge factor added due to Opamp offest
	writeDACVoltage(LUX2810_VREFN_VOLTAGE, 0.45);
	writeDACVoltage(LUX2810_VREFP_VOLTAGE, 1.35);
	writeDACVoltage(LUX2810_VTSTH_VOLTAGE, 2.5);
	writeDACVoltage(LUX2810_VTSTL_VOLTAGE, 1.5);
	writeDACVoltage(LUX2810_VRSTB_VOLTAGE, 2.7);
	writeDACVoltage(LUX2810_VRSTH_VOLTAGE, 3.6);
	writeDACVoltage(LUX2810_VRSTL_VOLTAGE, 0.7);


    delayms(10);		//Settling time


    setReset(true);

    setReset(false);

    delayms(1);		//Seems to be required for SCI clock to get running

    LUX2810RegWrite(0x15, 0x00A0); // increase ABN width in ABN hold mode to avoid being squeeze to 0
    LUX2810RegWrite(0x1C, 0x0028); // reduce TXN width as the sensor needs >400nsec
    LUX2810RegWrite(0x23, 0x0028); // reduce PRSTN width as the sensor needs >400nsec
    LUX2810RegWrite(0x0D, 0x001C); // increase ABN hold delay so that it does not overlap with digital readout
    LUX2810RegWrite(0x6D, 0x7A77); //
    LUX2810RegWrite(0x6E, 0x0077); //
    LUX2810RegWrite(0x7E, 0x0FFF); // set gain of 2 to match pixel output and ADC range
    LUX2810RegWrite(0x7F, 0x0FFF); // set gain of 2 to match pixel output and ADC range
    LUX2810RegWrite(0x80, 0x0FFF); // set gain of 2 to match pixel output and ADC range
    LUX2810RegWrite(0x81, 0x0FFF); // set gain of 2 to match pixel output and ADC range
    LUX2810RegWrite(0x8B, 0x0080); // change internal timing
    LUX2810RegWrite(0xA1, 0x010F); //

    //Enable training pattern
    LUX2810RegWrite(0x72, 0x1FC0);  //Custom pattern always enabled, pattern set to 0xFC0
    LUX2810RegWrite(0x57, 0xFC0);  //Pclk output 0xFC0 on linevalid
    LUX2810RegWrite(0x58, 0xFC0);  //Pclk output 0xFC0 on vblank
    LUX2810RegWrite(0x59, 0xFC0);  //Pclk output 0xFC0 on hblank
    LUX2810RegWrite(0x5A, 0xFC0);  //Pclk output 0xFC0 on optical_black

    //Set dclk output delay
    //LUX2810RegWrite(0x63, 0x0011);  //DCLK delay to ~500ps

    //Check receiver sync
    autoPhaseCal();

    //Disable training pattern
    LUX2810RegWrite(0x72, 0x0FC0);  //Custom pattern in blanking, pattern set to 0xFC0
    LUX2810RegWrite(0x57, 0xFC0);  //Pclk output 0xFC0 on linevalid
    LUX2810RegWrite(0x58, 0xF00);  //Pclk output 0xF00 on vblank
    LUX2810RegWrite(0x59, 0xF80);  //Pclk output 0xF80 on hblank
    LUX2810RegWrite(0x5A, 0xFE0);  //Pclk output 0xFE0 on optical_black

    //Load the wavetable
    LUX2810LoadWavetable(LUX2810_WT44, sizeof(LUX2810_WT44)/sizeof(LUX2810_WT44[0]));


    //Set resolution to 1920x1080
    LUX2810RegWrite(0x06, 0x80);  //X window start
    LUX2810RegWrite(0x07, 0x7FF);  //X window end   1920 active
    LUX2810RegWrite(0x08, 0x10);  //Y window start
    LUX2810RegWrite(0x09, 0x447);  //Y window end   1080 active

    LUX2810RegWrite(0x01, 0x4101); // enable timing engine, turn on abn hold and external shutter mode

    return SUCCESS;
}
