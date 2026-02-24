#include "Hud.h"
#include "SGF/Color565.h"
#include "SGF/FastILI9341.h"
#include <stdio.h>

Hud::Hud(DirtyRects& dirtyRef) : dirty(dirtyRef) {
  livesText[0] = '\0';
  scoreText[0] = '\0';
}

void Hud::update(int lives, uint32_t score, int screenW) {
  for (int i = 0; i < LIVES_SLOTS; i++) {
    livesText[i] = (i < lives) ? '*' : ' ';
  }
  livesText[LIVES_SLOTS] = '\0';

  snprintf(scoreText, sizeof(scoreText), "%*lu", SCORE_CHARS, (unsigned long)score);
  scoreX = screenW - MARGIN_X - Font5x7::textWidth(scoreText, FONT_SCALE);
}

void Hud::markDirty(int screenW) {
  dirty.add(0, 0, screenW - 1, HEIGHT - 1);
}

uint16_t Hud::pixelColor(int x, int y, int screenW) const {
  const uint16_t hudLivesColor = Color565::rgb(255, 255, 255);
  const uint16_t hudScoreColor = Color565::rgb(255, 220, 120);

  int ly = y - TEXT_Y;
  int lx = x - MARGIN_X;
  if (Font5x7::textPixel(livesText, FONT_SCALE, lx, ly)) return hudLivesColor;
  if (Font5x7::textPixel(scoreText, FONT_SCALE, x - scoreX, ly)) return hudScoreColor;
  return 0;
}
