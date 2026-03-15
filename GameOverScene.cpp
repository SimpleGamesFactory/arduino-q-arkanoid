#include "GameOverScene.h"

#include <stdio.h>

#include "ArkanoidGame.h"
#include "SGF/BufferFillRect.h"
#include "SGF/Color565.h"
#include "SGF/Font5x7.h"
#include "SGF/FontRenderer.h"

GameOverScene::GameOverScene(ArkanoidGame& game) : game(game) {}

void GameOverScene::onEnter() {
  game.resetActions();
  game.setGameplaySpritesVisible(false);
  game.invalidateScreen();
}

void GameOverScene::onExit() {
  game.dirty.clear();
}

void GameOverScene::onPhysics(float delta) {
  (void)delta;
  if (game.fireAction.isJustPressed()) {
    game.transitionFromGameOverToPlaying();
  }
}

void GameOverScene::onProcess(float delta) {
  (void)delta;
}

void GameOverScene::renderToBuffer(int x0, int y0, int w, int h, uint16_t* buf) const {
  const uint16_t bg = Color565::rgb(6, 8, 18);
  const uint16_t accent = Color565::rgb(255, 64, 64);
  const uint16_t scorec = Color565::rgb(255, 220, 120);
  const uint16_t textc = Color565::rgb(220, 230, 255);
  char scoreBuf[16];

  snprintf(scoreBuf, sizeof(scoreBuf), "%lu", (unsigned long)game.gameOverScore);

  BufferFillRect fillRect(x0, y0, w, h, buf);
  fillRect.fillRect565(x0, y0, w, h, bg);
  fillRect.fillRect565(24, 20, game.screenWidth() - 48, 4, accent);
  fillRect.fillRect565(24, game.screenHeight() - 20, game.screenWidth() - 48, 4, accent);
  FontRenderer::drawTextCentered(FONT_5X7, fillRect, game.screenWidth() / 2, 44, "GAME OVER",
                                 4, accent);
  FontRenderer::drawTextCentered(FONT_5X7, fillRect, game.screenWidth() / 2, 96, "SCORE", 3,
                                 textc);
  FontRenderer::drawTextCentered(FONT_5X7, fillRect, game.screenWidth() / 2, 128, scoreBuf, 6,
                                 scorec);
  FontRenderer::drawTextCentered(FONT_5X7, fillRect, game.screenWidth() / 2, 190,
                                 "PRESS FIRE", 2, textc);
}
