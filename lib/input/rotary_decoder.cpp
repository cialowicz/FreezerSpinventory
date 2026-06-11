#include "rotary_decoder.h"

namespace input {
namespace {

// Index: (previous state << 2) | current state of (A<<1 | B).
const int8_t kQuadratureLut[16] = {
    0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0,
};

}  // namespace

int8_t quadratureDelta(uint8_t previousState, uint8_t currentState) {
  const uint8_t index =
      static_cast<uint8_t>(((previousState << 2) | currentState) & 0x0F);
  return kQuadratureLut[index];
}

int consumeDetents(int32_t& remainder, int32_t transitions) {
  remainder += transitions;
  const int steps = remainder / 4;
  remainder -= steps * 4;
  return steps;
}

}  // namespace input
