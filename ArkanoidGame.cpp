#include <Arduino.h>

#include "ArkanoidGame.h"
#include "SGF/Color565.h"
#include "SGF/DirtyRects.h"
#include "SGF/FastILI9341.h"
#include "SGF/Font5x7.h"

const ArkanoidGame::BallVel ArkanoidGame::kPaddleBounceVel[ArkanoidGame::BALL_PADDLE_BOUNCE_ZONES] = {
  { -ArkanoidGame::BALL_SPEED_FAST, -ArkanoidGame::BALL_SPEED_SLOW },
  { -ArkanoidGame::BALL_SPEED_SLOW, -ArkanoidGame::BALL_SPEED_FAST },
  { -1, -ArkanoidGame::BALL_SPEED_FAST },
  { 0, -ArkanoidGame::BALL_SPEED_FAST },
  { 1, -ArkanoidGame::BALL_SPEED_FAST },
  { ArkanoidGame::BALL_SPEED_SLOW, -ArkanoidGame::BALL_SPEED_FAST },
  { ArkanoidGame::BALL_SPEED_FAST, -ArkanoidGame::BALL_SPEED_SLOW },
};

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
  paddle.setBounds(0, gfx.width() - paddle.w);
  paddle.x = (gfx.width() - paddle.w) / 2;
  paddle.xf = (float)paddle.x;

  ball.r = BALL_R;
  ball.speedScaleQ = (int32_t)BALL_SPEED_FAST << BALL_POS_FP_SHIFT;
  ball.speedPotFiltQ = -1;
  ball.dx = BALL_SPEED_SLOW;
  ball.dy = -BALL_SPEED_FAST;
  ball.attached = true;
  ball.x = paddle.x + paddle.w / 2;
  ball.y = paddle.y - ball.r - 1;
  ball.syncFixedFromInt();

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

void ArkanoidGame::fillRect565(int x0, int y0, int w, int h, uint16_t color565) {
  if (w <= 0 || h <= 0) return;

  if (x0 < 0) {
    w += x0;
    x0 = 0;
  }
  if (y0 < 0) {
    h += y0;
    y0 = 0;
  }
  if (x0 >= gfx.width() || y0 >= gfx.height()) return;
  if (x0 + w > gfx.width()) w = gfx.width() - x0;
  if (y0 + h > gfx.height()) h = gfx.height() - y0;
  if (w <= 0 || h <= 0) return;

  for (int ty = 0; ty < h; ty += MAX_RH) {
    int hh = min(MAX_RH, h - ty);
    for (int tx = 0; tx < w; tx += MAX_RW) {
      int ww = min(MAX_RW, w - tx);
      int n = ww * hh;
      for (int i = 0; i < n; i++) regionBuf[i] = color565;
      gfx.blit565(x0 + tx, y0 + ty, ww, hh, regionBuf);
    }
  }
}

void ArkanoidGame::drawText(int x, int y, const char* text, int scale, uint16_t color565) {
  if (!text || scale <= 0) return;

  const int w = Font5x7::textWidth(text, scale);
  const int h = 7 * scale;
  for (int yy = 0; yy < h; yy++) {
    for (int xx = 0; xx < w; xx++) {
      if (Font5x7::textPixel(text, scale, xx, yy)) {
        fillRect565(x + xx, y + yy, 1, 1, color565);
      }
    }
  }
}

void ArkanoidGame::drawCenteredText(int y, const char* text, int scale, uint16_t color565) {
  int x = (gfx.width() - Font5x7::textWidth(text, scale)) / 2;
  drawText(x, y, text, scale, color565);
}

void ArkanoidGame::resetGame() {
  paddle.setBounds(0, gfx.width() - paddle.w);
  paddle.velocityX = 0.0f;
  paddle.resetCentered(gfx.width());
  resetClock();
  lives = START_LIVES;
  score = 0;
  gameOverScore = 0;
  fireAction.reset(digitalRead(pinFire) == LOW);
  fireConfirmAction.reset();

  ball.r = BALL_R;
  ball.speedScaleQ = (int32_t)BALL_SPEED_FAST << BALL_POS_FP_SHIFT;
  ball.speedPotFiltQ = -1;

  hud.update(lives, score, gfx.width());
  resetBricks();
  clearBrickFlashes();
  ball.resetOnPaddle(paddle.x, paddle.w, paddle.y, BALL_SPEED_SLOW, -BALL_SPEED_FAST);
  dirty.clear();
  dirty.add(0, 0, gfx.width() - 1, gfx.height() - 1);
  paddle.updateSprite(sprites.sprite(0));
  ball.updateSprite(sprites.sprite(1));
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
  struct IliTarget : public IRenderTarget {
    FastILI9341& t;
    explicit IliTarget(FastILI9341& target) : t(target) {}
    int width() const override { return t.width(); }
    int height() const override { return t.height(); }
    void blit565(int x0, int y0, int w, int h, const uint16_t* pix) override {
      t.blit565(x0, y0, w, h, pix);
    }
  } target{gfx};

  flusher.flush(target, regionBuf, [this](int x0, int y0, int w, int h, uint16_t* buf) {
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
