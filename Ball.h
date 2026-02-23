#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int x;
  int y;
  int dx;
  int dy;
  int r;
  bool attached;
  float fx;
  float fy;
  int32_t speedScaleQ;
  int32_t speedPotFiltQ;
} BallState;

void ball_sync_fixed_from_int(BallState *b);
void ball_set_velocity(BallState *b, int dx, int dy);
void ball_attach_to_paddle(BallState *b, int paddleX, int paddleW, int paddleY);
void ball_reset_on_paddle(BallState *b, int paddleX, int paddleW, int paddleY, int launchDx, int launchDy);
void ball_launch(BallState *b, int launchDx, int launchDy);
void ball_update_speed_from_pot(BallState *b, int raw, int32_t minQ, int32_t maxQ);
void ball_step_scaled(BallState *b, float dtSec, uint32_t baseStepUs, int posFpOne, int speedFast);

#ifdef __cplusplus
}
#endif
