#include "app_controller.h"

namespace app {

Controller::Controller(inv::InventoryModel& model, const Config& config)
    : model_(model), config_(config) {}

void Controller::begin(uint32_t now) {
  lastActivityMs_ = now;
  dimmed_ = false;
  overview_ = false;
}

InputResult Controller::rotate(int steps, uint32_t now) {
  if (steps == 0) {
    return InputResult::kIgnored;
  }
  if (wakeIfIdle(now)) {
    return InputResult::kWakeOnly;
  }
  model_.rotate(steps);
  markActivity(now);
  return InputResult::kModelChanged;
}

InputResult Controller::press(uint32_t now) {
  if (wakeIfIdle(now)) {
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

InputResult Controller::swipeHorizontal(int direction, uint32_t now) {
  if (direction == 0) {
    return InputResult::kIgnored;
  }
  if (wakeIfIdle(now)) {
    return InputResult::kWakeOnly;
  }
  markActivity(now);
  if (model_.inEditMode()) {
    return InputResult::kIgnored;  // selection is locked while editing
  }
  model_.rotate(direction);
  return InputResult::kModelChanged;
}

InputResult Controller::swipeVertical(int direction, uint32_t now) {
  if (direction == 0) {
    return InputResult::kIgnored;
  }
  if (wakeIfIdle(now)) {
    return InputResult::kWakeOnly;
  }
  markActivity(now);
  if (model_.inEditMode()) {
    model_.rotate(direction);
    return InputResult::kModelChanged;
  }
  if (!model_.adjustSelectedQuantity(direction)) {
    return InputResult::kIgnored;  // clamped at a limit; nothing changed
  }
  scheduleSave(now);
  return InputResult::kCommitScheduled;
}

InputResult Controller::tap(uint32_t now) {
  if (wakeIfIdle(now)) {
    return InputResult::kWakeOnly;
  }
  markActivity(now);
  return InputResult::kIgnored;
}

TickResult Controller::tick(uint32_t now) {
  if (!dimmed_ && now - lastActivityMs_ >= config_.backlightDimMs) {
    dimmed_ = true;
  }
  if (model_.inEditMode()) {
    if (now - lastActivityMs_ >= config_.editTimeoutMs) {
      model_.cancelEdit();
      return TickResult::kEditCancelled;
    }
    return TickResult::kNoChange;
  }
  if (!overview_ && now - lastActivityMs_ >= config_.overviewDelayMs) {
    overview_ = true;
    return TickResult::kOverviewDue;
  }
  return TickResult::kNoChange;
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

bool Controller::wakeIfIdle(uint32_t now) {
  if (!dimmed_ && !overview_) {
    return false;
  }
  dimmed_ = false;
  overview_ = false;
  markActivity(now);
  return true;
}

void Controller::markActivity(uint32_t now) {
  lastActivityMs_ = now;
}

}  // namespace app
