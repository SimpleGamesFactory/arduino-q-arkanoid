#include "Arkanoid.h"
#include "FastILI9341.h"
#include "DirtyRects.h"
#include "Font5x7.h"
#include "RectFlashAnim.h"
#include "Paddle.h"
#include "Ball.h"
#include "gamelib.h"
#include "gamelib_runtime.h"
#include <stdio.h>

extern FastILI9341 gfx;
extern DirtyRects dirty;
static uint8_t sPinLeft = 0;
static uint8_t sPinRight = 0;
static uint8_t sPinFire = 0;

// ====== Parametry renderu regionu ======
static constexpr int MAX_RW = 120;
static constexpr int MAX_RH = 80;
static uint16_t regionbuf[MAX_RW * MAX_RH];

// ====== HUD ======
static constexpr int HUD_FONT_SCALE = 1;
static constexpr int HUD_TEXT_Y = 1;
static constexpr int HUD_MARGIN_X = 2;
static constexpr int HUD_H = 10;
static constexpr int HUD_SCORE_CHARS = 8;
static constexpr int HUD_LIVES_SLOTS = 3;
static char hudLivesText[HUD_LIVES_SLOTS + 1] = "***";
static char hudScoreText[HUD_SCORE_CHARS + 1] = "       0";
static int hudScoreX = 0;


// Paddle

static constexpr int PADDLE_W = 60;
static constexpr int PADDLE_H = 8;
static constexpr int PADDLE_Y = 220;
static constexpr int START_LIVES = 3;
static constexpr uint16_t SCORE_PER_BRICK = 10;

static PaddleState paddle = {
  (320 - PADDLE_W) / 2,
  (float)((320 - PADDLE_W) / 2)
};
static int &paddleX = paddle.x;
static int paddleSpeed = 5;
static constexpr uint32_t PADDLE_BASE_STEP_US = 10000;  // referencja starego delay(10)
static int lives = START_LIVES;
static uint32_t score = 0;
static bool gameOver = false;
static uint32_t gameOverScore = 0;



// ====== Bricks bitmask ======
static constexpr int BRICK_W = 20;
static constexpr int BRICK_H = 12;
static constexpr int BRICK_COLS = 320 / BRICK_W;  // 16
static constexpr int BRICK_ROWS = 8;
static constexpr int BRICK_Y0 = HUD_H + 10;



static uint32_t brickmask[BRICK_ROWS];

static uint16_t rowColor[BRICK_ROWS];
static uint16_t rowColorLight[BRICK_ROWS];
static uint16_t rowColorDark[BRICK_ROWS];
static constexpr int BRICK_FLASH_SLOTS = 40;
static constexpr uint32_t BRICK_FLASH_TOTAL_US = 50000;  // ~50 ms
static constexpr uint16_t GAMEOVER_FADE_OUT_MS = 140;
static constexpr uint16_t GAMEOVER_FADE_IN_MS = 180;
static constexpr uint16_t GAMEOVER_EXIT_FADE_OUT_MS = 140;
static constexpr uint16_t GAMEOVER_EXIT_FADE_IN_MS = 180;

static RectFlashAnimSlot brickFlashSlots[BRICK_FLASH_SLOTS];
static RectFlashAnim brickFlashAnim(
  brickFlashSlots,
  BRICK_FLASH_SLOTS,
  FastILI9341::rgb565(255, 255, 255),
  FastILI9341::rgb565(255, 245, 180));

static inline uint16_t lighten565(uint16_t c) {
  return gamelib_lighten565(c);
}

static inline uint16_t darken565(uint16_t c) {
  return gamelib_darken565(c);
}

static void rebuildBrickShades() {
  for (int r = 0; r < BRICK_ROWS; r++) {
    rowColorLight[r] = lighten565(rowColor[r]);
    rowColorDark[r] = darken565(rowColor[r]);
  }
}

static void clearBrickFlashes() {
  brickFlashAnim.clear();
}

static void spawnBrickFlashWithDurationUs(int x0, int y0, int x1, int y1, uint32_t durationUs, uint16_t baseColor, uint16_t lightColor) {
  brickFlashAnim.spawn(x0, y0, x1, y1, durationUs, baseColor, lightColor);
}

static inline void spawnBrickFlash(int x0, int y0, int x1, int y1, uint16_t baseColor, uint16_t lightColor) {
  spawnBrickFlashWithDurationUs(x0, y0, x1, y1, BRICK_FLASH_TOTAL_US, baseColor, lightColor);
}

