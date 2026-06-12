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
#include "ui.h"

namespace {

inv::InventoryModel model;
IoExpander expander;
RotaryEncoder knob;
app::Controller controller(
    model, app::Config(EDIT_TIMEOUT_MS, SAVE_DEBOUNCE_MS, SAVE_RETRY_MS,
                       BACKLIGHT_DIM_MS, BACKLIGHT_FULL, BACKLIGHT_DIM));
input::ButtonDebouncer buttonDebouncer(15, 30);
uint8_t consecutiveButtonReadErrors = 0;

enum class ButtonPollResult {
  kNotPolled,
  kNoChange,
  kPressed,
  kReadError,
};

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

  bool migrationSavePending = false;
  const storage::LoadResult loadResult = storage::load(model);
  if (loadResult == storage::LoadResult::kInvalid ||
      loadResult == storage::LoadResult::kOpenFailed) {
    Serial.println("Stored inventory could not be loaded");
  } else if (loadResult == storage::LoadResult::kLoadedLegacy) {
    const storage::SaveResult migrationResult = storage::save(model);
    if (migrationResult != storage::SaveResult::kSaved) {
      Serial.println("Legacy inventory migration failed");
      migrationSavePending = true;
    }
  }
  knob.begin(PIN_ENCODER_A, PIN_ENCODER_B, ENCODER_REVERSED);
  ui::init(&model);

  const uint32_t now = millis();
  controller.begin(now);
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
    const app::InputResult result = controller.rotate(steps, now);
    if (result == app::InputResult::kModelChanged) {
      ui::refresh();
    }
  }

  const ButtonPollResult buttonEvent = pollButton(now);
  if (buttonEvent == ButtonPollResult::kReadError) {
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
  } else if (buttonEvent == ButtonPollResult::kPressed) {
    consecutiveButtonReadErrors = 0;
    const app::InputResult result = controller.press(now);
    if (result == app::InputResult::kCommitScheduled) {
      ui::showSavingToast();
    }
    if (result == app::InputResult::kModelChanged ||
        result == app::InputResult::kCommitScheduled) {
      ui::refresh();
    }
  } else if (buttonEvent == ButtonPollResult::kNoChange) {
    consecutiveButtonReadErrors = 0;
  }

  if (controller.tick(now) == app::TickResult::kEditCancelled) {
    ui::refresh();
    ui::showEditCancelledToast();
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
