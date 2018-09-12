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

UInt8 sram80Clk[427] = {0x81, 0xFF, 0x27, 0xFA, 0x21, 0xA0, 0x7F, 0xCB, 0xFE, 0x8C, 0x78, 0x1F, 0xF2, 0xFF, 0xA3,
						0x1E, 0x07, 0xFC, 0xBF, 0xE8, 0xC7, 0x81, 0xFF, 0x2F, 0xFA, 0x31, 0xE0, 0x7F, 0xCB, 0xFE,
						0x8C, 0x78, 0x1F, 0xF2, 0xFF, 0xA3, 0x1E, 0x07, 0xFC, 0xBF, 0xE8, 0xC7, 0x81, 0xFF, 0x2F,
						0xFA, 0x31, 0xE0, 0x7F, 0xCB, 0xFE, 0x8C, 0x78, 0x1F, 0xF2, 0xFF, 0xA3, 0x1E, 0x07, 0xFC,
						0xBF, 0xE8, 0xC7, 0x81, 0xFF, 0x2F, 0xFA, 0x31, 0xE0, 0x7F, 0xCB, 0xFE, 0x8C, 0x78, 0x1F,
						0xF2, 0xFF, 0xA3, 0x1E, 0x07, 0xFC, 0xBF, 0xE8, 0xC7, 0x81, 0xFF, 0x2F, 0xFA, 0x31, 0xE0,
						0x7F, 0xCB, 0xFE, 0x8C, 0x78, 0x1F, 0xF2, 0xFF, 0xA3, 0x1E, 0x07, 0xFC, 0xBF, 0xE8, 0xC7,
						0x81, 0xFF, 0x2F, 0xFA, 0x31, 0xE0, 0x7F, 0xCB, 0xFE, 0x8C, 0x78, 0x1F, 0xF2, 0xFF, 0xA3,
						0x1E, 0x07, 0xFC, 0xBF, 0xE8, 0xC7, 0x81, 0xFF, 0x2F, 0xFA, 0x31, 0xE0, 0x7F, 0xCB, 0xFE,
						0x84, 0x78, 0x1F, 0xF2, 0xFF, 0xA1, 0x1E, 0x07, 0xFC, 0xBF, 0xE8, 0x47, 0x81, 0xFF, 0x2F,
						0xFA, 0x11, 0xE0, 0x7F, 0xCB, 0xFE, 0x84, 0x78, 0x1F, 0xF2, 0xFF, 0xA1, 0x1E, 0x07, 0xFC,
						0xBF, 0xE8, 0x47, 0x81, 0xFF, 0x2F, 0xFA, 0x11, 0xE0, 0x7F, 0xCB, 0xFE, 0x84, 0x78, 0x1F,
						0xF2, 0xFF, 0xA1, 0x1E, 0x07, 0xFC, 0xBF, 0xE8, 0x47, 0x81, 0xFF, 0x2F, 0xFA, 0x11, 0xE0,
						0x7F, 0xCB, 0xFE, 0x84, 0x78, 0x1F, 0xF2, 0xFF, 0xA1, 0x1E, 0x07, 0xFC, 0xBF, 0xE8, 0x47,
						0x80, 0xFF, 0x2F, 0xFA, 0x11, 0xE0, 0x3F, 0xC3, 0xFE, 0x94, 0x78, 0x0F, 0xF0, 0xFF, 0xA5,
						0x1E, 0x03, 0xFC, 0x3F, 0xE9, 0x47, 0x80, 0xFF, 0x0F, 0xFA, 0x51, 0xE0, 0x3F, 0xC3, 0xFE,
						0x94, 0x78, 0x0F, 0xF0, 0xFF, 0xB5, 0x1E, 0x03, 0xFC, 0x3F, 0xED, 0x47, 0x80, 0xFF, 0x0F,
						0xFB, 0x51, 0xE0, 0x3F, 0xC3, 0xFE, 0xD4, 0x78, 0x0F, 0xF0, 0xFF, 0xB5, 0x1E, 0x03, 0xFC,
						0x3F, 0xED, 0x47, 0x80, 0xFF, 0x0F, 0xFB, 0x51, 0xE0, 0x3F, 0xC3, 0xFE, 0xD4, 0x78, 0x0F,
						0xF0, 0xFF, 0xB5, 0x1E, 0x03, 0xFC, 0x3F, 0xED, 0x47, 0x80, 0xFF, 0x0F, 0xFA, 0x51, 0xE0,
						0x3F, 0xC3, 0xFE, 0x94, 0x78, 0x0F, 0xF0, 0xFF, 0xA5, 0x1E, 0x03, 0xFC, 0x3F, 0xE9, 0x47,
						0x80, 0xFF, 0x0F, 0xFA, 0x51, 0xE0, 0x3F, 0xC3, 0xFE, 0x94, 0x78, 0x0F, 0xF0, 0xFF, 0xA5,
						0x1E, 0x03, 0xFC, 0x3F, 0xE8, 0x47, 0x80, 0xFF, 0x0F, 0xFA, 0x11, 0xE0, 0x3F, 0xC3, 0xFE,
						0x84, 0x78, 0x0F, 0xF0, 0xFF, 0xA1, 0x1E, 0x03, 0xFC, 0x3F, 0xE8, 0x47, 0x80, 0xFF, 0x0F,
						0xFA, 0x11, 0xE0, 0x3F, 0xC3, 0xFE, 0x84, 0x78, 0x0F, 0xF0, 0xFF, 0xA1, 0x1E, 0x03, 0xFC,
						0x3F, 0xE8, 0x47, 0x80, 0xFF, 0x0F, 0xFA, 0x11, 0xE0, 0x3F, 0xC3, 0xFE, 0x84, 0x78, 0x0F,
						0xF0, 0xFF, 0xA1, 0x1E, 0x03, 0xFC, 0x3F, 0xE8, 0x47, 0x80, 0xFF, 0x0F, 0xFA, 0x11, 0xE0,
						0x3F, 0xC3, 0xFE, 0x84, 0x78, 0x0F, 0xF0, 0xFF, 0xA1, 0x1E, 0x03, 0xFC, 0x1F, 0xE8, 0x47,
						0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


UInt8 sram39ClkBlk[211] = {0x81, 0xFF, 0x2F, 0xFA, 0x31, 0xE0, 0x7F, 0xCB, 0xFE, 0x8C, 0x78, 0x1F, 0xF2, 0xFF, 0xA3,
						0x1E, 0x07, 0xFC, 0xBF, 0xE8, 0xC7, 0x81, 0xFF, 0x2F, 0xFA, 0x31, 0xE0, 0x7F, 0xCB, 0xFE,
						0x8C, 0x78, 0x1F, 0xF2, 0xFF, 0xA3, 0x1E, 0x07, 0xFC, 0xBF, 0xE8, 0x47, 0x81, 0xFF, 0x2F,
						0xFA, 0x11, 0xE0, 0x7F, 0xCB, 0xFE, 0x84, 0x78, 0x1F, 0xF2, 0xFF, 0xA1, 0x1E, 0x07, 0xFC,
						0xBF, 0xE8, 0x47, 0x81, 0xFF, 0x2F, 0xFA, 0x11, 0xE0, 0x3F, 0xCB, 0xFE, 0x84, 0x78, 0x0F,
						0xF0, 0xFF, 0xB5, 0x1E, 0x03, 0xFC, 0x3F, 0xED, 0x47, 0x80, 0xFF, 0x0F, 0xFB, 0x51, 0xE0,
						0x3F, 0xC3, 0xFE, 0xD4, 0x78, 0x0F, 0xF0, 0xFF, 0xB5, 0x1E, 0x03, 0xFC, 0x3F, 0xED, 0x47,
						0x80, 0xFF, 0x0F, 0xFB, 0x51, 0xE0, 0x3F, 0xC3, 0xFE, 0xD4, 0x78, 0x0F, 0xF0, 0xFF, 0xB5,
						0x1E, 0x03, 0xFC, 0x3F, 0xE9, 0x47, 0x80, 0xFF, 0x0F, 0xFA, 0x51, 0xE0, 0x3F, 0xC3, 0xFE,
						0x94, 0x78, 0x0F, 0xF0, 0xFF, 0xA5, 0x1E, 0x03, 0xFC, 0x3F, 0xE8, 0x47, 0x80, 0xFF, 0x0F,
						0xFA, 0x11, 0xE0, 0x3F, 0xC3, 0xFE, 0x84, 0x78, 0x0F, 0xF0, 0xFF, 0xA1, 0x1E, 0x03, 0xFC,
						0x3F, 0xE8, 0x47, 0x80, 0xFF, 0x0F, 0xFA, 0x11, 0xE0, 0x3F, 0xC3, 0xFE, 0x84, 0x78, 0x0F,
						0xF0, 0xFF, 0xA1, 0x1E, 0x03, 0xFC, 0x3F, 0xE8, 0x47, 0x80, 0xFF, 0x0F, 0xFA, 0x11, 0xE8,
						0x3F, 0xC3, 0xFE, 0x84, 0x78, 0x0F, 0xF0, 0x7F, 0xA1, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00,
						0x00};

UInt8 sram39Clk[211] = {0x81, 0xFF, 0x2F, 0xFA, 0x31, 0xE0, 0x7F, 0xCB, 0xFE, 0x8C, 0x78, 0x1F, 0xF2, 0xFF, 0xA3,
						0x1E, 0x07, 0xFC, 0xBF, 0xE8, 0xC7, 0x81, 0xFF, 0x2F, 0xFA, 0x31, 0xE0, 0x7F, 0xCB, 0xFE,
						0x8C, 0x78, 0x1F, 0xF2, 0xFF, 0xA3, 0x1E, 0x07, 0xFC, 0xBF, 0xE8, 0x47, 0x81, 0xFF, 0x2F,
						0xFA, 0x11, 0xE0, 0x7F, 0xCB, 0xFE, 0x84, 0x78, 0x1F, 0xF2, 0xFF, 0xA1, 0x1E, 0x07, 0xFC,
						0xBF, 0xE8, 0x47, 0x81, 0xFF, 0x2F, 0xFA, 0x11, 0xE0, 0x3F, 0xCB, 0xFE, 0x84, 0x78, 0x0F,
						0xF0, 0xFF, 0xB5, 0x1E, 0x03, 0xFC, 0x3F, 0xED, 0x47, 0x80, 0xFF, 0x0F, 0xFB, 0x51, 0xE0,
						0x3F, 0xC3, 0xFE, 0xD4, 0x78, 0x0F, 0xF0, 0xFF, 0xB5, 0x1E, 0x03, 0xFC, 0x3F, 0xED, 0x47,
						0x80, 0xFF, 0x0F, 0xFB, 0x51, 0xE0, 0x3F, 0xC3, 0xFE, 0xD4, 0x78, 0x0F, 0xF0, 0xFF, 0xB5,
						0x1E, 0x03, 0xFC, 0x3F, 0xE9, 0x47, 0x80, 0xFF, 0x0F, 0xFA, 0x51, 0xE0, 0x3F, 0xC3, 0xFE,
						0x94, 0x78, 0x0F, 0xF0, 0xFF, 0xA5, 0x1E, 0x03, 0xFC, 0x3F, 0xE8, 0x47, 0x80, 0xFF, 0x0F,
						0xFA, 0x11, 0xE0, 0x3F, 0xC3, 0xFE, 0x84, 0x78, 0x0F, 0xF0, 0xFF, 0xA1, 0x1E, 0x03, 0xFC,
						0x3F, 0xE8, 0x47, 0x80, 0xFF, 0x0F, 0xFA, 0x11, 0xE0, 0x3F, 0xC3, 0xFE, 0x84, 0x78, 0x0F,
						0xF0, 0xFF, 0xA1, 0x1E, 0x03, 0xFC, 0x3F, 0xE8, 0x47, 0x80, 0xFF, 0x0F, 0xFA, 0x11, 0xE0,
						0x3F, 0xC3, 0xFE, 0x84, 0x78, 0x0F, 0xF0, 0x7F, 0xA1, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00,
						0x00};

UInt8 sram22Clk[126] = {0x81, 0xFF, 0x2F, 0xFA, 0x21, 0xA0, 0x7F, 0xCB, 0xFE, 0x8C, 0x78, 0x1F, 0xF2, 0xFF, 0xA3,
						0x1E, 0x07, 0xFC, 0xBF, 0xE8, 0xC7, 0x81, 0xFF, 0x2F, 0xFA, 0x11, 0xE0, 0x7F, 0xCB, 0xFE,
						0xC4, 0x78, 0x1F, 0xF2, 0xFF, 0xB1, 0x1E, 0x03, 0xFC, 0xBF, 0xEC, 0x47, 0x80, 0xFF, 0x0F,
						0xFB, 0x51, 0xE0, 0x3F, 0xC3, 0xFE, 0xD4, 0x78, 0x0F, 0xF0, 0xFF, 0xB5, 0x1E, 0x03, 0xFC,
						0x3F, 0xE9, 0x47, 0x80, 0xFF, 0x0F, 0xFA, 0x51, 0xE0, 0x3F, 0xC3, 0xFE, 0x94, 0x78, 0x0F,
						0xF0, 0xFF, 0xA5, 0x1E, 0x03, 0xFC, 0x3F, 0xE8, 0x47, 0x80, 0xFF, 0x0F, 0xFA, 0x11, 0xE0,
						0x3F, 0xC3, 0xFE, 0x84, 0x78, 0x0F, 0xF0, 0xFF, 0xA1, 0x1E, 0x03, 0xFC, 0x3F, 0xE8, 0x47,
						0x80, 0xFF, 0x0F, 0xFA, 0x11, 0xE0, 0x3F, 0xC1, 0xFE, 0x84, 0x70, 0x00, 0x00, 0x00, 0x00,
						0x00, 0x00};

UInt8 sram30Clk[174] = {0x81, 0xFF, 0x2F, 0xFA, 0x31, 0xE0, 0x7F, 0xCB, 0xFE, 0x8C, 0x78, 0x1F, 0xF2, 0xFF, 0xA3,
						0x1E, 0x07, 0xFC, 0xBF, 0xE8, 0xC7, 0x81, 0xFF, 0x2F, 0xFA, 0x31, 0xE0, 0x7F, 0xCB, 0xFE,
						0x8C, 0x78, 0x1F, 0xF2, 0xFF, 0xA3, 0x1E, 0x07, 0xFC, 0xBF, 0xE8, 0x47, 0x81, 0xFF, 0x2F,
						0xFA, 0x11, 0xE0, 0x7F, 0xCB, 0xFE, 0x84, 0x78, 0x1F, 0xF2, 0xFF, 0xA1, 0x1E, 0x07, 0xFC,
						0xBF, 0xE8, 0x47, 0x81, 0xFF, 0x2F, 0xFA, 0x11, 0xE0, 0x3F, 0xCB, 0xFE, 0x84, 0x78, 0x0F,
						0xF0, 0xFF, 0xB5, 0x1E, 0x03, 0xFC, 0x3F, 0xED, 0x47, 0x80, 0xFF, 0x0F, 0xFB, 0x51, 0xE0,
						0x3F, 0xC3, 0xFE, 0xD4, 0x78, 0x0F, 0xF0, 0xFF, 0xB5, 0x1E, 0x03, 0xFC, 0x3F, 0xED, 0x47,
						0x80, 0xFF, 0x0F, 0xFA, 0x51, 0xE0, 0x3F, 0xC3, 0xFE, 0x94, 0x78, 0x0F, 0xF0, 0xFF, 0xA1,
						0x1E, 0x03, 0xFC, 0x3F, 0xE8, 0x47, 0x80, 0xFF, 0x0F, 0xFA, 0x11, 0xE0, 0x3F, 0xC3, 0xFE,
						0x84, 0x78, 0x0F, 0xF0, 0xFF, 0xA1, 0x1E, 0x03, 0xFC, 0x3F, 0xE8, 0x47, 0x80, 0xFF, 0x0F,
						0xFA, 0x11, 0xE0, 0x3F, 0xC1, 0xFE, 0x84, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
						0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

UInt8 sram25Clk[148] = {0x81, 0xFF, 0x2F, 0xFA, 0x31, 0xE0, 0x7F, 0xCB, 0xFE, 0x8C, 0x78, 0x1F, 0xF2, 0xFF, 0xA3,
						0x1E, 0x07, 0xFC, 0xBF, 0xE8, 0xC7, 0x81, 0xFF, 0x2F, 0xFA, 0x31, 0xE0, 0x7F, 0xCB, 0xFE,
						0x8C, 0x78, 0x1F, 0xF2, 0xFF, 0xA1, 0x1E, 0x07, 0xFC, 0xBF, 0xE8, 0x47, 0x81, 0xFF, 0x2F,
						0xFA, 0x11, 0xE0, 0x7F, 0xCB, 0xFE, 0x84, 0x78, 0x1F, 0xF2, 0xFF, 0xA1, 0x1E, 0x03, 0xFC,
						0xBF, 0xE8, 0x47, 0x80, 0xFF, 0x0F, 0xFB, 0x51, 0xE0, 0x3F, 0xC3, 0xFE, 0xD4, 0x78, 0x0F,
						0xF0, 0xFF, 0xB5, 0x1E, 0x03, 0xFC, 0x3F, 0xED, 0x47, 0x80, 0xFF, 0x0F, 0xFB, 0x51, 0xE0,
						0x3F, 0xC3, 0xFE, 0xD4, 0x78, 0x0F, 0xF0, 0xFF, 0xA5, 0x1E, 0x03, 0xFC, 0x3F, 0xE9, 0x47,
						0x80, 0xFF, 0x0F, 0xFA, 0x11, 0xE0, 0x3F, 0xC3, 0xFE, 0x84, 0x78, 0x0F, 0xF0, 0xFF, 0xA1,
						0x1E, 0x03, 0xFC, 0x3F, 0xE8, 0x47, 0x80, 0xFF, 0x07, 0xFA, 0x11, 0xC0, 0x00, 0x00, 0x00,
						0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

UInt8 sram20Clk[122] = {0x81, 0xFF, 0x2F, 0xFA, 0x31, 0xE0, 0x7F, 0xCB, 0xFE, 0x8C, 0x78, 0x1F, 0xF2, 0xFF, 0xA3,
						0x1E, 0x07, 0xFC, 0xBF, 0xE8, 0xC7, 0x81, 0xFF, 0x2F, 0xFA, 0x31, 0xE0, 0x7F, 0xCB, 0xFE,
						0x84, 0x78, 0x1F, 0xF2, 0xFF, 0xA1, 0x1E, 0x07, 0xFC, 0xBF, 0xE8, 0x47, 0x81, 0xFF, 0x2F,
						0xFA, 0x11, 0xE0, 0x3F, 0xCB, 0xFE, 0x84, 0x78, 0x0F, 0xF0, 0xFF, 0xB5, 0x1E, 0x03, 0xFC,
						0x3F, 0xED, 0x47, 0x80, 0xFF, 0x0F, 0xFB, 0x51, 0xE0, 0x3F, 0xC3, 0xFE, 0xD4, 0x78, 0x0F,
						0xF0, 0xFF, 0xB5, 0x1E, 0x03, 0xFC, 0x3F, 0xE9, 0x47, 0x80, 0xFF, 0x0F, 0xFA, 0x51, 0xE0,
						0x3F, 0xC3, 0xFE, 0x84, 0x78, 0x0F, 0xF0, 0xFF, 0xA1, 0x1E, 0x03, 0xFC, 0x1F, 0xE8, 0x47,
						0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
						0x00, 0x00};


#define round(x) (floor(x + 0.5))

LUX1310::LUX1310()
{
	spi = new SPI();
	wavetableSelect = LUX1310_WAVETABLE_AUTO;
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
	wavetableSize = 80;
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

	SCIWrite(0x7E, 0);	//reset all registers
	//delayms(1);
	SCIWrite(0x57, 0x0FC0); //custom pattern for ADC training pattern
	//delayms(1);
	qDebug() << "SCI Read returned" << SCIRead(0x57);
	SCIWrite(0x56, 0x0002); //Enable test pattern
	SCIWrite(0x4C, 0x0FC0);	//pclk channel output during vertical blanking
	SCIWrite(0x5A, 0xE1);	//Invert dclk output

	//Sensor is now outputting 0xFC0 on all ports. Receiver will automatically sync when the clock phase is right

	autoPhaseCal();

	//Return to normal data mode

	SCIWrite(0x4C, 0x0F00);	//pclk channel output during vertical blanking
	SCIWrite(0x56, 0x0000); //Disable test pattern

	//Set for 80 clock wavetable
	SCIWrite(0x37, 80); //non-overlapping readout delay
	SCIWrite(0x7A, 80); //wavetable size

	//Set internal control registers to fine tune the performance of the sensor

	SCIWrite(0x71, 0x0007); //line valid delay to match internal ADC latency
	SCIWrite(0x2D, 0xE08E); //state for idle controls
	SCIWrite(0x2E, 0xFC1F); //state for idle controls
	SCIWrite(0x2F, 0x0003); //state for idle controls

	SCIWrite(0x5C, 0x2202); //ADC clock controls
	SCIWrite(0x62, 0x4B76); //ADC range to match pixel saturation level
	SCIWrite(0x74, 0x041F); //internal clock timing

	//Read out the version register to determine the silicon revision
	UInt16 rev = SCIRead(0x00);
	qDebug() << "Read rev of " << rev;
	sensorVersion = rev & 0xFF;

	qDebug() << "Found LUX1310 sensor, silicon revision" << sensorVersion;

	//Load the appropriate values based on the silicon revision
	if(LUX1310_VERSION_2 == sensorVersion)
	{
		SCIWrite(0x5B, 0x307F); //internal control register
		SCIWrite(0x7B, 0x3007); //internal control register
	}
	else if(LUX1310_VERSION_1 == sensorVersion)
	{
		SCIWrite(0x5B, 0x301F); //internal control register
		SCIWrite(0x7B, 0x3001); //internal control register
	}
	else
	{	//Unknown version, default to version 1 values
		SCIWrite(0x5B, 0x301F); //internal control register
		SCIWrite(0x7B, 0x3001); //internal control register
		qDebug() << "Found LUX1310 sensor, unexpected Silicon revision found:" << sensorVersion;

	}

	for(int i = 0; i < 16; i++)
		SCIWrite(0x3a+i, 0x0); //internal control register

	SCIWrite(0x39, 0x1); //ADC offset enable??

	//Set ADC offsets
	loadADCOffsetsFromFile();

	//Set Gain
	SCIWrite(0x51, 0x007F);	//gain selection sampling cap (11)	12 bit
	SCIWrite(0x52, 0x007F);	//gain selection feedback cap (8) 7 bit
	SCIWrite(0x53, 0x03);	//Serial gain



	//Load the wavetable
	SCIWriteBuf(0x7F, sram80Clk, 427 /*sizeof(sram80Clk)*/);

	//Enable internal timing engine
	SCIWrite(0x01, 0x0001);

	delayms(10);

	gpmc->write32(IMAGER_FRAME_PERIOD_ADDR, 100*4000);
	gpmc->write32(IMAGER_INT_TIME_ADDR, 100*3900);
	delayms(50);


	currentHRes = 1280;
	currentVRes = 1024;
	setFramePeriod(getMinFramePeriod(currentHRes, currentVRes)/100000000.0, currentHRes, currentVRes);
	//mem problem before this
	setIntegrationTime((double)getMaxExposure(currentPeriod) / 100000000.0, currentHRes, currentVRes);

	return SUCCESS;
}

void LUX1310::SCIWrite(UInt8 address, UInt16 data)
{
	int i = 0;

	//qDebug() << "sci write" << data;

	//Clear RW and reset FIFO
	gpmc->write16(SENSOR_SCI_CONTROL_ADDR, 0x8000 | (gpmc->read16(SENSOR_SCI_CONTROL_ADDR) & ~SENSOR_SCI_CONTROL_RW_MASK));

	//Set up address, transfer length and put data into FIFO
	gpmc->write16(SENSOR_SCI_ADDRESS_ADDR, address);
	gpmc->write16(SENSOR_SCI_DATALEN_ADDR, 2);
	gpmc->write16(SENSOR_SCI_FIFO_WR_ADDR_ADDR, data >> 8);
	gpmc->write16(SENSOR_SCI_FIFO_WR_ADDR_ADDR, data & 0xFF);

	//Start transfer
	gpmc->write16(SENSOR_SCI_CONTROL_ADDR, gpmc->read16(SENSOR_SCI_CONTROL_ADDR) | SENSOR_SCI_CONTROL_RUN_MASK);

	int first = gpmc->read16(SENSOR_SCI_CONTROL_ADDR) & SENSOR_SCI_CONTROL_RUN_MASK;
	//Wait for completion
	while(gpmc->read16(SENSOR_SCI_CONTROL_ADDR) & SENSOR_SCI_CONTROL_RUN_MASK)
		i++;

	if(!first && i != 0)
		qDebug() << "First busy was missed, address:" << address;
	else if(i == 0)
		qDebug() << "No busy detected, something is probably very wrong, address:" << address;

	int readback = SCIRead(address);
	int readback2 = SCIRead(address);
	int readback3 = SCIRead(address);
	if(data != readback)
		qDebug() << "SCI readback wrong, address: " << address << " expected: " << data << " got: " << readback << readback2 << readback3;
}

void LUX1310::SCIWriteBuf(UInt8 address, UInt8 * data, UInt32 dataLen)
{
	//Clear RW and reset FIFO
	gpmc->write16(SENSOR_SCI_CONTROL_ADDR, 0x8000 | (gpmc->read16(SENSOR_SCI_CONTROL_ADDR) & ~SENSOR_SCI_CONTROL_RW_MASK));

	//Set up address, transfer length and put data into FIFO
	gpmc->write16(SENSOR_SCI_ADDRESS_ADDR, address);
	gpmc->write16(SENSOR_SCI_DATALEN_ADDR, dataLen);
	for(UInt32 i = 0; i < dataLen; i++)
		gpmc->write16(SENSOR_SCI_FIFO_WR_ADDR_ADDR, data[i]);

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
	gpmc->write32(IMAGER_INT_TIME_ADDR, 0);	//Disable integration
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

void LUX1310::setResolution(UInt32 hStart, UInt32 hWidth, UInt32 vStart, UInt32 vEnd)
{
	UInt32 hStartBlocks = hStart / LUX1310_HRES_INCREMENT;
	UInt32 hWidthblocks = hWidth / LUX1310_HRES_INCREMENT;
	SCIWrite(0x05, 0x20 + hStartBlocks * LUX1310_HRES_INCREMENT);//X Start
	SCIWrite(0x06, 0x20 + (hStartBlocks + hWidthblocks) * LUX1310_HRES_INCREMENT - 1);//X End
	SCIWrite(0x07, vStart);//Y Start
	SCIWrite(0x08, vEnd);//Y End

	currentHRes = hWidth;
	currentVRes = vEnd - vStart + 1;
}

bool LUX1310::isValidResolution(UInt32 hRes, UInt32 vRes, UInt32 hOffset, UInt32 vOffset)
{
	/* Enforce resolution limits. */
	if ((hRes < LUX1310_MIN_HRES) || (hRes + hOffset > LUX1310_MAX_H_RES)) {
		return false;
	}
	if ((vRes < LUX1310_MIN_VRES) || (vRes + vOffset > LUX1310_MAX_V_RES)) {
		return false;
	}
	/* Enforce minimum pixel increments. */
	if ((hRes % LUX1310_HRES_INCREMENT) || (hOffset % LUX1310_HRES_INCREMENT)) {
		return false;
	}
	if ((vRes % LUX1310_VRES_INCREMENT) || (vOffset % LUX1310_VRES_INCREMENT)) {
		return false;
	}
	/* Otherwise, the resultion and offset are valid. */
	return true;
}

//Used by init functions only
UInt32 LUX1310::getMinFramePeriod(UInt32 hRes, UInt32 vRes, UInt32 wtSize)
{
	if(!isValidResolution(hRes, vRes, 0, 0))
		return 0;

	if(hRes == 1280)
		wtSize = 80;

	double tRead = (double)(hRes / LUX1310_HRES_INCREMENT) * LUX1310_CLOCK_PERIOD;
	double tHBlank = 2.0 * LUX1310_CLOCK_PERIOD;
	double tWavetable = wtSize * LUX1310_CLOCK_PERIOD;
	double tRow = max(tRead+tHBlank, tWavetable+3*LUX1310_CLOCK_PERIOD);
	double tTx = 25 * LUX1310_CLOCK_PERIOD;
	double tFovf = 50 * LUX1310_CLOCK_PERIOD;
	double tFovb = (50) * LUX1310_CLOCK_PERIOD;//Duration between PRSTN falling and TXN falling (I think)
	double tFrame = tRow * vRes + tTx + tFovf + tFovb;
	qDebug() << "getMinFramePeriod:" << tFrame;
	return (UInt64)(ceil(tFrame * 100000000.0));
}

double LUX1310::getMinMasterFramePeriod(UInt32 hRes, UInt32 vRes)
{
	if(!isValidResolution(hRes, vRes, 0, 0))
		return 0.0;

	return (double)getMinFramePeriod(hRes, vRes) / 100000000.0;
	int wtSize;

	if(hRes == 1280)
		wtSize = 80;
	else
		wtSize = LUX1310_MIN_WAVETABLE_SIZE;

	double tRead = (double)(hRes / LUX1310_HRES_INCREMENT) * LUX1310_CLOCK_PERIOD;
	double tHBlank = 2.0 * LUX1310_CLOCK_PERIOD;
	double tWavetable = wtSize * LUX1310_CLOCK_PERIOD;
	double tRow = max(tRead+tHBlank, tWavetable+3*LUX1310_CLOCK_PERIOD);
	double tTx = 25 * LUX1310_CLOCK_PERIOD;
	double tFovf = 50 * LUX1310_CLOCK_PERIOD;
	double tFovb = (50) * LUX1310_CLOCK_PERIOD;//Duration between PRSTN falling and TXN falling (I think)
	double tFrame = tRow * vRes + tTx + tFovf + tFovb;
	qDebug() << "getMinMasterFramePeriod:" << tFrame;
	return tFrame;
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

double LUX1310::getActualFramePeriod(double targetPeriod, UInt32 hRes, UInt32 vRes)
{
	//Round to nearest 10ns period
	targetPeriod = round(targetPeriod * (100000000.0)) / 100000000.0;

	double minPeriod = getMinMasterFramePeriod(hRes, vRes);
	double maxPeriod = LUX1310_MAX_SLAVE_PERIOD;

	return within(targetPeriod, minPeriod, maxPeriod);
}

double LUX1310::setFramePeriod(double period, UInt32 hRes, UInt32 vRes)
{
	//Round to nearest 10ns period
	period = round(period * (100000000.0)) / 100000000.0;
	qDebug() << "Requested period" << period;
	double minPeriod = getMinMasterFramePeriod(hRes, vRes);
	double maxPeriod = LUX1310_MAX_SLAVE_PERIOD / 100000000.0;

	period = within(period, minPeriod, maxPeriod);
	currentPeriod = period * 100000000.0;

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
double LUX1310::getMaxIntegrationTime(double period, UInt32 hRes, UInt32 vRes)
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
double LUX1310::getActualIntegrationTime(double intTime, double period, UInt32 hRes, UInt32 vRes)
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
double LUX1310::setIntegrationTime(double intTime, UInt32 hRes, UInt32 vRes)
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
	double linePeriod = max((currentHRes / LUX1310_HRES_INCREMENT)+2, (wavetableSize + 3)) * 1.0/LUX1310_SENSOR_CLOCK;	//Line period in seconds
	UInt32 startDelay = (double)startDelaySensorClocks * TIMING_CLOCK_FREQ / LUX1310_SENSOR_CLOCK;
	double targetExp = (double)exposure / 100000000.0;
	UInt32 expLines = round(targetExp / linePeriod);

	exposure = startDelay + (linePeriod * expLines * 100000000.0);
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


void LUX1310::setWavetable(UInt8 mode)
{
	switch(mode)
	{
	case LUX1310_WAVETABLE_80:
		SCIWrite(0x01, 0x0000);	//Disable internal timing engine
		SCIWrite(0x37, 80); //non-overlapping readout delay
		SCIWrite(0x7A, 80); //wavetable size
		SCIWriteBuf(0x7F, sram80Clk, sizeof(sram80Clk));
		SCIWrite(0x01, 0x0001);	//Enable internal timing engine
		wavetableSize = 80;
		setABNDelayClocks(ABN_DELAY_WT80);
		break;

	case LUX1310_WAVETABLE_39:
		SCIWrite(0x01, 0x0000);	//Disable internal timing engine
		SCIWrite(0x37, 39); //non-overlapping readout delay
		SCIWrite(0x7A, 39); //wavetable size
		SCIWriteBuf(0x7F, sram39Clk, sizeof(sram39Clk));
		SCIWrite(0x01, 0x0001);	//Enable internal timing engine
		wavetableSize = 39;
		setABNDelayClocks(ABN_DELAY_WT39);
		break;

	case LUX1310_WAVETABLE_30:
		SCIWrite(0x01, 0x0000);	//Disable internal timing engine
		SCIWrite(0x37, 30); //non-overlapping readout delay
		SCIWrite(0x7A, 30); //wavetable size
		SCIWriteBuf(0x7F, sram30Clk, sizeof(sram30Clk));
		SCIWrite(0x01, 0x0001);	//Enable internal timing engine
		wavetableSize = 30;
		setABNDelayClocks(ABN_DELAY_WT30);
		break;

	case LUX1310_WAVETABLE_25:
		SCIWrite(0x01, 0x0000);	//Disable internal timing engine
		SCIWrite(0x37, 25); //non-overlapping readout delay
		SCIWrite(0x7A, 25); //wavetable size
		SCIWriteBuf(0x7F, sram25Clk, sizeof(sram25Clk));
		SCIWrite(0x01, 0x0001);	//Enable internal timing engine
		wavetableSize = 25;
		setABNDelayClocks(ABN_DELAY_WT25);
		break;

	case LUX1310_WAVETABLE_20:
		SCIWrite(0x01, 0x0000);	//Disable internal timing engine
		SCIWrite(0x37, 20); //non-overlapping readout delay
		SCIWrite(0x7A, 20); //wavetable size
		SCIWriteBuf(0x7F, sram20Clk, sizeof(sram20Clk));
		SCIWrite(0x01, 0x0001);	//Enable internal timing engine
		wavetableSize = 20;
		setABNDelayClocks(ABN_DELAY_WT20);
		break;


	}
	qDebug() << "Wavetable size set to" << wavetableSize;
}


void LUX1310::updateWavetableSetting()
{
	if(wavetableSelect == LUX1310_WAVETABLE_AUTO)
	{
		qDebug() << "currentPeriod" << currentPeriod << "Min period" << getMinFramePeriod(currentHRes, currentVRes, 80);
		if(currentPeriod < getMinFramePeriod(currentHRes, currentVRes, 80))
		{
			if(currentPeriod < getMinFramePeriod(currentHRes, currentVRes, 39))
			{
				if(currentPeriod < getMinFramePeriod(currentHRes, currentVRes, 30))
				{
					if(currentPeriod < getMinFramePeriod(currentHRes, currentVRes, 25))
					{
						setWavetable(LUX1310_WAVETABLE_20);
					}
					else
						setWavetable(LUX1310_WAVETABLE_25);
				}
				else
					setWavetable(LUX1310_WAVETABLE_30);
			}
			else
				setWavetable(LUX1310_WAVETABLE_39);
		}
		else
		{
			setWavetable(LUX1310_WAVETABLE_80);
		}
	}
	else
	{
		setWavetable(wavetableSelect);
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

Int32 LUX1310::loadADCOffsetsFromFile(void)
{
	Int16 offsets[LUX1310_HRES_INCREMENT];

	QString filename;
	
	//Generate the filename for this particular resolution and offset
	filename.sprintf("cal:lux1310Offsets");
	
	std::string fn;
	fn = getFilename("", ".bin");
	filename.append(fn.c_str());
	QFileInfo adcOffsetsFile(filename);
	if (adcOffsetsFile.exists() && adcOffsetsFile.isFile()) 
		fn = adcOffsetsFile.absoluteFilePath().toLocal8Bit().constData();
	else 
		return CAMERA_FILE_NOT_FOUND;

	qDebug() << "attempting to load ADC offsets from" << fn.c_str();

	//If the offsets file exists, read it in
	if( access( fn.c_str(), R_OK ) == -1 )
		return CAMERA_FILE_NOT_FOUND;

	FILE * fp;
	fp = fopen(fn.c_str(), "rb");
	if(!fp)
		return CAMERA_FILE_ERROR;

	fread(offsets, sizeof(offsets[0]), LUX1310_HRES_INCREMENT, fp);
	fclose(fp);

	//Write the values into the sensor
	for(int i = 0; i < LUX1310_HRES_INCREMENT; i++)
		setADCOffset(i, offsets[i]);

	return SUCCESS;
}

Int32 LUX1310::saveADCOffsetsToFile(void)
{
	Int16 offsets[LUX1310_HRES_INCREMENT];

	QString filename;
	
	//Generate the filename for this particular resolution and offset
	if (!QDir("./cal").exists())
		filename.append("/opt/camera/cal/lux1310Offsets");
	else
		filename.append("./cal/lux1310Offsets");
	
	std::string fn;
	fn = getFilename("", ".bin");
	filename.append(fn.c_str());
	fn = filename.toLocal8Bit().constData();

	qDebug("writing ADC offsets to %s", fn.c_str());
	
	FILE * fp;
	fp = fopen(fn.c_str(), "wb");
	if(!fp)
		return CAMERA_FILE_ERROR;

	for(int i = 0; i < LUX1310_HRES_INCREMENT; i++)
		offsets[i] = offsetsA[i];

	//Ugly print to keep it all on one line
	qDebug() << "Saving offsets to file" << fn.c_str() << "Offsets:"
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
	const char * gName, * wtName;

	switch(gain)
	{
		case LUX1310_GAIN_1:		gName = LUX1310_GAIN_1_FN;		break;
		case LUX1310_GAIN_2:		gName = LUX1310_GAIN_2_FN;		break;
		case LUX1310_GAIN_4:		gName = LUX1310_GAIN_4_FN;		break;
		case LUX1310_GAIN_8:		gName = LUX1310_GAIN_8_FN;		break;
		case LUX1310_GAIN_16:		gName = LUX1310_GAIN_16_FN;		break;
	default:						gName = "";						break;

	}

	switch(wavetableSize)
	{
		case 80:	wtName = LUX1310_WAVETABLE_80_FN; break;
		case 39:	wtName = LUX1310_WAVETABLE_39_FN; break;
		case 30:	wtName = LUX1310_WAVETABLE_30_FN; break;
		case 25:	wtName = LUX1310_WAVETABLE_25_FN; break;
		case 20:	wtName = LUX1310_WAVETABLE_20_FN; break;
		default:	wtName = "";					  break;
	}

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
