#include "Ball.h"

#include <stdlib.h>

#include "Paddle.h"
#include "SGF/Color565.h"

Ball::Ball(int radius, int speedSlow, int speedFast, int posFpOne)
  : r(radius), speedSlow_(speedSlow), speedFast_(speedFast), posFpOne_(posFpOne) {
  rebuildSprite();
}

void Ball::syncFixedFromInt() {
  fx = static_cast<float>(x);
  fy = static_cast<float>(y);
}

void Ball::setVelocity(int newDx, int newDy) {
  dx = newDx;
  dy = newDy;
}

void Ball::resetSpeedControl() {
  speedScaleQ = defaultSpeedScaleQ();
  speedPotFiltQ = -1;
}

void Ball::attachToPaddle(const Paddle& paddle) {
  x = paddle.x + paddle.w / 2;
  y = paddle.y - r - 1;
  syncFixedFromInt();
}

void Ball::resetOnPaddle(const Paddle& paddle) {
  attached = true;
  setVelocity(defaultLaunchDx(), defaultLaunchDy());
  attachToPaddle(paddle);
}

void Ball::launch() {
  attached = false;
  setVelocity(defaultLaunchDx(), defaultLaunchDy());
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

void Ball::onPhysics(float delta) {
  float speedPxPerStep = static_cast<float>(speedScaleQ) / static_cast<float>(posFpOne_);
  float stepScale =
    (speedPxPerStep / static_cast<float>(speedFast_)) * (delta * (1000000.0f / static_cast<float>(baseStepUs)));
  fx += static_cast<float>(dx) * stepScale;
  fy += static_cast<float>(dy) * stepScale;
  x = static_cast<int>(fx + 0.5f);
  y = static_cast<int>(fy + 0.5f);
}

void Ball::bounceFromPaddleHit(int hitX, int paddleW) {
  if (paddleW <= 0) {
    return;
  }

  int clampedHit = hitX;
  if (clampedHit < 0) {
    clampedHit = 0;
  } else if (clampedHit >= paddleW) {
    clampedHit = paddleW - 1;
  }

  int zone = (clampedHit * PADDLE_BOUNCE_ZONES) / paddleW;
  if (zone < 0) {
    zone = 0;
  } else if (zone >= PADDLE_BOUNCE_ZONES) {
    zone = PADDLE_BOUNCE_ZONES - 1;
  }

  switch (zone) {
    case 0:
      setVelocity(-speedFast_, -speedSlow_);
      break;
    case 1:
      setVelocity(-speedSlow_, -speedFast_);
      break;
    case 2:
      setVelocity(-1, -speedFast_);
      break;
    case 3:
      setVelocity(0, -speedFast_);
      break;
    case 4:
      setVelocity(1, -speedFast_);
      break;
    case 5:
      setVelocity(speedSlow_, -speedFast_);
      break;
    default:
      setVelocity(speedFast_, -speedSlow_);
      break;
  }
}

void Ball::snapAngles() {
  int sx = (dx >= 0) ? 1 : -1;
  int sy = (dy >= 0) ? 1 : -1;
  int ax = abs(dx);
  int ay = abs(dy);
  if (ax == ay) {
    ax = speedSlow_;
    ay = speedFast_;
  } else if (ax > ay) {
    ax = speedFast_;
    ay = speedSlow_;
  } else {
    ax = speedSlow_;
    ay = speedFast_;
  }
  dx = sx * ax;
  dy = sy * ay;
}

int Ball::spriteSize() const {
  return r * 2 + 1;
}

int Ball::defaultLaunchDx() const {
  return speedSlow_;
}

int Ball::defaultLaunchDy() const {
  return -speedFast_;
}

void Ball::rebuildSprite() {
  const uint16_t ballColor = Color565::rgb(255, 255, 255);
  int size = spriteSize();
  for (int py = 0; py < size; ++py) {
    for (int px = 0; px < size; ++px) {
      int ddx = px - r;
      int ddy = py - r;
      bool inside = (ddx * ddx + ddy * ddy) <= (r * r);
      spritePixels[py * size + px] = inside ? ballColor : 0;
    }
  }
}

void Ball::bindSprite(SpriteLayer::Sprite& sprite) const {
  int size = spriteSize();
  sprite.w = size;
  sprite.h = size;
  sprite.pixels565 = spritePixels;
  sprite.transparent = 0;
  sprite.scale = spriteScale;
  sprite.setAnchor(0.5f, 0.5f);
  updateSprite(sprite);
}

void Ball::updateSprite(SpriteLayer::Sprite& sprite) const {
  sprite.active = true;
  sprite.setPosition(x, y);
}
