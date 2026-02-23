#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int x;
  float xf;
} PaddleState;

void paddle_reset_centered(PaddleState *p, int screenW, int paddleW);
bool paddle_update_dt(PaddleState *p, int dir, int speed, float dtSec, uint32_t baseStepUs,
                      int screenW, int paddleW, int *oldX);

#ifdef __cplusplus
}
#endif
