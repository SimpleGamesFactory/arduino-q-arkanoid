#include "Paddle.h"
#include "SGF/Color565.h"

Paddle::Paddle() {
  SpriteCharacter::setPosition(0, DEFAULT_Y);
  xf = 0.0f;
  SpriteCharacter::setSize(DEFAULT_W, DEFAULT_H);
  rebuildSprite();
}

void Paddle::setX(int newX) {
  xf = static_cast<float>(newX);
  Position pos = getPosition();
  SpriteCharacter::setPosition(newX, pos.y);
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
  Vector2i paddleSize = getSize();
  setX((screenW - paddleSize.x) / 2);
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
  SpriteCharacter::setPosition(newX, pos.y);
  result.newPosition = getPosition();
  result.moved = (result.newPosition.x != result.oldPosition.x) ||
                 (result.newPosition.y != result.oldPosition.y);
  return result;
}

void Paddle::rebuildSprite() {
  buildSprite565(spritePixels);
}

void Paddle::configureBoundSprite(Renderer2D::SpriteHandle& sprite) {
  Vector2i paddleSize = getSize();
  sprite.setBitmap(spritePixels, paddleSize.x, paddleSize.y, 0);
  sprite.setScale(spriteScale);
  sprite.setAnchor(Vector2f{0.0f, 0.0f});
}

void Paddle::buildSprite565(uint16_t* pixels) const {
  if (!pixels) {
    return;
  }

  const uint16_t face = Color565::rgb(196, 200, 208);
  const uint16_t light = Color565::rgb(232, 236, 244);
  const uint16_t dark = Color565::rgb(130, 136, 146);
  const uint16_t midShadow = Color565::rgb(86, 92, 102);
  Vector2i paddleSize = getSize();

  for (int py = 0; py < paddleSize.y; ++py) {
    for (int px = 0; px < paddleSize.x; ++px) {
      bool corner =
        ((py == 0 || py == paddleSize.y - 1) && (px == 0 || px == paddleSize.x - 1));
      if (corner) {
        pixels[py * paddleSize.x + px] = 0;
        continue;
      }
      if (py == 0 || px == 0) {
        pixels[py * paddleSize.x + px] = light;
      } else if (py == paddleSize.y - 1 || px == paddleSize.x - 1) {
        pixels[py * paddleSize.x + px] = dark;
      } else if (py == paddleSize.y - 2 || px == paddleSize.x - 2) {
        pixels[py * paddleSize.x + px] = midShadow;
      } else {
        pixels[py * paddleSize.x + px] = face;
      }
    }
  }
}

bool Paddle::roundedBodyAt(int px, int py) const {
  Position pos = getPosition();
  Vector2i paddleSize = getSize();
  if (py < pos.y || py >= pos.y + paddleSize.y || px < pos.x || px >= pos.x + paddleSize.x) {
    return false;
  }

  int lx = px - pos.x;
  int ly = py - pos.y;
  if ((ly == 0 || ly == paddleSize.y - 1) && (lx == 0 || lx == paddleSize.x - 1)) {
    return false;
  }
  return true;
}

bool Paddle::shadowAt(int px, int py) const {
  Position pos = getPosition();
  Vector2i paddleSize = getSize();
  const int sx = pos.x + 1;
  const int sy = pos.y + 1;
  if (py < sy || py >= sy + paddleSize.y || px < sx || px >= sx + paddleSize.x) {
    return false;
  }

  int lx = px - sx;
  int ly = py - sy;
  if ((ly == 0 || ly == paddleSize.y - 1) && (lx == 0 || lx == paddleSize.x - 1)) {
    return false;
  }

  if (roundedBodyAt(px, py)) {
    return false;
  }
  return true;
}
