#pragma once

#include <stdint.h>

#include "SGF/DirtyRects.h"
#include "SGF/Font5x7.h"

class Hud {
public:
  static constexpr int FONT_SCALE = 1;
  static constexpr int TEXT_Y = 1;
  static constexpr int MARGIN_X = 2;
  static constexpr int HEIGHT = 10;
  static constexpr int SCORE_CHARS = 8;
  static constexpr int LIVES_SLOTS = 3;

  explicit Hud(DirtyRects& dirty);

  void update(int lives, uint32_t score, int screenW);
  void markDirty(int screenW);
  uint16_t pixelColor(int x, int y, int screenW) const;

private:
  DirtyRects& dirty;
  char livesText[LIVES_SLOTS + 1]{};
  char scoreText[SCORE_CHARS + 1]{};
  int scoreX = 0;
};