static inline uint16_t brickFlashColorAt(int x, int y) {
  return brickFlashAnim.colorAt(x, y);
}

static void markBrickFlashesDirty() {
  brickFlashAnim.markDirty(dirty);
}

static void advanceBrickFlashes(uint32_t dtUs) {
  brickFlashAnim.advance(dtUs, dirty);
}

static inline bool brickPresent(int c, int r) {
  return (brickmask[r] >> c) & 1u;
}
static inline void brickClear(int c, int r) {
  brickmask[r] &= ~(1u << c);
}

static void resetBricks() {
  uint32_t full = (BRICK_COLS == 32) ? 0xFFFFFFFFu : ((1u << BRICK_COLS) - 1u);
  for (int r = 0; r < BRICK_ROWS; r++) brickmask[r] = full;
}

// ====== Ball ======
static constexpr int BALL_R = 6;
static constexpr int BALL_SPEED_SLOW = 1;
static constexpr int BALL_SPEED_FAST = 3;
static constexpr int BALL_PADDLE_BOUNCE_ZONES = 7;
static constexpr int BALL_POS_FP_SHIFT = 8;
static constexpr int BALL_POS_FP_ONE = 1 << BALL_POS_FP_SHIFT;
static constexpr uint32_t BALL_BASE_STEP_US = 10000;  // referencja: 10 ms / frame
// Zakres potencjometru A5 (Q8 px/krok): może być szerszy niż BALL_SPEED_* używane do kątów odbić
static constexpr int32_t POT_BALL_SPEED_MIN_Q = BALL_POS_FP_ONE / 2;      // 0.5
static constexpr int32_t POT_BALL_SPEED_MAX_Q = BALL_POS_FP_ONE * 5;      // 5.0

struct BallVel {
  int dx;
  int dy;
};

static constexpr BallVel kPaddleBounceVel[BALL_PADDLE_BOUNCE_ZONES] = {
  { -BALL_SPEED_FAST, -BALL_SPEED_SLOW },
  { -BALL_SPEED_SLOW, -BALL_SPEED_FAST },
  { -1, -BALL_SPEED_FAST },
  { 0, -BALL_SPEED_FAST },
  { 1, -BALL_SPEED_FAST },
  { BALL_SPEED_SLOW, -BALL_SPEED_FAST },
  { BALL_SPEED_FAST, -BALL_SPEED_SLOW },
};

static BallState ball = {
  60, 120,
  BALL_SPEED_SLOW, -BALL_SPEED_FAST,
  BALL_R,
  true,
  60.0f, 120.0f,
  (int32_t)BALL_SPEED_FAST << BALL_POS_FP_SHIFT,
  -1
};
static int &bx = ball.x;
static int &by = ball.y;
static int &bdx = ball.dx;
static int &bdy = ball.dy;
static int &br = ball.r;
static bool &ballAttached = ball.attached;
static bool prevFirePressed = false;

static inline void syncBallFixedFromInt() {
  ball_sync_fixed_from_int(&ball);
}

static inline void updateBallSpeedFromPot() {
  ball_update_speed_from_pot(&ball, analogRead(A5), POT_BALL_SPEED_MIN_Q, POT_BALL_SPEED_MAX_Q);
}

static inline void stepBallWithPotSpeed(float dtSec) {
  ball_step_scaled(&ball, dtSec, BALL_BASE_STEP_US, BALL_POS_FP_ONE, BALL_SPEED_FAST);
}

static inline void setBallVelocity(int dx, int dy) {
  ball_set_velocity(&ball, dx, dy);
}

static inline void attachBallToPaddle() {
  ball_attach_to_paddle(&ball, paddleX, PADDLE_W, PADDLE_Y);
}

static inline void resetBallOnPaddle() {
  ball_reset_on_paddle(&ball, paddleX, PADDLE_W, PADDLE_Y, BALL_SPEED_SLOW, -BALL_SPEED_FAST);
}

static inline void launchBall() {
  ball_launch(&ball, BALL_SPEED_SLOW, -BALL_SPEED_FAST);
}

static inline void applyPaddleBounceAngle() {
  int hit = constrain(bx - paddleX, 0, PADDLE_W - 1);
  int zone = (hit * BALL_PADDLE_BOUNCE_ZONES) / PADDLE_W;
  zone = constrain(zone, 0, BALL_PADDLE_BOUNCE_ZONES - 1);
  setBallVelocity(kPaddleBounceVel[zone].dx, kPaddleBounceVel[zone].dy);
}

