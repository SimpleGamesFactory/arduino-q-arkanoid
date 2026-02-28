#include "GameOverScene.h"

#include <stdio.h>

#include "ArkanoidGame.h"
#include "SGF/Color565.h"

GameOverScene::GameOverScene(ArkanoidGame& game) : game(game) {}

void GameOverScene::onEnter() {
  const uint16_t bg = Color565::rgb(6, 8, 18);
  const uint16_t accent = Color565::rgb(255, 64, 64);
  const uint16_t scorec = Color565::rgb(255, 220, 120);
  const uint16_t textc = Color565::rgb(220, 230, 255);
  char scoreBuf[16];

  snprintf(scoreBuf, sizeof(scoreBuf), "%lu", (unsigned long)game.gameOverScore);
  game.fireConfirmAction.reset();
  game.dirty.clear();
  game.fillScreen(bg);
  game.fillRect(24, 20, game.screenWidth() - 48, 4, accent);
  game.fillRect(24, game.screenHeight() - 20, game.screenWidth() - 48, 4, accent);

  game.drawCenteredText(44, "GAME OVER", 4, accent);
  game.drawCenteredText(96, "SCORE", 3, textc);
  game.drawCenteredText(128, scoreBuf, 6, scorec);
  game.drawCenteredText(190, "PRESS FIRE", 2, textc);
}

void GameOverScene::onPhysics(float delta) {
  (void)delta;
  if (game.fireConfirmAction.update(game.fireAction)) {
    game.transitionFromGameOverToPlaying();
  }
}

void GameOverScene::onProcess(float delta) {
  (void)delta;
}
