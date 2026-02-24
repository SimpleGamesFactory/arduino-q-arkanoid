#pragma once

#include <stdint.h>

class Paddle {
public:
  int x = 0;
  float xf = 0.0f;

  void resetCentered(int screenW, int paddleW);
  bool updateDt(int dir, int speed, float dtSec, uint32_t baseStepUs, int screenW, int paddleW, int* oldX);
};
