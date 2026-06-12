#include "rotary_decoder.h"

// quadratureDelta() runs inside an IRAM GPIO ISR on the device, which may
// fire while the flash cache is disabled (NVS writes). Both the function and
// its lookup table must therefore live in RAM there; on the native test build
// these attributes compile away.
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP_PLATFORM)
#include <esp_attr.h>
#define INPUT_ISR_ATTR IRAM_ATTR
#define INPUT_ISR_DATA_ATTR DRAM_ATTR
#else
#define INPUT_ISR_ATTR
#define INPUT_ISR_DATA_ATTR
#endif

namespace input {
namespace {

// Index: (previous state << 2) | current state of (A<<1 | B).
INPUT_ISR_DATA_ATTR const int8_t kQuadratureLut[16] = {
    0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0,
};

}  // namespace

int8_t INPUT_ISR_ATTR quadratureDelta(uint8_t previousState,
                                      uint8_t currentState) {
  const uint8_t index =
      static_cast<uint8_t>(((previousState << 2) | currentState) & 0x0F);
  return kQuadratureLut[index];
}

int consumeDetents(int32_t& remainder, int32_t transitions,
                   int32_t transitionsPerDetent) {
  remainder += transitions;
  const int steps = remainder / transitionsPerDetent;
  remainder -= steps * transitionsPerDetent;
  return steps;
}

}  // namespace input
