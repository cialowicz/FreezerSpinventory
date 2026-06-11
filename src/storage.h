// NVS persistence for item quantities (ESP32 Preferences API).
#pragma once

#include "inventory_model.h"

namespace storage {

enum class LoadResult {
  kLoaded,
  kLoadedLegacy,
  kNotFound,
  kInvalid,
  kOpenFailed,
};

enum class SaveResult {
  kSaved,
  kOpenFailed,
  kWriteFailed,
  kVerifyFailed,
};

LoadResult load(inv::InventoryModel& model);

// Writes and verifies all quantities. The model is marked clean only after a
// successful read-back.
SaveResult save(inv::InventoryModel& model);

}  // namespace storage
