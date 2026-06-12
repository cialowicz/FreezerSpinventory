// Hardware-independent rotary encoder transition helpers.
#pragma once

#include <stdint.h>

namespace input {

int8_t quadratureDelta(uint8_t previousState, uint8_t currentState);
// Converts accumulated quadrature transitions into whole detents, carrying
// the partial remainder. transitionsPerDetent depends on the encoder: 4 for
// full-quadrature-per-click parts, 2 for half-quadrature ones.
int consumeDetents(int32_t& remainder, int32_t transitions,
                   int32_t transitionsPerDetent);

}  // namespace input
