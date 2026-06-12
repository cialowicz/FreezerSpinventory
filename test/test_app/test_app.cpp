#include <stdint.h>

#include <unity.h>

#include "app_controller.h"
#include "inventory_model.h"

namespace {

const app::Config kConfig{
    .commitIdleMs = 100,
    .saveDebounceMs = 20,
    .saveRetryMs = 50,
    .overviewDelayMs = 150,
    .backlightDimMs = 200,
    .backlightFullLevel = 255,
    .backlightDimLevel = 40,
};

}  // namespace

void setUp() {}
void tearDown() {}

static void test_press_commit_saves_immediately_and_failed_save_retries() {
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

  // An explicit press-commit saves without further debounce.
  TEST_ASSERT_TRUE(controller.saveDue(3));
  controller.recordSaveResult(false, 3);
  TEST_ASSERT_FALSE(controller.saveDue(52));
  TEST_ASSERT_TRUE(controller.saveDue(53));
  controller.recordSaveResult(true, 53);
  TEST_ASSERT_FALSE(controller.saveDue(UINT32_MAX));
}

static void test_idle_edit_auto_commits_and_saves() {
  inv::InventoryModel model;
  app::Controller controller(model, kConfig);
  controller.begin(0);

  controller.press(1);
  controller.rotate(7, 2);
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(app::TickResult::kNoChange),
      static_cast<int>(controller.tick(101)));
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(app::TickResult::kAutoCommitted),
      static_cast<int>(controller.tick(102)));
  TEST_ASSERT_FALSE(model.inEditMode());
  TEST_ASSERT_EQUAL_UINT8(7, model.item(0).quantity);
  // The idle wait was the debounce: the save is due at once.
  TEST_ASSERT_TRUE(controller.saveDue(102));
}

static void test_idle_edit_without_change_just_closes() {
  inv::InventoryModel model;
  app::Controller controller(model, kConfig);
  controller.begin(0);

  controller.press(1);  // enter edit, never turn
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(app::TickResult::kAutoCommitted),
      static_cast<int>(controller.tick(101)));
  TEST_ASSERT_FALSE(model.inEditMode());
  TEST_ASSERT_FALSE(model.dirty());
  TEST_ASSERT_FALSE(controller.saveDue(UINT32_MAX));
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

static void test_reswipe_during_debounce_restarts_timer() {
  inv::InventoryModel model;
  app::Controller controller(model, kConfig);
  controller.begin(0);

  controller.swipeVertical(1, 3);   // save due at 23
  controller.swipeVertical(1, 12);  // restarts the debounce window
  TEST_ASSERT_FALSE(controller.saveDue(31));
  TEST_ASSERT_TRUE(controller.saveDue(32));
}

static void test_swipe_during_retry_window_uses_debounce_delay() {
  inv::InventoryModel model;
  app::Controller controller(model, kConfig);
  controller.begin(0);

  controller.swipeVertical(1, 3);
  TEST_ASSERT_TRUE(controller.saveDue(23));
  controller.recordSaveResult(false, 23);  // retry would be due at 73

  // A new change supersedes the pending retry: the save is rescheduled on
  // the (shorter) debounce delay, not the retry delay.
  controller.swipeVertical(1, 30);
  TEST_ASSERT_FALSE(controller.saveDue(49));
  TEST_ASSERT_TRUE(controller.saveDue(50));
}

static void test_overview_after_idle_and_first_input_only_wakes() {
  inv::InventoryModel model;
  app::Controller controller(model, kConfig);
  controller.begin(0);

  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(app::TickResult::kNoChange),
      static_cast<int>(controller.tick(149)));
  TEST_ASSERT_FALSE(controller.overviewActive());
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(app::TickResult::kOverviewDue),
      static_cast<int>(controller.tick(150)));
  TEST_ASSERT_TRUE(controller.overviewActive());
  // Only signalled once.
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(app::TickResult::kNoChange),
      static_cast<int>(controller.tick(151)));

  // First input leaves the overview without touching the model.
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(app::InputResult::kWakeOnly),
      static_cast<int>(controller.rotate(1, 160)));
  TEST_ASSERT_FALSE(controller.overviewActive());
  TEST_ASSERT_EQUAL_size_t(0, model.selectedIndex());
}

