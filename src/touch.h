// Minimal polled CST816 capacitive-touch reader on the shared I2C bus.
// No interrupt pin needed: the chip ACKs while a finger is down and NACKs
// when idle/asleep, which doubles as release detection.
#pragma once

#include <stdint.h>

class TouchPanel {
 public:
  // Wire.begin() and the PCF8574 touch reset must have happened already.
  // Best-effort: disables the controller's auto-sleep so polling stays
  // responsive between touches. Returns false if the chip didn't ACK.
  bool begin();

  // True while a finger is on the panel; x/y are updated only then.
  bool readTouch(int16_t& x, int16_t& y);
};
