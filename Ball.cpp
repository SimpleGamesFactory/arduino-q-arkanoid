#include "Ball.h"

#include <stdlib.h>

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

void Ball::onPhysics(float delta) {
  float speedPxPerStep = static_cast<float>(speedScaleQ) / static_cast<float>(posFpOne_);
  float stepScale =
    (speedPxPerStep / static_cast<float>(speedFast_)) * (delta * (1000000.0f / static_cast<float>(baseStepUs)));
  fx += static_cast<float>(dx) * stepScale;
  fy += static_cast<float>(dy) * stepScale;
  x = static_cast<int>(fx + 0.5f);
  y = static_cast<int>(fy + 0.5f);
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
