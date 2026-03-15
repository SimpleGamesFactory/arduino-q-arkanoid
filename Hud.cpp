#include "Hud.h"

#include <stdio.h>

#include "SGF/Color565.h"

namespace {

bool textPixel(const char* text, int scale, int x, int y) {
  if (!text || scale <= 0 || x < 0 || y < 0) {
    return false;
  }

  int row = y / scale;
  if (row >= FONT_5X7.glyphHeight()) {
    return false;
  }

  int cursorX = 0;
  int glyphAdvance = FONT_5X7.glyphAdvance() * scale;
  int glyphWidth = FONT_5X7.glyphWidth() * scale;
  for (const char* p = text; *p != '\0'; ++p, cursorX += glyphAdvance) {
    if (x < cursorX || x >= cursorX + glyphWidth) {
      continue;
    }

    int col = (x - cursorX) / scale;
    uint8_t rowBits = FONT_5X7.glyphRowBits(*p, row);
    return (rowBits & (1u << (FONT_5X7.glyphWidth() - 1 - col))) != 0;
  }

  return false;
}

}  // namespace

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
  scoreX = screenW - MARGIN_X - FontRenderer::textWidth(FONT_5X7, scoreText, FONT_SCALE);
}

void Hud::markDirty(int screenW) {
  dirty.add(0, 0, screenW - 1, HEIGHT - 1);
}

uint16_t Hud::pixelColor(int x, int y, int screenW) const {
  (void)screenW;
  const uint16_t hudLivesColor = Color565::rgb(255, 255, 255);
  const uint16_t hudScoreColor = Color565::rgb(255, 220, 120);

  int ly = y - TEXT_Y;
  int lx = x - MARGIN_X;
  if (textPixel(livesText, FONT_SCALE, lx, ly)) return hudLivesColor;
  if (textPixel(scoreText, FONT_SCALE, x - scoreX, ly)) return hudScoreColor;
  return 0;
}
