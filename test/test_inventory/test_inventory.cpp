#include <string.h>

#include <unity.h>

#include "inventory_model.h"
#include "inventory_persistence.h"

using inv::InventoryModel;
using inv::kItemCount;
using inv::kMaxQuantity;

void setUp() {}
void tearDown() {}

// Deliberately re-implements the production FNV-1a checksum as an
// independent oracle. Do not "deduplicate" this against the copy in
// inventory_persistence.cpp — sharing the implementation would make the
// checksum tests tautological.
static uint32_t persistenceChecksum(const uint8_t* data, size_t size) {
  uint32_t hash = 2166136261u;
  for (size_t i = 0; i < size; i++) {
    hash ^= data[i];
    hash *= 16777619u;
  }
  return hash;
}

static void rewritePersistenceChecksum(uint8_t* data, size_t size) {
  const uint32_t checksum =
      persistenceChecksum(data, size - inv::persistence::kChecksumSize);
  for (size_t i = 0; i < inv::persistence::kChecksumSize; i++) {
    data[size - inv::persistence::kChecksumSize + i] =
        static_cast<uint8_t>(checksum >> (8 * i));
  }
}

static void test_initial_state() {
  InventoryModel m;
  TEST_ASSERT_EQUAL_size_t(9, m.itemCount());
  TEST_ASSERT_EQUAL_size_t(0, m.selectedIndex());
  TEST_ASSERT_FALSE(m.inEditMode());
  TEST_ASSERT_FALSE(m.dirty());
  for (size_t i = 0; i < m.itemCount(); i++) {
    TEST_ASSERT_EQUAL_UINT8(0, m.item(i).quantity);
  }
}

