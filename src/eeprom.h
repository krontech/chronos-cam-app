#ifndef EEPROM_H
#define EEPROM_H

int eeprom_write(int fd, unsigned int addr, unsigned int offset, unsigned char *buf, unsigned char len);
int eeprom_read(int fd, unsigned int addr, unsigned int offset, void *buf, unsigned char len);
int eeprom_write_large(int fd, unsigned int addr, unsigned int offset, unsigned char *buf, unsigned char len);
int eeprom_read_large(int fd, unsigned int addr, unsigned int offset, void *buf, unsigned char len);

#endif // EEPROM_H

