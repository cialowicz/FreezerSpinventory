#include "button_debouncer.h"

namespace input {

bool ButtonDebouncer::pollDue(uint32_t now) {
  if (hasPolled_ && now - lastPollMs_ < pollIntervalMs_) {
    return false;
  }
  hasPolled_ = true;
  lastPollMs_ = now;
  return true;
}

ButtonEvent ButtonDebouncer::update(bool down, uint32_t now) {
  if (!initialized_) {
    initialized_ = true;
    // The first sample is the baseline: a button already held at boot must
    // not debounce into a press event.
    rawDown_ = down;
    stableDown_ = down;
    rawSinceMs_ = now;
  } else if (down != rawDown_) {
    rawDown_ = down;
    rawSinceMs_ = now;
  }

  if (rawDown_ == stableDown_ ||
      now - rawSinceMs_ < debounceMs_) {
    return ButtonEvent::kNone;
  }

  stableDown_ = rawDown_;
  return stableDown_ ? ButtonEvent::kPressed
                     : ButtonEvent::kReleased;
}

}  // namespace input
