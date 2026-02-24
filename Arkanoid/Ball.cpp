#include "Ball.h"

Ball::Ball(int radius, int speedSlow, int speedFast, int posFpOne)
  : r(radius), speedSlow_(speedSlow), speedFast_(speedFast), posFpOne_(posFpOne) {}

void Ball::syncFixedFromInt() {
  fx = static_cast<float>(x);
  fy = static_cast<float>(y);
}

void Ball::setVelocity(int newDx, int newDy) {
  dx = newDx;
  dy = newDy;
}

void Ball::attachToPaddle(int paddleX, int paddleW, int paddleY) {
  x = paddleX + paddleW / 2;
  y = paddleY - r - 1;
  syncFixedFromInt();
}

void Ball::resetOnPaddle(int paddleX, int paddleW, int paddleY, int launchDx, int launchDy) {
  attached = true;
  setVelocity(launchDx, launchDy);
  attachToPaddle(paddleX, paddleW, paddleY);
}

void Ball::launch(int launchDx, int launchDy) {
  attached = false;
  setVelocity(launchDx, launchDy);
}

void Ball::updateSpeedFromPot(int raw, int32_t minQ, int32_t maxQ) {
  int32_t rawQ = static_cast<int32_t>(raw) << 4;
  if (speedPotFiltQ < 0) {
    speedPotFiltQ = rawQ;
  } else {
    speedPotFiltQ += (rawQ - speedPotFiltQ) >> 3;
  }

  int32_t rawSmooth = speedPotFiltQ >> 4;
  int32_t rangeQ = maxQ - minQ;
  speedScaleQ = minQ + (rangeQ * rawSmooth) / 1023;
}

void Ball::stepScaled(float dtSec, uint32_t baseStepUs) {
  float speedPxPerStep = static_cast<float>(speedScaleQ) / static_cast<float>(posFpOne_);
  float stepScale = (speedPxPerStep / static_cast<float>(speedFast_)) * (dtSec * (1000000.0f / static_cast<float>(baseStepUs)));
  fx += static_cast<float>(dx) * stepScale;
  fy += static_cast<float>(dy) * stepScale;
  x = static_cast<int>(fx + 0.5f);
  y = static_cast<int>(fy + 0.5f);
}
