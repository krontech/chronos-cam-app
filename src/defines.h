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
#ifndef DEFINES_H
#define DEFINES_H

#define FPN_FILENAME		"fpn"

#define IMAGE_SENSOR_SPI	"/dev/spidev3.0"
#define	IMAGE_SENSOR_SPI_SPEED	500000
#define	IMAGE_SENSOR_SPI_BITS	16

//FPGA config
#define FPGA_PROGRAMN_PATH		"/sys/class/gpio/gpio47/value"
#define FPGA_INITN_PATH			"/sys/class/gpio/gpio45/value"
#define FPGA_DONE_PATH			"/sys/class/gpio/gpio52/value"
#define FPGA_SN_PATH			"/sys/class/gpio/gpio58/value"
#define FPGA_HOLDN_PATH			"/sys/class/gpio/gpio53/value"

//Base address of registers
#define GPMC_REGISTER_OFFSET		0x0
#define	GPMC_REGISTER_LEN			0x1000000
#define GPMC_RAM_OFFSET				0x1000000
#define GPMC_RAM_LEN				0x1000000

//IO
#define IO1DAC_TIMER_BASE			0x48048000		//Timer6
#define IO2DAC_TIMER_BASE			0x4804A000		//Timer7
#define	IO_DAC_FS					6.6				//DAC threshold at full scale
#define IO2IN_PATH					"/sys/class/gpio/gpio127/value"

#define RAM_SPD_I2C_BUS_FILE		"/dev/i2c-1"
#define RAM_SPD_I2C_ADDRESS_STICK_0 0x50
#define RAM_SPD_I2C_ADDRESS_STICK_1 0x51
#define CAMERA_EEPROM_I2C_ADDR		0x54

#define SERIAL_NUMBER_OFFSET	0
#define SERIAL_NUMBER_MAX_LEN	32		//Maximum number of characters in serial number

#define CAMERA_APP_VERSION		"0.2.1"
#define ACCEPTABLE_FPGA_VERSION	2

#endif // DEFINES_H
