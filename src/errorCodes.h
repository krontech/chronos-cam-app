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
#ifndef ERRORCODES_H
#define ERRORCODES_H

/* Please do not re-number
 * 
 * If you need to add new errors, add them at the end of the local list (CAMERA, ECP5, etc)
 * so that error codes will be the same between all builds from here on.
 * 
 * New regions should be given a separate segment at the end of the current list.
 */

typedef enum CameraErrortype
{
	SUCCESS                                   =   0,
	

	CAMERA_SUCCESS                            =   0, /* try to use SUCCESS instead */
	CAMERA_THREAD_ERROR                       =   1,
	CAMERA_ALREADY_RECORDING                  =   2,
	CAMERA_NOT_RECORDING                      =   3,
	CAMERA_NO_RECORDING_PRESENT               =   4,
	CAMERA_IN_PLAYBACK_MODE                   =   5,
	CAMERA_ERROR_SENSOR                       =   6,
	CAMERA_INVALID_IMAGER_SETTINGS            =   7,
	CAMERA_FILE_NOT_FOUND                     =   8,
	CAMERA_FILE_ERROR                         =   9,
	CAMERA_ERROR_IO                           =  10,
	CAMERA_INVALID_SETTINGS                   =  11,
	CAMERA_FPN_CORRECTION_ERROR               =  12,
	CAMERA_CLIPPED_ERROR                      =  13,
	CAMERA_LOW_SIGNAL_ERROR                   =  14,
	CAMERA_RECORD_FRAME_ERROR                 =  15,
	CAMERA_ITERATION_LIMIT_EXCEEDED           =  16,
	CAMERA_GAIN_CORRECTION_ERROR              =  17,
	CAMERA_MEM_ERROR                          =  18,
	CAMERA_WRONG_FPGA_VERSION                 =  19,
	
	
	ECP5_ALREAY_OPEN                          = 101,
	ECP5_NOT_OPEN                             = 102,
	ECP5_GPIO_ERROR                           = 103,
	ECP5_SPI_OPEN_FAIL                        = 104,
	ECP5_IOCTL_FAIL                           = 105,
	ECP5_FILE_IO_ERROR                        = 106,
	ECP5_MEMORY_ERROR                         = 107,
	ECP5_WRONG_DEVICE_ID                      = 108,
	ECP5_DONE_NOT_ASSERTED                    = 109,
	
	
	GPMCERR_FOPEN                             = 201,
	GPMCERR_MAP                               = 202,
	
	
	IO_ERROR_OPEN                             = 301,
	IO_FILE_ERROR                             = 302,
	
	
	LUPA1300_SPI_OPEN_FAIL                    = 401,
	LUPA1300_NO_DATA_VALID_WINDOW             = 402,
	LUPA1300_INSUFFICIENT_DATA_VALID_WINDOW   = 403,
	LUX1310_FILE_ERROR                        = 404,
	
	
	SPI_NOT_OPEN                              = 501,
	SPI_ALREAY_OPEN                           = 502,
	SPI_OPEN_FAIL                             = 503,
	SPI_IOCTL_FAIL                            = 504,
	
	
	UI_FILE_ERROR                             = 601,
	UI_THREAD_ERROR                           = 602,
	
	
	VIDEO_NOT_RUNNING                         = 701,
	VIDEO_RESOLUTION_ERROR                    = 702,
	VIDEO_OMX_ERROR                           = 703,
	VIDEO_FILE_ERROR                          = 704,
	VIDEO_THREAD_ERROR                        = 705,
	
	
	RECORD_THREAD_ERROR                       = 801,
	RECORD_NOT_RUNNING                        = 802,
	RECORD_ALREADY_RUNNING                    = 803,
	RECORD_NO_DIRECTORY_SET                   = 804,
	RECORD_FILE_EXISTS                        = 805,
	RECORD_DIRECTORY_NOT_WRITABLE             = 806,
	RECORD_ERROR                              = 807,
	
	
	SETTINGS_LOAD_ERROR                       = 901,
	SETTINGS_SAVE_ERROR                       = 902
} CameraErrortype;

#endif // ERRORCODES_H
