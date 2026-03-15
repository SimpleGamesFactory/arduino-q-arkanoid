#include "TitleScene.h"

#include "ArkanoidGame.h"
#include "SGF/BufferFillRect.h"
#include "SGF/Color565.h"
#include "SGF/Font5x7.h"
#include "SGF/FontRenderer.h"

TitleScene::TitleScene(ArkanoidGame& game) : game(game) {}

void TitleScene::onEnter() {
  game.resetActions();
  game.setGameplaySpritesVisible(false);
  game.invalidateScreen();
}

void TitleScene::onExit() {
  game.dirty.clear();
}

void TitleScene::onPhysics(float delta) {
  (void)delta;
  if (game.fireAction.isJustPressed()) {
    game.transitionFromTitleToPlaying();
  }
}

void TitleScene::onProcess(float delta) {
  (void)delta;
}

void TitleScene::renderToBuffer(int x0, int y0, int w, int h, uint16_t* buf) const {
  const uint16_t bg = Color565::rgb(4, 8, 20);
  const uint16_t accent = Color565::rgb(64, 200, 255);
  const uint16_t accent2 = Color565::rgb(255, 180, 64);
  const uint16_t textc = Color565::rgb(220, 230, 255);

  BufferFillRect fillRect(x0, y0, w, h, buf);
  fillRect.fillRect565(x0, y0, w, h, bg);
  fillRect.fillRect565(16, 18, game.screenWidth() - 32, 4, accent);
  fillRect.fillRect565(16, 26, game.screenWidth() - 32, 2, accent2);
  fillRect.fillRect565(16, game.screenHeight() - 28, game.screenWidth() - 32, 2, accent2);
  fillRect.fillRect565(16, game.screenHeight() - 20, game.screenWidth() - 32, 4, accent);
  FontRenderer::drawTextCentered(FONT_5X7, fillRect, game.screenWidth() / 2, 58, "ARDUNOID",
                                 5, accent);
  FontRenderer::drawTextCentered(FONT_5X7, fillRect, game.screenWidth() / 2, 116,
                                 "ARKANOID FOR UC", 2, textc);
  FontRenderer::drawTextCentered(FONT_5X7, fillRect, game.screenWidth() / 2, 190,
                                 "PRESS FIRE", 2, textc);
}
