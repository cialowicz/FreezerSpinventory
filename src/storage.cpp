#include "storage.h"

#include <Preferences.h>
#include <string.h>

#include "inventory_persistence.h"

namespace storage {
namespace {
const char* kNamespace = "freezerspin";
const char* kKey = "inventory_v2";
const char* kLegacyKey = "qty_v1";

LoadResult loadLegacy(Preferences& prefs, inv::InventoryModel& model) {
  uint8_t quantities[inv::kItemCount] = {0};
  const size_t got =
      prefs.getBytes(kLegacyKey, quantities, sizeof(quantities));
  if (got == 0) {
    return LoadResult::kNotFound;
  }
  if (got != sizeof(quantities)) {
    return LoadResult::kInvalid;
  }
  model.restoreQuantities(quantities);
  // Legacy data still needs to be rewritten in the current format.
  model.markDirty();
  return LoadResult::kLoadedLegacy;
}
}  // namespace

LoadResult load(inv::InventoryModel& model) {
  Preferences prefs;
  if (!prefs.begin(kNamespace, /*readOnly=*/false)) {
    return LoadResult::kOpenFailed;
  }

  const size_t length = prefs.getBytesLength(kKey);
  if (length == 0) {
    const LoadResult result = loadLegacy(prefs, model);
    prefs.end();
    return result;
  }
  if (length > inv::persistence::kMaxEncodedSize) {
    prefs.end();
    return LoadResult::kInvalid;
  }

  uint8_t data[inv::persistence::kMaxEncodedSize] = {0};
  const size_t got = prefs.getBytes(kKey, data, sizeof(data));
  prefs.end();
  if (got != length ||
      !inv::persistence::decode(data, got, model)) {
    return LoadResult::kInvalid;
  }
  return LoadResult::kLoaded;
}

SaveResult save(inv::InventoryModel& model) {
  uint8_t data[inv::persistence::kEncodedSize] = {0};
  if (!inv::persistence::encode(model, data, sizeof(data))) {
    return SaveResult::kWriteFailed;
  }

  Preferences prefs;
  if (!prefs.begin(kNamespace, /*readOnly=*/false)) {
    return SaveResult::kOpenFailed;
  }

  if (prefs.putBytes(kKey, data, sizeof(data)) != sizeof(data)) {
    prefs.end();
    return SaveResult::kWriteFailed;
  }

  uint8_t verification[sizeof(data)] = {0};
  const size_t got = prefs.getBytes(kKey, verification, sizeof(verification));
  if (got != sizeof(data) ||
      memcmp(data, verification, sizeof(data)) != 0) {
    prefs.end();
    return SaveResult::kVerifyFailed;
  }

  // The verified v2 record supersedes any legacy data; dropping the old key
  // prevents a lost v2 key from silently resurrecting stale quantities.
  if (prefs.isKey(kLegacyKey)) {
    prefs.remove(kLegacyKey);
  }
  prefs.end();

  model.clearDirty();
  return SaveResult::kSaved;
}

}  // namespace storage
