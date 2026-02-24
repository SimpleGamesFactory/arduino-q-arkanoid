#pragma once

#include <stdint.h>

class Ball {
public:
  Ball(int radius, int speedSlow, int speedFast, int posFpOne);

  int x = 0;
  int y = 0;
  int dx = 0;
  int dy = 0;
  int r = 0;
  bool attached = true;
  float fx = 0.0f;
  float fy = 0.0f;
  int32_t speedScaleQ = 0;
  int32_t speedPotFiltQ = -1;

  void syncFixedFromInt();
  void setVelocity(int newDx, int newDy);
  void attachToPaddle(int paddleX, int paddleW, int paddleY);
  void resetOnPaddle(int paddleX, int paddleW, int paddleY, int launchDx, int launchDy);
  void launch(int launchDx, int launchDy);
  void updateSpeedFromPot(int raw, int32_t minQ, int32_t maxQ);
  void stepScaled(float dtSec, uint32_t baseStepUs);

  int speedSlow() const { return speedSlow_; }
  int speedFast() const { return speedFast_; }
  int posFpOne() const { return posFpOne_; }

private:
  const int speedSlow_;
  const int speedFast_;
  const int posFpOne_;
};
