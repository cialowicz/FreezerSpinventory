// Hardware-independent application timing and interaction orchestration.
#pragma once

#include <stdint.h>

#include "inventory_model.h"

namespace app {

// Aggregate; construct with designated initializers so the four
// indistinguishable uint32_t durations cannot be silently transposed.
struct Config {
  uint32_t editTimeoutMs;
  uint32_t saveDebounceMs;
  uint32_t saveRetryMs;
  uint32_t overviewDelayMs;
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
  kOverviewDue,
};

class Controller {
 public:
  Controller(inv::InventoryModel& model, const Config& config);

  void begin(uint32_t now);
  InputResult rotate(int steps, uint32_t now);
  InputResult press(uint32_t now);

  // Touch gestures (secondary input mode). Horizontal swipes browse like
  // rotation; vertical swipes adjust the count — directly committed in
  // browse mode, pending in edit mode. Taps only count as activity.
  InputResult swipeHorizontal(int direction, uint32_t now);
  InputResult swipeVertical(int direction, uint32_t now);
  InputResult tap(uint32_t now);

  TickResult tick(uint32_t now);

  void scheduleSave(uint32_t now);
  bool saveDue(uint32_t now) const;
  void recordSaveResult(bool success, uint32_t now);

  bool dimmed() const { return dimmed_; }
  uint8_t backlightLevel() const;
  // True while the idle at-a-glance inventory view should be shown.
  bool overviewActive() const { return overview_; }

 private:
  bool wakeIfIdle(uint32_t now);
  void markActivity(uint32_t now);

  inv::InventoryModel& model_;
  Config config_;
  uint32_t lastActivityMs_ = 0;
  uint32_t saveTimerStartedMs_ = 0;
  uint32_t saveDelayMs_ = 0;
  bool savePending_ = false;
  bool dimmed_ = false;
  bool overview_ = false;
};

}  // namespace app
