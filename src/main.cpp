// FreezerSpinventory — chest freezer inventory on an ELECROW CrowPanel 2.1"
// ESP32-S3 rotary display. Turn the knob to browse items, press to edit,
// turn to adjust the count, press again to save.
#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>

#include "app_controller.h"
#include "button_debouncer.h"
#include "config.h"
#include "display.h"
#include "encoder.h"
#include "inventory_model.h"
#include "io_expander.h"
#include "storage.h"
#include "swipe_detector.h"
#include "touch.h"
#include "ui.h"

namespace {

inv::InventoryModel model;
IoExpander expander;
RotaryEncoder knob;
app::Controller controller(model,
                           app::Config{
                               .commitIdleMs = COMMIT_IDLE_MS,
                               .saveDebounceMs = SAVE_DEBOUNCE_MS,
                               .saveRetryMs = SAVE_RETRY_MS,
                               .overviewDelayMs = OVERVIEW_DELAY_MS,
                               .backlightDimMs = BACKLIGHT_DIM_MS,
                               .backlightFullLevel = BACKLIGHT_FULL,
                               .backlightDimLevel = BACKLIGHT_DIM,
                           });
input::ButtonDebouncer buttonDebouncer(15, 30);
TouchPanel touch;
input::SwipeDetector swipeDetector(TOUCH_SWIPE_MIN_PX, TOUCH_TAP_MAX_MS);
uint8_t consecutiveButtonReadErrors = 0;

enum class ButtonPollResult {
  kNotPolled,
  kNoChange,
  kPressed,
  kReadError,
};

void initI2cBus() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(400000);
}

// Rate-limited logging plus a full bus reset after persistent failures.
void handleButtonReadError(uint32_t now) {
  static uint32_t lastErrorLogMs = 0;
  consecutiveButtonReadErrors++;
  if (now - lastErrorLogMs >= 1000) {
    Serial.println("Knob button I2C read failed");
    lastErrorLogMs = now;
  }
  if (consecutiveButtonReadErrors >= 5) {
    Serial.println("Reinitializing I2C bus");
    Wire.end();
    delay(5);
    initI2cBus();
    if (!expander.begin()) {
      Serial.println("PCF8574 recovery failed");
    }
    consecutiveButtonReadErrors = 0;
  }
}

[[noreturn]] void restartAfterStartupFailure(const char* message) {
  Serial.println(message);
  Serial.println("Restarting in 3 seconds");
  delay(3000);
  ESP.restart();
  while (true) {
    delay(1000);
  }
}

// Mirrors a controller decision to the UI. Wake-only input restores the
// carousel (the overview/dim state consumed the gesture); model changes
// refresh; scheduled commits additionally announce the pending save.
void applyInputResult(app::InputResult result) {
  switch (result) {
    case app::InputResult::kWakeOnly:
      ui::showCarousel();
      break;
    case app::InputResult::kCommitScheduled:
      ui::showSavingToast();
      ui::refresh();
      break;
    case app::InputResult::kModelChanged:
      ui::refresh();
      break;
    case app::InputResult::kIgnored:
      break;
  }
}

// Polls the CST816 and feeds samples to the gesture classifier. Swipes are
// the secondary input mode: horizontal = browse, vertical = adjust count.
void pollTouch(uint32_t now) {
  static uint32_t lastPollMs = 0;
  static bool hasPolled = false;
  if (hasPolled && now - lastPollMs < TOUCH_POLL_MS) {
    return;
  }
  hasPolled = true;
  lastPollMs = now;

  int16_t x = 0;
  int16_t y = 0;
  const bool touched = touch.readTouch(x, y);
  const input::Gesture gesture = swipeDetector.update(touched, x, y, now);
  if (gesture == input::Gesture::kNone) {
    return;
  }
  ui::noteUserActivity();
  switch (gesture) {
    case input::Gesture::kSwipeLeft:  // flick the list left: next item
      applyInputResult(controller.swipeHorizontal(1, now));
      break;
    case input::Gesture::kSwipeRight:
      applyInputResult(controller.swipeHorizontal(-1, now));
      break;
    case input::Gesture::kSwipeUp:
      applyInputResult(controller.swipeVertical(1, now));
      break;
    case input::Gesture::kSwipeDown:
      applyInputResult(controller.swipeVertical(-1, now));
      break;
    case input::Gesture::kTap:
      applyInputResult(controller.tap(now));
      break;
    default:
      break;
  }
}

