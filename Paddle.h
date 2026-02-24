#pragma once

#include <stdint.h>

class Paddle {
public:
  struct MoveResult {
    int oldX = 0;
    int newX = 0;
    bool moved = false;
  };

  static constexpr int DEFAULT_W = 60;
  static constexpr int DEFAULT_H = 8;
  static constexpr int DEFAULT_Y = 220;
  static constexpr float DEFAULT_SPEED_PX_PER_SEC = 500.0f;

  int x = 0;
  int y = DEFAULT_Y;
  int w = DEFAULT_W;
  int h = DEFAULT_H;
  float xf = 0.0f;
  int minX = 0;
  int maxX = 320 - DEFAULT_W;
  float velocityX = 0.0f;
  float speedPxPerSec = DEFAULT_SPEED_PX_PER_SEC;

  void resetCentered(int screenW);
  void setBounds(int newMinX, int newMaxX);
  MoveResult onPhysics(float delta);
  void buildSprite565(uint16_t* pixels) const;
  bool roundedBodyAt(int px, int py) const;
  bool shadowAt(int px, int py) const;
};
