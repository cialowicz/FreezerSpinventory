// Hardware-independent touch gesture classification. The firmware feeds it
// polled CST816 samples; gestures are emitted when the finger lifts.
#pragma once

#include <stdint.h>

namespace input {

enum class Gesture {
  kNone,
  kTap,
  kSwipeLeft,
  kSwipeRight,
  kSwipeUp,
  kSwipeDown,
};

class SwipeDetector {
 public:
  // minSwipePx: minimum travel along the dominant axis to count as a swipe.
  // maxTapMs: maximum touch duration for sub-threshold motion to be a tap.
  SwipeDetector(int16_t minSwipePx, uint32_t maxTapMs)
      : minSwipePx_(minSwipePx), maxTapMs_(maxTapMs) {}

  // Feed one poll sample; x/y are ignored while touched is false. Returns
  // the gesture completed by this sample, kNone otherwise.
  Gesture update(bool touched, int16_t x, int16_t y, uint32_t now);

  // Position of the most recent tap (the last sampled touch point).
  // Only meaningful immediately after update() returned kTap.
  int16_t lastTapX() const { return lastX_; }
  int16_t lastTapY() const { return lastY_; }

 private:
  int16_t minSwipePx_;
  uint32_t maxTapMs_;
  bool tracking_ = false;
  int16_t startX_ = 0;
  int16_t startY_ = 0;
  int16_t lastX_ = 0;
  int16_t lastY_ = 0;
  uint32_t startMs_ = 0;
};

}  // namespace input
