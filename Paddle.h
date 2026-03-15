#pragma once

#include <stdint.h>

#include "SGF/SpriteCharacter.h"

class Paddle : public SpriteCharacter {
public:
  using Position = Vector2i;

  struct MoveResult {
    Vector2i oldPosition{};
    Vector2i newPosition{};
    bool moved = false;
  };

  static constexpr int DEFAULT_W = 60;
  static constexpr int DEFAULT_H = 8;
  static constexpr int DEFAULT_Y = 220;
  static constexpr float DEFAULT_SPEED_PX_PER_SEC = 500.0f;

  float xf = 0.0f;
  int minX = 0;
  int maxX = 320 - DEFAULT_W;
  float velocityX = 0.0f;
  float speedPxPerSec = DEFAULT_SPEED_PX_PER_SEC;
  SpriteScale spriteScale = SpriteScale::Normal;

  Paddle();

  void setX(int newX);
  void setY(int newY);
  void setPosition(int newX, int newY);
  void setSize(int w, int h);
  void resetCentered(int screenW);
  void setBounds(int newMinX, int newMaxX);
  MoveResult onPhysics(float delta);
  void rebuildSprite();
  bool roundedBodyAt(int px, int py) const;
  bool shadowAt(int px, int py) const;

private:
  uint16_t spritePixels[DEFAULT_W * DEFAULT_H]{};

  void buildSprite565(uint16_t* pixels) const;
  void configureBoundSprite(Renderer2D::SpriteHandle& sprite) override;
};
