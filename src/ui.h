// LVGL UI for the round display: item carousel + quantity editor.
#pragma once

#include "inventory_model.h"

namespace ui {

void init(inv::InventoryModel* model);

// Syncs all widgets to the current model state. Call after any model change.
void refresh();

// Brief "Saved" confirmation in the hint area.
void showSavedToast();

}  // namespace ui