static void test_prebaked_item_names() {
  InventoryModel m;
  for (size_t i = 0; i < kItemCount; i++) {
    TEST_ASSERT_EQUAL_UINT16(inv::kCatalog[i].id, m.item(i).id);
    TEST_ASSERT_EQUAL_STRING(inv::kCatalog[i].name, m.item(i).name);
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

static void test_restore_quantities_does_not_mark_dirty() {
  InventoryModel m;
  uint8_t quantities[kItemCount] = {1, 2, 3, 4, 5, 6, 7, 8, 200};
  m.restoreQuantities(quantities);
  for (size_t i = 0; i < kItemCount - 1; i++) {
    TEST_ASSERT_EQUAL_UINT8(i + 1, m.item(i).quantity);
  }
  TEST_ASSERT_EQUAL_UINT8(kMaxQuantity,
                          m.item(kItemCount - 1).quantity);
  TEST_ASSERT_FALSE(m.dirty());
}

static void test_persistence_round_trip() {
  InventoryModel source;
  for (size_t i = 0; i < kItemCount; i++) {
    source.setQuantity(i, static_cast<uint8_t>(i + 3));
  }
  uint8_t encoded[inv::persistence::kEncodedSize] = {0};
  TEST_ASSERT_TRUE(
      inv::persistence::encode(source, encoded, sizeof(encoded)));

  InventoryModel restored;
  TEST_ASSERT_TRUE(
      inv::persistence::decode(encoded, sizeof(encoded), restored));
  for (size_t i = 0; i < kItemCount; i++) {
    TEST_ASSERT_EQUAL_UINT8(source.item(i).quantity,
                            restored.item(i).quantity);
  }
  TEST_ASSERT_FALSE(restored.dirty());
}

static void test_persistence_rejects_corruption_without_mutating_model() {
  InventoryModel source;
  source.setQuantity(0, 9);
  uint8_t encoded[inv::persistence::kEncodedSize] = {0};
  TEST_ASSERT_TRUE(
      inv::persistence::encode(source, encoded, sizeof(encoded)));
  encoded[8] ^= 0xFF;

  InventoryModel restored;
  restored.setQuantity(0, 4);
  restored.clearDirty();
  TEST_ASSERT_FALSE(
      inv::persistence::decode(encoded, sizeof(encoded), restored));
  TEST_ASSERT_EQUAL_UINT8(4, restored.item(0).quantity);
  TEST_ASSERT_FALSE(restored.dirty());
}

static void test_persistence_maps_quantities_by_stable_id() {
  InventoryModel source;
  source.setQuantity(0, 11);
  source.setQuantity(1, 22);
  uint8_t encoded[inv::persistence::kEncodedSize] = {0};
  TEST_ASSERT_TRUE(
      inv::persistence::encode(source, encoded, sizeof(encoded)));

  // Swap complete records while leaving their IDs attached to quantities.
  for (size_t i = 0; i < inv::persistence::kEntrySize; i++) {
    uint8_t tmp = encoded[inv::persistence::kHeaderSize + i];
    encoded[inv::persistence::kHeaderSize + i] =
        encoded[inv::persistence::kHeaderSize +
                inv::persistence::kEntrySize + i];
    encoded[inv::persistence::kHeaderSize +
            inv::persistence::kEntrySize + i] = tmp;
  }
  rewritePersistenceChecksum(encoded, sizeof(encoded));

  InventoryModel restored;
  TEST_ASSERT_TRUE(
      inv::persistence::decode(encoded, sizeof(encoded), restored));
  TEST_ASSERT_EQUAL_UINT8(11, restored.item(0).quantity);
  TEST_ASSERT_EQUAL_UINT8(22, restored.item(1).quantity);
}

static void test_select_jumps_to_item() {
  InventoryModel m;
  m.select(5);
  TEST_ASSERT_EQUAL_size_t(5, m.selectedIndex());
  m.select(kItemCount);  // out of range: ignored
  TEST_ASSERT_EQUAL_size_t(5, m.selectedIndex());
  m.click();             // editing: selection is locked
  m.select(2);
  TEST_ASSERT_EQUAL_size_t(5, m.selectedIndex());
}

static void test_adjust_selected_quantity_clamps_and_marks_dirty() {
  InventoryModel m;
  TEST_ASSERT_FALSE(m.adjustSelectedQuantity(-1));  // clamped at 0
  TEST_ASSERT_FALSE(m.dirty());
  TEST_ASSERT_TRUE(m.adjustSelectedQuantity(1));
  TEST_ASSERT_EQUAL_UINT8(1, m.item(0).quantity);
  TEST_ASSERT_TRUE(m.dirty());

  m.setQuantity(0, kMaxQuantity);
  m.clearDirty();
  TEST_ASSERT_FALSE(m.adjustSelectedQuantity(1));  // clamped at max
  TEST_ASSERT_FALSE(m.dirty());

  m.rotate(2);  // adjustment follows the selection
  TEST_ASSERT_TRUE(m.adjustSelectedQuantity(1));
  TEST_ASSERT_EQUAL_UINT8(1, m.item(2).quantity);
}

static void test_mark_dirty_sets_dirty() {
  InventoryModel m;
  TEST_ASSERT_FALSE(m.dirty());
  m.markDirty();
  TEST_ASSERT_TRUE(m.dirty());
  m.clearDirty();
  TEST_ASSERT_FALSE(m.dirty());
}

static void test_encode_rejects_null_and_small_buffer() {
  InventoryModel m;
  uint8_t encoded[inv::persistence::kEncodedSize] = {0};
  TEST_ASSERT_FALSE(
      inv::persistence::encode(m, nullptr, sizeof(encoded)));
  TEST_ASSERT_FALSE(
      inv::persistence::encode(m, encoded, sizeof(encoded) - 1));
}

static void test_decode_rejects_null_truncated_and_oversized() {
  InventoryModel source;
  uint8_t encoded[inv::persistence::kEncodedSize] = {0};
  TEST_ASSERT_TRUE(
      inv::persistence::encode(source, encoded, sizeof(encoded)));

  InventoryModel restored;
  TEST_ASSERT_FALSE(
      inv::persistence::decode(nullptr, sizeof(encoded), restored));
  TEST_ASSERT_FALSE(
      inv::persistence::decode(encoded, sizeof(encoded) - 1, restored));

  // Trailing bytes beyond what the record count implies must be rejected.
  uint8_t padded[inv::persistence::kEncodedSize +
                 inv::persistence::kEntrySize] = {0};
  memcpy(padded, encoded, sizeof(encoded));
  TEST_ASSERT_FALSE(
      inv::persistence::decode(padded, sizeof(padded), restored));
}

static void test_decode_rejects_wrong_magic_and_version() {
  InventoryModel source;
  uint8_t encoded[inv::persistence::kEncodedSize] = {0};
  TEST_ASSERT_TRUE(
      inv::persistence::encode(source, encoded, sizeof(encoded)));

  InventoryModel restored;
  uint8_t bad[inv::persistence::kEncodedSize];

  // Checksum rewritten so rejection is attributable to the magic/version
  // field itself, not a checksum mismatch.
  memcpy(bad, encoded, sizeof(bad));
  bad[0] = 'X';
  rewritePersistenceChecksum(bad, sizeof(bad));
  TEST_ASSERT_FALSE(inv::persistence::decode(bad, sizeof(bad), restored));

  memcpy(bad, encoded, sizeof(bad));
  bad[4] = inv::persistence::kFormatVersion + 1;
  rewritePersistenceChecksum(bad, sizeof(bad));
  TEST_ASSERT_FALSE(inv::persistence::decode(bad, sizeof(bad), restored));
}

static void test_decode_rejects_excess_record_count() {
  InventoryModel source;
  uint8_t encoded[inv::persistence::kEncodedSize] = {0};
  TEST_ASSERT_TRUE(
      inv::persistence::encode(source, encoded, sizeof(encoded)));
  encoded[5] = inv::persistence::kMaxRecordCount + 1;
  rewritePersistenceChecksum(encoded, sizeof(encoded));

  InventoryModel restored;
  TEST_ASSERT_FALSE(
      inv::persistence::decode(encoded, sizeof(encoded), restored));
}

static void test_decode_rejects_duplicate_ids() {
  InventoryModel source;
  source.setQuantity(0, 9);
  uint8_t encoded[inv::persistence::kEncodedSize] = {0};
  TEST_ASSERT_TRUE(
      inv::persistence::encode(source, encoded, sizeof(encoded)));

  // Overwrite record 1 with a copy of record 0 so the same ID appears twice.
  memcpy(encoded + inv::persistence::kHeaderSize +
             inv::persistence::kEntrySize,
         encoded + inv::persistence::kHeaderSize,
         inv::persistence::kEntrySize);
  rewritePersistenceChecksum(encoded, sizeof(encoded));

  InventoryModel restored;
  restored.setQuantity(0, 4);
  restored.clearDirty();
  TEST_ASSERT_FALSE(
      inv::persistence::decode(encoded, sizeof(encoded), restored));
  TEST_ASSERT_EQUAL_UINT8(4, restored.item(0).quantity);
}

static void test_decode_ignores_unknown_ids() {
  InventoryModel source;
  source.setQuantity(0, 9);
  source.setQuantity(1, 22);
  uint8_t encoded[inv::persistence::kEncodedSize] = {0};
  TEST_ASSERT_TRUE(
      inv::persistence::encode(source, encoded, sizeof(encoded)));

  // Rewrite record 1's ID to one not in the catalog.
  encoded[inv::persistence::kHeaderSize + inv::persistence::kEntrySize] =
      0xFF;
  encoded[inv::persistence::kHeaderSize + inv::persistence::kEntrySize +
          1] = 0xFF;
  rewritePersistenceChecksum(encoded, sizeof(encoded));

  InventoryModel restored;
  TEST_ASSERT_TRUE(
      inv::persistence::decode(encoded, sizeof(encoded), restored));
  TEST_ASSERT_EQUAL_UINT8(9, restored.item(0).quantity);
  // The unknown record is skipped; item 1 falls back to zero.
  TEST_ASSERT_EQUAL_UINT8(0, restored.item(1).quantity);
}

static void test_decode_clamps_oversized_quantities() {
  InventoryModel source;
  uint8_t encoded[inv::persistence::kEncodedSize] = {0};
  TEST_ASSERT_TRUE(
      inv::persistence::encode(source, encoded, sizeof(encoded)));
  encoded[inv::persistence::kHeaderSize + 2] = 200;
  rewritePersistenceChecksum(encoded, sizeof(encoded));

  InventoryModel restored;
  TEST_ASSERT_TRUE(
      inv::persistence::decode(encoded, sizeof(encoded), restored));
  TEST_ASSERT_EQUAL_UINT8(kMaxQuantity, restored.item(0).quantity);
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
  RUN_TEST(test_restore_quantities_does_not_mark_dirty);
  RUN_TEST(test_persistence_round_trip);
  RUN_TEST(test_persistence_rejects_corruption_without_mutating_model);
  RUN_TEST(test_persistence_maps_quantities_by_stable_id);
  RUN_TEST(test_select_jumps_to_item);
  RUN_TEST(test_adjust_selected_quantity_clamps_and_marks_dirty);
  RUN_TEST(test_mark_dirty_sets_dirty);
  RUN_TEST(test_encode_rejects_null_and_small_buffer);
  RUN_TEST(test_decode_rejects_null_truncated_and_oversized);
  RUN_TEST(test_decode_rejects_wrong_magic_and_version);
  RUN_TEST(test_decode_rejects_excess_record_count);
  RUN_TEST(test_decode_rejects_duplicate_ids);
  RUN_TEST(test_decode_ignores_unknown_ids);
  RUN_TEST(test_decode_clamps_oversized_quantities);
  return UNITY_END();
}
