#include <Arduino.h>

#include "ArkanoidGame.h"
#include "SGF/DirtyRects.h"
#include "SGF/FastILI9341.h"
#include "SGF/Font5x7.h"
#include "SGF/Collision.h"
#include <stdio.h>

const ArkanoidGame::BallVel ArkanoidGame::kPaddleBounceVel[ArkanoidGame::BALL_PADDLE_BOUNCE_ZONES] = {
  { -ArkanoidGame::BALL_SPEED_FAST, -ArkanoidGame::BALL_SPEED_SLOW },
  { -ArkanoidGame::BALL_SPEED_SLOW, -ArkanoidGame::BALL_SPEED_FAST },
  { -1, -ArkanoidGame::BALL_SPEED_FAST },
  { 0, -ArkanoidGame::BALL_SPEED_FAST },
  { 1, -ArkanoidGame::BALL_SPEED_FAST },
  { ArkanoidGame::BALL_SPEED_SLOW, -ArkanoidGame::BALL_SPEED_FAST },
  { ArkanoidGame::BALL_SPEED_FAST, -ArkanoidGame::BALL_SPEED_SLOW },
};

static constexpr int BALL_SPEED_POT_PIN = A5;

ArkanoidGame::ArkanoidGame(FastILI9341& gfx, uint8_t leftPin, uint8_t rightPin, uint8_t firePin)
  : Game(ARKANOID_FRAME_DEFAULT_STEP_US, ARKANOID_FRAME_MAX_STEP_US),
    gfx(gfx),
    pinLeft(leftPin),
    pinRight(rightPin),
    pinFire(firePin),
    brickFlashAnim(brickFlashSlots,
                    BRICK_FLASH_SLOTS,
                    FastILI9341::rgb565(255, 255, 255),
                    FastILI9341::rgb565(255, 245, 180)),
    hud(dirty),
    flusher(dirty, MAX_RW, MAX_RH) {
  paddle.x = (gfx.width() - PADDLE_W) / 2;
  paddle.xf = (float)paddle.x;

  ball.r = BALL_R;
  ball.speedScaleQ = (int32_t)BALL_SPEED_FAST << BALL_POS_FP_SHIFT;
  ball.speedPotFiltQ = -1;
  ball.dx = BALL_SPEED_SLOW;
  ball.dy = -BALL_SPEED_FAST;
  ball.attached = true;
  ball.x = paddle.x + PADDLE_W / 2;
  ball.y = PADDLE_Y - ball.r - 1;
  ball.syncFixedFromInt();
}

