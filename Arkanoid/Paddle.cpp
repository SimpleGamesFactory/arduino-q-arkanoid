#include "Paddle.h"

void Paddle::resetCentered(int screenW, int paddleW) {
  x = (screenW - paddleW) / 2;
  xf = static_cast<float>(x);
}

bool Paddle::updateDt(int dir, int speed, float dtSec, uint32_t baseStepUs, int screenW, int paddleW, int* oldX) {
  int prev = x;
  if (oldX) *oldX = prev;

  float dtSteps = dtSec * (1000000.0f / static_cast<float>(baseStepUs));
  xf += static_cast<float>(dir) * static_cast<float>(speed) * dtSteps;

  float minX = 0.0f;
  float maxX = static_cast<float>(screenW - paddleW);
  if (xf < minX) xf = minX;
  if (xf > maxX) xf = maxX;

  x = static_cast<int>(xf + 0.5f);
  return x != prev;
}
