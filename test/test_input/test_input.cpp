#include <stdint.h>

#include <unity.h>

#include "button_debouncer.h"
#include "rotary_decoder.h"
#include "swipe_detector.h"

static void assertGesture(input::Gesture expected, input::Gesture actual) {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(expected),
                        static_cast<int>(actual));
}

void setUp() {}
void tearDown() {}

static void test_button_polling_and_debounce() {
  input::ButtonDebouncer button(15, 30);
  TEST_ASSERT_TRUE(button.pollDue(0));
  TEST_ASSERT_FALSE(button.pollDue(14));
  TEST_ASSERT_TRUE(button.pollDue(15));

  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(input::ButtonEvent::kNone),
      static_cast<int>(button.update(false, 15)));
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(input::ButtonEvent::kNone),
      static_cast<int>(button.update(true, 30)));
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(input::ButtonEvent::kNone),
      static_cast<int>(button.update(false, 40)));
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(input::ButtonEvent::kNone),
      static_cast<int>(button.update(true, 50)));
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(input::ButtonEvent::kPressed),
      static_cast<int>(button.update(true, 80)));
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(input::ButtonEvent::kNone),
      static_cast<int>(button.update(false, 110)));
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(input::ButtonEvent::kReleased),
      static_cast<int>(button.update(false, 140)));
}

static void test_missing_button_samples_do_not_change_state() {
  input::ButtonDebouncer button(15, 30);
  button.update(false, 0);
  button.update(true, 10);
  // An I2C failure is represented by not calling update().
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(input::ButtonEvent::kPressed),
      static_cast<int>(button.update(true, 100)));
}

static void test_button_held_at_boot_is_not_a_press() {
  input::ButtonDebouncer button(15, 30);
  // The first sample seeds the baseline: a button already held when the
  // device boots must not generate a press event.
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(input::ButtonEvent::kNone),
      static_cast<int>(button.update(true, 0)));
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(input::ButtonEvent::kNone),
      static_cast<int>(button.update(true, 50)));
  // Releasing and pressing again behaves normally afterwards.
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(input::ButtonEvent::kNone),
      static_cast<int>(button.update(false, 60)));
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(input::ButtonEvent::kReleased),
      static_cast<int>(button.update(false, 90)));
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(input::ButtonEvent::kNone),
      static_cast<int>(button.update(true, 100)));
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(input::ButtonEvent::kPressed),
      static_cast<int>(button.update(true, 130)));
}

static void test_quadrature_sequences_and_invalid_transition() {
  const uint8_t clockwise[] = {0, 2, 3, 1, 0};
  int transitions = 0;
  for (size_t i = 1; i < sizeof(clockwise); i++) {
    transitions +=
        input::quadratureDelta(clockwise[i - 1], clockwise[i]);
  }
  TEST_ASSERT_EQUAL_INT(4, transitions);
  TEST_ASSERT_EQUAL_INT(0, input::quadratureDelta(0, 3));
}

static void test_detent_accumulator_carries_partial_steps() {
  int32_t remainder = 0;
  TEST_ASSERT_EQUAL_INT(0, input::consumeDetents(remainder, 3, 4));
  TEST_ASSERT_EQUAL_INT(3, remainder);
  TEST_ASSERT_EQUAL_INT(1, input::consumeDetents(remainder, 2, 4));
  TEST_ASSERT_EQUAL_INT(1, remainder);
  TEST_ASSERT_EQUAL_INT(-1, input::consumeDetents(remainder, -5, 4));
  TEST_ASSERT_EQUAL_INT(0, remainder);
}

static void test_detent_divisor_matches_encoder_resolution() {
  // Encoders that emit two transitions per physical click use divisor 2.
  int32_t remainder = 0;
  TEST_ASSERT_EQUAL_INT(1, input::consumeDetents(remainder, 2, 2));
  TEST_ASSERT_EQUAL_INT(0, remainder);
  TEST_ASSERT_EQUAL_INT(0, input::consumeDetents(remainder, -1, 2));
  TEST_ASSERT_EQUAL_INT(-1, remainder);
  TEST_ASSERT_EQUAL_INT(-1, input::consumeDetents(remainder, -1, 2));
  TEST_ASSERT_EQUAL_INT(0, remainder);
}

static void test_swipes_classified_on_release() {
  input::SwipeDetector d(60, 300);

  // Rightward drag: nothing emitted while the finger is down.
  assertGesture(input::Gesture::kNone, d.update(true, 100, 200, 0));
  assertGesture(input::Gesture::kNone, d.update(true, 150, 205, 20));
  assertGesture(input::Gesture::kNone, d.update(true, 200, 210, 40));
  assertGesture(input::Gesture::kSwipeRight, d.update(false, 0, 0, 60));

  d.update(true, 300, 240, 100);
  d.update(true, 180, 250, 140);
  assertGesture(input::Gesture::kSwipeLeft, d.update(false, 0, 0, 160));

  // Screen coordinates grow downward: an upward swipe decreases y.
  d.update(true, 240, 300, 200);
  d.update(true, 245, 180, 240);
  assertGesture(input::Gesture::kSwipeUp, d.update(false, 0, 0, 260));

  d.update(true, 240, 180, 300);
  d.update(true, 250, 320, 340);
  assertGesture(input::Gesture::kSwipeDown, d.update(false, 0, 0, 360));
}

static void test_diagonal_swipe_uses_dominant_axis() {
  input::SwipeDetector d(60, 300);
  d.update(true, 100, 100, 0);
  d.update(true, 180, 220, 40);  // dx=80, dy=120: vertical wins
  assertGesture(input::Gesture::kSwipeDown, d.update(false, 0, 0, 60));
}

static void test_tap_and_subthreshold_motion() {
  input::SwipeDetector d(60, 300);

  // Short press with little travel is a tap, and its position is kept so
  // the firmware can hit-test it against UI rows.
  d.update(true, 240, 240, 0);
  d.update(true, 250, 245, 30);
  assertGesture(input::Gesture::kTap, d.update(false, 0, 0, 60));
  TEST_ASSERT_EQUAL_INT16(250, d.lastTapX());
  TEST_ASSERT_EQUAL_INT16(245, d.lastTapY());

  // A long lingering press with little travel is nothing.
  d.update(true, 240, 240, 1000);
  assertGesture(input::Gesture::kNone, d.update(false, 0, 0, 1500));

  // Release without any preceding touch is nothing.
  assertGesture(input::Gesture::kNone, d.update(false, 0, 0, 2000));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_button_polling_and_debounce);
  RUN_TEST(test_missing_button_samples_do_not_change_state);
  RUN_TEST(test_button_held_at_boot_is_not_a_press);
  RUN_TEST(test_quadrature_sequences_and_invalid_transition);
  RUN_TEST(test_detent_accumulator_carries_partial_steps);
  RUN_TEST(test_detent_divisor_matches_encoder_resolution);
  RUN_TEST(test_swipes_classified_on_release);
  RUN_TEST(test_diagonal_swipe_uses_dominant_axis);
  RUN_TEST(test_tap_and_subthreshold_motion);
  return UNITY_END();
}