void ArkanoidGame::rebuildBrickShades() {
  for (int r = 0; r < BRICK_ROWS; r++) {
    rowColorLight[r] = FastILI9341::lighten565(rowColor[r]);
    rowColorDark[r] = FastILI9341::darken565(rowColor[r]);
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

void ArkanoidGame::drawGameOverScreen() {
  const uint16_t bg = FastILI9341::rgb565(6, 8, 18);
  const uint16_t accent = FastILI9341::rgb565(255, 64, 64);
  const uint16_t scorec = FastILI9341::rgb565(255, 220, 120);
  const uint16_t textc = FastILI9341::rgb565(220, 230, 255);

  char scoreBuf[16];
  snprintf(scoreBuf, sizeof(scoreBuf), "%lu", (unsigned long)gameOverScore);

  dirty.clear();
  gfx.fillScreen565(bg);
  fillRect565(24, 20, gfx.width() - 48, 4, accent);
  fillRect565(24, gfx.height() - 20, gfx.width() - 48, 4, accent);

  drawCenteredText(44, "GAME OVER", 4, accent);
  drawCenteredText(96, "SCORE", 3, textc);
  drawCenteredText(128, scoreBuf, 6, scorec);
  drawCenteredText(190, "PRESS FIRE", 2, textc);
}

void ArkanoidGame::enterGameOver() {
  gfx.fadeOutBacklight(GAMEOVER_FADE_OUT_MS);
  gameOver = true;
  gameOverScore = score;
  drawGameOverScreen();
  gfx.fadeInBacklight(GAMEOVER_FADE_IN_MS);
  resetClock();
}

void ArkanoidGame::resetGameFromGameOverWithFade() {
  gfx.fadeOutBacklight(GAMEOVER_EXIT_FADE_OUT_MS);
  resetGame();
  flushDirty();
  gfx.fadeInBacklight(GAMEOVER_EXIT_FADE_IN_MS);
  resetClock();
}

void ArkanoidGame::resetGame() {
  paddle.resetCentered(gfx.width(), PADDLE_W);
  resetClock();
  lives = START_LIVES;
  score = 0;
  gameOver = false;
  gameOverScore = 0;
  prevFirePressed = false;

  ball.r = BALL_R;
  ball.speedScaleQ = (int32_t)BALL_SPEED_FAST << BALL_POS_FP_SHIFT;
  ball.speedPotFiltQ = -1;

  hud.update(lives, score, gfx.width());
  resetBricks();
  clearBrickFlashes();
  ball.resetOnPaddle(paddle.x, PADDLE_W, PADDLE_Y, BALL_SPEED_SLOW, -BALL_SPEED_FAST);
  dirty.clear();
  dirty.add(0, 0, gfx.width() - 1, gfx.height() - 1);
}

bool ArkanoidGame::insideBall(int x, int y) const {
  int dx = x - ball.x;
  int dy = y - ball.y;
  return dx * dx + dy * dy <= ball.r * ball.r;
}

bool ArkanoidGame::circleRectHit(int cx, int cy, int r, int x0, int y0, int x1, int y1) const {
  return ::circleRectHit(cx, cy, r, x0, y0, x1, y1);
}

bool ArkanoidGame::paddleRoundedBodyAt(int x, int y) const {
  if (y < PADDLE_Y || y >= PADDLE_Y + PADDLE_H || x < paddle.x || x >= paddle.x + PADDLE_W) return false;

  int lx = x - paddle.x;
  int ly = y - PADDLE_Y;

  if ((ly == 0 || ly == PADDLE_H - 1) && (lx == 0 || lx == PADDLE_W - 1)) return false;
  return true;
}

bool ArkanoidGame::paddleShadowAt(int x, int y) const {
  const int sx = paddle.x + 1;
  const int sy = PADDLE_Y + 1;
  const int sw = PADDLE_W;
  const int sh = PADDLE_H;
  if (y < sy || y >= sy + sh || x < sx || x >= sx + sw) return false;

  int lx = x - sx;
  int ly = y - sy;
  if ((ly == 0 || ly == sh - 1) && (lx == 0 || lx == sw - 1)) return false;

  if (paddleRoundedBodyAt(x, y)) return false;
  return true;
}

uint16_t ArkanoidGame::bgAt(int x, int y) const {
  const uint16_t black = FastILI9341::rgb565(0, 0, 0);
  const uint16_t paddleFace = FastILI9341::rgb565(196, 200, 208);
  const uint16_t paddleLight = FastILI9341::rgb565(232, 236, 244);
  const uint16_t paddleDark = FastILI9341::rgb565(130, 136, 146);
  const uint16_t paddleMidShadow = FastILI9341::rgb565(86, 92, 102);
  const uint16_t paddleShadow = FastILI9341::rgb565(28, 32, 38);

  if (y < Hud::HEIGHT) {
    uint16_t hudColor = hud.pixelColor(x, y, gfx.width());
    return hudColor ? hudColor : black;
  }

  if (paddleRoundedBodyAt(x, y)) {
    int lx = x - paddle.x;
    int ly = y - PADDLE_Y;

    if (ly == 0 || lx == 0) return paddleLight;
    if (ly == PADDLE_H - 1 || lx == PADDLE_W - 1) return paddleDark;
    if (ly == PADDLE_H - 2 || lx == PADDLE_W - 2) return paddleMidShadow;
    return paddleFace;
  }
  if (paddleShadowAt(x, y)) {
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
  const uint16_t ballc = FastILI9341::rgb565(255, 255, 255);

  for (int yy = 0; yy < h; yy++) {
    int y = y0 + yy;
    for (int xx = 0; xx < w; xx++) {
      int x = x0 + xx;
      uint16_t c = bgAt(x, y);
      if (insideBall(x, y)) c = ballc;
      buf[yy * w + xx] = c;
    }
  }
}

void ArkanoidGame::flushDirty() {
  struct IliTarget : public IRenderTarget {
    FastILI9341& t;
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

void ArkanoidGame::snapAngles() {
  int sx = (ball.dx >= 0) ? 1 : -1;
  int sy = (ball.dy >= 0) ? 1 : -1;
  int ax = abs(ball.dx);
  int ay = abs(ball.dy);
  if (ax == ay) {
    ax = BALL_SPEED_SLOW;
    ay = BALL_SPEED_FAST;
  } else if (ax > ay) {
    ax = BALL_SPEED_FAST;
    ay = BALL_SPEED_SLOW;
  } else {
    ax = BALL_SPEED_SLOW;
    ay = BALL_SPEED_FAST;
  }
  ball.dx = sx * ax;
  ball.dy = sy * ay;
}

void ArkanoidGame::updatePaddle(float dtSec) {
  int dir = 0;
  if (digitalRead(pinLeft) == LOW) dir--;
  if (digitalRead(pinRight) == LOW) dir++;
  int old = paddle.x;
  if (paddle.updateDt(dir, paddleSpeed, dtSec, PADDLE_BASE_STEP_US, gfx.width(), PADDLE_W, &old)) {
    dirty.add(old - 2, PADDLE_Y - 2, old + PADDLE_W + 2, PADDLE_Y + PADDLE_H + 2);
    dirty.add(paddle.x - 2, PADDLE_Y - 2, paddle.x + PADDLE_W + 2, PADDLE_Y + PADDLE_H + 2);
  }
}

void ArkanoidGame::applyPaddleBounceAngle() {
  int hit = constrain(ball.x - paddle.x, 0, PADDLE_W - 1);
  int zone = (hit * BALL_PADDLE_BOUNCE_ZONES) / PADDLE_W;
  zone = constrain(zone, 0, BALL_PADDLE_BOUNCE_ZONES - 1);
  ball.setVelocity(kPaddleBounceVel[zone].dx, kPaddleBounceVel[zone].dy);
}

void ArkanoidGame::setup() {
  start();
}

void ArkanoidGame::onSetup() {
  pinMode(pinLeft, INPUT_PULLUP);
  pinMode(pinRight, INPUT_PULLUP);
  pinMode(pinFire, INPUT_PULLUP);

  bool ok = gfx.begin(DEFAULT_SPI_HZ);
  if (!ok) {
    while (1) delay(1000);
  }
  analogWriteResolution(12);
  gfx.screenRotation(DEFAULT_ROTATION);
  gfx.setBacklightPwmMax(BACKLIGHT_PWM_MAX);
  gfx.setBacklight(BACKLIGHT_START);

  rowColor[0] = FastILI9341::rgb565(255, 0, 0);
  rowColor[1] = FastILI9341::rgb565(255, 128, 0);
  rowColor[2] = FastILI9341::rgb565(255, 255, 0);
  rowColor[3] = FastILI9341::rgb565(0, 255, 0);
  rowColor[4] = FastILI9341::rgb565(0, 255, 255);
  rowColor[5] = FastILI9341::rgb565(0, 128, 255);
  rowColor[6] = FastILI9341::rgb565(0, 0, 255);
  rowColor[7] = FastILI9341::rgb565(255, 0, 255);
  rebuildBrickShades();

  resetGame();
  flushDirty();
  gfx.fadeInBacklight(START_FADE_IN_MS);
}

void ArkanoidGame::onPhysics(float frameDtSec) {
  bool firePressed = (digitalRead(pinFire) == LOW);
  bool fireEdge = firePressed && !prevFirePressed;
  bool fireReleaseEdge = !firePressed && prevFirePressed;
  prevFirePressed = firePressed;

  if (gameOver) {
    if (fireReleaseEdge) {
      resetGameFromGameOverWithFade();
    }
    return;
  }

  updatePaddle(frameDtSec);
  markBrickFlashesDirty();
  int oldx = ball.x;
  int oldy = ball.y;

  bool fell = false;
  if (ball.attached) {
    ball.attachToPaddle(paddle.x, PADDLE_W, PADDLE_Y);
    if (fireEdge) {
      ball.launch(BALL_SPEED_SLOW, -BALL_SPEED_FAST);
    }
  } else {
    ball.updateSpeedFromPot(analogRead(BALL_SPEED_POT_PIN), POT_BALL_SPEED_MIN_Q, POT_BALL_SPEED_MAX_Q);
    bool resyncBallPos = false;

    ball.stepScaled(frameDtSec, BALL_BASE_STEP_US);

    if (ball.x - ball.r < 0) {
      ball.x = ball.r;
      ball.dx = -ball.dx;
      resyncBallPos = true;
    }
    if (ball.x + ball.r >= gfx.width()) {
      ball.x = gfx.width() - ball.r - 1;
      ball.dx = -ball.dx;
      resyncBallPos = true;
    }
    if (ball.y - ball.r < Hud::HEIGHT) {
      ball.y = Hud::HEIGHT + ball.r;
      ball.dy = -ball.dy;
      resyncBallPos = true;
    }

    if (ball.dy > 0 && ball.y + ball.r >= PADDLE_Y && ball.y + ball.r <= PADDLE_Y + PADDLE_H && ball.x >= paddle.x && ball.x <= paddle.x + PADDLE_W) {
      ball.y = PADDLE_Y - ball.r - 1;
      applyPaddleBounceAngle();
      resyncBallPos = true;
    }

  if (ball.y + ball.r >= gfx.height()) {
    lives--;
    if (lives <= 0) {
      enterGameOver();
      return;
    } else {
      hud.update(lives, score, gfx.width());
      hud.markDirty(gfx.width());
      dirty.add(oldx - ball.r - 3, oldy - ball.r - 3, oldx + ball.r + 3, oldy + ball.r + 3);
        ball.resetOnPaddle(paddle.x, PADDLE_W, PADDLE_Y, BALL_SPEED_SLOW, -BALL_SPEED_FAST);
      }
      fell = true;
    }

    if (!fell) {
      bool hit = false;

      int cx0 = max(0, (ball.x - ball.r) / BRICK_W);
      int cx1 = min(BRICK_COLS - 1, (ball.x + ball.r) / BRICK_W);
      int ry0 = max(0, (ball.y - ball.r - BRICK_Y0) / BRICK_H);
      int ry1 = min(BRICK_ROWS - 1, (ball.y + ball.r - BRICK_Y0) / BRICK_H);

      for (int r = ry0; r <= ry1 && !hit; r++) {
        for (int c = cx0; c <= cx1; c++) {
          if (!brickPresent(c, r)) continue;
          int x0 = c * BRICK_W;
          int y0 = BRICK_Y0 + r * BRICK_H;
          int x1 = x0 + BRICK_W - 1;
          int y1 = y0 + BRICK_H - 1;
          if (circleRectHit(ball.x, ball.y, ball.r, x0, y0, x1, y1)) {
            hit = true;

            bool prev_out_y = (oldy < y0 - ball.r) || (oldy > y1 + ball.r);
            if (prev_out_y) ball.dy = -ball.dy;
            else ball.dx = -ball.dx;

            snapAngles();

            brickClear(c, r);
            spawnBrickFlash(x0, y0, x1, y1, rowColor[r], rowColorLight[r]);
            score += SCORE_PER_BRICK;
            hud.update(lives, score, gfx.width());
            hud.markDirty(gfx.width());

            if (!bricksRemaining()) {
              resetBricks();
              clearBrickFlashes();

              dirty.clear();
              dirty.add(0, 0, gfx.width() - 1, gfx.height() - 1);
            }

            dirty.add(x0 - 2, y0 - 2, x1 + 2, y1 + 2);
            break;
          }
        }
      }
    }

    if (resyncBallPos) {
      ball.syncFixedFromInt();
    }
  }

  bool ballMoved = (ball.x != oldx) || (ball.y != oldy);

  if (!fell && ballMoved) {
    dirty.add(oldx - ball.r - 3, oldy - ball.r - 3, oldx + ball.r + 3, oldy + ball.r + 3);
  }
  if (ballMoved || fell) {
    dirty.add(ball.x - ball.r - 3, ball.y - ball.r - 3, ball.x + ball.r + 3, ball.y + ball.r + 3);
  }
}

void ArkanoidGame::onProcess(float frameDtSec) {
  uint32_t frameDtUs = (uint32_t)(frameDtSec * 1000000.0f + 0.5f);
  flushDirty();
  advanceBrickFlashes(frameDtUs);
}
