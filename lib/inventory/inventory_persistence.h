// Hardware-independent serialization for inventory quantities.
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "inventory_model.h"

namespace inv {
namespace persistence {

constexpr uint8_t kFormatVersion = 1;
constexpr size_t kHeaderSize = 6;
constexpr size_t kEntrySize = 3;
constexpr size_t kChecksumSize = 4;
constexpr size_t kEncodedSize =
    kHeaderSize + kItemCount * kEntrySize + kChecksumSize;
constexpr size_t kMaxRecordCount = 32;
constexpr size_t kMaxEncodedSize =
    kHeaderSize + kMaxRecordCount * kEntrySize + kChecksumSize;

bool encode(const InventoryModel& model, uint8_t* output, size_t outputSize);
bool decode(const uint8_t* data, size_t dataSize, InventoryModel& model);

}  // namespace persistence
}  // namespace inv
