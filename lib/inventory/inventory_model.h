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

constexpr uint8_t kMaxQuantity = 50;

struct CatalogItem {
  uint16_t id;
  const char* name;
};

// Stable IDs are persisted. Entries may be reordered without moving quantities
// between products.
constexpr CatalogItem kCatalog[] = {
    {100, "Chicken Breasts"},
    {110, "Chicken Tenderloins"},
    {120, "Chicken Wings"},
    {130, "Chicken Nuggets"},
    {200, "Steaks"},
    {210, "Ground Beef"},
    {300, "Salmon"},
    {310, "White Fish"},
    {400, "Ice Cream"},
};
constexpr size_t kItemCount = sizeof(kCatalog) / sizeof(kCatalog[0]);

struct Item {
  uint16_t id;
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

  // Jumps the selection straight to item i (e.g. tapping an overview row).
  // Ignored when out of range or while editing (selection is locked).
  void select(size_t i);

  // Knob press. Returns true if a quantity was committed (i.e. we left edit
  // mode), so the caller knows when to schedule a save.
  bool click();

  // Leave edit mode discarding the pending quantity (e.g. inactivity).
  void cancelEdit();

  // Adjusts the selected item's committed quantity directly, bypassing edit
  // mode (touch swipes commit immediately). Clamps to [0, kMaxQuantity];
  // returns true and marks the model dirty if the quantity changed.
  bool adjustSelectedQuantity(int delta);

  // Direct write, used when loading persisted state. Clamps to kMaxQuantity.
  void setQuantity(size_t i, uint8_t q);

  // Replaces all quantities from validated persisted state without marking the
  // model dirty.
  void restoreQuantities(const uint8_t quantities[kItemCount]);

  // True when committed quantities haven't been persisted yet.
  bool dirty() const { return dirty_; }
  void clearDirty() { dirty_ = false; }
  // Marks the current state as not-yet-persisted, e.g. after loading from
  // the legacy format that still needs to be rewritten in the current one.
  void markDirty() { dirty_ = true; }

 private:
  Item items_[kItemCount];
  size_t selected_ = 0;
  bool editing_ = false;
  uint8_t pending_ = 0;
  bool dirty_ = false;
};

}  // namespace inv
