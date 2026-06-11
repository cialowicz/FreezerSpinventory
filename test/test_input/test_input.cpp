#include <stdint.h>

#include <unity.h>

#include "button_debouncer.h"
#include "rotary_decoder.h"

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
  TEST_ASSERT_EQUAL_INT(0, input::consumeDetents(remainder, 3));
  TEST_ASSERT_EQUAL_INT(3, remainder);
  TEST_ASSERT_EQUAL_INT(1, input::consumeDetents(remainder, 2));
  TEST_ASSERT_EQUAL_INT(1, remainder);
  TEST_ASSERT_EQUAL_INT(-1, input::consumeDetents(remainder, -5));
  TEST_ASSERT_EQUAL_INT(0, remainder);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_button_polling_and_debounce);
  RUN_TEST(test_missing_button_samples_do_not_change_state);
  RUN_TEST(test_quadrature_sequences_and_invalid_transition);
  RUN_TEST(test_detent_accumulator_carries_partial_steps);
  return UNITY_END();
}
