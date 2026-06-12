#include "swipe_detector.h"

namespace input {

Gesture SwipeDetector::update(bool touched, int16_t x, int16_t y,
                              uint32_t now) {
  if (touched) {
    if (!tracking_) {
      tracking_ = true;
      startX_ = lastX_ = x;
      startY_ = lastY_ = y;
      startMs_ = now;
    } else {
      lastX_ = x;
      lastY_ = y;
    }
    return Gesture::kNone;
  }

  if (!tracking_) {
    return Gesture::kNone;
  }
  tracking_ = false;

  const int dx = lastX_ - startX_;
  const int dy = lastY_ - startY_;
  const int adx = dx < 0 ? -dx : dx;
  const int ady = dy < 0 ? -dy : dy;

  if (adx < minSwipePx_ && ady < minSwipePx_) {
    return now - startMs_ <= maxTapMs_ ? Gesture::kTap : Gesture::kNone;
  }
  if (adx >= ady) {
    return dx > 0 ? Gesture::kSwipeRight : Gesture::kSwipeLeft;
  }
  // Screen coordinates grow downward.
  return dy > 0 ? Gesture::kSwipeDown : Gesture::kSwipeUp;
}

}  // namespace input
