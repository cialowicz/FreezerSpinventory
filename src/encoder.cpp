#include "encoder.h"

#include <Arduino.h>

RotaryEncoder* RotaryEncoder::instance_ = nullptr;

// Index: (previous state << 2) | current state of (A<<1 | B).
// Invalid transitions (bounce skipping a phase) count as 0.
static const int8_t kQuadratureLut[16] = {
    0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0,
};

void RotaryEncoder::begin(uint8_t pinA, uint8_t pinB, bool reversed) {
  pinA_ = pinA;
  pinB_ = pinB;
  reversed_ = reversed;
  instance_ = this;
  pinMode(pinA_, INPUT_PULLUP);
  pinMode(pinB_, INPUT_PULLUP);
  state_ = (digitalRead(pinA_) << 1) | digitalRead(pinB_);
  attachInterrupt(digitalPinToInterrupt(pinA_), isrTrampoline, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pinB_), isrTrampoline, CHANGE);
}

void IRAM_ATTR RotaryEncoder::isrTrampoline() {
  if (instance_ != nullptr) {
    instance_->handleIsr();
  }
}

void IRAM_ATTR RotaryEncoder::handleIsr() {
  uint8_t current = (digitalRead(pinA_) << 1) | digitalRead(pinB_);
  uint8_t index = ((state_ << 2) | current) & 0x0F;
  state_ = current;
  count_ += kQuadratureLut[index];
}

int RotaryEncoder::consumeSteps() {
  noInterrupts();
  int32_t delta = count_;
  count_ = 0;
  interrupts();
  accum_ += delta;
  int steps = accum_ / 4;
  accum_ -= steps * 4;
  return reversed_ ? -steps : steps;
}
