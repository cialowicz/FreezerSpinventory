#include "ui.h"

#include <lvgl.h>
#include <math.h>
#include <stdio.h>

#include "config.h"

namespace ui {
namespace {

// Palette
const lv_color_t kBg = lv_color_hex(0x0B1016);
const lv_color_t kText = lv_color_hex(0xE8EDF2);
const lv_color_t kHintGray = lv_color_hex(0x8A97A5);
const lv_color_t kDotIdle = lv_color_hex(0x2E3A46);
const lv_color_t kArcTrack = lv_color_hex(0x1C2630);
const lv_color_t kTeal = lv_color_hex(0x2EC4B6);   // browse accent
const lv_color_t kAmber = lv_color_hex(0xFFB347);  // edit accent
const lv_color_t kRed = lv_color_hex(0xE5484D);    // out of stock
const lv_color_t kGreen = lv_color_hex(0x46A758);  // saved toast

inv::InventoryModel* model = nullptr;

lv_obj_t* qtyArc = nullptr;
lv_obj_t* nameLabel = nullptr;
lv_obj_t* qtyLabel = nullptr;
lv_obj_t* hintLabel = nullptr;
lv_obj_t* dots[inv::kItemCount] = {nullptr};

bool toastActive = false;
lv_timer_t* toastTimer = nullptr;

void setHintForMode() {
  if (model->inEditMode()) {
    lv_label_set_text(hintLabel, "turn to adjust  -  press to save");
  } else {
    lv_label_set_text(hintLabel, "press knob to edit");
  }
  lv_obj_set_style_text_color(hintLabel, kHintGray, 0);
}

void toastTimerCb(lv_timer_t*) {
  toastActive = false;
  toastTimer = nullptr;
  setHintForMode();
}

}  // namespace

void init(inv::InventoryModel* m) {
  model = m;

  lv_obj_t* scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, kBg, 0);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  // Quantity ring around the rim, gap at the bottom.
  qtyArc = lv_arc_create(scr);
  lv_obj_set_size(qtyArc, SCREEN_WIDTH - 8, SCREEN_HEIGHT - 8);
  lv_obj_center(qtyArc);
  lv_arc_set_bg_angles(qtyArc, 135, 45);
  lv_arc_set_range(qtyArc, 0, inv::kMaxQuantity);
  lv_obj_remove_style(qtyArc, nullptr, LV_PART_KNOB);
  lv_obj_clear_flag(qtyArc, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_arc_width(qtyArc, 12, LV_PART_MAIN);
  lv_obj_set_style_arc_color(qtyArc, kArcTrack, LV_PART_MAIN);
  lv_obj_set_style_arc_width(qtyArc, 12, LV_PART_INDICATOR);

  // One position dot per item, fanned across the top of the circle.
  const float spreadDeg = 13.0f;
  for (size_t i = 0; i < inv::kItemCount; i++) {
    float angle =
        (-90.0f + ((float)i - (inv::kItemCount - 1) / 2.0f) * spreadDeg) *
        (float)M_PI / 180.0f;
    lv_coord_t dx = (lv_coord_t)lroundf(196.0f * cosf(angle));
    lv_coord_t dy = (lv_coord_t)lroundf(196.0f * sinf(angle));
    dots[i] = lv_obj_create(scr);
    lv_obj_set_size(dots[i], 12, 12);
    lv_obj_set_style_radius(dots[i], LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(dots[i], 0, 0);
    lv_obj_set_style_bg_color(dots[i], kDotIdle, 0);
    lv_obj_clear_flag(dots[i], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(dots[i], LV_ALIGN_CENTER, dx, dy);
  }

  nameLabel = lv_label_create(scr);
  lv_obj_set_style_text_font(nameLabel, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(nameLabel, kText, 0);
  lv_label_set_long_mode(nameLabel, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(nameLabel, 320);
  lv_obj_set_style_text_align(nameLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(nameLabel, LV_ALIGN_CENTER, 0, -70);

  qtyLabel = lv_label_create(scr);
  lv_obj_set_style_text_font(qtyLabel, &lv_font_montserrat_48, 0);
  lv_obj_align(qtyLabel, LV_ALIGN_CENTER, 0, 30);

  hintLabel = lv_label_create(scr);
  lv_obj_set_style_text_font(hintLabel, &lv_font_montserrat_20, 0);
  lv_obj_align(hintLabel, LV_ALIGN_CENTER, 0, 130);

  refresh();
}

void refresh() {
  if (model == nullptr) {
    return;
  }
  bool editing = model->inEditMode();
  uint8_t qty =
      editing ? model->pendingQuantity() : model->selectedItem().quantity;
  lv_color_t accent = editing ? kAmber : kTeal;

  lv_label_set_text(nameLabel, model->selectedItem().name);

  char buf[8];
  snprintf(buf, sizeof(buf), "%u", (unsigned)qty);
  lv_label_set_text(qtyLabel, buf);
  lv_obj_set_style_text_color(qtyLabel,
                              (qty == 0 && !editing) ? kRed : accent, 0);
  // Re-center after text width changes.
  lv_obj_align(qtyLabel, LV_ALIGN_CENTER, 0, 30);

  lv_arc_set_value(qtyArc, qty);
  lv_obj_set_style_arc_color(qtyArc, accent, LV_PART_INDICATOR);

  for (size_t i = 0; i < inv::kItemCount; i++) {
    bool selected = (i == model->selectedIndex());
    lv_obj_set_style_bg_color(dots[i], selected ? accent : kDotIdle, 0);
    lv_obj_set_size(dots[i], selected ? 18 : 12, selected ? 18 : 12);
  }

  if (!toastActive) {
    setHintForMode();
  }
}

void showSavedToast() {
  toastActive = true;
  lv_label_set_text(hintLabel, "Saved");
  lv_obj_set_style_text_color(hintLabel, kGreen, 0);
  if (toastTimer != nullptr) {
    lv_timer_del(toastTimer);
  }
  toastTimer = lv_timer_create(toastTimerCb, 1200, nullptr);
  lv_timer_set_repeat_count(toastTimer, 1);
}

void showSavingToast() {
  toastActive = true;
  lv_label_set_text(hintLabel, "Saving...");
  lv_obj_set_style_text_color(hintLabel, kHintGray, 0);
  if (toastTimer != nullptr) {
    lv_timer_del(toastTimer);
    toastTimer = nullptr;
  }
}

void showSaveFailedToast() {
  toastActive = true;
  lv_label_set_text(hintLabel, "Save failed - retrying");
  lv_obj_set_style_text_color(hintLabel, kRed, 0);
  if (toastTimer != nullptr) {
    lv_timer_del(toastTimer);
    toastTimer = nullptr;
  }
}

}  // namespace ui
