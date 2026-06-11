// Hardware-independent rotary encoder transition helpers.
#pragma once

#include <stdint.h>

namespace input {

int8_t quadratureDelta(uint8_t previousState, uint8_t currentState);
int consumeDetents(int32_t& remainder, int32_t transitions);

}  // namespace input
