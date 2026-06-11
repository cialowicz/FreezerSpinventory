#include "storage.h"

#include <Preferences.h>

namespace storage {
namespace {
const char* kNamespace = "freezerspin";
const char* kKey = "qty_v1";
}  // namespace

void load(inv::InventoryModel& model) {
  Preferences prefs;
  if (!prefs.begin(kNamespace, /*readOnly=*/true)) {
    return;  // first boot: nothing stored yet
  }
  uint8_t quantities[inv::kItemCount] = {0};
  size_t got = prefs.getBytes(kKey, quantities, sizeof(quantities));
  prefs.end();
  if (got != sizeof(quantities)) {
    return;
  }
  for (size_t i = 0; i < inv::kItemCount; i++) {
    model.setQuantity(i, quantities[i]);
  }
  model.clearDirty();
}

void save(inv::InventoryModel& model) {
  Preferences prefs;
  if (!prefs.begin(kNamespace, /*readOnly=*/false)) {
    return;
  }
  uint8_t quantities[inv::kItemCount];
  for (size_t i = 0; i < inv::kItemCount; i++) {
    quantities[i] = model.item(i).quantity;
  }
  prefs.putBytes(kKey, quantities, sizeof(quantities));
  prefs.end();
  model.clearDirty();
}

}  // namespace storage
