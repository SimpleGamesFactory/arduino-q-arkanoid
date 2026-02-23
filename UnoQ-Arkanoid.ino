#include "FastILI9341.h"
#include "DirtyRects.h"
#include "gamelib_runtime.h"
#include "Arkanoid.h"

#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8
#define TFT_LED D6

#define PIN_LEFT 2
#define PIN_RIGHT 3
#define PIN_FIRE D4

// MADCTL: dobierz pod orientację
// typowe zestawy: 0xE8, 0x48, 0x28, 0x88
static constexpr uint8_t MADCTL = 0xE8;

FastILI9341 gfx(TFT_CS, TFT_DC, TFT_RST, TFT_LED);
DirtyRects dirty;

void game_setup() {
  pinMode(PIN_LEFT, INPUT_PULLUP);
  pinMode(PIN_RIGHT, INPUT_PULLUP);
  pinMode(PIN_FIRE, INPUT_PULLUP);

  bool ok = gfx.begin(24000000, MADCTL);
  if (!ok) {
    while (1) delay(1000);
  }

  arkanoid_setup(PIN_LEFT, PIN_RIGHT, PIN_FIRE);
}

void game_physics(float dtSec) {
  arkanoid_physics(dtSec);
}

void game_process(float dtSec) {
  arkanoid_process(dtSec);
}

void setup() {
  gamelib_runtime_setup(ARKANOID_FRAME_DEFAULT_STEP_US, ARKANOID_FRAME_MAX_STEP_US);
}

void loop() {
  gamelib_runtime_loop();
}
