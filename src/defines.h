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

#define RAM_SPD_I2C_BUS_FILE		"/dev/i2c-1"
#define RAM_SPD_I2C_ADDRESS_STICK_0 0x50
#define RAM_SPD_I2C_ADDRESS_STICK_1 0x51
#define CAMERA_EEPROM_I2C_ADDR		0x54

#define SERIAL_NUMBER_OFFSET	0
#define SERIAL_NUMBER_MAX_LEN	32		//Maximum number of characters in serial number

#define CAMERA_APP_VERSION		"0.4.0-beta"
#define ACCEPTABLE_FPGA_VERSION	3

// Network Storage Paths
#define NFS_STORAGE_MOUNT	"/media/nfs"
#define SMB_STORAGE_MOUNT	"/media/smb"

#endif // DEFINES_H
