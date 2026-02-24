#include "Paddle.h"
#include "SGF/Color565.h"

Paddle::Paddle() {
  rebuildSprite();
}

void Paddle::resetCentered(int screenW) {
  x = (screenW - w) / 2;
  xf = static_cast<float>(x);
}

void Paddle::setBounds(int newMinX, int newMaxX) {
  minX = newMinX;
  maxX = newMaxX;
  if (maxX < minX) {
    maxX = minX;
  }
  if (x < minX) {
    x = minX;
  }
  if (x > maxX) {
    x = maxX;
  }
  xf = static_cast<float>(x);
}

Paddle::MoveResult Paddle::onPhysics(float delta) {
  MoveResult result;
  result.oldX = x;

  xf += velocityX * delta;

  float minXf = static_cast<float>(minX);
  float maxXf = static_cast<float>(maxX);
  if (xf < minXf) {
    xf = minXf;
  }
  if (xf > maxXf) {
    xf = maxXf;
  }

  x = static_cast<int>(xf + 0.5f);
  result.newX = x;
  result.moved = (x != result.oldX);
  return result;
}

void Paddle::rebuildSprite() {
  buildSprite565(spritePixels);
}

void Paddle::bindSprite(SpriteLayer::Sprite& sprite) const {
  sprite.w = w;
  sprite.h = h;
  sprite.pixels565 = spritePixels;
  sprite.transparent = 0;
  sprite.scale = spriteScale;
  sprite.setAnchor(0.0f, 0.0f);
  updateSprite(sprite);
}

void Paddle::updateSprite(SpriteLayer::Sprite& sprite) const {
  sprite.active = true;
  sprite.setPosition(x, y);
}

void Paddle::buildSprite565(uint16_t* pixels) const {
  if (!pixels) {
    return;
  }

  const uint16_t face = Color565::rgb(196, 200, 208);
  const uint16_t light = Color565::rgb(232, 236, 244);
  const uint16_t dark = Color565::rgb(130, 136, 146);
  const uint16_t midShadow = Color565::rgb(86, 92, 102);

  for (int py = 0; py < h; ++py) {
    for (int px = 0; px < w; ++px) {
      bool corner = ((py == 0 || py == h - 1) && (px == 0 || px == w - 1));
      if (corner) {
        pixels[py * w + px] = 0;
        continue;
      }
      if (py == 0 || px == 0) {
        pixels[py * w + px] = light;
      } else if (py == h - 1 || px == w - 1) {
        pixels[py * w + px] = dark;
      } else if (py == h - 2 || px == w - 2) {
        pixels[py * w + px] = midShadow;
      } else {
        pixels[py * w + px] = face;
      }
    }
  }
}

bool Paddle::roundedBodyAt(int px, int py) const {
  if (py < y || py >= y + h || px < x || px >= x + w) {
    return false;
  }

  int lx = px - x;
  int ly = py - y;
  if ((ly == 0 || ly == h - 1) && (lx == 0 || lx == w - 1)) {
    return false;
  }
  return true;
}

bool Paddle::shadowAt(int px, int py) const {
  const int sx = x + 1;
  const int sy = y + 1;
  if (py < sy || py >= sy + h || px < sx || px >= sx + w) {
    return false;
  }

  int lx = px - sx;
  int ly = py - sy;
  if ((ly == 0 || ly == h - 1) && (lx == 0 || lx == w - 1)) {
    return false;
  }

  if (roundedBodyAt(px, py)) {
    return false;
  }
  return true;
}
