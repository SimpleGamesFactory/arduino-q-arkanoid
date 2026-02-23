#include "Paddle.h"

void paddle_reset_centered(PaddleState *p, int screenW, int paddleW) {
  if (!p) return;
  p->x = (screenW - paddleW) / 2;
  p->xf = (float)p->x;
}

bool paddle_update_dt(PaddleState *p, int dir, int speed, float dtSec, uint32_t baseStepUs,
                      int screenW, int paddleW, int *oldX) {
  if (!p) return false;

  int prev = p->x;
  if (oldX) *oldX = prev;

  float dtSteps = dtSec * (1000000.0f / (float)baseStepUs);
  p->xf += (float)dir * (float)speed * dtSteps;

  float minX = 0.0f;
  float maxX = (float)(screenW - paddleW);
  if (p->xf < minX) p->xf = minX;
  if (p->xf > maxX) p->xf = maxX;

  p->x = (int)(p->xf + 0.5f);
  return p->x != prev;
}