static inline void markHudDirty() {
  dirty.add(0, 0, gfx.width() - 1, HUD_H - 1);
}

static void updateHudCache() {
  for (int i = 0; i < HUD_LIVES_SLOTS; i++) {
    hudLivesText[i] = (i < lives) ? '*' : ' ';
  }
  hudLivesText[HUD_LIVES_SLOTS] = '\0';

  snprintf(hudScoreText, sizeof(hudScoreText), "%*lu", HUD_SCORE_CHARS, (unsigned long)score);
  hudScoreX = gfx.width() - HUD_MARGIN_X - Font5x7::textWidth(hudScoreText, HUD_FONT_SCALE);
}

static void resetGame();
static void flushDirty();

static void fillRect565(int x0, int y0, int w, int h, uint16_t color565) {
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
      for (int i = 0; i < n; i++) regionbuf[i] = color565;
      gfx.blit565(x0 + tx, y0 + ty, ww, hh, regionbuf);
    }
  }
}

static void drawGameOverScreen() {
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

  Font5x7::drawCenteredText(gfx.width(), 44, "GAME OVER", 4, accent, fillRect565);
  Font5x7::drawCenteredText(gfx.width(), 96, "SCORE", 3, textc, fillRect565);
  Font5x7::drawCenteredText(gfx.width(), 128, scoreBuf, 6, scorec, fillRect565);
  Font5x7::drawCenteredText(gfx.width(), 190, "PRESS FIRE", 2, textc, fillRect565);
}

static void enterGameOver() {
  gameOver = true;
  gameOverScore = score;
  drawGameOverScreen();
}

static void enterGameOverWithFade() {
  gfx.fadeOutBacklight(GAMEOVER_FADE_OUT_MS);
  enterGameOver();
  gfx.fadeInBacklight(GAMEOVER_FADE_IN_MS);
  gamelib_runtime_reset_clock();
}

static void resetGameFromGameOverWithFade() {
  gfx.fadeOutBacklight(GAMEOVER_EXIT_FADE_OUT_MS);
  resetGame();
  flushDirty();
  gfx.fadeInBacklight(GAMEOVER_EXIT_FADE_IN_MS);
  gamelib_runtime_reset_clock();
}

static void resetGame() {
  paddle_reset_centered(&paddle, gfx.width(), PADDLE_W);
  gamelib_runtime_reset_clock();
  lives = START_LIVES;
  score = 0;
  gameOver = false;
  gameOverScore = 0;
  updateHudCache();
  resetBricks();
  clearBrickFlashes();
  resetBallOnPaddle();
  dirty.clear();
  dirty.add(0, 0, gfx.width() - 1, gfx.height() - 1);
}

static inline bool insideBall(int x, int y) {
  int dx = x - bx, dy = y - by;
  return dx * dx + dy * dy <= br * br;
}

static inline bool circleRectHit(int cx, int cy, int r, int x0, int y0, int x1, int y1) {
  return gamelib_circle_rect_hit(cx, cy, r, x0, y0, x1, y1);
}

static inline bool paddleRoundedBodyAt(int x, int y) {
  if (y < PADDLE_Y || y >= PADDLE_Y + PADDLE_H || x < paddleX || x >= paddleX + PADDLE_W) return false;

  int lx = x - paddleX;
  int ly = y - PADDLE_Y;

  // Proste "round rect" dla małej wysokości: ścinamy skrajne piksele rogów.
  if ((ly == 0 || ly == PADDLE_H - 1) && (lx == 0 || lx == PADDLE_W - 1)) return false;
  return true;
}

static inline bool paddleShadowAt(int x, int y) {
  const int sx = paddleX + 1;
  const int sy = PADDLE_Y + 1;
  const int sw = PADDLE_W;
  const int sh = PADDLE_H;
  if (y < sy || y >= sy + sh || x < sx || x >= sx + sw) return false;

  int lx = x - sx;
  int ly = y - sy;
  if ((ly == 0 || ly == sh - 1) && (lx == 0 || lx == sw - 1)) return false;

  // Nie rysuj cienia tam, gdzie jest właściwa paletka.
  if (paddleRoundedBodyAt(x, y)) return false;
  return true;
}

