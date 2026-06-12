#include "inventory_model.h"

namespace inv {

namespace {
uint8_t clampQuantity(long q) {
  if (q < 0) return 0;
  if (q > kMaxQuantity) return kMaxQuantity;
  return (uint8_t)q;
}
}  // namespace

InventoryModel::InventoryModel() {
  for (size_t i = 0; i < kItemCount; i++) {
    items_[i] = Item{kCatalog[i].id, kCatalog[i].name, 0};
  }
}

void InventoryModel::rotate(int steps) {
  if (steps == 0) return;
  if (editing_) {
    pending_ = clampQuantity((long)pending_ + steps);
  } else {
    long n = (long)kItemCount;
    long idx = ((long)selected_ + steps) % n;
    if (idx < 0) idx += n;
    selected_ = (size_t)idx;
  }
}

void InventoryModel::select(size_t i) {
  if (i >= kItemCount || editing_) {
    return;
  }
  selected_ = i;
}

bool InventoryModel::click() {
  if (!editing_) {
    editing_ = true;
    pending_ = items_[selected_].quantity;
    return false;
  }
  editing_ = false;
  if (pending_ != items_[selected_].quantity) {
    items_[selected_].quantity = pending_;
    dirty_ = true;
  }
  return true;
}

void InventoryModel::cancelEdit() {
  editing_ = false;
}

bool InventoryModel::adjustSelectedQuantity(int delta) {
  const uint8_t clamped =
      clampQuantity((long)items_[selected_].quantity + delta);
  if (clamped == items_[selected_].quantity) {
    return false;
  }
  items_[selected_].quantity = clamped;
  dirty_ = true;
  return true;
}

void InventoryModel::setQuantity(size_t i, uint8_t q) {
  if (i >= kItemCount) return;
  uint8_t clamped = clampQuantity(q);
  if (items_[i].quantity != clamped) {
    items_[i].quantity = clamped;
    dirty_ = true;
  }
}

void InventoryModel::restoreQuantities(
    const uint8_t quantities[kItemCount]) {
  for (size_t i = 0; i < kItemCount; i++) {
    items_[i].quantity = clampQuantity(quantities[i]);
  }
  dirty_ = false;
}

}  // namespace inv
