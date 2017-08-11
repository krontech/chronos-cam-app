#ifndef SPI_H
#define SPI_H

#include <stdint.h>
#include "errorCodes.h"
#include "types.h"


class SPI
{
public:

	SPI();
	CameraErrortype Init(const char * dev, uint8_t bits_val, uint32_t speed_val, bool cpol = false, bool cpha = false);
	CameraErrortype Open(const char * dev);
	void Close(void);
	Int32 Transfer(uint64_t txBuf, uint64_t rxBuf, uint32_t len);

	int fd;

	bool isOpen;
	uint8_t mode;
	uint8_t bits;
	uint32_t speed;
	uint16_t delay;
};
/*
enum {
	SPI_SUCCESS = 0,
	SPI_NOT_OPEN,
	SPI_ALREAY_OPEN,
	SPI_OPEN_FAIL,
	SPI_IOCTL_FAIL
};
*/
#endif // SPI_H
