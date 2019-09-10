
/****************************************************************************
 *  Copyright (C) 2018-2019 Kron Technologies Inc <http://www.krontech.ca>. *
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

#include "errorCodes.h"

/* Force an error if the switch is missing cases. */
#pragma GCC diagnostic error "-Wswitch"

const char *
errorCodeString(int code)
{
	CameraErrortype errCode = (CameraErrortype)code;

	switch (errCode) {
	case SUCCESS:	return "Success";

	/* Camera error group. */
	case CAMERA_THREAD_ERROR:			return "Thread error";
	case CAMERA_ALREADY_RECORDING:		return "Already recording";
	case CAMERA_NOT_RECORDING:			return "Not recording";
	case CAMERA_NO_RECORDING_PRESENT:	return "No recording present";
	case CAMERA_IN_PLAYBACK_MODE:		return "Camera in playback mode";
	case CAMERA_ERROR_SENSOR:			return "Sensor error";
	case CAMERA_INVALID_IMAGER_SETTINGS: return "Invalid imager settings";
	case CAMERA_FILE_NOT_FOUND:			return "File not found";
	case CAMERA_FILE_ERROR:				return "File error";
	case CAMERA_ERROR_IO:				return "I/O error";
	case CAMERA_INVALID_SETTINGS:		return "Invalid settings";
	case CAMERA_FPN_CORRECTION_ERROR:	return "FPN correction error";
	case CAMERA_CLIPPED_ERROR:			return "Clipping";
	case CAMERA_LOW_SIGNAL_ERROR:		return "Low signal";
	case CAMERA_RECORD_FRAME_ERROR:		return "Record frame error";
	case CAMERA_ITERATION_LIMIT_EXCEEDED: return "Iteration limit exceeded";
	case CAMERA_GAIN_CORRECTION_ERROR:	return "Gain correction error";
	case CAMERA_MEM_ERROR:				return "Memory error";
	case CAMERA_WRONG_FPGA_VERSION:		return "Wrong FPGA version";
	case CAMERA_DEAD_PIXEL_RECORD_ERROR: return "Dead pixel recording error";
	case CAMERA_DEAD_PIXEL_FAILED:		return "Dead pixel detection failed";

	/* ECP5 FPGA programming error. */
	case ECP5_ALREAY_OPEN:				return "FPGA already open";
	case ECP5_NOT_OPEN:					return "FPGA not open";
	case ECP5_GPIO_ERROR:				return "FPGA GPIO error";
	case ECP5_SPI_OPEN_FAIL:			return "FPGA SPI failure";
	case ECP5_IOCTL_FAIL:				return "FPGA IOCTL failure";
	case ECP5_FILE_IO_ERROR:			return "FPGA file I/O error";
	case ECP5_MEMORY_ERROR:				return "FPGA memory error";
	case ECP5_WRONG_DEVICE_ID:			return "FPGA wrong device ID";
	case ECP5_DONE_NOT_ASSERTED:		return "FPGA CDONE not asserted";

	/* I/O error group. */
	case IO_ERROR_OPEN:					return "IO open error";
	case IO_FILE_ERROR:					return "IO file error";

	/* LUPA1300 error group. */
	case LUPA1300_SPI_OPEN_FAIL:		return "LUPA1300 SPI failure";
	case LUPA1300_NO_DATA_VALID_WINDOW:	return "LUPA1300 no valid data window";
	case LUPA1300_INSUFFICIENT_DATA_VALID_WINDOW: return "LUPA1300 insufficient data valid";
	/* LUX1310 error group */
	case LUX1310_FILE_ERROR:			return "LUX1310 file error";
	case LUX1310_SERIAL_READBACK_ERROR: return "LUX1310 serial readback error";

	/* SPI error group. */
	case SPI_NOT_OPEN:					return "SPI device not open";
	case SPI_ALREAY_OPEN:				return "SPI device already open";
	case SPI_OPEN_FAIL:					return "SPI device open failure";
	case SPI_IOCTL_FAIL:				return "SPI device ioctl failure";

	/* UI error group */
	case UI_FILE_ERROR:                 return "UI file error";
	case UI_THREAD_ERROR:				return "UI thread error";

	/* Video error group */
	case VIDEO_NOT_RUNNING:				return "Video not running";
	case VIDEO_RESOLUTION_ERROR:		return "Video resolution error";
	case VIDEO_OMX_ERROR:				return "Video OMX internal error";
	case VIDEO_FILE_ERROR:				return "Video file error";
	case VIDEO_THREAD_ERROR:			return "Video threading error";

	/* Recording error group. */
	case RECORD_THREAD_ERROR:			return "Recording thread error";
	case RECORD_NOT_RUNNING:			return "Recording not running";
	case RECORD_ALREADY_RUNNING:		return "Recording already running";
	case RECORD_NO_DIRECTORY_SET:		return "Recording directory unset";
	case RECORD_FILE_EXISTS:			return "Recording file already exists";
	case RECORD_DIRECTORY_NOT_WRITABLE: return "Recording directory not writeable";
	case RECORD_ERROR:					return "Recording error";
	case RECORD_INSUFFICIENT_SPACE:		return "Recording insufficient space available";
	case RECORD_FOLDER_DOES_NOT_EXIST:	return "Recording folder does not exist";
	/* QSettings error group. */
	case SETTINGS_LOAD_ERROR:			return "Settings load error";
	case SETTINGS_SAVE_ERROR:			return "Settings save error";

	/* API error group. */
	case CAMERA_API_CALL_FAIL:			return "API call fail";
	case DBUS_CONTROL_FAIL:				return "DBus control fail";

	}

	/* If we got here, then some other error occured. */
	return "Unknown";
}
