#include "app_controller.h"

namespace app {

Controller::Controller(inv::InventoryModel& model, const Config& config)
    : model_(model), config_(config) {}

void Controller::begin(uint32_t now) {
  lastActivityMs_ = now;
  dimmed_ = false;
}

InputResult Controller::rotate(int steps, uint32_t now) {
  if (steps == 0) {
    return InputResult::kIgnored;
  }
  if (wakeIfDimmed(now)) {
    return InputResult::kWakeOnly;
  }
  model_.rotate(steps);
  markActivity(now);
  return InputResult::kModelChanged;
}

InputResult Controller::press(uint32_t now) {
  if (wakeIfDimmed(now)) {
    return InputResult::kWakeOnly;
  }

  const uint8_t quantityBefore = model_.selectedItem().quantity;
  const bool committed = model_.click();
  const bool quantityChanged =
      committed && model_.selectedItem().quantity != quantityBefore;
  markActivity(now);

  if (quantityChanged) {
    scheduleSave(now);
    return InputResult::kCommitScheduled;
  }
  return InputResult::kModelChanged;
}

TickResult Controller::tick(uint32_t now) {
  TickResult result = TickResult::kNoChange;
  if (model_.inEditMode() &&
      now - lastActivityMs_ >= config_.editTimeoutMs) {
    model_.cancelEdit();
    result = TickResult::kEditCancelled;
  }
  if (!dimmed_ && now - lastActivityMs_ >= config_.backlightDimMs) {
    dimmed_ = true;
  }
  return result;
}

void Controller::scheduleSave(uint32_t now) {
  savePending_ = true;
  saveTimerStartedMs_ = now;
  saveDelayMs_ = config_.saveDebounceMs;
}

bool Controller::saveDue(uint32_t now) const {
  return savePending_ && now - saveTimerStartedMs_ >= saveDelayMs_;
}

void Controller::recordSaveResult(bool success, uint32_t now) {
  if (success) {
    savePending_ = false;
    return;
  }
  savePending_ = true;
  saveTimerStartedMs_ = now;
  saveDelayMs_ = config_.saveRetryMs;
}

uint8_t Controller::backlightLevel() const {
  return dimmed_ ? config_.backlightDimLevel
                  : config_.backlightFullLevel;
}

bool Controller::wakeIfDimmed(uint32_t now) {
  if (!dimmed_) {
    return false;
  }
  dimmed_ = false;
  markActivity(now);
  return true;
}

void Controller::markActivity(uint32_t now) {
  lastActivityMs_ = now;
}

}  // namespace app
