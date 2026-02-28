#include <Arduino.h>

#include "ArkanoidGame.h"
#include "SGF/Color565.h"
#include "SGF/DirtyRects.h"
#include "SGF/Font5x7.h"

namespace {

void fillRectOnScreen(void* ctx, int x, int y, int w, int h, uint16_t color565) {
  static_cast<IScreen*>(ctx)->fillRect565(x, y, w, h, color565);
}

}  // namespace

ArkanoidGame::ArkanoidGame(
  IRenderTarget& renderTargetRef,
  IScreen& screenRef,
  const SGFHardware::HardwareProfile& hardwareProfileIn,
  uint8_t ballSpeedPotPin
)
  : Game(ARKANOID_FRAME_DEFAULT_STEP_US, ARKANOID_FRAME_MAX_STEP_US),
    renderTarget(renderTargetRef),
    screen(screenRef),
    hardwareProfile(hardwareProfileIn),
    pinBallSpeedPot(ballSpeedPotPin),
    brickFlashAnim(brickFlashSlots,
                    BRICK_FLASH_SLOTS,
                    Color565::rgb(255, 255, 255),
                    Color565::rgb(255, 245, 180)),
    hud(dirty),
    flusher(dirty, MAX_RW, MAX_RH),
    sprites(),
    sceneSwitcher(),
    titleScene(*this),
    playingScene(*this),
    gameOverScene(*this) {
  pinLeft = hardwareProfile.input.left;
  pinRight = hardwareProfile.input.right;
  pinFire = hardwareProfile.input.fire;

  paddle.setBounds(0, renderTarget.width() - paddle.getSize().x);
  paddle.resetCentered(renderTarget.width());

  ball.resetSpeedControl();
  ball.resetOnPaddle(paddle);

  paddle.bindSprite(sprites.sprite(0));
  ball.bindSprite(sprites.sprite(1));
}

void ArkanoidGame::rebuildBrickShades() {
  for (int r = 0; r < BRICK_ROWS; r++) {
    rowColorLight[r] = Color565::lighten(rowColor[r]);
    rowColorDark[r] = Color565::darken(rowColor[r]);
  }
}

void ArkanoidGame::clearBrickFlashes() {
  brickFlashAnim.clear();
}

void ArkanoidGame::spawnBrickFlashWithDurationUs(int x0, int y0, int x1, int y1, uint32_t durationUs, uint16_t baseColor, uint16_t lightColor) {
  brickFlashAnim.spawn(x0, y0, x1, y1, durationUs, baseColor, lightColor);
}

void ArkanoidGame::spawnBrickFlash(int x0, int y0, int x1, int y1, uint16_t baseColor, uint16_t lightColor) {
  spawnBrickFlashWithDurationUs(x0, y0, x1, y1, BRICK_FLASH_TOTAL_US, baseColor, lightColor);
}

uint16_t ArkanoidGame::brickFlashColorAt(int x, int y) const {
  return brickFlashAnim.colorAt(x, y);
}

void ArkanoidGame::markBrickFlashesDirty() {
  brickFlashAnim.markDirty(dirty);
}

void ArkanoidGame::advanceBrickFlashes(uint32_t dtUs) {
  brickFlashAnim.advance(dtUs, dirty);
}

bool ArkanoidGame::brickPresent(int c, int r) const {
  return (brickmask[r] >> c) & 1u;
}

void ArkanoidGame::brickClear(int c, int r) {
  brickmask[r] &= ~(1u << c);
}

void ArkanoidGame::resetBricks() {
  uint32_t full = (BRICK_COLS == 32) ? 0xFFFFFFFFu : ((1u << BRICK_COLS) - 1u);
  for (int r = 0; r < BRICK_ROWS; r++) brickmask[r] = full;
}

bool ArkanoidGame::bricksRemaining() const {
  for (int r = 0; r < BRICK_ROWS; r++) {
    if (brickmask[r] != 0) return true;
  }
  return false;
}

void ArkanoidGame::resetGame() {
  paddle.setBounds(0, renderTarget.width() - paddle.getSize().x);
  paddle.velocityX = 0.0f;
  paddle.resetCentered(renderTarget.width());
  resetClock();
  lives = START_LIVES;
  score = 0;
  gameOverScore = 0;
  fireAction.reset(digitalRead(pinFire) == LOW);
  fireConfirmAction.reset();

  ball.resetSpeedControl();

  hud.update(lives, score, renderTarget.width());
  resetBricks();
  clearBrickFlashes();
  ball.resetOnPaddle(paddle);
  invalidateScreen();
}

void ArkanoidGame::invalidateScreen() {
  dirty.invalidate(renderTarget);
}

void ArkanoidGame::updateBallSpeedControl() {
  if (pinBallSpeedPot == BALL_SPEED_CONTROL_DISABLED_PIN) {
    return;
  }

  ball.updateSpeedFromPot(
    analogRead(pinBallSpeedPot),
    ball.speedPotMinQ(),
    ball.speedPotMaxQ()
  );
}

void ArkanoidGame::transitionFromTitleToPlaying() {
  fadeOutBacklight(TITLE_EXIT_FADE_OUT_MS);
  resetGame();
  flushDirty();
  sceneSwitcher.switchTo(playingScene);
  fadeInBacklight(TITLE_EXIT_FADE_IN_MS);
  resetClock();
}

