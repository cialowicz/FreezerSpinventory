#include <string.h>

#include <unity.h>

#include "inventory_model.h"

using inv::InventoryModel;
using inv::kItemCount;
using inv::kMaxQuantity;

void setUp() {}
void tearDown() {}

static void test_initial_state() {
  InventoryModel m;
  TEST_ASSERT_EQUAL_size_t(8, m.itemCount());
  TEST_ASSERT_EQUAL_size_t(0, m.selectedIndex());
  TEST_ASSERT_FALSE(m.inEditMode());
  TEST_ASSERT_FALSE(m.dirty());
  for (size_t i = 0; i < m.itemCount(); i++) {
    TEST_ASSERT_EQUAL_UINT8(0, m.item(i).quantity);
  }
}

static void test_prebaked_item_names() {
  InventoryModel m;
  const char* expected[] = {
      "Chicken Breasts", "Chicken Tenderloins", "Chicken Wings",
      "Chicken Nuggets", "Steaks",              "Salmon",
      "White Fish",      "Ice Cream",
  };
  for (size_t i = 0; i < kItemCount; i++) {
    TEST_ASSERT_EQUAL_STRING(expected[i], m.item(i).name);
  }
}

static void test_browse_rotation_moves_selection() {
  InventoryModel m;
  m.rotate(1);
  TEST_ASSERT_EQUAL_size_t(1, m.selectedIndex());
  m.rotate(2);
  TEST_ASSERT_EQUAL_size_t(3, m.selectedIndex());
  m.rotate(-1);
  TEST_ASSERT_EQUAL_size_t(2, m.selectedIndex());
}

static void test_browse_rotation_wraps_both_directions() {
  InventoryModel m;
  m.rotate(-1);
  TEST_ASSERT_EQUAL_size_t(kItemCount - 1, m.selectedIndex());
  m.rotate(1);
  TEST_ASSERT_EQUAL_size_t(0, m.selectedIndex());
  // Multi-step wrap past either end.
  m.rotate((int)kItemCount + 3);
  TEST_ASSERT_EQUAL_size_t(3, m.selectedIndex());
  m.rotate(-(int)(2 * kItemCount));
  TEST_ASSERT_EQUAL_size_t(3, m.selectedIndex());
}

static void test_click_enters_edit_with_current_quantity() {
  InventoryModel m;
  m.setQuantity(0, 7);
  m.clearDirty();
  bool committed = m.click();
  TEST_ASSERT_FALSE(committed);
  TEST_ASSERT_TRUE(m.inEditMode());
  TEST_ASSERT_EQUAL_UINT8(7, m.pendingQuantity());
}

static void test_edit_rotation_adjusts_pending_not_item() {
  InventoryModel m;
  m.click();
  m.rotate(5);
  TEST_ASSERT_EQUAL_UINT8(5, m.pendingQuantity());
  TEST_ASSERT_EQUAL_UINT8(0, m.item(0).quantity);  // not committed yet
  m.rotate(-2);
  TEST_ASSERT_EQUAL_UINT8(3, m.pendingQuantity());
  // Selection must not move while editing.
  TEST_ASSERT_EQUAL_size_t(0, m.selectedIndex());
}

static void test_edit_clamps_at_zero_and_max() {
  InventoryModel m;
  m.click();
  m.rotate(-10);
  TEST_ASSERT_EQUAL_UINT8(0, m.pendingQuantity());
  m.rotate((int)kMaxQuantity + 25);
  TEST_ASSERT_EQUAL_UINT8(kMaxQuantity, m.pendingQuantity());
}

static void test_click_commits_and_marks_dirty() {
  InventoryModel m;
  m.click();
  m.rotate(4);
  bool committed = m.click();
  TEST_ASSERT_TRUE(committed);
  TEST_ASSERT_FALSE(m.inEditMode());
  TEST_ASSERT_EQUAL_UINT8(4, m.item(0).quantity);
  TEST_ASSERT_TRUE(m.dirty());
}

static void test_commit_without_change_is_not_dirty() {
  InventoryModel m;
  m.click();
  bool committed = m.click();  // no rotation in between
  TEST_ASSERT_TRUE(committed);
  TEST_ASSERT_FALSE(m.dirty());
}

static void test_cancel_edit_discards_pending() {
  InventoryModel m;
  m.setQuantity(2, 9);
  m.clearDirty();
  m.rotate(2);  // select item 2
  m.click();
  m.rotate(-4);
  TEST_ASSERT_EQUAL_UINT8(5, m.pendingQuantity());
  m.cancelEdit();
  TEST_ASSERT_FALSE(m.inEditMode());
  TEST_ASSERT_EQUAL_UINT8(9, m.item(2).quantity);
  TEST_ASSERT_FALSE(m.dirty());
}

static void test_set_quantity_clamps_and_marks_dirty() {
  InventoryModel m;
  m.setQuantity(1, 200);
  TEST_ASSERT_EQUAL_UINT8(kMaxQuantity, m.item(1).quantity);
  TEST_ASSERT_TRUE(m.dirty());
  m.clearDirty();
  TEST_ASSERT_FALSE(m.dirty());
}

static void test_rotate_zero_steps_is_noop() {
  InventoryModel m;
  m.rotate(0);
  TEST_ASSERT_EQUAL_size_t(0, m.selectedIndex());
  m.click();
  m.rotate(0);
  TEST_ASSERT_EQUAL_UINT8(0, m.pendingQuantity());
  TEST_ASSERT_TRUE(m.inEditMode());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_initial_state);
  RUN_TEST(test_prebaked_item_names);
  RUN_TEST(test_browse_rotation_moves_selection);
  RUN_TEST(test_browse_rotation_wraps_both_directions);
  RUN_TEST(test_click_enters_edit_with_current_quantity);
  RUN_TEST(test_edit_rotation_adjusts_pending_not_item);
  RUN_TEST(test_edit_clamps_at_zero_and_max);
  RUN_TEST(test_click_commits_and_marks_dirty);
  RUN_TEST(test_commit_without_change_is_not_dirty);
  RUN_TEST(test_cancel_edit_discards_pending);
  RUN_TEST(test_set_quantity_clamps_and_marks_dirty);
  RUN_TEST(test_rotate_zero_steps_is_noop);
  return UNITY_END();
}
