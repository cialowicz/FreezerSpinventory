#include "io_expander.h"

#include <Arduino.h>
#include <Wire.h>

#include "config.h"

bool IoExpander::begin() {
  Wire.beginTransmission(PCF8574_ADDR);
  if (Wire.endTransmission() != 0) {
    return false;
  }
  return write(0xFF);
}

bool IoExpander::panelPowerOnReset() {
  // LCD power on, button/touch lines high, both resets asserted (low).
  shadow_ = 0xFF;
  shadow_ &= ~(1 << PCF_BIT_LCD_RST);
  shadow_ &= ~(1 << PCF_BIT_TOUCH_RST);
  if (!write(shadow_)) {
    return false;
  }
  delay(20);
  // Release resets; ST7701 wants >120ms before init commands.
  shadow_ |= (1 << PCF_BIT_LCD_RST) | (1 << PCF_BIT_TOUCH_RST);
  if (!write(shadow_)) {
    return false;
  }
  delay(150);
  return true;
}

bool IoExpander::readButton(bool& down) {
  uint8_t value;
  if (!read(value)) {
    return false;
  }
  down = (value & (1 << PCF_BIT_ENC_BTN)) == 0;  // active low
  return true;
}

bool IoExpander::write(uint8_t value) {
  shadow_ = value;
  Wire.beginTransmission(PCF8574_ADDR);
  Wire.write(shadow_);
  return Wire.endTransmission() == 0;
}

bool IoExpander::read(uint8_t& value) {
  if (Wire.requestFrom((uint8_t)PCF8574_ADDR, (uint8_t)1) != 1) {
    return false;
  }
  value = Wire.read();
  return true;
}
