/* 
  Arduino Slow Software I2C Master 
  Copyright (c) 2017 Bernhard Nebel.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef I2C_SOFT_H
#define I2C_SOFT_H

#include <Arduino.h>
#include <inttypes.h>

class SoftI2C {
  private:
    void setHigh(uint8_t pin);
    void setLow(uint8_t pin);
    uint8_t _sda;
    uint8_t _scl;
    uint8_t pin_mode;
    uint16_t _delay_us; // default 4us

    bool i2c_start(uint8_t addr);
    
  public:
    SoftI2C(uint8_t sda, uint8_t scl, bool internal_pullup);

    bool i2c_init(uint16_t delay_us = 4);

    bool i2c_begin_write(uint8_t addr) { return i2c_start(addr<<1); }
    bool i2c_begin_read(uint8_t addr);
    bool i2c_write(uint8_t data);
    uint8_t i2c_read() { return i2c_read_continue(true); }
    void i2c_read(uint8_t* data, uint8_t count);
    void i2c_read_long(uint8_t* data, uint16_t count);
    uint8_t i2c_read_continue(bool last);
    void i2c_end();
};

#endif
