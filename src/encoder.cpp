#include "encoder.h"

#include <Arduino.h>

#include "rotary_decoder.h"

RotaryEncoder* RotaryEncoder::instance_ = nullptr;

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
  const int8_t delta = input::quadratureDelta(state_, current);
  state_ = current;
  count_ += delta;
}

int RotaryEncoder::consumeSteps() {
  noInterrupts();
  int32_t delta = count_;
  count_ = 0;
  interrupts();
  const int steps = input::consumeDetents(accum_, delta);
  return reversed_ ? -steps : steps;
}
