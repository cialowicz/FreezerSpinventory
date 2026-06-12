#include "touch.h"

#include <Arduino.h>
#include <Wire.h>

#include "config.h"

namespace {
constexpr uint8_t kRegGesture = 0x01;  // gesture, fingers, x, y block
constexpr uint8_t kRegDisAutoSleep = 0xFE;
}  // namespace

bool TouchPanel::begin() {
  Wire.beginTransmission(CST816_ADDR);
  Wire.write(kRegDisAutoSleep);
  Wire.write(0x01);
  return Wire.endTransmission() == 0;
}

bool TouchPanel::readTouch(int16_t& x, int16_t& y) {
  Wire.beginTransmission(CST816_ADDR);
  Wire.write(kRegGesture);
  if (Wire.endTransmission() != 0) {
    return false;  // asleep or busy: treat as no touch
  }
  if (Wire.requestFrom((uint8_t)CST816_ADDR, (uint8_t)6) != 6) {
    return false;
  }
  Wire.read();  // gesture id — unused; SwipeDetector classifies instead
  const uint8_t fingers = Wire.read();
  const uint8_t xHigh = Wire.read();
  const uint8_t xLow = Wire.read();
  const uint8_t yHigh = Wire.read();
  const uint8_t yLow = Wire.read();
  if (fingers == 0) {
    return false;
  }
  x = (int16_t)(((xHigh & 0x0F) << 8) | xLow);
  y = (int16_t)(((yHigh & 0x0F) << 8) | yLow);
  return true;
}
