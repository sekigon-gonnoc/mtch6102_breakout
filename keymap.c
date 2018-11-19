#include QMK_KEYBOARD_H

#include <string.h>
#include <print.h>
#include "i2c.h"

extern keymap_config_t keymap_config;

// Fillers to make layering more clear
#define _______ KC_TRNS
#define XXXXXXX KC_NO

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
// Your key map here
};

#define MTCH6102_READ_ADDR ((0x25<<1))
#define MTCH6102_REG_STAT 0x10

typedef union {
  struct {
    uint8_t status, x_msb, y_msb, xy_lsb, gesture;
  };
  uint8_t dat[5];
} mtch6102_reg_t;

enum {
  GES_TAP = 0x10,
  GES_DOUBLE_TAP = 0x20
} GESTURE_CODE;

enum {
  TOUCH = 1<<0,
  GESTURE= 1<<1,
} MTCH6102_STAT_BIT;

/** https://github.com/swharden/AVR-projects/tree/master/ATMega328%202017-02-08%20i2c%20LM75A%20thermometer */

uint8_t i2c_read_ack(void)
{

	// start TWI module and acknowledge data after reception
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);
	// wait for end of transmission
	while( !(TWCR & (1<<TWINT)) );
	// return received data from TWDR
	return TWDR;
}

uint8_t i2c_read_nack(void)
{

	// start receiving without acknowledging reception
	TWCR = (1<<TWINT) | (1<<TWEN);
	// wait for end of transmission
	while( !(TWCR & (1<<TWINT)) );
	// return received data from TWDR
	return TWDR;
}

uint8_t i2c_transmit(uint8_t address, uint8_t* data, uint16_t length)
{
	if (i2c_master_start(address | I2C_WRITE)) return 1;

	for (uint16_t i = 0; i < length; i++)
	{
		if (i2c_master_write(data[i])) return 1;
	}

	i2c_master_stop();

	return 0;
}

uint8_t i2c_receive(uint8_t address, uint8_t* data, uint16_t length)
{
	if (i2c_master_start(address | I2C_READ)) return 1;

	for (uint16_t i = 0; i < (length-1); i++)
	{
		data[i] = i2c_read_ack();
	}
	data[(length-1)] = i2c_read_nack();

	i2c_master_stop();

	return 0;
}

uint8_t i2c_writeReg(uint8_t devaddr, uint8_t regaddr, uint8_t* data, uint16_t length)
{
	if (i2c_master_start(devaddr | 0x00)) return 1;

	i2c_master_write(regaddr);

	for (uint16_t i = 0; i < length; i++)
	{
		if (i2c_master_write(data[i])) return 1;
	}

	i2c_master_stop();

	return 0;
}

uint8_t i2c_readReg(uint8_t devaddr, uint8_t regaddr, uint8_t* data, uint16_t length)
{
	if (i2c_master_start(devaddr)) {
    print("Cant master start 0\n");
    return 1;
  }

	i2c_master_write(regaddr);

	if (i2c_master_start(devaddr | 0x01)) {
    // print("Cant master start 1\n");
    return 1;
  }

	for (uint16_t i = 0; i < (length-1); i++)
	{
		data[i] = i2c_read_ack();
	}
	data[(length-1)] = i2c_read_nack();

	i2c_master_stop();

	return 0;
}

static int process_mtch6102(void) {
  mtch6102_reg_t reg;
  static uint16_t x, y;
  static uint16_t x_buf, y_buf;
  static bool touch_state;
  static bool release_button = false;
  bool send_flag = false;

  report_mouse_t rep_mouse;
  uint16_t x_dif, y_dif;

  if (i2c_readReg(MTCH6102_READ_ADDR, MTCH6102_REG_STAT, reg.dat, sizeof(mtch6102_reg_t))) {
    print("regread error\n");
    return 1;
  }

  memset(&rep_mouse, 0, sizeof(report_mouse_t));
  x = (reg.x_msb << 4) | (reg.xy_lsb >> 4);
  y = (reg.y_msb << 4) | (reg.xy_lsb & 0xF);

  if (touch_state && (reg.status & TOUCH)) {
    x_dif = x - x_buf;
    y_dif = y - y_buf;
    rep_mouse.x = x_dif * -1;
    rep_mouse.y = y_dif * -1;
    send_flag = true;
  }
  if ((reg.status & GESTURE)
      && (reg.gesture == GES_TAP || reg.gesture == GES_DOUBLE_TAP)) {
    rep_mouse.buttons = 1;
    release_button = true;
    send_flag = true;
  } else if (release_button) {
    rep_mouse.buttons = 0;
    send_flag = true;
  }
  if (send_flag) {
    host_get_driver()->send_mouse(&rep_mouse);
  }
  touch_state = reg.status & TOUCH;
  x_buf = x;
  y_buf = y;

  return 0;
}

__attribute__((weak)) // overridable
void matrix_scan_user(void) {
  process_mtch6102();
}
