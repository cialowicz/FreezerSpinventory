// FreezerSpinventory — chest freezer inventory on an ELECROW CrowPanel 2.1"
// ESP32-S3 rotary display. Turn the knob to browse items, press to edit,
// turn to adjust the count, press again to save.
#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>

#include "config.h"
#include "display.h"
#include "encoder.h"
#include "inventory_model.h"
#include "io_expander.h"
#include "storage.h"
#include "ui.h"

namespace {

inv::InventoryModel model;
IoExpander expander;
RotaryEncoder knob;

uint32_t lastActivityMs = 0;
uint32_t lastSaveAttemptMs = 0;
bool savePending = false;
uint8_t consecutiveButtonReadErrors = 0;

enum class ButtonEvent {
  kNotPolled,
  kNoChange,
  kPressed,
  kReadError,
};

void markActivity(uint32_t now) {
  lastActivityMs = now;
}

void onCommit(uint32_t now) {
  if (model.dirty()) {
    savePending = true;
    lastSaveAttemptMs = now;
    ui::showSavingToast();
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

// Debounced press-edge detection for the knob button (polled over I2C).
ButtonEvent pollButton(uint32_t now) {
  static uint32_t lastPollMs = 0;
  static bool rawPrev = false;
  static bool stable = false;
  static uint32_t rawSinceMs = 0;

  if (now - lastPollMs < 15) {
    return ButtonEvent::kNotPolled;
  }
  lastPollMs = now;

  bool raw = false;
  if (!expander.readButton(raw)) {
    return ButtonEvent::kReadError;
  }
  if (raw != rawPrev) {
    rawPrev = raw;
    rawSinceMs = now;
    return ButtonEvent::kNoChange;
  }
  if (raw != stable && now - rawSinceMs >= 30) {
    stable = raw;
    return stable ? ButtonEvent::kPressed : ButtonEvent::kNoChange;
  }
  return ButtonEvent::kNoChange;
}

}  // namespace

void setup() {
  Serial.begin(115200);

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(400000);

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

  const storage::LoadResult loadResult = storage::load(model);
  if (loadResult == storage::LoadResult::kInvalid ||
      loadResult == storage::LoadResult::kOpenFailed) {
    Serial.println("Stored inventory could not be loaded");
  } else if (loadResult == storage::LoadResult::kLoadedLegacy) {
    const storage::SaveResult migrationResult = storage::save(model);
    if (migrationResult != storage::SaveResult::kSaved) {
      Serial.println("Legacy inventory migration failed");
    }
  }
  knob.begin(PIN_ENCODER_A, PIN_ENCODER_B, ENCODER_REVERSED);
  ui::init(&model);

  lastActivityMs = millis();
  Serial.println("FreezerSpinventory ready");
}

void loop() {
  lv_timer_handler();
  uint32_t now = millis();

  int steps = knob.consumeSteps();
  if (steps != 0) {
    model.rotate(steps);
    markActivity(now);
    ui::refresh();
  }

  const ButtonEvent buttonEvent = pollButton(now);
  if (buttonEvent == ButtonEvent::kReadError) {
    static uint32_t lastButtonErrorLogMs = 0;
    consecutiveButtonReadErrors++;
    if (now - lastButtonErrorLogMs >= 1000) {
      Serial.println("Knob button I2C read failed");
      lastButtonErrorLogMs = now;
    }
    if (consecutiveButtonReadErrors >= 5) {
      Serial.println("Reinitializing I2C bus");
      Wire.end();
      delay(5);
      Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
      Wire.setClock(400000);
      if (!expander.begin()) {
        Serial.println("PCF8574 recovery failed");
      }
      consecutiveButtonReadErrors = 0;
    }
  } else if (buttonEvent == ButtonEvent::kPressed) {
    consecutiveButtonReadErrors = 0;
    bool committed = model.click();
    if (committed) {
      onCommit(now);
    }
    markActivity(now);
    ui::refresh();
  } else if (buttonEvent == ButtonEvent::kNoChange) {
    consecutiveButtonReadErrors = 0;
  }

  // Walked away mid-edit: commit whatever is on screen and return to browse.
  if (model.inEditMode() && now - lastActivityMs >= EDIT_TIMEOUT_MS) {
    model.click();
    onCommit(now);
    ui::refresh();
  }

  // Debounced NVS write so rapid edits don't wear flash.
  if (savePending && now - lastSaveAttemptMs >= SAVE_DEBOUNCE_MS) {
    lastSaveAttemptMs = now;
    const storage::SaveResult result = storage::save(model);
    if (result == storage::SaveResult::kSaved) {
      savePending = false;
      ui::showSavedToast();
    } else {
      ui::showSaveFailedToast();
      Serial.println("Inventory save failed; retrying");
    }
  }

  display::setBacklight(now - lastActivityMs >= BACKLIGHT_DIM_MS
                            ? BACKLIGHT_DIM
                            : BACKLIGHT_FULL);

  delay(5);
}
