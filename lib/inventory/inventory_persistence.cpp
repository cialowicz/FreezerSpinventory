#include "inventory_persistence.h"

#include <string.h>

namespace inv {
namespace persistence {
namespace {

constexpr uint8_t kMagic[] = {'F', 'S', 'I', '2'};

uint32_t checksum(const uint8_t* data, size_t size) {
  uint32_t hash = 2166136261u;
  for (size_t i = 0; i < size; i++) {
    hash ^= data[i];
    hash *= 16777619u;
  }
  return hash;
}

void write16(uint8_t* destination, uint16_t value) {
  destination[0] = static_cast<uint8_t>(value);
  destination[1] = static_cast<uint8_t>(value >> 8);
}

void write32(uint8_t* destination, uint32_t value) {
  for (size_t i = 0; i < 4; i++) {
    destination[i] = static_cast<uint8_t>(value >> (8 * i));
  }
}

uint16_t read16(const uint8_t* source) {
  return static_cast<uint16_t>(source[0]) |
         static_cast<uint16_t>(source[1] << 8);
}

uint32_t read32(const uint8_t* source) {
  uint32_t value = 0;
  for (size_t i = 0; i < 4; i++) {
    value |= static_cast<uint32_t>(source[i]) << (8 * i);
  }
  return value;
}

}  // namespace

bool encode(const InventoryModel& model, uint8_t* output,
            size_t outputSize) {
  if (output == nullptr || outputSize < kEncodedSize) {
    return false;
  }

  memcpy(output, kMagic, sizeof(kMagic));
  output[4] = kFormatVersion;
  output[5] = static_cast<uint8_t>(kItemCount);

  size_t offset = kHeaderSize;
  for (size_t i = 0; i < kItemCount; i++) {
    write16(output + offset, model.item(i).id);
    output[offset + 2] = model.item(i).quantity;
    offset += kEntrySize;
  }

  write32(output + offset, checksum(output, offset));
  return true;
}

bool decode(const uint8_t* data, size_t dataSize, InventoryModel& model) {
  if (data == nullptr || dataSize < kHeaderSize + kChecksumSize ||
      memcmp(data, kMagic, sizeof(kMagic)) != 0 ||
      data[4] != kFormatVersion) {
    return false;
  }

  const size_t recordCount = data[5];
  if (recordCount > kMaxRecordCount) {
    return false;
  }
  const size_t expectedSize =
      kHeaderSize + recordCount * kEntrySize + kChecksumSize;
  if (dataSize != expectedSize ||
      read32(data + expectedSize - kChecksumSize) !=
          checksum(data, expectedSize - kChecksumSize)) {
    return false;
  }

  uint8_t quantities[kItemCount] = {0};
  bool seen[kItemCount] = {false};
  size_t offset = kHeaderSize;
  for (size_t record = 0; record < recordCount; record++) {
    const uint16_t id = read16(data + offset);
    const uint8_t quantity = data[offset + 2];
    offset += kEntrySize;

    for (size_t i = 0; i < kItemCount; i++) {
      if (kCatalog[i].id != id) {
        continue;
      }
      if (seen[i]) {
        return false;
      }
      quantities[i] =
          quantity > kMaxQuantity ? kMaxQuantity : quantity;
      seen[i] = true;
      break;
    }
  }

  model.restoreQuantities(quantities);
  return true;
}

}  // namespace persistence
}  // namespace inv