void ArkanoidGame::transitionToGameOver() {
  fadeOutBacklight(GAMEOVER_FADE_OUT_MS);
  sceneSwitcher.switchTo(gameOverScene);
  fadeInBacklight(GAMEOVER_FADE_IN_MS);
  resetClock();
}

void ArkanoidGame::transitionFromGameOverToPlaying() {
  fadeOutBacklight(GAMEOVER_EXIT_FADE_OUT_MS);
  resetGame();
  flushDirty();
  sceneSwitcher.switchTo(playingScene);
  fadeInBacklight(GAMEOVER_EXIT_FADE_IN_MS);
  resetClock();
}

uint16_t ArkanoidGame::bgAt(int x, int y) const {
  const uint16_t black = Color565::rgb(0, 0, 0);
  const uint16_t paddleShadow = Color565::rgb(28, 32, 38);

  if (y < Hud::HEIGHT) {
    uint16_t hudColor = hud.pixelColor(x, y, renderTarget.width());
    return hudColor ? hudColor : black;
  }

  if (paddle.shadowAt(x, y)) {
    return paddleShadow;
  }

  if (y >= BRICK_Y0) {
    int yy = y - BRICK_Y0;
    int row = yy / BRICK_H;

    if (row >= 0 && row < BRICK_ROWS) {
      int col = x / BRICK_W;

      if (col >= 0 && col < BRICK_COLS) {
        uint16_t flashColor = brickFlashColorAt(x, y);
        if (flashColor) return flashColor;

        if (brickPresent(col, row)) {
          int localX = x - col * BRICK_W;
          int localY = yy - row * BRICK_H;

          if (localY == 0 || localX == 0) return rowColorLight[row];
          if (localY == BRICK_H - 1 || localX == BRICK_W - 1) return rowColorDark[row];
          return rowColor[row];
        }
      }
    }
  }

  return black;
}

void ArkanoidGame::renderRegionToBuffer(int x0, int y0, int w, int h, uint16_t* buf) {
  for (int yy = 0; yy < h; yy++) {
    int y = y0 + yy;
    for (int xx = 0; xx < w; xx++) {
      int x = x0 + xx;
      uint16_t c = bgAt(x, y);
      buf[yy * w + xx] = c;
    }
  }

  sprites.renderRegion(x0, y0, w, h, buf);
}

void ArkanoidGame::flushDirty() {
  flusher.flush(renderTarget, regionBuf, [this](int x0, int y0, int w, int h, uint16_t* buf) {
    renderRegionToBuffer(x0, y0, w, h, buf);
  });
}

void ArkanoidGame::setup() {
  start();
}

void ArkanoidGame::onSetup() {
  pinMode(pinLeft, INPUT_PULLUP);
  pinMode(pinRight, INPUT_PULLUP);
  pinMode(pinFire, INPUT_PULLUP);
  fireAction.reset(digitalRead(pinFire) == LOW);
  fireConfirmAction.reset();
  screen.setBacklight(0);

  rowColor[0] = Color565::rgb(255, 0, 0);
  rowColor[1] = Color565::rgb(255, 128, 0);
  rowColor[2] = Color565::rgb(255, 255, 0);
  rowColor[3] = Color565::rgb(0, 255, 0);
  rowColor[4] = Color565::rgb(0, 255, 255);
  rowColor[5] = Color565::rgb(0, 128, 255);
  rowColor[6] = Color565::rgb(0, 0, 255);
  rowColor[7] = Color565::rgb(255, 0, 255);
  rebuildBrickShades();

  sceneSwitcher.setInitial(titleScene);
  fadeInBacklight(START_FADE_IN_MS);
  resetClock();
}

void ArkanoidGame::onPhysics(float delta) {
  fireAction.update(digitalRead(pinFire) == LOW);
  sceneSwitcher.onPhysics(delta);
}

void ArkanoidGame::onProcess(float delta) {
  sceneSwitcher.onProcess(delta);
}

void ArkanoidGame::drawCenteredText(int y, const char* text, int scale, uint16_t color565) {
  Font5x7::drawCenteredText(
    renderTarget.width(), y, text, scale, color565, &screen, fillRectOnScreen);
}

void ArkanoidGame::fadeBacklightTo(uint8_t targetLevel, uint16_t durationMs) {
  const uint8_t startLevel = screen.backlight();
  if (durationMs == 0 || startLevel == targetLevel) {
    screen.setBacklight(targetLevel);
    return;
  }

  const int delta = static_cast<int>(targetLevel) - static_cast<int>(startLevel);
  const int stepCount = abs(delta);
  if (stepCount == 0) {
    screen.setBacklight(targetLevel);
    return;
  }

  const uint32_t startMs = millis();
  for (int step = 1; step <= stepCount; step++) {
    uint32_t elapsedMs = (static_cast<uint32_t>(step) * durationMs) / stepCount;
    while ((millis() - startMs) < elapsedMs) {
      delay(1);
    }
    int nextLevel = static_cast<int>(startLevel) + (delta * step) / stepCount;
    screen.setBacklight(static_cast<uint8_t>(nextLevel));
  }
}
