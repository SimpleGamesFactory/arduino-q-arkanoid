#pragma once

#include <stdint.h>

#include "SGF/Sprites.h"

class Ball {
public:
  static constexpr uint32_t DEFAULT_BASE_STEP_US = 10000;
  static constexpr int MAX_R = 16;
  static constexpr int MAX_SPRITE_SIZE = MAX_R * 2 + 1;

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
  uint32_t baseStepUs = DEFAULT_BASE_STEP_US;
  SpriteLayer::Scale spriteScale = SpriteLayer::Scale::Normal;

  void syncFixedFromInt();
  void setVelocity(int newDx, int newDy);
  void attachToPaddle(int paddleX, int paddleW, int paddleY);
  void resetOnPaddle(int paddleX, int paddleW, int paddleY, int launchDx, int launchDy);
  void launch(int launchDx, int launchDy);
  void updateSpeedFromPot(int raw, int32_t minQ, int32_t maxQ);
  void onPhysics(float delta);
  void snapAngles();
  void rebuildSprite();
  void bindSprite(SpriteLayer::Sprite& sprite) const;
  void updateSprite(SpriteLayer::Sprite& sprite) const;

  int speedSlow() const {
    return speedSlow_;
  }
  int speedFast() const {
    return speedFast_;
  }
  int posFpOne() const {
    return posFpOne_;
  }

private:
  const int speedSlow_;
  const int speedFast_;
  const int posFpOne_;
  uint16_t spritePixels[MAX_SPRITE_SIZE * MAX_SPRITE_SIZE]{};

  int spriteSize() const;
};
