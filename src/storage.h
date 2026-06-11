// NVS persistence for item quantities (ESP32 Preferences API).
#pragma once

#include "inventory_model.h"

namespace storage {

void load(inv::InventoryModel& model);
// Writes all quantities and clears the model's dirty flag.
void save(inv::InventoryModel& model);

}  // namespace storage
