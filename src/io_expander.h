// Minimal PCF8574 driver for the CrowPanel's expander: drives the LCD
// power/reset lines and reads the encoder push button. Talks straight to
// Wire — no library needed for one quasi-bidirectional byte.
#pragma once

#include <stdint.h>

class IoExpander {
 public:
  // Wire.begin() must have been called already.
  bool begin();

  // Power up the panel and pulse the LCD + touch reset lines.
  // Must run before the ST7701 init sequence is sent.
  void panelPowerOnReset();

  // Raw debounced-by-caller button level; true while knob is held down.
  bool buttonDown();

 private:
  void write(uint8_t value);
  bool read(uint8_t& value);
  // PCF8574 quasi-bidirectional: bits we read must stay written high.
  uint8_t shadow_ = 0xFF;
};
