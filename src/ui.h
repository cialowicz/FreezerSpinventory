// LVGL UI for the round display: item carousel + quantity editor.
#pragma once

#include "inventory_model.h"

namespace ui {

void init(inv::InventoryModel* model);

// Syncs all widgets to the current model state. Call after any model change.
void refresh();

// Brief persistence status in the hint area.
void showSavingToast();
void showSavedToast();
void showSaveFailedToast();
void showEditCancelledToast();

// Persistent boot-time warning that stored inventory could not be restored
// and the displayed zero counts are not to be trusted.
void showLoadFailedNotice();

// Call on any user input. Dismisses an active toast/notice so the hint area
// reflects the interaction the user is actually having.
void noteUserActivity();

}  // namespace ui
