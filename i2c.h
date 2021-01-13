// minimal I2C library
// version 1.0
// based on code from QRP Labs
// (c) Andrew Bilokon, UR5FFR
// mailto:ban.relayer@gmail.com
// http://dspview.com
// https://github.com/andrey-belokon

#ifndef I2C_H
#define I2C_H

#include <inttypes.h>

void i2c_init(uint32_t i2c_freq = 100000);
bool i2c_begin_write(uint8_t addr);
bool i2c_begin_read(uint8_t addr);
bool i2c_write(uint8_t data);
uint8_t i2c_read();
void i2c_read(uint8_t* data, uint8_t count);
void i2c_read_long(uint8_t* data, uint16_t count);
uint8_t i2c_read_continue(bool last);
void i2c_end();
bool i2c_device_found(uint8_t addr);

#endif
