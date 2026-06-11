#include "display.h"

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>

#include "config.h"

namespace display {
namespace {

constexpr int kBacklightChannel = 0;
constexpr int kDrawBufferLines = 60;

Arduino_ESP32RGBPanel* bus = nullptr;
Arduino_ST7701_RGBPanel* gfx = nullptr;
lv_disp_draw_buf_t drawBuf;
lv_color_t* buf1 = nullptr;

void flushCallback(lv_disp_drv_t* drv, const lv_area_t* area,
                   lv_color_t* pixels) {
  uint32_t w = area->x2 - area->x1 + 1;
  uint32_t h = area->y2 - area->y1 + 1;
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t*)pixels, w, h);
  lv_disp_flush_ready(drv);
}

}  // namespace

bool init() {
  bus = new Arduino_ESP32RGBPanel(
      PIN_LCD_CS, PIN_LCD_SCK, PIN_LCD_SDA,
      PIN_LCD_DE, PIN_LCD_VSYNC, PIN_LCD_HSYNC, PIN_LCD_PCLK,
      PIN_LCD_R0, PIN_LCD_R1, PIN_LCD_R2, PIN_LCD_R3, PIN_LCD_R4,
      PIN_LCD_G0, PIN_LCD_G1, PIN_LCD_G2, PIN_LCD_G3, PIN_LCD_G4, PIN_LCD_G5,
      PIN_LCD_B0, PIN_LCD_B1, PIN_LCD_B2, PIN_LCD_B3, PIN_LCD_B4);

  // Timing values from Elecrow's reference code for this exact panel.
  gfx = new Arduino_ST7701_RGBPanel(
      bus, GFX_NOT_DEFINED /* RST handled by PCF8574 */, 0 /* rotation */,
      false /* IPS */, SCREEN_WIDTH, SCREEN_HEIGHT,
      st7701_type5_init_operations, sizeof(st7701_type5_init_operations),
      true /* BGR */,
      10 /* hsync FP */, 4 /* hsync PW */, 20 /* hsync BP */,
      10 /* vsync FP */, 4 /* vsync PW */, 20 /* vsync BP */);

  gfx->begin();
  gfx->fillScreen(BLACK);

  ledcSetup(kBacklightChannel, 5000, 8);
  ledcAttachPin(PIN_BACKLIGHT, kBacklightChannel);
  setBacklight(BACKLIGHT_FULL);

  lv_init();

  size_t bufPixels = SCREEN_WIDTH * kDrawBufferLines;
  buf1 = (lv_color_t*)heap_caps_malloc(bufPixels * sizeof(lv_color_t),
                                       MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (buf1 == nullptr) {
    buf1 = (lv_color_t*)ps_malloc(bufPixels * sizeof(lv_color_t));
  }
  if (buf1 == nullptr) {
    return false;
  }
  lv_disp_draw_buf_init(&drawBuf, buf1, nullptr, bufPixels);

  static lv_disp_drv_t dispDrv;
  lv_disp_drv_init(&dispDrv);
  dispDrv.hor_res = SCREEN_WIDTH;
  dispDrv.ver_res = SCREEN_HEIGHT;
  dispDrv.flush_cb = flushCallback;
  dispDrv.draw_buf = &drawBuf;
  lv_disp_drv_register(&dispDrv);

  return true;
}

void setBacklight(uint8_t level) {
  ledcWrite(kBacklightChannel, level);
}

}  // namespace display
