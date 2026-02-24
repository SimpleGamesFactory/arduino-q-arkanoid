#include "Paddle.h"
#include "SGF/Color565.h"

Paddle::Paddle() {
  Character::setPosition(0, DEFAULT_Y);
  xf = 0.0f;
  SpriteCharacter::setSize(DEFAULT_W, DEFAULT_H);
  rebuildSprite();
}

void Paddle::setX(int newX) {
  xf = static_cast<float>(newX);
  SpriteCharacter::setX(newX);
}

void Paddle::setPosition(int newX, int newY) {
  xf = static_cast<float>(newX);
  SpriteCharacter::setPosition(newX, newY);
}

void Paddle::setY(int newY) {
  Position pos = getPosition();
  SpriteCharacter::setPosition(pos.x, newY);
}

void Paddle::setSize(int w, int h) {
  SpriteCharacter::setSize(w, h);
  rebuildSprite();
}

void Paddle::resetCentered(int screenW) {
  Vector2 size = getSize();
  setX((screenW - size.x) / 2);
}

void Paddle::setBounds(int newMinX, int newMaxX) {
  minX = newMinX;
  maxX = newMaxX;
  if (maxX < minX) {
    maxX = minX;
  }
  Position pos = getPosition();
  if (pos.x < minX) {
    setPosition(minX, pos.y);
  }
  pos = getPosition();
  if (pos.x > maxX) {
    setPosition(maxX, pos.y);
  }
}

Paddle::MoveResult Paddle::onPhysics(float delta) {
  MoveResult result;
  Position pos = getPosition();
  result.oldPosition = pos;

  xf += velocityX * delta;

  float minXf = static_cast<float>(minX);
  float maxXf = static_cast<float>(maxX);
  if (xf < minXf) {
    xf = minXf;
  }
  if (xf > maxXf) {
    xf = maxXf;
  }

  int newX = static_cast<int>(xf + 0.5f);
  Character::setPosition(newX, positionY());
  result.newPosition = getPosition();
  result.moved = (result.newPosition.x != result.oldPosition.x) ||
                 (result.newPosition.y != result.oldPosition.y);
  return result;
}

void Paddle::rebuildSprite() {
  buildSprite565(spritePixels);
}

void Paddle::configureBoundSprite(SpriteLayer::Sprite& sprite) {
  Vector2 size = getSize();
  sprite.w = size.x;
  sprite.h = size.y;
  sprite.pixels565 = spritePixels;
  sprite.transparent = 0;
  sprite.scale = spriteScale;
  sprite.setAnchor(0.0f, 0.0f);
}

void Paddle::buildSprite565(uint16_t* pixels) const {
  if (!pixels) {
    return;
  }

  const uint16_t face = Color565::rgb(196, 200, 208);
  const uint16_t light = Color565::rgb(232, 236, 244);
  const uint16_t dark = Color565::rgb(130, 136, 146);
  const uint16_t midShadow = Color565::rgb(86, 92, 102);
  Vector2 size = getSize();

  for (int py = 0; py < size.y; ++py) {
    for (int px = 0; px < size.x; ++px) {
      bool corner = ((py == 0 || py == size.y - 1) && (px == 0 || px == size.x - 1));
      if (corner) {
        pixels[py * size.x + px] = 0;
        continue;
      }
      if (py == 0 || px == 0) {
        pixels[py * size.x + px] = light;
      } else if (py == size.y - 1 || px == size.x - 1) {
        pixels[py * size.x + px] = dark;
      } else if (py == size.y - 2 || px == size.x - 2) {
        pixels[py * size.x + px] = midShadow;
      } else {
        pixels[py * size.x + px] = face;
      }
    }
  }
}

bool Paddle::roundedBodyAt(int px, int py) const {
  Position pos = getPosition();
  Vector2 size = getSize();
  if (py < pos.y || py >= pos.y + size.y || px < pos.x || px >= pos.x + size.x) {
    return false;
  }

  int lx = px - pos.x;
  int ly = py - pos.y;
  if ((ly == 0 || ly == size.y - 1) && (lx == 0 || lx == size.x - 1)) {
    return false;
  }
  return true;
}

bool Paddle::shadowAt(int px, int py) const {
  Position pos = getPosition();
  Vector2 size = getSize();
  const int sx = pos.x + 1;
  const int sy = pos.y + 1;
  if (py < sy || py >= sy + size.y || px < sx || px >= sx + size.x) {
    return false;
  }

  int lx = px - sx;
  int ly = py - sy;
  if ((ly == 0 || ly == size.y - 1) && (lx == 0 || lx == size.x - 1)) {
    return false;
  }

  if (roundedBodyAt(px, py)) {
    return false;
  }
  return true;
}
