#pragma once

#include <stdint.h>

#include "SGF/Sprites.h"

class Paddle {
public:
  struct Position {
    int x = 0;
    int y = 0;
  };

  struct MoveResult {
    int oldX = 0;
    int newX = 0;
    bool moved = false;
  };

  static constexpr int DEFAULT_W = 60;
  static constexpr int DEFAULT_H = 8;
  static constexpr int DEFAULT_Y = 220;
  static constexpr float DEFAULT_SPEED_PX_PER_SEC = 500.0f;

  int w = DEFAULT_W;
  int h = DEFAULT_H;
  float xf = 0.0f;
  int minX = 0;
  int maxX = 320 - DEFAULT_W;
  float velocityX = 0.0f;
  float speedPxPerSec = DEFAULT_SPEED_PX_PER_SEC;
  SpriteLayer::Scale spriteScale = SpriteLayer::Scale::Normal;

  Paddle();

  void setX(int newX);
  void setPosition(int newX, int newY);
  Position getPosition() const { return Position{x, y}; }
  void resetCentered(int screenW);
  void setBounds(int newMinX, int newMaxX);
  MoveResult onPhysics(float delta);
  void rebuildSprite();
  void bindSprite(SpriteLayer::Sprite& sprite);
  bool roundedBodyAt(int px, int py) const;
  bool shadowAt(int px, int py) const;

private:
  int x = 0;
  int y = DEFAULT_Y;
  SpriteLayer::Sprite* sprite = nullptr;
  uint16_t spritePixels[DEFAULT_W * DEFAULT_H]{};

  void buildSprite565(uint16_t* pixels) const;
};
