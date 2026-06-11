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
uint32_t lastCommitMs = 0;
bool savePending = false;

void markActivity(uint32_t now) {
  lastActivityMs = now;
}

void onCommit(uint32_t now) {
  if (model.dirty()) {
    savePending = true;
    lastCommitMs = now;
  }
}

// Debounced press-edge detection for the knob button (polled over I2C).
bool buttonPressed(uint32_t now) {
  static uint32_t lastPollMs = 0;
  static bool rawPrev = false;
  static bool stable = false;
  static uint32_t rawSinceMs = 0;

  if (now - lastPollMs < 15) {
    return false;
  }
  lastPollMs = now;

  bool raw = expander.buttonDown();
  if (raw != rawPrev) {
    rawPrev = raw;
    rawSinceMs = now;
    return false;
  }
  if (raw != stable && now - rawSinceMs >= 30) {
    stable = raw;
    return stable;  // true only on the press edge
  }
  return false;
}

}  // namespace

void setup() {
  Serial.begin(115200);

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(400000);

  if (!expander.begin()) {
    Serial.println("PCF8574 expander not found at 0x21");
  }
  expander.panelPowerOnReset();

  if (!display::init()) {
    Serial.println("Display init failed");
  }

  storage::load(model);
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

  if (buttonPressed(now)) {
    bool committed = model.click();
    if (committed) {
      onCommit(now);
      ui::showSavedToast();
    }
    markActivity(now);
    ui::refresh();
  }

  // Walked away mid-edit: commit whatever is on screen and return to browse.
  if (model.inEditMode() && now - lastActivityMs >= EDIT_TIMEOUT_MS) {
    model.click();
    onCommit(now);
    ui::refresh();
  }

  // Debounced NVS write so rapid edits don't wear flash.
  if (savePending && now - lastCommitMs >= SAVE_DEBOUNCE_MS) {
    storage::save(model);
    savePending = false;
  }

  display::setBacklight(now - lastActivityMs >= BACKLIGHT_DIM_MS
                            ? BACKLIGHT_DIM
                            : BACKLIGHT_FULL);

  delay(5);
}
