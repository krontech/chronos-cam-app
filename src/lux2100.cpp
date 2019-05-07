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
#include "lux2100.h"

#include <QSettings>

/* Select binning or windowed mode. */
#define LUX2100_BINNING_MODE	1

//1080p binned
//Binned half length
UInt8 LUX2100_sram66Clk[414] = {0x80, 0x00, 0x00, 0x20, 0x08, 0xD0, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20,
                                0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08,
                                0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3,
                                0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20,
                                0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08,
                                0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3,
                                0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20,
                                0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08,
                                0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53,
                                0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20,
                                0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08,
                                0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53,
                                0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x00, 0x09, 0x53, 0x80, 0x3C, 0x00, 0x80,
                                0x01, 0x53, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C,
                                0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43,
                                0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80,
                                0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C,
                                0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43,
                                0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x1C, 0x00, 0x80,
                                0x01, 0x43, 0x80, 0x1C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C,
                                0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43,
                                0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80,
                                0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C,
                                0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43,
                                0xA0, 0x98, 0x00, 0x80, 0x01, 0x43, 0x80, 0x98, 0x00, 0x00, 0x01, 0x43, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

//Full resolution 4k
//Non-binned full length
UInt8 LUX8M_sram126Clk[774] =  {0x80, 0x00, 0x00, 0x20, 0x08, 0xD0, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20,
                                0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08,
                                0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3,
                                0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20,
                                0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08,
                                0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3,
                                0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20,
                                0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08,
                                0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3,
                                0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20,
                                0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08,
                                0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3,
                                0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20,
                                0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08,
                                0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53,
                                0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20,
                                0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08,
                                0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53,
                                0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20,
                                0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08,
                                0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53,
                                0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20,
                                0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08,
                                0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x00, 0x09, 0x53, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x53,
                                0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80,
                                0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C,
                                0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43,
                                0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80,
                                0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C,
                                0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43,
                                0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80,
                                0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C,
                                0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43,
                                0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80,
                                0x01, 0x43, 0x80, 0x1C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x1C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x1C,
                                0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43,
                                0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80,
                                0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C,
                                0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43,
                                0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80,
                                0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C,
                                0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43,
                                0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80,
                                0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C,
                                0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43,
                                0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80,
                                0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x98, 0x00, 0x80, 0x01, 0x43, 0x80, 0x98,
                                0x00, 0x00, 0x01, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

//1080p windowed from 4k
//Non-binned half length
UInt8 LUX8M_sram66ClkFRWindow[774] =
                               {0x80, 0x00, 0x00, 0x20, 0x08, 0xD0, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20,
                                0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08,
                                0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3,
                                0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20,
                                0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08,
                                0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3,
                                0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20, 0x09, 0xD3, 0x80, 0x08, 0x00, 0x20,
                                0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08,
                                0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53,
                                0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20,
                                0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08,
                                0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x20, 0x09, 0x53,
                                0x80, 0x08, 0x00, 0x20, 0x09, 0x53, 0x80, 0x08, 0x00, 0x00, 0x09, 0x53, 0x80, 0x3C, 0x00, 0x80,
                                0x01, 0x53, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C,
                                0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43,
                                0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80,
                                0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C,
                                0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43,
                                0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x3C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x1C, 0x00, 0x80,
                                0x01, 0x43, 0x80, 0x1C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C,
                                0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43,
                                0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80,
                                0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C,
                                0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43, 0x80, 0x9C, 0x00, 0x80, 0x01, 0x43,
                                0x80, 0x98, 0x00, 0x80, 0x01, 0x43, 0x80, 0x98, 0x00, 0x00, 0x01, 0x43, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


#define round(x) (floor(x + 0.5))
//#define SCI_DEBUG_PRINTS

#define LUX2100_MIN_HBLANK 2
#define LUX2100_SOF_DELAY  10
#define LUX2100_LV_DELAY   8

LUX2100::LUX2100()
{
	spi = new SPI();
	startDelaySensorClocks = LUX2100_MAGIC_ABN_DELAY;
}

LUX2100::~LUX2100()
{
	delete spi;
}

CameraErrortype LUX2100::init(GPMC * gpmc_inst)
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

CameraErrortype LUX2100::initSensor()
{
	wavetableSize = 66;
	gain = 1;

	gpmc->write32(IMAGER_FRAME_PERIOD_ADDR, 100*4000);	//Disable integration
	gpmc->write32(IMAGER_INT_TIME_ADDR, 100*4100);

	initDAC();

#if LUX2100_BINNING_MODE
	colorBinning = true;
	initLUX2100();
#else
	initLUX8M();
#endif


	delayms(10);

    gpmc->write32(IMAGER_FRAME_PERIOD_ADDR, 100000000/100);	//Enable integration
    gpmc->write32(IMAGER_INT_TIME_ADDR, 100000000/200);

	delayms(50);


	currentRes.hRes = LUX2100_MAX_H_RES;
	currentRes.vRes = LUX2100_MAX_V_RES;
	currentRes.hOffset = 0;
	currentRes.vOffset = 0;
	currentRes.vDarkRows = 0;
	currentRes.bitDepth = LUX2100_BITS_PER_PIXEL;
	setFramePeriod(getMinFramePeriod(&currentRes), &currentRes);
	//mem problem before this
	setIntegrationTime(getMaxIntegrationTime(currentPeriod, &currentRes), &currentRes);

	return SUCCESS;
}

void LUX2100::SCIWrite(UInt8 address, UInt16 data)
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

#ifdef SCI_DEBUG_PRINTS
	if(!first && i != 0)
		qDebug() << "First busy was missed, address:" << address;
	else if(i == 0)
		qDebug() << "No busy detected, something is probably very wrong, address:" << address;

	int readback = SCIRead(address);
	int readback2 = SCIRead(address);
	int readback3 = SCIRead(address);
	if(data != readback)
		qDebug() << "SCI readback wrong, address: " << address << " expected: " << data << " got: " << readback << readback2 << readback3;
#endif
}

void LUX2100::SCIWriteBuf(UInt8 address, const UInt8 * data, UInt32 dataLen)
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

UInt16 LUX2100::SCIRead(UInt8 address)
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

#ifdef SCI_DEBUG_PRINTS
	if(!first && i != 0)
		qDebug() << "Read First busy was missed, address:" << address;
	else if(i == 0)
		qDebug() << "Read No busy detected, something is probably very wrong, address:" << address;
#endif

	//Wait for completion
//	while(gpmc->read16(SENSOR_SCI_CONTROL_ADDR) & SENSOR_SCI_CONTROL_RUN_MASK)
//		count++;
	delayms(1);
	return gpmc->read16(SENSOR_SCI_READ_DATA_ADDR);
}

CameraErrortype LUX2100::autoPhaseCal(void)
{
	setClkPhase(0);
	setClkPhase(1);

	setClkPhase(0);


	int dataCorrect = getDataCorrect();
	qDebug() << "datacorrect" << dataCorrect;
	return SUCCESS;
}

Int32 LUX2100::seqOnOff(bool on)
{
	if (on) {
		gpmc->write32(IMAGER_INT_TIME_ADDR, currentExposure);
	} else {
		gpmc->write32(IMAGER_INT_TIME_ADDR, 0);	//Disable integration
	}
	return SUCCESS;
}

void LUX2100::setReset(bool reset)
{
		gpmc->write16(IMAGE_SENSOR_CONTROL_ADDR, (gpmc->read16(IMAGE_SENSOR_CONTROL_ADDR) & ~IMAGE_SENSOR_RESET_MASK) | (reset ? IMAGE_SENSOR_RESET_MASK : 0));
}

void LUX2100::setClkPhase(UInt8 phase)
{
		gpmc->write16(IMAGE_SENSOR_CLK_PHASE_ADDR, phase);
}

UInt8 LUX2100::getClkPhase(void)
{
		return gpmc->read16(IMAGE_SENSOR_CLK_PHASE_ADDR);
}

UInt32 LUX2100::getDataCorrect(void)
{
	return gpmc->read32(IMAGE_SENSOR_DATA_CORRECT_ADDR);
}

void LUX2100::setSyncToken(UInt16 token)
{
	gpmc->write16(IMAGE_SENSOR_SYNC_TOKEN_ADDR, token);
}

FrameGeometry LUX2100::getMaxGeometry(void)
{
	FrameGeometry size = {
		.hRes = LUX2100_MAX_H_RES,
		.vRes = LUX2100_MAX_V_RES,
		.hOffset = 0,
		.vOffset = 0,
		.vDarkRows = LUX2100_MAX_V_DARK,
		.bitDepth = LUX2100_BITS_PER_PIXEL,
	};
	return size;
}

void LUX2100::setResolution(FrameGeometry *size)
{
	UInt32 hStartBlocks = size->hOffset / LUX2100_HRES_INCREMENT;
	UInt32 hEndblocks = hStartBlocks + (size->hRes / LUX2100_HRES_INCREMENT);
	UInt32 vLastRow = LUX2100_MAX_V_RES + LUX2100_LOW_BOUNDARY_ROWS + LUX2100_HIGH_BOUNDARY_ROWS + LUX2100_HIGH_DARK_ROWS;

#if LUX2100_BINNING_MODE
	/* Binned operation - everything is x2 because it's really a 4K sensor. */
	SCIWrite(0x06, (LUX2100_LEFT_DARK_COLUMNS + hStartBlocks * LUX2100_HRES_INCREMENT) * 2);
	SCIWrite(0x07, (LUX2100_LEFT_DARK_COLUMNS + hEndblocks * LUX2100_HRES_INCREMENT) * 2 - 1);
	SCIWrite(0x08, (LUX2100_LOW_BOUNDARY_ROWS + size->vOffset * 2));
	SCIWrite(0x09, (LUX2100_LOW_BOUNDARY_ROWS + size->vOffset + size->vRes - 1) * 2);
	if (size->vDarkRows) {
		SCIWrite(0x2A, (vLastRow - size->vDarkRows) * 2);
	}
	SCIWrite(0x2B, size->vDarkRows * 2);
#else
	/* Windowed operation - add extra offset to put the readout at the centre. */
	SCIWrite(0x06, (LUX2100_LEFT_DARK_COLUMNS * 2 + 0x3E0) + hStartBlocks * LUX2100_HRES_INCREMENT);
	SCIWrite(0x07, (LUX2100_LEFT_DARK_COLUMNS * 2 + 0x3E0) + hEndblocks * LUX2100_HRES_INCREMENT - 1);
	SCIWrite(0x08, (LUX2100_LOW_BOUNDARY_ROWS * 2 + 0x22C) + size->vOffset);
	SCIWrite(0x09, (LUX2100_LOW_BOUNDARY_ROWS * 2 + 0x22C) + size->vOffset + size->vRes - 1);
	if (size->vDarkRows) {
		SCIWrite(0x2A, (vLastRow * 2) - size->vDarkRows);
	}
	SCIWrite(0x2B, size->vDarkRows);
#endif

	memcpy(&currentRes, size, sizeof(currentRes));
}

UInt32 LUX2100::getMinWavetablePeriod(FrameGeometry *frameSize, UInt32 wtSize)
{
	if(!isValidResolution(frameSize))
		return 0;

	/* Updated to v3.0 datasheet and computed directly in LUX2100 clocks. */
	unsigned int tRead = frameSize->hRes / LUX2100_HRES_INCREMENT;
	unsigned int tTx = 50; /* TODO: Need to check the FPGA build for the TXN pulse width. */
	unsigned int tRow = max(tRead + LUX2100_MIN_HBLANK, wtSize + 3);
	unsigned int tFovf = LUX2100_SOF_DELAY + wtSize + LUX2100_LV_DELAY + 10;
	unsigned int tFovfb = 50; /* TODO: It's not entirely clear what the minimum limit is here. */
	unsigned int tFrame = tRow * (frameSize->vRes + frameSize->vDarkRows) + tTx + tFovf + tFovfb - LUX2100_MIN_HBLANK;

	/* Convert from 75MHz sensor clocks to 100MHz FPGA clocks. */
	qDebug() << "getMinFramePeriod:" << tFrame;
	return ceil(tFrame * 100 / 75);
}

UInt32 LUX2100::getMinFramePeriod(FrameGeometry *frameSize)
{
	unsigned int wtIdeal = (frameSize->hRes / LUX2100_HRES_INCREMENT);
	unsigned int wtSize = lux2100wt[0]->clocks;

	if(!isValidResolution(frameSize))
		return 0;

	/*
	 * Select the longest wavetable that exceeds line readout time,
	 * or fall back to the shortest wavetable for small resolutions.
	 */
	for (int i = 0; lux2100wt[i] != NULL; i++) {
		if (lux2100wt[i]->clocks < wtIdeal) break;
		wtSize = lux2100wt[i]->clocks;
	}

	return getMinWavetablePeriod(frameSize, wtSize);
}

UInt32 LUX2100::getFramePeriod(void)
{
	return currentPeriod;
}

UInt32 LUX2100::getActualFramePeriod(double targetPeriod, FrameGeometry *size)
{
	//Round to nearest timing clock period
	UInt32 clocks = round(targetPeriod * LUX2100_TIMING_CLOCK_FREQ);
	UInt32 minPeriod = getMinFramePeriod(size);
	UInt32 maxPeriod = LUX2100_MAX_SLAVE_PERIOD;

	return within(clocks, minPeriod, maxPeriod);
}

void LUX2100::updateWavetableSetting(void)
{
	/* Search for the longest wavetable that it shorter than the current frame period. */
	const lux2100wavetab_t *wt = NULL;
	int i;

	qDebug() << "Selecting wavetable for period of" << currentPeriod;

	for (i = 0; lux2100wt[i] != NULL; i++) {
		wt = lux2100wt[i];
		if (currentPeriod >= getMinWavetablePeriod(&currentRes, wt->clocks)) break;
	}

	/* Update the wavetable. */
	if (wt) {
		SCIWrite(0x01, 0x0010);         //Disable internal timing engine
		SCIWrite(0x34, wt->clocks);		//non-overlapping readout delay
		//SCIWrite(0x7A, wt->clocks);     //wavetable size ???
		SCIWriteBuf(0x7F, wt->wavetab, wt->length);
		SCIWrite(0x01, 0x0011);			// enable the internal timing engine

		wavetableSize = wt->clocks;
		gpmc->write16(SENSOR_MAGIC_START_DELAY_ADDR, wt->abnDelay);

		qDebug() << "Wavetable size set to" << wavetableSize;
	}
}

UInt32 LUX2100::setFramePeriod(UInt32 period, FrameGeometry *size)
{
	qDebug() << "Requested period" << period;
	UInt32 minPeriod = getMinFramePeriod(size);
	UInt32 maxPeriod = LUX2100_MAX_SLAVE_PERIOD;

	currentPeriod = within(period, minPeriod, maxPeriod);

	// Set the timing generator to handle the frame and line period
	updateWavetableSetting();
	gpmc->write16(SENSOR_LINE_PERIOD_ADDR, max((size->hRes / LUX2100_HRES_INCREMENT)+2, (wavetableSize + 3)) - 1);
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
UInt32 LUX2100::getMaxIntegrationTime(UInt32 period, FrameGeometry *size)
{
	return period - 750;
}

/* setIntegrationTime
 *
 * Sets the integration time of the image sensor to a value as close as possible to requested
 *
 * intTime:	Desired integration time in clocks
 *
 * returns: Actual integration time that was set
 */
UInt32 LUX2100::setIntegrationTime(UInt32 intTime, FrameGeometry *size)
{
	//Set integration time to within limits
	UInt32 maxIntTime = getMaxIntegrationTime(currentPeriod, size);
	UInt32 minIntTime = getMinIntegrationTime(currentPeriod, size);
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
UInt32 LUX2100::getIntegrationTime(void)
{
	return currentExposure;
}

void LUX2100::setSlaveExposure(UInt32 exposure)
{
	//hack to fix line issue. Not perfect, need to properly register this on the sensor clock.
	double linePeriod = max((currentRes.hRes / LUX2100_HRES_INCREMENT)+2, (wavetableSize + 3)) * 1.0/LUX2100_SENSOR_CLOCK;	//Line period in seconds
	UInt32 startDelay = (double)startDelaySensorClocks * LUX2100_TIMING_CLOCK_FREQ / LUX2100_SENSOR_CLOCK;
	double targetExp = (double)exposure / 100000000.0;
	UInt32 expLines = round(targetExp / linePeriod);

	exposure = startDelay + (linePeriod * expLines * 100000000.0);
	//qDebug() << "linePeriod" << linePeriod << "startDelaySensorClocks" << startDelaySensorClocks << "startDelay" << startDelay
	//		 << "targetExp" << targetExp << "expLines" << expLines << "exposure" << exposure;
	gpmc->write32(IMAGER_INT_TIME_ADDR, exposure);
	currentExposure = exposure;
}

//Set up DAC
void LUX2100::initDAC()
{
	//Put DAC in Write through mode (DAC channels update immediately on register write)
	writeDACSPI(0x9000, 0x9000);
}

//Write the data into the selected channel
void LUX2100::writeDAC(UInt16 data, UInt8 channel)
{
    UInt16 dacVal = ((channel & 0x7) << 12) | (data & 0x0FFF);

    if((channel & 0x8) == 0)    //Writing to DAC 0
        writeDACSPI(dacVal, LUX2100_DAC_NOP_COMMAND);
    else
        writeDACSPI(LUX2100_DAC_NOP_COMMAND, dacVal);
}

void LUX2100::writeDACVoltage(UInt8 channel, float voltage)
{
	switch(channel)
	{
        case LUX2100_VABL_VOLTAGE:
            writeDAC((UInt16)(voltage * LUX2100_VABL_SCALE), LUX2100_VABL_VOLTAGE);
			break;
        case LUX2100_VTX2L_VOLTAGE:
            writeDAC((UInt16)(voltage * LUX2100_VTX2L_SCALE), LUX2100_VTX2L_VOLTAGE);
			break;
        case LUX2100_VTXH_VOLTAGE:
            writeDAC((UInt16)(voltage * LUX2100_VTXH_SCALE), LUX2100_VTXH_VOLTAGE);
			break;
        case LUX2100_VDR1_VOLTAGE:
            writeDAC((UInt16)(voltage * LUX2100_VDR1_SCALE), LUX2100_VDR1_VOLTAGE);
			break;
        case LUX2100_VRSTH_VOLTAGE:
            writeDAC((UInt16)(voltage * LUX2100_VRSTH_SCALE), LUX2100_VRSTH_VOLTAGE);
			break;
        case LUX2100_VDR3_VOLTAGE:
            writeDAC((UInt16)(voltage * LUX2100_VDR3_SCALE), LUX2100_VDR3_VOLTAGE);
			break;
        case LUX2100_VRDH_VOLTAGE:
            writeDAC((UInt16)(voltage * LUX2100_VRDH_SCALE), LUX2100_VRDH_VOLTAGE);
			break;
        case LUX2100_VTX2H_VOLTAGE:
            writeDAC((UInt16)(voltage * LUX2100_VTX2H_SCALE), LUX2100_VTX2H_VOLTAGE);
            break;
        case LUX2100_VTXL_VOLTAGE:
            writeDAC((UInt16)((LUX2100_VTXL_OFFSET - (float)voltage) * LUX2100_VTXL_SCALE), LUX2100_VTXL_VOLTAGE);
            break;
        case LUX2100_VPIX_OP_VOLTAGE:
            writeDAC((UInt16)(voltage * LUX2100_VPIX_OP_SCALE), LUX2100_VPIX_OP_VOLTAGE);
            break;
        case LUX2100_VDR2_VOLTAGE:
            writeDAC((UInt16)(voltage * LUX2100_VDR2_SCALE), LUX2100_VDR2_VOLTAGE);
            break;
        case LUX2100_VPIX_LDO_VOLTAGE:
            writeDAC((UInt16)((LUX2100_VPIX_LDO_OFFSET - (float)voltage) * LUX2100_VPIX_LDO_SCALE), LUX2100_VPIX_LDO_VOLTAGE);
            break;
        case LUX2100_VRSTPIX_VOLTAGE:
            writeDAC((UInt16)(voltage * LUX2100_VRSTPIX_SCALE), LUX2100_VRSTPIX_VOLTAGE);
        break;
		default:
			return;
        break;
	}
}

//Performs an SPI write to the DAC
int LUX2100::writeDACSPI(UInt16 data0, UInt16 data1)
{
	UInt8 tx[4];
	UInt8 rx[sizeof(tx)];
	int retval;

	tx[3] = data0 >> 8;
	tx[2] = data0 & 0xFF;
	tx[1] = data1 >> 8;
	tx[0] = data1 & 0xFF;

	setDACCS(false);
	//delayms(1);
	retval = spi->Transfer((uint64_t) tx, (uint64_t) rx, sizeof(tx));
	//delayms(1);
	setDACCS(true);
	return retval;
}

void LUX2100::setDACCS(bool on)
{
	lseek(dacCSFD, 0, SEEK_SET);
	write(dacCSFD, on ? "1" : "0", 1);
}

unsigned int LUX2100::enableAnalogTestMode(void)
{
	seqOnOff(false);
	delayms(10);

	/* Switch to analog test mode. */
	SCIWrite(0x04, 0x0000);
#if LUX2100_BINNING_MODE
	SCIWrite(0x56, colorBinning ? 0xB120 : 0xB121);
#else
	SCIWrite(0x56, 0xB001);
#endif

	/* Configure the desired test voltage. */
	SCIWrite(0x67, 0x6D11);

	seqOnOff(true);
	return 31;
}

void LUX2100::disableAnalogTestMode(void)
{
	seqOnOff(false);
	delayms(10);

	/* Disable the test voltage. */
	SCIWrite(0x04, 0x0000);
	SCIWrite(0x67, 0x1E11);
#if LUX2100_BINNING_MODE
	SCIWrite(0x56, colorBinning ? 0xA120 : 0xA121);
#else
	SCIWrite(0x56, 0x9001);
#endif
	seqOnOff(true);
}

void LUX2100::setAnalogTestVoltage(unsigned int voltage)
{
	SCIWrite(0x67, 0x6011 | (voltage << 8));
}

//Remaps data channels to ADC channel index
const char adcChannelRemap[] = {
	0,  8, 16, 24, 4, 12, 20, 28,	/* Data channels 0 to 7 */
	2, 10, 18, 26, 6, 14, 22, 30,	/* Data channels 8 to 15 */
	1,  9, 17, 25, 5, 13, 21, 29,	/* Data channels 16 to 23 */
	3, 11, 19, 27, 7, 15, 23, 31	/* Data channels 24 to 31 */
};

//Sets ADC offset for one channel
//Converts the input 2s complement value to the sensors's weird sign bit plus value format (sort of like float, with +0 and -0)
void LUX2100::setADCOffset(UInt8 channel, Int16 offset)
{
    UInt8 intChannel = adcChannelRemap[channel];
    SCIWrite(0x04, 0x0001); // switch to datapath register space

	if(offset < 0)
    {
        SCIWrite(0x10 + (intChannel >= 16 ? 0x40 + intChannel - 16 : intChannel), (-offset & 0x3FF));
        SCIWrite(0x0F + (intChannel >= 16 ? 0x40 : 0x0), SCIRead(0x0F + (intChannel >= 16 ? 0x40 : 0x0)) | (1 << (intChannel & 0xF)));
    }
	else
    {
        SCIWrite(0x10 + (intChannel >= 16 ? 0x40 + intChannel - 16 : intChannel), offset & 0x3FF);
        SCIWrite(0x0F + (intChannel >= 16 ? 0x40 : 0x0), SCIRead(0x0F + (intChannel >= 16 ? 0x40 : 0x0)) & ~(1 << (intChannel & 0xF)));
    }

    SCIWrite(0x04, 0x0000); // switch back to sensor register space
}

//This doesn't seem to work. Sensor locks up
#if 0
void LUX2100::adcOffsetTraining(FrameGeometry *frameSize, UInt32 address, UInt32 numFrames)
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
}
#else
void LUX2100::offsetCorrectionIteration(FrameGeometry *geometry, int *offsets, UInt32 address, UInt32 framesToAverage, int iter)
{
	UInt32 numRows = geometry->vDarkRows + geometry->vRes;
	UInt32 rowSize = (geometry->hRes * LUX2100_BITS_PER_PIXEL) / 8;
	UInt32 samples = (numRows * framesToAverage * geometry->hRes / LUX2100_HRES_INCREMENT);
	UInt32 adcAverage[LUX2100_HRES_INCREMENT];
	UInt32 adcStdDev[LUX2100_HRES_INCREMENT];

	UInt32 *pxbuffer = (UInt32 *)malloc(rowSize * numRows * framesToAverage);

	for(int i = 0; i < LUX2100_HRES_INCREMENT; i++) {
		adcAverage[i] = 0;
		adcStdDev[i] = 0;
	}
	/* Read out the black regions from all frames. */
	for (int i = 0; i < framesToAverage; i++) {
		UInt32 *rowbuffer = pxbuffer + (rowSize * numRows * i) / sizeof(UInt32);
		gpmc->readAcqMem(rowbuffer, address, rowSize * numRows);
		address += gpmc->read32(SEQ_FRAME_SIZE_ADDR);
	}

	/* Find the per-ADC averages and standard deviation */
	for (int row = 0; row < (numRows * framesToAverage); row++) {
		for (int col = 0; col < geometry->hRes; col++) {
			adcAverage[col % LUX2100_HRES_INCREMENT] += readPixelBuf12((UInt8 *)pxbuffer, row * geometry->hRes + col);
		}
	}
	for (int row = 0; row < (numRows * framesToAverage); row++) {
		for (int col = 0; col < geometry->hRes; col++) {
			UInt16 pix = readPixelBuf12((UInt8 *)pxbuffer, row * geometry->hRes + col);
			UInt16 avg = adcAverage[col % LUX2100_HRES_INCREMENT] / samples;
			adcStdDev[col % LUX2100_HRES_INCREMENT] += (pix - avg) * (pix - avg);
		}
	}
	for(int col = 0; col < LUX2100_HRES_INCREMENT; col++) {
		adcStdDev[col] = sqrt(adcStdDev[col] / (samples - 1));
		adcAverage[col] /= samples;
	}
	free(pxbuffer);

	/* Train the ADC for a target of: Average = Footroom + StandardDeviation */
	for(int col = 0; col < LUX2100_HRES_INCREMENT; col++) {
		UInt16 avg = adcAverage[col];
		UInt16 dev = adcStdDev[col];
		if (iter == 0) {
			offsets[col] = -(avg - dev - 32) * 3;
		} else {
			//offsets[col] = offsets[col] - (avg - dev - 32) / 2;
			//HACK Pushing the offset a little lower to make up for the lack of iterations.
			offsets[col] = offsets[col] - (avg - dev - 64) / 2;
		}
		offsets[col] = within(offsets[col], -1023, 1023);
		setADCOffset(col, offsets[col]);
	}

	char debugstr[8*LUX2100_HRES_INCREMENT];
	int debuglen = 0;

	if (iter == 0) {
		debuglen = 0;
		for (int col=0; col < LUX2100_HRES_INCREMENT; col++) {
			debuglen += sprintf(debugstr + debuglen, " %+4d", adcAverage[col]);
		}
		qDebug("ADC Initial:%s", debugstr);

		debuglen = 0;
		for (int col=0; col < LUX2100_HRES_INCREMENT; col++) {
			debuglen += sprintf(debugstr + debuglen, " %+4d", adcStdDev[col]);
		}
		qDebug("ADC StdDev: %s", debugstr);
	}

	debuglen = 0;
	for (int col=0; col < LUX2100_HRES_INCREMENT; col++) {
		debuglen += sprintf(debugstr + debuglen, " %+4d", offsets[col]);
	}
	qDebug("ADC Offsets:%s", debugstr);
}

void LUX2100::adcOffsetTraining(FrameGeometry *size, UInt32 address, UInt32 numFrames)
{
	Int32 offsets[LUX2100_HRES_INCREMENT];
	struct timespec tRefresh;
	//unsigned int iterations = 8;
	// HACK: We really need like 8-16 iterations, to get good results, but reduce it
	// down further to speed up the user experience and make up for it with a larger
	// footroom.
	unsigned int iterations = 4;

	tRefresh.tv_sec = 0;
	tRefresh.tv_nsec = ((numFrames+10) * currentPeriod * 1000000000ULL) / LUX2100_TIMING_CLOCK_FREQ;

	/* Clear out the ADC Offsets. */
	for (int i = 0; i < LUX2100_HRES_INCREMENT; i++) {
		offsets[i] = 0;
		setADCOffset(i, 0);
	}

	/* Tune the ADC offset calibration. */
	for (int i = 0; i < iterations; i++) {
		nanosleep(&tRefresh, NULL);
		offsetCorrectionIteration(size, offsets, address, numFrames, i);
	}
}
#endif

//Generate a filename string used for calibration values that is specific to the current gain and wavetable settings
std::string LUX2100::getFilename(const char * filename, const char * extension)
{
	char gName[16];
	char wtName[16];

	snprintf(gName, sizeof(gName), "G%d", gain);
	snprintf(wtName, sizeof(wtName), "WT%d", wavetableSize);
	return std::string(filename) + "_" + gName + "_" + wtName + extension;
}


Int32 LUX2100::setGain(UInt32 gainSetting)
{
	switch(gainSetting)
	{
	case 1:
		/* Set the real minimum gain is of 1.333 */
		SCIWrite(0x57, 0x007F);
		SCIWrite(0x58, 0x037F);
	break;

	case 2:
		/* Set Luxima's minimum recommended gain of x2.6 */
		SCIWrite(0x57, 0x01FF);	//gain selection sampling cap (11)	12 bit
		SCIWrite(0x58, 0x030F);	//serial gain and gain feedback cap (8) 7 bit
	break;

	case 4:
		//writeDACVoltage(LUX2100_VRSTH_VOLTAGE, 3.6);

		SCIWrite(0x57, 0x0FFF);	//gain selection sampling cap (11)	12 bit
		SCIWrite(0x58, 0x011F); //serial gain and gain feedback cap (8) 7 bit
	break;

	case 8:
		//writeDACVoltage(LUX2100_VRSTH_VOLTAGE, 3.6);

		SCIWrite(0x57, 0x0FFF);	//gain selection sampling cap (11)	12 bit
		SCIWrite(0x58, 0x001F); //serial gain and gain feedback cap (8) 7 bit
	break;

	case 16:
		//writeDACVoltage(LUX2100_VRSTH_VOLTAGE, 2.6);

		SCIWrite(0x57, 0x0FFF);	//gain selection sampling cap (11)	12 bit
		SCIWrite(0x58, 0x000F); //serial gain and gain feedback cap (8) 7 bit
	break;

	default:
		return CAMERA_INVALID_SETTINGS;
	}

	gain = gainSetting;
	return SUCCESS;
}

// GR
// BG
//
UInt8 LUX2100::getFilterColor(UInt32 h, UInt32 v)
{
	if((v & 1) == 0)	//If on an even line
		return ((h & 1) == 0) ? FILTER_COLOR_GREEN : FILTER_COLOR_RED;
	else	//Odd line
		return ((h & 1) == 0) ? FILTER_COLOR_BLUE : FILTER_COLOR_GREEN;

}

Int32 LUX2100::initLUX2100(void)
{
    writeDACVoltage(LUX2100_VRSTH_VOLTAGE, 3.3);
    writeDACVoltage(LUX2100_VTX2L_VOLTAGE, 0.0);
    writeDACVoltage(LUX2100_VRSTPIX_VOLTAGE, 2.0);
    writeDACVoltage(LUX2100_VABL_VOLTAGE, 0.0);
    writeDACVoltage(LUX2100_VTXH_VOLTAGE, 3.3);
    writeDACVoltage(LUX2100_VTX2H_VOLTAGE, 3.3);
    writeDACVoltage(LUX2100_VTXL_VOLTAGE, 0.0);

    writeDACVoltage(LUX2100_VDR1_VOLTAGE, 2.5);
    writeDACVoltage(LUX2100_VDR2_VOLTAGE, 2.0);
    writeDACVoltage(LUX2100_VDR3_VOLTAGE, 1.5);
    writeDACVoltage(LUX2100_VRDH_VOLTAGE, 0.0);  //Datasheet says this should be grounded
    writeDACVoltage(LUX2100_VPIX_LDO_VOLTAGE, 3.3);
    writeDACVoltage(LUX2100_VPIX_OP_VOLTAGE, 0.0);



    delayms(10);		//Settling time


    setReset(true);

    setReset(false);

    delayms(1);		//Seems to be required for SCI clock to get running

    SCIWrite(0x7E, 0);	//reset all registers
    //delayms(1);

    //Set gain to 2.6
    SCIWrite(0x57, 0x007F);     //gain = gain=(#1's in 0x57[11:0]+4)/(#1's in 0x58[6:0]+1) = 12/8 = 1.333
    SCIWrite(0x58, 0x037F);

//    SCIWrite(0x57, 0x01FF);     //gain = gain=(#1's in 0x57[11:0]+4)/(#1's in 0x58[6:0]+1) = 13/5 = 2.5
//    SCIWrite(0x58, 0x030F);
    SCIWrite(0x76, 0x0079);    //Serial gain setting V2: Serial Gain = (#1's in 0x76[8:4])/(#1's in 0x58[10:8]+1) = 3/3

    //Set up for 66Tclk wavetable
	SCIWrite(0x34, 0x0042); //Readout delay
    SCIWrite(0x56, colorBinning ? 0xA120 : 0xA121); //Set up binning mode for mono sensor
    SCIWrite(0x06, 0x0040); //Start of standard window (x direction)
    SCIWrite(0x07, 0x0F3F); //End of standard window (x direction)
    SCIWrite(0x08, 0x0010); //Start of standard window (Y direction)
    SCIWrite(0x09, 0x087E); //End of standard window (Y direction)

    //Internal control registers
	SCIWrite(0x03, LUX2100_MIN_HBLANK);
	SCIWrite(0x30, LUX2100_SOF_DELAY << 8); //Start of frame delay
	SCIWrite(0x5B, LUX2100_LV_DELAY);       // line valid delay to match ADC latency
    SCIWrite(0x5F, 0x0000); // internal control register
    SCIWrite(0x78, 0x0803); // internal clock timing
    SCIWrite(0x2D, 0x0084); // state for idle controls
    SCIWrite(0x2E, 0x0000); // state for idle controls
    SCIWrite(0x2F, 0x0040); // state for idle controls
    SCIWrite(0x62, 0x2603); // ADC clock controls
    SCIWrite(0x60, 0x0300); // enable on chip termination
    SCIWrite(0x79, 0x0003); // internal control register
    SCIWrite(0x7D, 0x0001); // internal control register
    SCIWrite(0x6A, 0xAA88); // internal control register
    SCIWrite(0x6B, 0xAC88); // internal control register
    SCIWrite(0x6C, 0x8AAA); // internal control register
    SCIWrite(0x04, 0x0001); // switch to datapath register space
    SCIWrite(0x05, 0x0007); // delay to match ADC latency
    SCIWrite(0x04, 0x0000); // switch back to main register space


    //Training sequence
    SCIWrite(0x53, 0x0FC0); // pclk channel output during vertical blanking
    SCIWrite(0x04, 0x0001); // switch to datapath register space
    SCIWrite(0x03, 0x1FC0); // always output custom digital pattern
    SCIWrite(0x04, 0x0000); // switch back to sensor register space

    //Sensor is now outputting 0xFC0 on all ports. Receiver will automatically sync when the clock phase is right
    autoPhaseCal();

    //Return to data valid mode
    SCIWrite(0x53, 0x0F00); // pclk channel output during vertical blanking
    SCIWrite(0x55, 0x0FC0); // pclk channel output during optical black
    SCIWrite(0x04, 0x0001); // switch to datapath register space
    SCIWrite(0x03, 0x0AB8); // custom digital pattern for blanking output
    SCIWrite(0x05, 0x0007); // delay to match adc latency
    SCIWrite(0x04, 0x0000); // switch to sensor register space

    //Load the wavetable
	SCIWriteBuf(0x7F, lux2100wt[0]->wavetab, lux2100wt[0]->length);

    for(int i = 0; i < LUX2100_HRES_INCREMENT; i++)
        setADCOffset(i, 0);

    SCIWrite(0x04, 0x0001); // switch to datapath register space
    SCIWrite(0x0E, 0x0001); // Enable application of ADC offsets during v blank
    SCIWrite(0x04, 0x0000); // switch back to sensor register space

    SCIWrite(0x04, 0x0000); // switch to sensor register space


    //Enable timing engine
    SCIWrite(0x01, 0x0011); // enable the internal timing engine

    return SUCCESS;
}

Int32 LUX2100::initLUX8M_2()
{
    writeDACVoltage(LUX2100_VRSTH_VOLTAGE, 3.3);
    writeDACVoltage(LUX2100_VTX2L_VOLTAGE, 0.0);
    writeDACVoltage(LUX2100_VRSTPIX_VOLTAGE, 2.0);
    writeDACVoltage(LUX2100_VABL_VOLTAGE, 0.3);
    writeDACVoltage(LUX2100_VTXH_VOLTAGE, 3.3);
    writeDACVoltage(LUX2100_VTX2H_VOLTAGE, 3.3);
    writeDACVoltage(LUX2100_VTXL_VOLTAGE, 0.0);

    writeDACVoltage(LUX2100_VDR1_VOLTAGE, 2.5);
    writeDACVoltage(LUX2100_VDR2_VOLTAGE, 2.0);
    writeDACVoltage(LUX2100_VDR3_VOLTAGE, 1.5);
    writeDACVoltage(LUX2100_VRDH_VOLTAGE, 0.0);  //Datasheet says this should be grounded
    writeDACVoltage(LUX2100_VPIX_LDO_VOLTAGE, 3.3);
    writeDACVoltage(LUX2100_VPIX_OP_VOLTAGE, 0.0);



    delayms(10);		//Settling time


    setReset(true);

    setReset(false);

    delayms(1);		//Seems to be required for SCI clock to get running

    SCIWrite(0x7E, 0);	//reset all registers
    //delayms(1);

    //Set gain to 2.6
    SCIWrite(0x57, 0x01FF);
    SCIWrite(0x58, 0x030F);
    SCIWrite(0x76, 0x0079);    //Serial gain setting

    //Set up for 66Tclk wavetable
     SCIWrite(0x34, 0x0080); //Readout delay
    SCIWrite(0x30, 0x0A00); //Start of frame delay
     SCIWrite(0x56, 0x1001); //Set ub binning mode for mono sensor
     SCIWrite(0x06, 0x0040); //Start of standard window (x direction)
     SCIWrite(0x07, 0x0F7F); //End of standard window (x direction)
     SCIWrite(0x08, 0x0000); //Start of standard window (Y direction)
     SCIWrite(0x09, 0x088F); //End of standard window (Y direction)


/*
    SCIWrite(0x34, 0x0080); //readout delay
    SCIWrite(0x30, 0x0A01); //start of frame delay
    SCIWrite(0x56, 0x1001); //set up operation mode
    SCIWrite(0x06, 0x0040); //Start of standard window (x direction)
    SCIWrite(0x07, 0x0F7F); //End of standard window (x direction)
    SCIWrite(0x08, 0x0000); //Start of standard window (Y direction)
    SCIWrite(0x09, 0x088F); //End of standard window (Y direction)
*/
    //Internal control registers
    SCIWrite(0x5B, 0x0008); // line valid delay to match ADC latency
    SCIWrite(0x5F, 0x0000); // internal control register
    SCIWrite(0x78, 0x0803); // internal clock timing
    SCIWrite(0x2D, 0x0084); // state for idle controls
    SCIWrite(0x2E, 0x0000); // state for idle controls
    SCIWrite(0x2F, 0x0040); // state for idle controls
    SCIWrite(0x62, 0x2603); // ADC clock controls
    SCIWrite(0x60, 0x0300); // enable on chip termination
    SCIWrite(0x79, 0x0003); // internal control register
    SCIWrite(0x7D, 0x0001); // internal control register
    SCIWrite(0x6A, 0xAA88); // internal control register
    SCIWrite(0x6B, 0xAC88); // internal control register
    SCIWrite(0x6C, 0x8AAA); // internal control register
    SCIWrite(0x04, 0x0001); // switch to datapath register space
    SCIWrite(0x05, 0x0007); // delay to match ADC latency
    SCIWrite(0x04, 0x0000); // switch back to main register space


    //Training sequence
    SCIWrite(0x53, 0x0FC0); // pclk channel output during vertical blanking
    SCIWrite(0x04, 0x0001); // switch to datapath register space
    SCIWrite(0x03, 0x1FC0); // always output custom digital pattern
    SCIWrite(0x04, 0x0000); // switch back to sensor register space

    //Sensor is now outputting 0xFC0 on all ports. Receiver will automatically sync when the clock phase is right
    autoPhaseCal();

    //Return to data valid mode
    SCIWrite(0x53, 0x0F00); // pclk channel output during vertical blanking
    SCIWrite(0x04, 0x0001); // switch to datapath register space
    SCIWrite(0x03, 0x0AB8); // custom digital pattern for blanking output
    SCIWrite(0x05, 0x0007); // delay to match adc latency
    SCIWrite(0x04, 0x0000); // switch to sensor register space

    //Load the wavetable
    SCIWriteBuf(0x7F, LUX8M_sram126Clk, 774 /*sizeof(sram80Clk)*/);

    for(int i = 0; i < LUX2100_HRES_INCREMENT; i++)
        setADCOffset(i, 0);

    SCIWrite(0x04, 0x0001); // switch to datapath register space
    SCIWrite(0x0E, 0x0001); // Enable application of ADC offsets during v blank
    SCIWrite(0x04, 0x0000); // switch back to sensor register space

    SCIWrite(0x04, 0x0000); // switch to sensor register space


    //Enable timing engine
    SCIWrite(0x01, 0x0011); // enable the internal timing engine

    return SUCCESS;
}

Int32 LUX2100::initLUX8M()
{
    writeDACVoltage(LUX2100_VRSTH_VOLTAGE, 3.3);
    writeDACVoltage(LUX2100_VTX2L_VOLTAGE, 0.0);
    writeDACVoltage(LUX2100_VRSTPIX_VOLTAGE, 2.0);
    writeDACVoltage(LUX2100_VABL_VOLTAGE, 0.3);
    writeDACVoltage(LUX2100_VTXH_VOLTAGE, 3.3);
    writeDACVoltage(LUX2100_VTX2H_VOLTAGE, 3.3);
    writeDACVoltage(LUX2100_VTXL_VOLTAGE, 0.0);

    writeDACVoltage(LUX2100_VDR1_VOLTAGE, 2.5);
    writeDACVoltage(LUX2100_VDR2_VOLTAGE, 2.0);
    writeDACVoltage(LUX2100_VDR3_VOLTAGE, 1.5);
    writeDACVoltage(LUX2100_VRDH_VOLTAGE, 0.0);  //Datasheet says this should be grounded
    writeDACVoltage(LUX2100_VPIX_LDO_VOLTAGE, 3.3);
    writeDACVoltage(LUX2100_VPIX_OP_VOLTAGE, 0.0);



    delayms(10);		//Settling time


    setReset(true);

    setReset(false);

    delayms(1);		//Seems to be required for SCI clock to get running

    SCIWrite(0x7E, 0);	//reset all registers
    //delayms(1);

   /* //Set gain to 2.6
    SCIWrite(0x57, 0x01FF);
    SCIWrite(0x58, 0x030F);
    SCIWrite(0x76, 0x0079);    //Serial gain setting
*/
    //Set up for 126Tclk wavetable

    //SCIWrite(0x34, 0x0080); //readout delay (full res)
    SCIWrite(0x34, 0x0042); //Readout delay (for half size wavetable)
    SCIWrite(0x30, 0x0A00); //start of frame delay. The datasheet says to write 0x0A01, but this causes the sensor to not output any data or sync codes other than H blank
    SCIWrite(0x56, 0x9001); //set up operation mode
    SCIWrite(0x06, 0x0040+0x3E0); //Start of standard window (x direction)
    SCIWrite(0x07, 0x07BF+0x3E0); //End of standard window (x direction)
    SCIWrite(0x08, 0x0010+0x22C); //Start of standard window (Y direction)
    SCIWrite(0x09, 0x0447+0x22C); //End of standard window (Y direction)
/*
    SCIWrite(0x06, 0x0040); //Start of standard window (x direction)
    SCIWrite(0x07, 0x07BF); //End of standard window (x direction)
    SCIWrite(0x08, 0x0010); //Start of standard window (Y direction)
    SCIWrite(0x09, 0x0447); //End of standard window (Y direction)
*/
    SCIWrite(0x79, 0x0133); //???

    SCIWrite(0x03, 0x0003); //H blank to 3 clocks

    SCIWrite(0x57, 0x01FF); //gain_sel_samp
    SCIWrite(0x58, 0x030F); //gain_serial, gain_sel_fb
    SCIWrite(0x76, 0x0079); //col_cap_en

    SCIWrite(0x5B, 0x0008); //lv_delay of 8
    SCIWrite(0x5F, 0x0000); //pwr_en_serializer_b (all seralizers enabled)
    SCIWrite(0x78, 0x0D03); //internal clock timing
    SCIWrite(0x62, 0x2603); //ADC clock controls
    SCIWrite(0x60, 0x0300); //disable on chip termination
    SCIWrite(0x66, 0x0000); //???
    SCIWrite(0x2D, 0x0084); //state for idle controls
    SCIWrite(0x2E, 0x0000); //state for idle controls
    SCIWrite(0x2F, 0x0040); //state for idle controls
    SCIWrite(0x7D, 0x0001); //internal control register
    SCIWrite(0x6A, 0xAA8A); //internal control register
    SCIWrite(0x6B, 0xA88A); //internal control register
    SCIWrite(0x6C, 0x89AA); //internal control register

    //Training sequence
    SCIWrite(0x53, 0x0FC0); // pclk channel output during vertical blanking
    SCIWrite(0x04, 0x0001); // switch to datapath register space
    SCIWrite(0x03, 0x1FC0); // always output custom digital pattern
    SCIWrite(0x04, 0x0000); // switch back to sensor register space

    //Sensor is now outputting 0xFC0 on all ports. Receiver will automatically sync when the clock phase is right
    autoPhaseCal();
    //SCIWrite(0x0100, 0x0010);

    //Return to data valid mode
    SCIWrite(0x53, 0x0F00); // pclk channel output during vertical blanking
    SCIWrite(0x55, 0x0FC0); // pclk channel output during optical black
    SCIWrite(0x04, 0x0001); // switch to datapath register space
    SCIWrite(0x03, 0x0AB8); // custom digital pattern for blanking output
    SCIWrite(0x05, 0x0007); // delay to match adc latency
    SCIWrite(0x0E, 0x0001); // enable applying adc offset registers during vertical blanking
    SCIWrite(0x04, 0x0000); // switch to sensor register space

    //Load the wavetable
    SCIWriteBuf(0x7F, LUX8M_sram66ClkFRWindow, 774 );



/*


    //Internal control registers

    SCIWrite(0x5B, 0x0008); //line valid delay to match adc latency
    SCIWrite(0x5F, 0x0000); //internal control register
    SCIWrite(0x78, 0x0803); //internal clock timing
    SCIWrite(0x2D, 0x0084); //state for idle controls
    SCIWrite(0x2E, 0x0000); //state for idle controls
    SCIWrite(0x2F, 0x0040); //state for idle controls
    SCIWrite(0x62, 0x2603); //ADC clock controls
    SCIWrite(0x60, 0x0300); //enable on chip termination
    SCIWrite(0x7D, 0x0001); //internal control register
    SCIWrite(0x6A, 0xAA35); //internal control register
    SCIWrite(0x6B, 0xA388); //internal control register
    SCIWrite(0x6C, 0x8AAA); //internal control register

    //Training sequence
    SCIWrite(0x53, 0x0FC0); // pclk channel output during vertical blanking
    SCIWrite(0x04, 0x0001); // switch to datapath register space
    SCIWrite(0x03, 0x1FC0); // always output custom digital pattern
    SCIWrite(0x04, 0x0000); // switch back to sensor register space

    //Sensor is now outputting 0xFC0 on all ports. Receiver will automatically sync when the clock phase is right
    autoPhaseCal();

    //Return to data valid mode
    SCIWrite(0x53, 0x0F00); // pclk channel output during vertical blanking
    SCIWrite(0x04, 0x0001); // switch to datapath register space
    SCIWrite(0x03, 0x0AB8); // custom digital pattern for blanking output
    SCIWrite(0x05, 0x0007); // delay to match adc latency
    SCIWrite(0x04, 0x0000); // switch to sensor register space


    //Load the wavetable
    SCIWriteBuf(0x7F, LUX8M_sram126Clk, 774 );


    for(int i = 0; i < LUX2100_HRES_INCREMENT; i++)
        setADCOffset(i, 0);

    SCIWrite(0x04, 0x0001); // switch to datapath register space
    SCIWrite(0x0E, 0x0001); // Enable application of ADC offsets during v blank
    SCIWrite(0x04, 0x0000); // switch back to sensor register space

    SCIWrite(0x04, 0x0000); // switch to sensor register space
*/

    //Enable timing engine
    SCIWrite(0x01, 0x0011); // enable the internal timing engine

    return SUCCESS;
}