// Debounced press-edge detection for the knob button (polled over I2C).
ButtonPollResult pollButton(uint32_t now) {
  if (!buttonDebouncer.pollDue(now)) {
    return ButtonPollResult::kNotPolled;
  }

  bool raw = false;
  if (!expander.readButton(raw)) {
    return ButtonPollResult::kReadError;
  }
  const input::ButtonEvent event = buttonDebouncer.update(raw, now);
  if (event == input::ButtonEvent::kPressed) {
    return ButtonPollResult::kPressed;
  }
  return ButtonPollResult::kNoChange;
}

}  // namespace

void setup() {
  Serial.begin(115200);

  initI2cBus();

  if (!expander.begin()) {
    restartAfterStartupFailure("PCF8574 expander initialization failed");
  }
  if (!expander.panelPowerOnReset()) {
    restartAfterStartupFailure("Panel reset sequence failed");
  }

  const display::InitResult displayResult = display::init();
  if (displayResult != display::InitResult::kOk) {
    restartAfterStartupFailure(display::initResultMessage(displayResult));
  }

  bool migrationSavePending = false;
  bool loadFailed = false;
  const storage::LoadResult loadResult = storage::load(model);
  if (loadResult == storage::LoadResult::kInvalid ||
      loadResult == storage::LoadResult::kOpenFailed) {
    Serial.println("Stored inventory could not be loaded");
    loadFailed = true;
  } else if (loadResult == storage::LoadResult::kLoadedLegacy) {
    const storage::SaveResult migrationResult = storage::save(model);
    if (migrationResult != storage::SaveResult::kSaved) {
      Serial.println("Legacy inventory migration failed");
      migrationSavePending = true;
    }
  }
  knob.begin(PIN_ENCODER_A, PIN_ENCODER_B, ENCODER_REVERSED);
  if (!touch.begin()) {
    // Non-fatal: the chip wakes on the first touch event and answers polls
    // while a finger is down, so gestures still work without this.
    Serial.println("Touch auto-sleep disable failed");
  }
  ui::init(&model);

  const uint32_t now = millis();
  controller.begin(now);
  if (loadFailed) {
    // The zero counts on screen are not real data; say so instead of
    // letting them masquerade as an empty freezer.
    ui::showLoadFailedNotice();
  }
  if (migrationSavePending) {
    controller.scheduleSave(now);
    ui::showSaveFailedToast();
  }
  Serial.println("FreezerSpinventory ready");
}

void loop() {
  lv_timer_handler();
  uint32_t now = millis();

  int steps = knob.consumeSteps();
  if (steps != 0) {
    ui::noteUserActivity();
    applyInputResult(controller.rotate(steps, now));
  }

  const ButtonPollResult buttonEvent = pollButton(now);
  if (buttonEvent == ButtonPollResult::kReadError) {
    handleButtonReadError(now);
  } else if (buttonEvent == ButtonPollResult::kPressed) {
    consecutiveButtonReadErrors = 0;
    ui::noteUserActivity();
    applyInputResult(controller.press(now));
  } else if (buttonEvent == ButtonPollResult::kNoChange) {
    consecutiveButtonReadErrors = 0;
  }

  pollTouch(now);

  const app::TickResult tickResult = controller.tick(now);
  if (tickResult == app::TickResult::kAutoCommitted) {
    ui::refresh();  // back to browse styling; any save announces itself
  } else if (tickResult == app::TickResult::kOverviewDue) {
    ui::showOverview();
  }

  if (controller.saveDue(now)) {
    if (!model.dirty()) {
      // Nothing left to persist; resolve the pending save without an
      // NVS write to spare flash wear.
      controller.recordSaveResult(true, now);
      ui::showSavedToast();
    } else {
      const storage::SaveResult result = storage::save(model);
      const bool saved = result == storage::SaveResult::kSaved;
      controller.recordSaveResult(saved, now);
      if (saved) {
        ui::showSavedToast();
      } else {
        ui::showSaveFailedToast();
        Serial.println("Inventory save failed; retrying");
      }
    }
  }

  display::setBacklight(controller.backlightLevel());

  delay(5);
}