// ====== Background sampling ======
static inline uint16_t bgAt(int x, int y) {
  const uint16_t black = FastILI9341::rgb565(0, 0, 0);
  const uint16_t hudLivesColor = FastILI9341::rgb565(255, 255, 255);
  const uint16_t hudScoreColor = FastILI9341::rgb565(255, 220, 120);
  const uint16_t paddleFace = FastILI9341::rgb565(196, 200, 208);
  const uint16_t paddleLight = FastILI9341::rgb565(232, 236, 244);
  const uint16_t paddleDark = FastILI9341::rgb565(130, 136, 146);
  const uint16_t paddleMidShadow = FastILI9341::rgb565(86, 92, 102);
  const uint16_t paddleShadow = FastILI9341::rgb565(28, 32, 38);

  if (y < HUD_H) {
    int ly = y - HUD_TEXT_Y;
    int lx = x - HUD_MARGIN_X;
    if (Font5x7::textPixel(hudLivesText, HUD_FONT_SCALE, lx, ly)) return hudLivesColor;
    if (Font5x7::textPixel(hudScoreText, HUD_FONT_SCALE, x - hudScoreX, ly)) return hudScoreColor;
    return black;
  }

  // PADDLE (lekko szara, zaokrąglone rogi, delikatny cień)
  if (paddleRoundedBodyAt(x, y)) {
    int lx = x - paddleX;
    int ly = y - PADDLE_Y;

    if (ly == 0 || lx == 0) return paddleLight;
    if (ly == PADDLE_H - 1 || lx == PADDLE_W - 1) return paddleDark;
    if (ly == PADDLE_H - 2 || lx == PADDLE_W - 2) return paddleMidShadow;
    return paddleFace;
  }
  if (paddleShadowAt(x, y)) {
    return paddleShadow;
  }

  // BRICKS
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

// Render dowolnego regionu do bufora regionbuf
static void renderRegionToBuffer(int x0, int y0, int w, int h) {
  const uint16_t ballc = FastILI9341::rgb565(255, 255, 255);

  for (int yy = 0; yy < h; yy++) {
    int y = y0 + yy;
    for (int xx = 0; xx < w; xx++) {
      int x = x0 + xx;
      uint16_t c = bgAt(x, y);
      if (insideBall(x, y)) c = ballc;
      regionbuf[yy * w + xx] = c;
    }
  }
}

// Flush: przerysuj wszystkie dirty rects
static void flushDirty() {
  dirty.clip(gfx.width(), gfx.height());
  dirty.mergeAll();

  for (int i = 0; i < dirty.count(); i++) {
    auto &r = dirty[i];
    int w = r.x1 - r.x0 + 1;
    int h = r.y1 - r.y0 + 1;

    // twarde ograniczenie bufora regionu: jeśli większe, tnij na kafle
    for (int ty = r.y0; ty <= r.y1; ty += MAX_RH) {
      int hh = min(MAX_RH, r.y1 - ty + 1);
      for (int tx = r.x0; tx <= r.x1; tx += MAX_RW) {
        int ww = min(MAX_RW, r.x1 - tx + 1);
        renderRegionToBuffer(tx, ty, ww, hh);
        gfx.blit565(tx, ty, ww, hh, regionbuf);
      }
    }
  }
  dirty.clear();
}

static void fullRedraw() {
  gfx.fillScreen565(FastILI9341::rgb565(0, 0, 0));
  dirty.add(0, 0, gfx.width() - 1, gfx.height() - 1);
  flushDirty();
}

// “snap angles”: deterministycznie eliminuje pętle 45°
static void snapAngles() {
  int sx = (bdx >= 0) ? 1 : -1;
  int sy = (bdy >= 0) ? 1 : -1;
  int ax = abs(bdx);
  int ay = abs(bdy);
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
  bdx = sx * ax;
  bdy = sy * ay;
}

void updatePaddle(float dtSec) {
  int dir = 0;
  if (digitalRead(sPinLeft) == LOW) dir--;
  if (digitalRead(sPinRight) == LOW) dir++;
  int old = paddleX;
  if (paddle_update_dt(&paddle, dir, paddleSpeed, dtSec, PADDLE_BASE_STEP_US, gfx.width(), PADDLE_W, &old)) {
    dirty.add(old - 2, PADDLE_Y - 2,
              old + PADDLE_W + 2, PADDLE_Y + PADDLE_H + 2);

    dirty.add(paddleX - 2, PADDLE_Y - 2,
              paddleX + PADDLE_W + 2, PADDLE_Y + PADDLE_H + 2);
  }
}

void arkanoid_setup(uint8_t pinLeft, uint8_t pinRight, uint8_t pinFire) {
  sPinLeft = pinLeft;
  sPinRight = pinRight;
  sPinFire = pinFire;

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
}

bool bricksRemaining() {
  for (int r = 0; r < BRICK_ROWS; r++) {
    if (brickmask[r] != 0) return true;
  }
  return false;
}

void arkanoid_physics(float frameDtSec) {
  bool firePressed = (digitalRead(sPinFire) == LOW);
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
  int oldx = bx, oldy = by;

  bool fell = false;
  if (ballAttached) {
    attachBallToPaddle();
    if (fireEdge) {
      launchBall();
    }
  } else {
    updateBallSpeedFromPot();
    bool resyncBallPos = false;

    // ruch
    stepBallWithPotSpeed(frameDtSec);

    // ściany
    if (bx - br < 0) {
      bx = br;
      bdx = -bdx;
      resyncBallPos = true;
    }
    if (bx + br >= gfx.width()) {
      bx = gfx.width() - br - 1;
      bdx = -bdx;
      resyncBallPos = true;
    }
    if (by - br < HUD_H) {
      by = HUD_H + br;
      bdy = -bdy;
      resyncBallPos = true;
    }

    // paddle collision
    if (bdy > 0 && by + br >= PADDLE_Y && by + br <= PADDLE_Y + PADDLE_H && bx >= paddleX && bx <= paddleX + PADDLE_W) {
      by = PADDLE_Y - br - 1;
      applyPaddleBounceAngle();
      resyncBallPos = true;
    }

    if (by + br >= gfx.height()) {
      lives--;
      if (lives <= 0) {
        enterGameOverWithFade();
        return;
      } else {
        updateHudCache();
        markHudDirty();
        // dodaj rect starej piłki, potem reset na paddle
        dirty.add(oldx - br - 3, oldy - br - 3, oldx + br + 3, oldy + br + 3);
        resetBallOnPaddle();
      }
      fell = true;
    }

    // kolizje z klockami (lokalne)
    if (!fell) {
      bool hit = false;

      int cx0 = max(0, (bx - br) / BRICK_W);
      int cx1 = min(BRICK_COLS - 1, (bx + br) / BRICK_W);
      int ry0 = max(0, (by - br - BRICK_Y0) / BRICK_H);
      int ry1 = min(BRICK_ROWS - 1, (by + br - BRICK_Y0) / BRICK_H);

      for (int r = ry0; r <= ry1 && !hit; r++) {
        for (int c = cx0; c <= cx1; c++) {
          if (!brickPresent(c, r)) continue;
          int x0 = c * BRICK_W;
          int y0 = BRICK_Y0 + r * BRICK_H;
          int x1 = x0 + BRICK_W - 1;
          int y1 = y0 + BRICK_H - 1;
          if (circleRectHit(bx, by, br, x0, y0, x1, y1)) {
            hit = true;

            bool prev_out_y = (oldy < y0 - br) || (oldy > y1 + br);
            if (prev_out_y) bdy = -bdy;
            else bdx = -bdx;

            snapAngles();

            brickClear(c, r);
            spawnBrickFlash(x0, y0, x1, y1, rowColor[r], rowColorLight[r]);
            score += SCORE_PER_BRICK;
            updateHudCache();
            markHudDirty();

            if (!bricksRemaining()) {
              resetBricks();
              clearBrickFlashes();

              // pełny redraw
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
      syncBallFixedFromInt();
    }
  }

  bool ballMoved = (bx != oldx) || (by != oldy);

  // dirty dla piłki: stara + nowa (tylko gdy pozycja się zmieniła albo był reset po spadnięciu)
  if (!fell && ballMoved) {
    dirty.add(oldx - br - 3, oldy - br - 3, oldx + br + 3, oldy + br + 3);
  }
  if (ballMoved || fell) {
    dirty.add(bx - br - 3, by - br - 3, bx + br + 3, by + br + 3);
  }

}

void arkanoid_process(float frameDtSec) {
  uint32_t frameDtUs = (uint32_t)(frameDtSec * 1000000.0f + 0.5f);
  flushDirty();
  advanceBrickFlashes(frameDtUs);
}
