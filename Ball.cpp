#include "Ball.h"

void ball_sync_fixed_from_int(BallState *b) {
  if (!b) return;
  b->fx = (float)b->x;
  b->fy = (float)b->y;
}

void ball_set_velocity(BallState *b, int dx, int dy) {
  if (!b) return;
  b->dx = dx;
  b->dy = dy;
}

void ball_attach_to_paddle(BallState *b, int paddleX, int paddleW, int paddleY) {
  if (!b) return;
  b->x = paddleX + paddleW / 2;
  b->y = paddleY - b->r - 1;
  ball_sync_fixed_from_int(b);
}

void ball_reset_on_paddle(BallState *b, int paddleX, int paddleW, int paddleY, int launchDx, int launchDy) {
  if (!b) return;
  b->attached = true;
  ball_set_velocity(b, launchDx, launchDy);
  ball_attach_to_paddle(b, paddleX, paddleW, paddleY);
}

void ball_launch(BallState *b, int launchDx, int launchDy) {
  if (!b) return;
  b->attached = false;
  ball_set_velocity(b, launchDx, launchDy);
}

void ball_update_speed_from_pot(BallState *b, int raw, int32_t minQ, int32_t maxQ) {
  if (!b) return;

  int32_t rawQ = (int32_t)raw << 4;
  if (b->speedPotFiltQ < 0) {
    b->speedPotFiltQ = rawQ;
  } else {
    b->speedPotFiltQ += (rawQ - b->speedPotFiltQ) >> 3;
  }

  int32_t rawSmooth = b->speedPotFiltQ >> 4;
  int32_t rangeQ = maxQ - minQ;
  b->speedScaleQ = minQ + (rangeQ * rawSmooth) / 1023;
}

void ball_step_scaled(BallState *b, float dtSec, uint32_t baseStepUs, int posFpOne, int speedFast) {
  if (!b) return;

  float speedPxPerStep = (float)b->speedScaleQ / (float)posFpOne;
  float stepScale = (speedPxPerStep / (float)speedFast) * (dtSec * (1000000.0f / (float)baseStepUs));
  b->fx += (float)b->dx * stepScale;
  b->fy += (float)b->dy * stepScale;
  b->x = (int)(b->fx + 0.5f);
  b->y = (int)(b->fy + 0.5f);
}
