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
  game.gfx.fillScreen565(bg);
  game.gfx.fillRect565(16, 18, game.gfx.width() - 32, 4, accent);
  game.gfx.fillRect565(16, 26, game.gfx.width() - 32, 2, accent2);
  game.gfx.fillRect565(16, game.gfx.height() - 28, game.gfx.width() - 32, 2, accent2);
  game.gfx.fillRect565(16, game.gfx.height() - 20, game.gfx.width() - 32, 4, accent);

  game.gfx.drawCenteredText(58, "ARDUNOID", 5, accent);
  game.gfx.drawCenteredText(116, "ARKANOID FOR UNO Q", 2, textc);
  game.gfx.drawCenteredText(190, "PRESS FIRE", 2, textc);
}

void TitleScene::onPhysics(float delta) {
  (void)delta;
  if (game.fireConfirmAction.update(game.fireAction)) {
    game.resetGame();
    game.flushDirty();
    game.sceneSwitcher.switchTo(game.playingScene);
    game.resetClock();
  }
}

void TitleScene::onProcess(float delta) {
  (void)delta;
}
