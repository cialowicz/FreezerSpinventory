// Interrupt-driven quadrature decoder for the rotary knob.
// One detent on the knob = 4 quadrature transitions; consumeSteps()
// returns whole detents and carries the remainder.
#pragma once

#include <stdint.h>

class RotaryEncoder {
 public:
  void begin(uint8_t pinA, uint8_t pinB, bool reversed);

  // Detents turned since the last call. Positive = clockwise.
  int consumeSteps();

 private:
  static void isrTrampoline(void* arg);
  void handleIsr();

  static RotaryEncoder* instance_;
  uint8_t pinA_ = 0;
  uint8_t pinB_ = 0;
  bool reversed_ = false;
  volatile int32_t count_ = 0;
  volatile uint8_t state_ = 0;
  int32_t accum_ = 0;
};
