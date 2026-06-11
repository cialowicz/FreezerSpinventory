#include <stdint.h>

#include <unity.h>

#include "app_controller.h"
#include "inventory_model.h"

namespace {

const app::Config kConfig(
    100, 20, 50, 200, 255, 40);

}  // namespace

void setUp() {}
void tearDown() {}

static void test_commit_is_debounced_and_failed_save_retries() {
  inv::InventoryModel model;
  app::Controller controller(model, kConfig);
  controller.begin(0);

  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(app::InputResult::kModelChanged),
      static_cast<int>(controller.press(1)));
  controller.rotate(5, 2);
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(app::InputResult::kCommitScheduled),
      static_cast<int>(controller.press(3)));
  TEST_ASSERT_EQUAL_UINT8(5, model.item(0).quantity);

  TEST_ASSERT_FALSE(controller.saveDue(22));
  TEST_ASSERT_TRUE(controller.saveDue(23));
  controller.recordSaveResult(false, 23);
  TEST_ASSERT_FALSE(controller.saveDue(72));
  TEST_ASSERT_TRUE(controller.saveDue(73));
  controller.recordSaveResult(true, 73);
  TEST_ASSERT_FALSE(controller.saveDue(UINT32_MAX));
}

static void test_timeout_discards_pending_edit() {
  inv::InventoryModel model;
  app::Controller controller(model, kConfig);
  controller.begin(0);

  controller.press(1);
  controller.rotate(7, 2);
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(app::TickResult::kNoChange),
      static_cast<int>(controller.tick(101)));
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(app::TickResult::kEditCancelled),
      static_cast<int>(controller.tick(102)));
  TEST_ASSERT_FALSE(model.inEditMode());
  TEST_ASSERT_EQUAL_UINT8(0, model.item(0).quantity);
  TEST_ASSERT_FALSE(model.dirty());
}

static void test_first_input_after_dimming_only_wakes() {
  inv::InventoryModel model;
  app::Controller controller(model, kConfig);
  controller.begin(0);

  controller.tick(200);
  TEST_ASSERT_TRUE(controller.dimmed());
  TEST_ASSERT_EQUAL_UINT8(40, controller.backlightLevel());

  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(app::InputResult::kWakeOnly),
      static_cast<int>(controller.rotate(1, 201)));
  TEST_ASSERT_EQUAL_size_t(0, model.selectedIndex());
  TEST_ASSERT_FALSE(controller.dimmed());
  TEST_ASSERT_EQUAL_UINT8(255, controller.backlightLevel());

  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(app::InputResult::kModelChanged),
      static_cast<int>(controller.rotate(1, 202)));
  TEST_ASSERT_EQUAL_size_t(1, model.selectedIndex());

  controller.tick(402);
  TEST_ASSERT_TRUE(controller.dimmed());
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(app::InputResult::kWakeOnly),
      static_cast<int>(controller.press(403)));
  TEST_ASSERT_FALSE(model.inEditMode());
}

static void test_timers_handle_millis_rollover() {
  inv::InventoryModel model;
  app::Controller controller(model, kConfig);
  controller.begin(UINT32_MAX - 10);
  controller.scheduleSave(UINT32_MAX - 5);

  TEST_ASSERT_FALSE(controller.saveDue(13));
  TEST_ASSERT_TRUE(controller.saveDue(14));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_commit_is_debounced_and_failed_save_retries);
  RUN_TEST(test_timeout_discards_pending_edit);
  RUN_TEST(test_first_input_after_dimming_only_wakes);
  RUN_TEST(test_timers_handle_millis_rollover);
  return UNITY_END();
}
