/* LVGL 8.3 configuration for FreezerSpinventory.
 * Only overrides are listed; everything else takes the LVGL default
 * via lv_conf_internal.h. */
#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0

/* Static LVGL working memory (widgets, styles). */
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (64U * 1024U)

/* Drive lv_tick from millis() so no explicit lv_tick_inc() is needed. */
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE "Arduino.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())

#define LV_DISP_DEF_REFR_PERIOD 30
#define LV_INDEV_DEF_READ_PERIOD 30

/* Fonts: 14 default/hints, 28 item names, 48 quantity. */
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_48 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

#endif /* LV_CONF_H */
