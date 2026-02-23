#include <Arduino.h>

#include "gamelib.h"
#include "gamelib_runtime.h"

static GameFrameClock s_runtimeClock = {0, 10000u, 30000u};

// Wymagane callbacki gry (linker wymusi implementację po stronie .ino).
extern void game_setup(void);
extern void game_physics(float dtSec);
extern void game_process(float dtSec);

void gamelib_runtime_setup(uint32_t defaultStepUs, uint32_t maxStepUs) {
  s_runtimeClock.lastUs = 0;
  s_runtimeClock.defaultStepUs = defaultStepUs;
  s_runtimeClock.maxStepUs = maxStepUs;

  game_setup();

  // Zerujemy zegar po game_setup(), bo init TFT/render może trwać długo.
  gamelib_frame_clock_reset(&s_runtimeClock, micros());
}

void gamelib_runtime_loop(void) {
  float dtSec = gamelib_frame_clock_tick_sec(&s_runtimeClock, micros());
  game_physics(dtSec);
  game_process(dtSec);
}

void gamelib_runtime_reset_clock(void) {
  gamelib_frame_clock_reset(&s_runtimeClock, micros());
}
