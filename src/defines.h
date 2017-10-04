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