static void test_overview_waits_for_edit_to_commit() {
  inv::InventoryModel model;
  app::Controller controller(model, kConfig);
  controller.begin(0);

  controller.press(1);  // enter edit
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(app::TickResult::kAutoCommitted),
      static_cast<int>(controller.tick(120)));
  TEST_ASSERT_FALSE(controller.overviewActive());
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(app::TickResult::kOverviewDue),
      static_cast<int>(controller.tick(151)));
}

static void test_vertical_swipe_in_browse_commits_directly() {
  inv::InventoryModel model;
  app::Controller controller(model, kConfig);
  controller.begin(0);

  // Swiping down at zero is clamped: nothing changes, no save.
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(app::InputResult::kIgnored),
      static_cast<int>(controller.swipeVertical(-1, 1)));
  TEST_ASSERT_FALSE(controller.saveDue(UINT32_MAX));

  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(app::InputResult::kCommitScheduled),
      static_cast<int>(controller.swipeVertical(1, 2)));
  TEST_ASSERT_EQUAL_UINT8(1, model.item(0).quantity);
  TEST_ASSERT_FALSE(model.inEditMode());
  TEST_ASSERT_FALSE(controller.saveDue(21));
  TEST_ASSERT_TRUE(controller.saveDue(22));
}

static void test_vertical_swipe_in_edit_adjusts_pending() {
  inv::InventoryModel model;
  app::Controller controller(model, kConfig);
  controller.begin(0);

  controller.press(1);
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(app::InputResult::kModelChanged),
      static_cast<int>(controller.swipeVertical(1, 2)));
  TEST_ASSERT_TRUE(model.inEditMode());
  TEST_ASSERT_EQUAL_UINT8(1, model.pendingQuantity());
  TEST_ASSERT_EQUAL_UINT8(0, model.item(0).quantity);  // not committed
}

static void test_horizontal_swipe_browses_unless_editing() {
  inv::InventoryModel model;
  app::Controller controller(model, kConfig);
  controller.begin(0);

  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(app::InputResult::kModelChanged),
      static_cast<int>(controller.swipeHorizontal(1, 1)));
  TEST_ASSERT_EQUAL_size_t(1, model.selectedIndex());

  controller.press(2);  // selection is locked while editing
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(app::InputResult::kIgnored),
      static_cast<int>(controller.swipeHorizontal(1, 3)));
  TEST_ASSERT_EQUAL_size_t(1, model.selectedIndex());
}

static void test_tap_wakes_and_postpones_idle() {
  inv::InventoryModel model;
  app::Controller controller(model, kConfig);
  controller.begin(0);

  controller.tick(150);
  TEST_ASSERT_TRUE(controller.overviewActive());
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(app::InputResult::kWakeOnly),
      static_cast<int>(controller.tap(160)));
  TEST_ASSERT_FALSE(controller.overviewActive());

  // An awake tap changes nothing but still counts as activity.
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(app::InputResult::kIgnored),
      static_cast<int>(controller.tap(170)));
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(app::TickResult::kNoChange),
      static_cast<int>(controller.tick(319)));
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(app::TickResult::kOverviewDue),
      static_cast<int>(controller.tick(320)));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_press_commit_saves_immediately_and_failed_save_retries);
  RUN_TEST(test_idle_edit_auto_commits_and_saves);
  RUN_TEST(test_idle_edit_without_change_just_closes);
  RUN_TEST(test_first_input_after_dimming_only_wakes);
  RUN_TEST(test_timers_handle_millis_rollover);
  RUN_TEST(test_reswipe_during_debounce_restarts_timer);
  RUN_TEST(test_swipe_during_retry_window_uses_debounce_delay);
  RUN_TEST(test_overview_after_idle_and_first_input_only_wakes);
  RUN_TEST(test_overview_waits_for_edit_to_commit);
  RUN_TEST(test_vertical_swipe_in_browse_commits_directly);
  RUN_TEST(test_vertical_swipe_in_edit_adjusts_pending);
  RUN_TEST(test_horizontal_swipe_browses_unless_editing);
  RUN_TEST(test_tap_wakes_and_postpones_idle);
  return UNITY_END();
}
