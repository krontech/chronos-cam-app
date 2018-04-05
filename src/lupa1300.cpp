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

#include "spi.h"

#include "defines.h"
#include "cameraRegisters.h"



#include "types.h"
#include "lupa1300.h"
/*
 *
 * LUPA1300-2 defaults
 * In granularity clock cycles(63MHz)
 * 126 clocks per line
 * 2.057415ms/frame
 *
 *Changing from 14 to 9 ROT setting changed it to 124 clocks per line
 *Changing rowselect_timer from 9 to 6 changed it to 121 clocks per line
 *
 *Changing x res by 24 changed period by 32.5082us = 2048 clocks
 *
 *1.9760493 1296 1024	124491.1059
 *1.0983273 648 1024	69194.6199
 *519.7825	648 480		32746.2975
 *165.4935	336 240		10426.0905
 *
 *Frame rate equation
 *periods = (2*ts+ROT)*(tint_timer+res_length) +	//Row readout time * number of rows
 *			2*(54-ts) +								//Emperically determined fudge factor (two extra clocks for every timeslot not read out)
 *			(54*2+ROT) +							//Black cal line
 *			FOT										//Frame overhead time
 *
 *Where
 *		ROT = rot_timer + 4
 *		FOT = fot_timer + 29
 *		tint_timer = tint_timer register setting
 *		res_length = res_length register setting
 *		ts = number of x timeslots in window
 *		MAX_TS = maximum number of timeslots (54)
 *
 *simplified
 *
 *periods = (tint_timer+res_length)*(2*ts+ROT) - 2*ts + ROT + FOT + 4*MAX_TS
 *
 *G B
  R G
 *
 **/

#define round(x) (floor(x + 0.5))

#define SEQ_DEFAULT	(0x1 | 0x60*0)

LUPA1300::LUPA1300()
{
	spi = new SPI();
}

LUPA1300::~LUPA1300()
{
	delete spi;
}

CameraErrortype LUPA1300::init(GPMC * gpmc_inst)
{
	CameraErrortype retVal;
	retVal = spi->Init(IMAGE_SENSOR_SPI, IMAGE_SENSOR_SPI_BITS, IMAGE_SENSOR_SPI_SPEED);
	if(SUCCESS != retVal)
		return retVal;

	gpmc = gpmc_inst;

	retVal = initSensor();
	if(SUCCESS != retVal)
		return retVal;

	return SUCCESS;
}

CameraErrortype LUPA1300::initSensor()
{
	CameraErrortype retVal;
	setReset(true);
	setReset(false);

	//dumpRegisters();

	if(SUCCESS != (retVal = autoPhaseCal()))
		return retVal;

	setSyncToken(0x059);

	currentHRes = 1296;
	currentVRes = 1024;
	setFramePeriod(getMinFramePeriod(1296, 1024), 1296, 1024);
	setIntegrationTime((double)getMaxExposure(currentPeriod) / 100000000.0, 1296, 1024);

	writeSPI(58, 0x08+1+4);//Disable column FPN correction, delay events to next ROT

	for(int i = 0; i < 12; i++)
	{
		writeSPI(30+i*2, 0);  //Disable FPN correction
	}
	//writeSPI(30+8*2, 4);	//Test
	writeSPI(79, 0xFF);   //set integration time
	writeSPI(80, 0x03*1);
	writeSPI(81, 0xFF);   //set integration time
	writeSPI(82, 0x02);
	writeSPI(83, 0x66);   //set integration time
	writeSPI(84, 0x0);

	//Set ROT and FOT
	writeSPI(86, 0x9);	//ROT to 9
	writeSPI(87, 0x3B);	//FOT to 0x13B
	writeSPI(88, 0x1);
	writeSPI(92, 0x6);	//rowselect_timer to 6

	writeSPI(57, 0x11);   //startup, 1 window
	writeSPI(56, SEQ_DEFAULT); //enable sequencer



	writeSPI(62, 0xFF);
	writeSPI(63, (0x36 << 2) | 3);
	writeSPI(23, 0xC4);   //enable user upload of dacvrefadc

	//writeSPI(56, 2); //turn off sensor sequencer

	setOffset(175);

	//clkPhase = 0;
	masterMode = true;

	return SUCCESS;
}

Int32 LUPA1300::setOffset(UInt8 offset)
{
	writeSPI(26, offset); //turn off sensor sequencer
}

