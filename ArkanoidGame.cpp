#include <Arduino.h>

#include "ArkanoidGame.h"
#include "SGF/Color565.h"
#include "SGF/DirtyRects.h"
#include "SGF/FastILI9341.h"

ArkanoidGame::ArkanoidGame(
  FastILI9341& gfx,
  uint8_t leftPin,
  uint8_t rightPin,
  uint8_t firePin,
  uint8_t ballSpeedPotPin
)
  : Game(ARKANOID_FRAME_DEFAULT_STEP_US, ARKANOID_FRAME_MAX_STEP_US),
    gfx(gfx),
    pinLeft(leftPin),
    pinRight(rightPin),
    pinFire(firePin),
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
  paddle.setBounds(0, gfx.width() - paddle.getSize().x);
  paddle.resetCentered(gfx.width());

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
  paddle.setBounds(0, gfx.width() - paddle.getSize().x);
  paddle.velocityX = 0.0f;
  paddle.resetCentered(gfx.width());
  resetClock();
  lives = START_LIVES;
  score = 0;
  gameOverScore = 0;
  fireAction.reset(digitalRead(pinFire) == LOW);
  fireConfirmAction.reset();

  ball.resetSpeedControl();

  hud.update(lives, score, gfx.width());
  resetBricks();
  clearBrickFlashes();
  ball.resetOnPaddle(paddle);
  invalidateScreen();
}

void ArkanoidGame::invalidateScreen() {
  dirty.invalidate(gfx);
}

uint16_t ArkanoidGame::bgAt(int x, int y) const {
  const uint16_t black = Color565::rgb(0, 0, 0);
  const uint16_t paddleShadow = Color565::rgb(28, 32, 38);

  if (y < Hud::HEIGHT) {
    uint16_t hudColor = hud.pixelColor(x, y, gfx.width());
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
  flusher.flush(gfx, regionBuf, [this](int x0, int y0, int w, int h, uint16_t* buf) {
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

  bool ok = gfx.begin(DEFAULT_SPI_HZ);
  if (!ok) {
    while (1) delay(1000);
  }
  analogWriteResolution(12);
  gfx.screenRotation(DEFAULT_ROTATION);
  gfx.setBacklightPwmMax(BACKLIGHT_PWM_MAX);
  gfx.setBacklight(BACKLIGHT_START);

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
  gfx.fadeInBacklight(START_FADE_IN_MS);
  resetClock();
}

void ArkanoidGame::onPhysics(float delta) {
  fireAction.update(digitalRead(pinFire) == LOW);
  sceneSwitcher.onPhysics(delta);
}

void ArkanoidGame::onProcess(float delta) {
  sceneSwitcher.onProcess(delta);
}
