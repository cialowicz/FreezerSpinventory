// Hardware-independent polling cadence and button edge debounce.
#pragma once

#include <stdint.h>

namespace input {

enum class ButtonEvent {
  kNone,
  kPressed,
  kReleased,
};

class ButtonDebouncer {
 public:
  ButtonDebouncer(uint32_t pollIntervalMs, uint32_t debounceMs)
      : pollIntervalMs_(pollIntervalMs), debounceMs_(debounceMs) {}

  bool pollDue(uint32_t now);
  ButtonEvent update(bool down, uint32_t now);

 private:
  uint32_t pollIntervalMs_;
  uint32_t debounceMs_;
  uint32_t lastPollMs_ = 0;
  uint32_t rawSinceMs_ = 0;
  bool hasPolled_ = false;
  bool initialized_ = false;
  bool rawDown_ = false;
  bool stableDown_ = false;
};

}  // namespace input