Int32 LUPA1300::phaseCal(void)
{
	writeSPI(56, SEQ_DEFAULT & ~1); //turn off sensor sequencer

	setSyncToken(0x32A);	//Set sync token to lock to the training pattern (0x32A or 1100101010)

	setClkPhase(clkPhase++);
	if(clkPhase >= 16)
		clkPhase = 0;

	return getDataCorrect();

}

CameraErrortype LUPA1300::autoPhaseCal(void)
{
	UInt16 valid = 0;
	UInt32 valid32;
	writeSPI(56, SEQ_DEFAULT & ~1); //turn off sensor sequencer

	setSyncToken(0x32A);	//Set sync token to lock to the training pattern (0x32A or 1100101010)

	for(int i = 0; i < 16; i++)
	{
		valid = (valid >> 1) | (getDataCorrect() ? 0x8000 : 0);

		//advance clock phase
		clkPhase = getClkPhase() + 1;
		if(clkPhase >= 16)
			clkPhase = 0;
		setClkPhase(clkPhase);
	}

	if(0 == valid)
	{
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

Int32 LUPA1300::seqOnOff(bool on)
{
	if(on)
		writeSPI(56, readSPI(56) | 1); //enable sequencer
	else
		writeSPI(56, readSPI(56) & ~1); //turn off sensor sequencer
}

void LUPA1300::setMasterMode(bool mMode)
{

	if(mMode)
		writeSPI(56, readSPI(56) | 0x2); //Enable master mode
	else
		writeSPI(56, readSPI(56) & ~0x2);

	masterMode = mMode;
}

//Write data d to image sensor address a
bool LUPA1300::writeSPI(UInt8 a, UInt8 d)
{
	UInt8 tx[2];
	UInt8 rx[sizeof(tx)];

	tx[1] = 0x80 | a;	//bit7 = 1 indicates write
	tx[0] = d;

	return spi->Transfer((uint64_t) tx, (uint64_t) rx, sizeof(tx));
}

//Write data d to image sensor address a
UInt8 LUPA1300::readSPI(UInt8 a)
{
	UInt8 tx[2];
	UInt8 rx[sizeof(tx)];

	tx[1] = a & 0x7F;	//bit7 = 0 indicates read
	tx[0] = 0;

	spi->Transfer((uint64_t) tx, (uint64_t) rx, sizeof(tx));

	return rx[0];
}

void LUPA1300::setReset(bool reset)
{
		gpmc->write16(IMAGE_SENSOR_CONTROL_ADDR, (gpmc->read16(IMAGE_SENSOR_CONTROL_ADDR) & ~IMAGE_SENSOR_RESET_MASK) | (reset ? IMAGE_SENSOR_RESET_MASK : 0));
}

void LUPA1300::setClkPhase(UInt8 phase)
{
		gpmc->write16(IMAGE_SENSOR_CLK_PHASE_ADDR, phase);
}

UInt8 LUPA1300::getClkPhase(void)
{
		return gpmc->read16(IMAGE_SENSOR_CLK_PHASE_ADDR);
}

UInt32 LUPA1300::getDataCorrect(void)
{
	return gpmc->read16(IMAGE_SENSOR_DATA_CORRECT_ADDR);
}

void LUPA1300::setSyncToken(UInt16 token)
{
	gpmc->write16(IMAGE_SENSOR_SYNC_TOKEN_ADDR, token);
}

void LUPA1300::setResolution(UInt32 hStart, UInt32 hWidth, UInt32 vStart, UInt32 vEnd)
{
	UInt32 hStartBlocks = hStart / LUPA1300_HRES_INCREMENT;
	UInt32 hWidthblocks = hWidth / LUPA1300_HRES_INCREMENT;

	writeSPI(60, vStart & 0xFF);
	writeSPI(61, (hStartBlocks << 2) | ((vStart >> 8) & 0x3));
	writeSPI(62, vEnd & 0xFF);
	writeSPI(63, (hWidthblocks << 2) | ((vEnd >> 8) & 0x3));

	currentHRes = hWidth;
	currentVRes = vEnd - vStart + 1;

	//If we're starting on an odd timeslot, set the register appropriately
	if(hStartBlocks & 1)
		gpmc->write16(IMAGE_SENSOR_CONTROL_ADDR, (gpmc->read16(IMAGE_SENSOR_CONTROL_ADDR) | IMAGE_SENSOR_EVEN_TIMESLOT_MASK));
	else
		gpmc->write16(IMAGE_SENSOR_CONTROL_ADDR, (gpmc->read16(IMAGE_SENSOR_CONTROL_ADDR) & ~IMAGE_SENSOR_EVEN_TIMESLOT_MASK));


}

bool LUPA1300::isValidResolution(UInt32 hRes, UInt32 vRes, UInt32 hOffset, UInt32 vOffset)
{
	if(	hRes % LUPA1300_HRES_INCREMENT |
		hOffset % LUPA1300_HRES_INCREMENT |
		(hRes + hOffset > LUPA1300_MAX_STRIDE) |
		vRes % 2 |
		vOffset % 2 |
		(vRes + vOffset > LUPA1300_MAX_V_RES))
		return false;
	else
		return true;

}

void LUPA1300::setSeqIntTime(UInt16 intTime)
{
	writeSPI(79, intTime & 0xFF);
	writeSPI(80, intTime >> 8);
}

void LUPA1300::setSeqResLength(UInt16 resLength)
{
	writeSPI(76, resLength & 0xFF);
	writeSPI(77, resLength >> 8);
}

UInt32 LUPA1300::getMinFramePeriod(UInt32 hRes, UInt32 vRes)
{
	if(!isValidResolution(hRes, vRes, 0, 0))
		return 0;
	return 0.0000000001;
	return (UInt64)(vRes * (hRes / LUPA1300_HRES_INCREMENT * 2 + LUPA1300_ROT) + LUPA1300_FOT + 5500) * 100000000ULL / 63000000ULL;//Fudge factor of 5500, TODO: need to figure out what it should be exactly
}

double LUPA1300::getMinMasterFramePeriod(UInt32 hRes, UInt32 vRes)
{
	if(!isValidResolution(hRes, vRes, 0, 0))
		return 0.0;

	return round((double)((vRes + 1) * (2 * hRes / 24 + ROT) - 2 * hRes / 24 + ROT + FOT + 4*MAX_TS) / 63000000.0 * 100000000.0) / 100000000.0;

	return (double)((vRes+1) * (hRes / LUPA1300_HRES_INCREMENT * 2 + LUPA1300_ROT) + LUPA1300_FOT) / 63000000.0;//Fudge factor of 5500, TODO: need to figure out what it should be exactly
}

UInt32 LUPA1300::getSlaveMaxFramePeriod(UInt32 hRes, UInt32 vRes)
{
	return 0xFFFFFFFF;
}

UInt32 LUPA1300::getMaxExposure(UInt32 period)
{
	return period - 800;//Fudge factor of 500, TODO: need to figure out what it should be exactly
}


double LUPA1300::getActualFramePeriod(double targetPeriod, UInt32 hRes, UInt32 vRes)
{
	//Round to nearest 10ns period
	targetPeriod = round(targetPeriod * (100000000.0)) / 100000000.0;
	
	if(targetPeriod < ((double)getMinFramePeriod(hRes, vRes) / 100000000.0))
	{ //If frame rate is not achievable with master mode
		//Compute the number of line periods required, round it, and ensure it's not below the minimum
		UInt32 lines = std::max(round((targetPeriod * 63000000.0 - LUPA1300_FOT) / (hRes / LUPA1300_HRES_INCREMENT * 2 + LUPA1300_ROT)), vRes + 1.0);
		return (double)(lines * (hRes / LUPA1300_HRES_INCREMENT * 2 + LUPA1300_ROT) + LUPA1300_FOT) / 63000000.0;
	}
	else	//Within capibilities of slave mode
	{
		double minPeriod = getMinMasterFramePeriod(hRes, vRes);
		double maxPeriod = LUPA1300_MAX_SLAVE_PERIOD;

		return clamp(targetPeriod, minPeriod, maxPeriod);
	}
	
}

double LUPA1300::setFramePeriod(double period, UInt32 hRes, UInt32 vRes)
{
	//Round to nearest 10ns period
	period = round(period * (100000000.0)) / 100000000.0;

	if(period < ((double)getMinFramePeriod(hRes, vRes) / 100000000.0))
	{
		UInt32 lines = std::max(round((period * 63000000.0 - LUPA1300_FOT) / (hRes / LUPA1300_HRES_INCREMENT * 2 + LUPA1300_ROT)), vRes + 1.0);
		masterModeTotalLines = lines;
		setMasterMode(true);
		setSeqIntTime(lines - 2);
		setSeqResLength(2);
	}
	else
	{
		setMasterMode(false);

		double minPeriod = getMinMasterFramePeriod(hRes, vRes);
		double maxPeriod = LUPA1300_MAX_SLAVE_PERIOD;

		period = clamp(period, minPeriod, maxPeriod);
		currentPeriod = period * 100000000.0;
		setSlavePeriod(currentPeriod);
		return period;

	}
	
}

/* getMaxIntegrationTime
 *
 * Gets the actual maximum integration time the sensor can be set to for the given settings
 *
 * intTime:	Desired integration time in seconds
 *
 * returns: Maximum integration time
 */
double LUPA1300::getMaxIntegrationTime(double period, UInt32 hRes, UInt32 vRes)
{
	//Round to nearest 10ns period
	period = round(period * (100000000.0)) / 100000000.0;
	double intTime = round(intTime * (100000000.0)) / 100000000.0;

	if(period < ((double)getMinFramePeriod(hRes, vRes) / 100000000.0))
	{ //Master mode
		UInt32 lines = std::max(round((period * 63000000.0 - LUPA1300_FOT) / (hRes / LUPA1300_HRES_INCREMENT * 2 + LUPA1300_ROT)), vRes + 1.0);

		return (lines - 2) * (double)(hRes / LUPA1300_HRES_INCREMENT * 2 + LUPA1300_ROT) / 63000000.0;
	}
	else	//Within capibilities of slave mode
	{
		return (double)getMaxExposure(period * 100000000.0) / 100000000.0;
	}
}

/* getActualIntegrationTime
 *
 * Gets the actual integration time that the sensor can be set to which is as close as possible to desired
 *
 * intTime:	Desired integration time in seconds
 *
 * returns: Actual closest integration time
 */
double LUPA1300::getActualIntegrationTime(double intTime, double period, UInt32 hRes, UInt32 vRes)
{

	//Round to nearest 10ns period
	period = round(period * (100000000.0)) / 100000000.0;
	intTime = round(intTime * (100000000.0)) / 100000000.0;

	if(period < ((double)getMinFramePeriod(hRes, vRes) / 100000000.0))
	{ //Master mode
		//One over the exposure time of each line
		double recipInc = 63000000.0 / (double)(hRes / LUPA1300_HRES_INCREMENT * 2 + LUPA1300_ROT);
		//Round to the closest integration time
		intTime = std::min(round(intTime * recipInc), 65535.0) / recipInc;

		return intTime;
	}
	else	//Within capibilities of slave mode
	{
		double maxIntTime = (double)getMaxExposure(period * 100000000.0) / 100000000.0;
		double minIntTime = LUPA1300_MIN_INT_TIME;
		return clamp(intTime, minIntTime, maxIntTime);
	}
}

/* setIntegrationTime
 *
 * Sets the integration time of the image sensor to a value as close as possible to requested
 *
 * intTime:	Desired integration time in seconds
 *
 * returns: Actual integration time that was set
 */
double LUPA1300::setIntegrationTime(double intTime, UInt32 hRes, UInt32 vRes)
{
	//Round to nearest 10ns period
	intTime = round(intTime * (100000000.0)) / 100000000.0;

	if(masterMode)
	{
		//One over the exposure time of each line
		double recipInc = 63000000.0 / (hRes / LUPA1300_HRES_INCREMENT * 2 + LUPA1300_ROT);

		UInt32 expLines = std::min(round(intTime * recipInc), 65535.0);
		intTime = std::min(round(intTime * recipInc), 65535.0) / recipInc;

		setSeqIntTime(expLines);

		setSeqResLength(masterModeTotalLines - expLines);
		return intTime;
	}
	else
	{
		//Set integration time to within limits
		double maxIntTime = (double)getMaxExposure(currentPeriod) / 100000000.0;
		double minIntTime = LUPA1300_MIN_INT_TIME;
		intTime = clamp(intTime, minIntTime, maxIntTime);
		currentExposure = intTime * 100000000.0;
		setSlaveExposure(currentExposure);
		return intTime;
	}

}


void LUPA1300::setSlavePeriod(UInt32 period)
{
	gpmc->write32(IMAGER_FRAME_PERIOD_ADDR, period);
}

void LUPA1300::setSlaveExposure(UInt32 exposure)
{
	gpmc->write32(IMAGER_INT_TIME_ADDR, exposure);
}

void LUPA1300::dumpRegisters(void)
{
	//return;
	for(int i = 0; i < 128; i++)
	{
		qDebug() << i << "\t" << readSPI(i);
	}
}
