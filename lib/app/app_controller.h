// Hardware-independent application timing and interaction orchestration.
#pragma once

#include <stdint.h>

#include "inventory_model.h"

namespace app {

struct Config {
  Config(uint32_t editTimeout, uint32_t saveDebounce, uint32_t saveRetry,
         uint32_t backlightDim, uint8_t backlightFull,
         uint8_t backlightDimmed)
      : editTimeoutMs(editTimeout),
        saveDebounceMs(saveDebounce),
        saveRetryMs(saveRetry),
        backlightDimMs(backlightDim),
        backlightFullLevel(backlightFull),
        backlightDimLevel(backlightDimmed) {}

  uint32_t editTimeoutMs;
  uint32_t saveDebounceMs;
  uint32_t saveRetryMs;
  uint32_t backlightDimMs;
  uint8_t backlightFullLevel;
  uint8_t backlightDimLevel;
};

enum class InputResult {
  kIgnored,
  kWakeOnly,
  kModelChanged,
  kCommitScheduled,
};

enum class TickResult {
  kNoChange,
  kEditCancelled,
};

class Controller {
 public:
  Controller(inv::InventoryModel& model, const Config& config);

  void begin(uint32_t now);
  InputResult rotate(int steps, uint32_t now);
  InputResult press(uint32_t now);
  TickResult tick(uint32_t now);

  void scheduleSave(uint32_t now);
  bool saveDue(uint32_t now) const;
  void recordSaveResult(bool success, uint32_t now);

  bool dimmed() const { return dimmed_; }
  uint8_t backlightLevel() const;

 private:
  bool wakeIfDimmed(uint32_t now);
  void markActivity(uint32_t now);

  inv::InventoryModel& model_;
  Config config_;
  uint32_t lastActivityMs_ = 0;
  uint32_t saveTimerStartedMs_ = 0;
  uint32_t saveDelayMs_ = 0;
  bool savePending_ = false;
  bool dimmed_ = false;
};

}  // namespace app
