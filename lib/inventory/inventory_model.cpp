#include "inventory_model.h"

namespace inv {

namespace {
const char* const kNames[kItemCount] = {
    "Chicken Breasts", "Chicken Tenderloins", "Chicken Wings",
    "Chicken Nuggets", "Steaks",              "Salmon",
    "White Fish",      "Ice Cream",
};

uint8_t clampQuantity(long q) {
  if (q < 0) return 0;
  if (q > kMaxQuantity) return kMaxQuantity;
  return (uint8_t)q;
}
}  // namespace

InventoryModel::InventoryModel() {
  for (size_t i = 0; i < kItemCount; i++) {
    items_[i] = Item{kNames[i], 0};
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

void InventoryModel::setQuantity(size_t i, uint8_t q) {
  if (i >= kItemCount) return;
  uint8_t clamped = clampQuantity(q);
  if (items_[i].quantity != clamped) {
    items_[i].quantity = clamped;
    dirty_ = true;
  }
}

}  // namespace inv
