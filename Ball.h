#pragma once

#include <stdint.h>

#include "SGF/SpriteCharacter.h"

class Paddle;

class Ball : public SpriteCharacter {
public:
  using Position = Vector2;

  static constexpr uint32_t DEFAULT_BASE_STEP_US = 10000;
  static constexpr int DEFAULT_RADIUS = 6;
  static constexpr int DEFAULT_SPEED_SLOW = 1;
  static constexpr int DEFAULT_SPEED_FAST = 3;
  static constexpr int DEFAULT_POS_FP_SHIFT = 8;
  static constexpr int DEFAULT_POS_FP_ONE = 1 << DEFAULT_POS_FP_SHIFT;
  static constexpr int MAX_R = 16;
  static constexpr int MAX_SPRITE_SIZE = MAX_R * 2 + 1;
  static constexpr int PADDLE_BOUNCE_ZONES = 7;

  Ball() : Ball(DEFAULT_RADIUS, DEFAULT_SPEED_SLOW, DEFAULT_SPEED_FAST, DEFAULT_POS_FP_ONE) {}
  Ball(int radius, int speedSlow, int speedFast, int posFpOne);

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
  void resetSpeedControl();
  void attachToPaddle(const Paddle& paddle);
  void resetOnPaddle(const Paddle& paddle);
  void launch();
  void updateSpeedFromPot(int raw, int32_t minQ, int32_t maxQ);
  void onPhysics(float delta);
  void bounceFromPaddleHit(int hitX, int paddleW);
  void snapAngles();
  void rebuildSprite();

  int speedSlow() const {
    return speedSlow_;
  }
  int speedFast() const {
    return speedFast_;
  }
  int posFpOne() const {
    return posFpOne_;
  }
  int32_t defaultSpeedScaleQ() const {
    return static_cast<int32_t>(speedFast_) * static_cast<int32_t>(posFpOne_);
  }
  int32_t speedPotMinQ() const {
    return posFpOne_ / 2;
  }
  int32_t speedPotMaxQ() const {
    return posFpOne_ * 5;
  }

private:
  const int speedSlow_;
  const int speedFast_;
  const int posFpOne_;
  uint16_t spritePixels[MAX_SPRITE_SIZE * MAX_SPRITE_SIZE]{};

  int defaultLaunchDx() const;
  int defaultLaunchDy() const;
  int spriteSize() const;
  void configureBoundSprite(SpriteLayer::Sprite& sprite) override;
};
