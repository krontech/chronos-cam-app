#ifndef ERRORCODES_H
#define ERRORCODES_H

typedef enum CameraErrortype
{
	CAMERA_SUCCESS = 0,
	CAMERA_THREAD_ERROR,
	CAMERA_ALREADY_RECORDING,
	CAMERA_NOT_RECORDING,
	CAMERA_NO_RECORDING_PRESENT,
	CAMERA_IN_PLAYBACK_MODE,
	CAMERA_ERROR_SENSOR,
	CAMERA_INVALID_IMAGER_SETTINGS,
	CAMERA_FILE_NOT_FOUND,
	CAMERA_FILE_ERROR,
	CAMERA_ERROR_IO,
	CAMERA_INVALID_SETTINGS,
	CAMERA_FPN_CORRECTION_ERROR,
	CAMERA_CLIPPED_ERROR,
	CAMERA_LOW_SIGNAL_ERROR,
	CAMERA_RECORD_FRAME_ERROR,
	CAMERA_ITERATION_LIMIT_EXCEEDED,
	CAMERA_GAIN_CORRECTION_ERROR,
	CAMERA_MEM_ERROR,
	CAMERA_WRONG_FPGA_VERSION,
	ECP5_ALREAY_OPEN,
	ECP5_NOT_OPEN,
	ECP5_GPIO_ERROR,
	ECP5_SPI_OPEN_FAIL,
	ECP5_IOCTL_FAIL,
	ECP5_FILE_IO_ERROR,
	ECP5_MEMORY_ERROR,
	ECP5_WRONG_DEVICE_ID,
	ECP5_DONE_NOT_ASSERTED,
	GPMCERR_FOPEN,
	GPMCERR_MAP,
	IO_ERROR_OPEN,
	IO_FILE_ERROR,
	LUPA1300_SPI_OPEN_FAIL,
	LUPA1300_NO_DATA_VALID_WINDOW,
	LUPA1300_INSUFFICIENT_DATA_VALID_WINDOW,
	LUX1310_FILE_ERROR,
	SPI_NOT_OPEN,
	SPI_ALREAY_OPEN,
	SPI_OPEN_FAIL,
	SPI_IOCTL_FAIL,
	UI_FILE_ERROR,
	UI_THREAD_ERROR,
	VIDEO_NOT_RUNNING,
	VIDEO_RESOLUTION_ERROR,
	VIDEO_OMX_ERROR,
	VIDEO_FILE_ERROR,
	VIDEO_THREAD_ERROR,
	RECORD_THREAD_ERROR,
	RECORD_NOT_RUNNING,
	RECORD_ALREADY_RUNNING,
	RECORD_NO_DIRECTORY_SET,
	RECORD_FILE_EXISTS,
	RECORD_DIRECTORY_NOT_WRITABLE,
	RECORD_ERROR
} CameraErrortype;

#endif // ERRORCODES_H
