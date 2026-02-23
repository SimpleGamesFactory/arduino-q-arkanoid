#include "gamelib.h"

static inline int clamp_i(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

void gamelib_frame_clock_reset(GameFrameClock *clock, uint32_t nowUs) {
  if (!clock) return;
  clock->lastUs = nowUs;
}

float gamelib_frame_clock_tick_sec(GameFrameClock *clock, uint32_t nowUs) {
  if (!clock) return 0.0f;
  if (clock->lastUs == 0) {
    clock->lastUs = nowUs;
    return (float)clock->defaultStepUs / 1000000.0f;
  }

  uint32_t dtUs = nowUs - clock->lastUs;  // wrap-safe for uint32_t
  clock->lastUs = nowUs;

  if (clock->maxStepUs != 0 && dtUs > clock->maxStepUs) dtUs = clock->maxStepUs;
  return (float)dtUs / 1000000.0f;
}

uint16_t gamelib_lighten565(uint16_t c) {
  int r = (c >> 11) & 0x1F;
  int g = (c >> 5) & 0x3F;
  int b = c & 0x1F;
  r = clamp_i(r + ((r / 3) > 0 ? (r / 3) : 1), 0, 31);
  g = clamp_i(g + ((g / 3) > 0 ? (g / 3) : 1), 0, 63);
  b = clamp_i(b + ((b / 3) > 0 ? (b / 3) : 1), 0, 31);
  return (uint16_t)((r << 11) | (g << 5) | b);
}

uint16_t gamelib_darken565(uint16_t c) {
  int r = (c >> 11) & 0x1F;
  int g = (c >> 5) & 0x3F;
  int b = c & 0x1F;
  r = (r * 2) / 3;
  g = (g * 2) / 3;
  b = (b * 2) / 3;
  return (uint16_t)((r << 11) | (g << 5) | b);
}

bool gamelib_circle_rect_hit(int cx, int cy, int r, int x0, int y0, int x1, int y1) {
  int nx = cx;
  if (nx < x0) nx = x0;
  else if (nx > x1) nx = x1;

  int ny = cy;
  if (ny < y0) ny = y0;
  else if (ny > y1) ny = y1;

  int dx = cx - nx;
  int dy = cy - ny;
  return dx * dx + dy * dy <= r * r;
}
