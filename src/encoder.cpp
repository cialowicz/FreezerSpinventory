#include "encoder.h"

#include <Arduino.h>
#include <driver/gpio.h>
#include <soc/gpio_struct.h>

#include "rotary_decoder.h"

namespace {

// Direct register read; unlike digitalRead(), guaranteed callable while the
// flash cache is disabled (i.e. during NVS writes).
inline bool IRAM_ATTR fastGpioRead(uint8_t pin) {
  if (pin < 32) {
    return (GPIO.in >> pin) & 0x1;
  }
  return (GPIO.in1.val >> (pin - 32)) & 0x1;
}

}  // namespace

RotaryEncoder* RotaryEncoder::instance_ = nullptr;

void RotaryEncoder::begin(uint8_t pinA, uint8_t pinB, bool reversed,
                          int32_t transitionsPerDetent) {
  pinA_ = pinA;
  pinB_ = pinB;
  reversed_ = reversed;
  transitionsPerDetent_ = transitionsPerDetent;
  instance_ = this;
  pinMode(pinA_, INPUT_PULLUP);
  pinMode(pinB_, INPUT_PULLUP);
  state_ = (fastGpioRead(pinA_) << 1) | fastGpioRead(pinB_);

  // Register through the IRAM-resident ISR service rather than
  // attachInterrupt(): the Arduino core installs its service without
  // ESP_INTR_FLAG_IRAM, so its handlers are masked during flash (NVS)
  // writes and detents turned in that window would be lost.
  const esp_err_t err = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    Serial.println("Encoder ISR service installation failed");
    return;
  }
  gpio_set_intr_type((gpio_num_t)pinA_, GPIO_INTR_ANYEDGE);
  gpio_set_intr_type((gpio_num_t)pinB_, GPIO_INTR_ANYEDGE);
  gpio_isr_handler_add((gpio_num_t)pinA_, isrTrampoline, nullptr);
  gpio_isr_handler_add((gpio_num_t)pinB_, isrTrampoline, nullptr);
}

void IRAM_ATTR RotaryEncoder::isrTrampoline(void*) {
  if (instance_ != nullptr) {
    instance_->handleIsr();
  }
}

void IRAM_ATTR RotaryEncoder::handleIsr() {
  uint8_t current = (fastGpioRead(pinA_) << 1) | fastGpioRead(pinB_);
  const int8_t delta = input::quadratureDelta(state_, current);
  state_ = current;
  count_ += delta;
}

int RotaryEncoder::consumeSteps() {
  noInterrupts();
  int32_t delta = count_;
  count_ = 0;
  interrupts();
  const int steps = input::consumeDetents(accum_, delta, transitionsPerDetent_);
  return reversed_ ? -steps : steps;
}
