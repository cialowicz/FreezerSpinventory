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

// Persistent boot-time warning that stored inventory could not be restored
// and the displayed zero counts are not to be trusted.
void showLoadFailedNotice();

// Idle at-a-glance view listing every item and count.
void showOverview();
// Returns to the single-item carousel (no-op when already shown).
void showCarousel();
// Hit-test: the overview item index at screen-y, or -1 if outside the list.
int overviewItemAt(int16_t y);

// Call on any user input. Dismisses an active toast/notice so the hint area
// reflects the interaction the user is actually having.
void noteUserActivity();

}  // namespace ui
