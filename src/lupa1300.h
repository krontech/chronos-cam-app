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
#ifndef LUPA1300_H
#define LUPA1300_H
#include "errorCodes.h"
#include "types.h"
#include "spi.h"
#include "gpmc.h"

#define LUPA1300_HRES_INCREMENT 24
#define LUPA1300_VRES_INCREMENT	2
#define LUPA1300_MAX_STRIDE		1296
#define LUPA1300_MAX_H_RES		1280
#define LUPA1300_MAX_V_RES		1024

#define LUPA1300_ROT			(9+4)		//Granularity clock cycles (63MHz periods by default)
#define LUPA1300_FOT			315		//Granularity clock cycles (63MHz periods by default)

#define ROT_TIMER				9
#define FOT_TIMER				315

#define ROT						(ROT_TIMER + 4)
#define FOT						(FOT_TIMER + 29)
#define MAX_TS					54

#define LUPA1300_MIN_INT_TIME	0.000002	//2us
#define LUPA1300_MAX_SLAVE_PERIOD ((double)0xFFFFFFFF)

class LUPA1300
{
public:
	LUPA1300();
	~LUPA1300();
	CameraErrortype init(GPMC * gpmc_inst);
	CameraErrortype initSensor();
	Int32 setOffset(UInt8 offset);
	Int32 phaseCal(void);
	CameraErrortype autoPhaseCal(void);
	UInt32 getDataCorrect(void);
	bool isValidResolution(UInt32 hRes, UInt32 vRes, UInt32 hOffset, UInt32 vOffset);
	void setSyncToken(UInt16 token);
	void setResolution(UInt32 hStart, UInt32 hWidth, UInt32 vStart, UInt32 vEnd);
	void setSeqIntTime(UInt16 intTime);
	void setSeqResLength(UInt16 resLength);
	UInt32 getMinFramePeriod(UInt32 hRes, UInt32 vRes);
	double getMinMasterFramePeriod(UInt32 hRes, UInt32 vRes);
	UInt32 getSlaveMaxFramePeriod(UInt32 hRes, UInt32 vRes);
	double getActualFramePeriod(double targetPeriod, UInt32 hRes, UInt32 vRes);
	double setFramePeriod(double period, UInt32 hRes, UInt32 vRes);
	double getMaxIntegrationTime(double period, UInt32 hRes, UInt32 vRes);
	double getActualIntegrationTime(double intTime, double period, UInt32 hRes, UInt32 vRes);
	double setIntegrationTime(double intTime, UInt32 hRes, UInt32 vRes);
	UInt32 getMaxExposure(UInt32 period);
	void setSlavePeriod(UInt32 period);
	void setSlaveExposure(UInt32 exposure);
	bool writeSPI(UInt8 a, UInt8 d);
	void setReset(bool reset);
	void setClkPhase(UInt8 phase);
	UInt8 getClkPhase(void);
	Int32 seqOnOff(bool on);
	void setMasterMode(bool mMode);
	UInt8 readSPI(UInt8 a);
	void dumpRegisters(void);
	inline UInt32 getHResIncrement() { return LUPA1300_HRES_INCREMENT; }
	inline UInt32 getVResIncrement()  { return LUPA1300_VRES_INCREMENT; }
	inline UInt32 getMaxHRes() { return LUPA1300_MAX_H_RES; }
	inline UInt32 getMaxHStride() { return LUPA1300_MAX_STRIDE; }
	inline UInt32 getMaxVRes()  { return LUPA1300_MAX_V_RES; }


	bool masterMode;
	UInt32 masterModeTotalLines;
	UInt32 currentHRes;
	UInt32 currentVRes;
	UInt32 currentPeriod;
	UInt32 currentExposure;

	SPI * spi;
	GPMC * gpmc;
	UInt8 clkPhase;

};
/*
enum {
	LUPA1300_SUCCESS = 0,
	LUPA1300_SPI_OPEN_FAIL
};
*/
#endif // LUPA1300_H
