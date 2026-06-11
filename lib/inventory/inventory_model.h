// Pure inventory + knob-interaction model. No Arduino/hardware dependencies
// so it can be unit-tested on the host (pio test -e native).
//
// Interaction model:
//   browse mode:  rotate = move selection through the item list (wraps)
//                 click  = enter edit mode for the selected item
//   edit mode:    rotate = raise/lower the pending quantity (clamped)
//                 click  = commit the pending quantity, back to browse
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace inv {

constexpr size_t kItemCount = 8;
constexpr uint8_t kMaxQuantity = 50;

struct Item {
  const char* name;
  uint8_t quantity;
};

class InventoryModel {
 public:
  InventoryModel();

  size_t itemCount() const { return kItemCount; }
  const Item& item(size_t i) const { return items_[i]; }
  size_t selectedIndex() const { return selected_; }
  const Item& selectedItem() const { return items_[selected_]; }

  bool inEditMode() const { return editing_; }
  // Quantity being adjusted; only meaningful while in edit mode.
  uint8_t pendingQuantity() const { return pending_; }

  // Apply encoder detents (positive = clockwise). Browse: moves selection.
  // Edit: adjusts pending quantity.
  void rotate(int steps);

  // Knob press. Returns true if a quantity was committed (i.e. we left edit
  // mode), so the caller knows when to schedule a save.
  bool click();

  // Leave edit mode discarding the pending quantity (e.g. inactivity).
  void cancelEdit();

  // Direct write, used when loading persisted state. Clamps to kMaxQuantity.
  void setQuantity(size_t i, uint8_t q);

  // True when committed quantities haven't been persisted yet.
  bool dirty() const { return dirty_; }
  void clearDirty() { dirty_ = false; }

 private:
  Item items_[kItemCount];
  size_t selected_ = 0;
  bool editing_ = false;
  uint8_t pending_ = 0;
  bool dirty_ = false;
};

}  // namespace inv
