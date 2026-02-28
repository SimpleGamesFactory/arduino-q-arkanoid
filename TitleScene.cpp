#include "TitleScene.h"

#include "ArkanoidGame.h"
#include "SGF/Color565.h"

TitleScene::TitleScene(ArkanoidGame& game) : game(game) {}

void TitleScene::onEnter() {
  const uint16_t bg = Color565::rgb(4, 8, 20);
  const uint16_t accent = Color565::rgb(64, 200, 255);
  const uint16_t accent2 = Color565::rgb(255, 180, 64);
  const uint16_t textc = Color565::rgb(220, 230, 255);

  game.fireConfirmAction.reset();
  game.dirty.clear();
  game.fillScreen(bg);
  game.fillRect(16, 18, game.screenWidth() - 32, 4, accent);
  game.fillRect(16, 26, game.screenWidth() - 32, 2, accent2);
  game.fillRect(16, game.screenHeight() - 28, game.screenWidth() - 32, 2, accent2);
  game.fillRect(16, game.screenHeight() - 20, game.screenWidth() - 32, 4, accent);

  game.drawCenteredText(58, "ARDUNOID", 5, accent);
  game.drawCenteredText(116, "ARKANOID FOR UNO Q", 2, textc);
  game.drawCenteredText(190, "PRESS FIRE", 2, textc);
}

void TitleScene::onPhysics(float delta) {
  (void)delta;
  if (game.fireConfirmAction.update(game.fireAction)) {
    game.transitionFromTitleToPlaying();
  }
}

void TitleScene::onProcess(float delta) {
  (void)delta;
}
