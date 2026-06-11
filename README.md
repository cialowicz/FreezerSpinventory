# FreezerSpinventory

A chest freezer inventory display powered and managed by a round ELECROW
ESP32 2.1" rotary display with control dial. A standalone, knob-driven
rewrite of [FridgePinventory](https://github.com/cialowicz/FridgePinventory)
— no Raspberry Pi, no voice control, no microphone. Just twist and click.

## Hardware

[ELECROW CrowPanel 2.1" HMI ESP32 Rotary Display](https://www.elecrow.com/crowpanel-2-1inch-hmi-esp32-rotary-display-480-480-ips-round-touch-knob-screen.html)

- ESP32-S3 (16MB flash, 8MB PSRAM)
- 2.1" round 480x480 IPS panel (ST7701S, parallel RGB)
- Rotary knob with push button (button via PCF8574 I/O expander)
- Capacitive touch (CST816 — unused; the knob is the whole UX)

No wiring required — everything is on the board. Power/flash over USB-C.

## UX

- **Turn** the knob to spin through the item list (wraps around).
- **Press** to edit the selected item; the accent ring turns amber.
- **Turn** to raise/lower the count (0–50), **press** to save.
- Walk away mid-edit and the pending change is discarded after 15s.
- Counts persist to flash (NVS), debounced to limit wear. The UI confirms only
  after a verified write and retries transient failures.
- Backlight dims after a minute idle; the first input wakes it without changing
  inventory state.
- An item at 0 shows its count in red.

Items and their stable persistence IDs are defined once in
`lib/inventory/inventory_model.h`:
Chicken Breasts, Chicken Tenderloins, Chicken Wings, Chicken Nuggets,
Steaks, Salmon, White Fish, Ice Cream.

## Building

Uses [PlatformIO](https://platformio.org/). Platform, framework libraries, and
the Arduino_GFX revision are pinned for reproducible builds. The
`espressif32@6.5.0` platform provides arduino-esp32 2.0.14, the core version
Elecrow's ST7701 demo code targets.

```sh
pio run -e crowpanel -t upload   # build + flash over USB-C
pio device monitor               # serial log at 115200
pio test -e native               # run model unit tests on your machine
```

If clockwise rotation moves the wrong way on your unit, flip
`ENCODER_REVERSED` in `src/config.h`.

## Layout

```
lib/inventory/    Pure inventory model + persistence format
lib/app/          Pure application timing and interaction controller
lib/input/        Pure button debounce and encoder decoding helpers
test/             Host-side unit tests (Unity)
src/              Firmware: display/LVGL, encoder, PCF8574, NVS, UI, main
include/lv_conf.h LVGL 8.3 configuration
```

`InventoryModel`, the persistence codec, input decoding, and the application
timing controller are unit-tested on the host. The firmware layer handles
hardware I/O, verified NVS writes, and mirroring state to the LVGL UI.
