// ST7701 RGB panel bring-up (Arduino_GFX) + LVGL glue + backlight PWM.
#pragma once

#include <stdint.h>

namespace display {

enum class InitResult {
  kOk,
  kObjectAllocationFailed,
  kPsramUnavailable,
  kFramebufferUnavailable,
  kDrawBufferAllocationFailed,
  kLvglRegistrationFailed,
};

// Initializes the panel and LVGL. The PCF8574 panel reset sequence must
// have completed first.
InitResult init();
const char* initResultMessage(InitResult result);

void setBacklight(uint8_t level);  // 0..255

}  // namespace display
