#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint32_t lastUs;
  uint32_t defaultStepUs;
  uint32_t maxStepUs;
} GameFrameClock;

void gamelib_frame_clock_reset(GameFrameClock *clock, uint32_t nowUs);
float gamelib_frame_clock_tick_sec(GameFrameClock *clock, uint32_t nowUs);

uint16_t gamelib_lighten565(uint16_t c);
uint16_t gamelib_darken565(uint16_t c);
bool gamelib_circle_rect_hit(int cx, int cy, int r, int x0, int y0, int x1, int y1);

#ifdef __cplusplus
}
#endif
