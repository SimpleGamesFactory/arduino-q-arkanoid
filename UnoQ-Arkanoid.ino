#include "FastILI9341.h"
#include "DirtyRects.h"
#include "Font5x7.h"
#include <stdio.h>

#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8
#define TFT_LED D6

#define PIN_LEFT 2
#define PIN_RIGHT 3
#define PIN_FIRE D4

// MADCTL: dobierz pod orientację
// typowe zestawy: 0xE8, 0x48, 0x28, 0x88
static constexpr uint8_t MADCTL = 0xE8;

FastILI9341 gfx(TFT_CS, TFT_DC, TFT_RST, TFT_LED);
DirtyRects dirty;

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

static int paddleX = (320 - PADDLE_W) / 2;
static int paddleSpeed = 5;
static constexpr uint32_t PADDLE_BASE_STEP_US = 10000;  // referencja starego delay(10)
static float paddleXf = (float)((320 - PADDLE_W) / 2);
static uint32_t frameLastUs = 0;
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

struct BrickFlash {
  bool active;
  uint32_t remUs;
  uint32_t totalUs;
  int x0, y0, x1, y1;
  uint16_t baseColor;
  uint16_t lightColor;
};

static BrickFlash brickFlashes[BRICK_FLASH_SLOTS];

static inline uint16_t lighten565(uint16_t c) {
  int r = (c >> 11) & 0x1F;
  int g = (c >> 5) & 0x3F;
  int b = c & 0x1F;
  r = min(31, r + max(1, r / 3));
  g = min(63, g + max(1, g / 3));
  b = min(31, b + max(1, b / 3));
  return (uint16_t)((r << 11) | (g << 5) | b);
}

static inline uint16_t darken565(uint16_t c) {
  int r = (c >> 11) & 0x1F;
  int g = (c >> 5) & 0x3F;
  int b = c & 0x1F;
  r = (r * 2) / 3;
  g = (g * 2) / 3;
  b = (b * 2) / 3;
  return (uint16_t)((r << 11) | (g << 5) | b);
}

static void rebuildBrickShades() {
  for (int r = 0; r < BRICK_ROWS; r++) {
    rowColorLight[r] = lighten565(rowColor[r]);
    rowColorDark[r] = darken565(rowColor[r]);
  }
}

static void clearBrickFlashes() {
  for (int i = 0; i < BRICK_FLASH_SLOTS; i++) brickFlashes[i].active = false;
}

static void spawnBrickFlashWithDurationUs(int x0, int y0, int x1, int y1, uint32_t durationUs, uint16_t baseColor, uint16_t lightColor) {
  int slot = -1;
  for (int i = 0; i < BRICK_FLASH_SLOTS; i++) {
    if (!brickFlashes[i].active) {
      slot = i;
      break;
    }
  }
  if (slot < 0) slot = 0;  // fallback: nadpisz najstarszy slot[0]

  brickFlashes[slot].active = true;
  brickFlashes[slot].remUs = durationUs;
  brickFlashes[slot].totalUs = durationUs;
  brickFlashes[slot].x0 = x0;
  brickFlashes[slot].y0 = y0;
  brickFlashes[slot].x1 = x1;
  brickFlashes[slot].y1 = y1;
  brickFlashes[slot].baseColor = baseColor;
  brickFlashes[slot].lightColor = lightColor;
}

static inline void spawnBrickFlash(int x0, int y0, int x1, int y1, uint16_t baseColor, uint16_t lightColor) {
  spawnBrickFlashWithDurationUs(x0, y0, x1, y1, BRICK_FLASH_TOTAL_US, baseColor, lightColor);
}

static inline uint16_t brickFlashColorAt(int x, int y) {
  for (int i = 0; i < BRICK_FLASH_SLOTS; i++) {
    const BrickFlash &f = brickFlashes[i];
    if (!f.active) continue;
    if (x < f.x0 || x > f.x1 || y < f.y0 || y > f.y1) continue;

    if (f.totalUs == 0) return f.baseColor;
    uint64_t rem4 = (uint64_t)f.remUs * 4u;
    uint64_t tot = (uint64_t)f.totalUs;
    if (rem4 > tot * 3u) return FastILI9341::rgb565(255, 255, 255);
    if (rem4 > tot * 2u) return FastILI9341::rgb565(255, 245, 180);
    if (rem4 > tot * 1u) return f.lightColor;
    return f.baseColor;
  }
  return 0;
}

static void markBrickFlashesDirty() {
  for (int i = 0; i < BRICK_FLASH_SLOTS; i++) {
    if (!brickFlashes[i].active) continue;
    dirty.add(brickFlashes[i].x0 - 1, brickFlashes[i].y0 - 1, brickFlashes[i].x1 + 1, brickFlashes[i].y1 + 1);
  }
}

static void advanceBrickFlashes(uint32_t dtUs) {
  if (dtUs == 0) return;
  for (int i = 0; i < BRICK_FLASH_SLOTS; i++) {
    if (!brickFlashes[i].active) continue;
    if (brickFlashes[i].remUs > dtUs) {
      brickFlashes[i].remUs -= dtUs;
      continue;
    }
    if (brickFlashes[i].remUs > 0) {
      brickFlashes[i].remUs = 0;
    }
    if (brickFlashes[i].remUs == 0) {
      // Po wygaśnięciu trzeba przerysować obszar, inaczej zostaje "duch" flasha.
      dirty.add(brickFlashes[i].x0 - 1, brickFlashes[i].y0 - 1, brickFlashes[i].x1 + 1, brickFlashes[i].y1 + 1);
      brickFlashes[i].active = false;
    }
  }
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

static int bx = 60, by = 120;
static int bdx = BALL_SPEED_SLOW, bdy = -BALL_SPEED_FAST;
static int br = BALL_R;
static bool ballAttached = true;
static bool prevFirePressed = false;
static float ballFx = 0.0f;
static float ballFy = 0.0f;
static int32_t ballSpeedScaleQ = (int32_t)BALL_SPEED_FAST << BALL_POS_FP_SHIFT;  // actual speed in Q8
static int32_t ballSpeedPotFiltQ = -1;  // filtered analogRead(A5) in Q4

static inline void syncBallFixedFromInt() {
  ballFx = (float)bx;
  ballFy = (float)by;
}

static inline float readFrameDtSec() {
  uint32_t nowUs = micros();
  if (frameLastUs == 0) {
    frameLastUs = nowUs;
    return (float)BALL_BASE_STEP_US / 1000000.0f;
  }

  uint32_t dtUs = nowUs - frameLastUs;  // wrap-safe for uint32_t
  frameLastUs = nowUs;

  // clamp tylko górny (duże hitch'e); dolnego nie clampujemy, bo bez delay()
  // zawyżał prędkość piłki na szybkich klatkach i robił "nierówny timing".
  if (dtUs > 30000u) dtUs = 30000u;

  return (float)dtUs / 1000000.0f;
}

static inline void updateBallSpeedFromPot() {
  int raw = analogRead(A5);  // 0..1023
  int32_t rawQ = (int32_t)raw << 4;

  if (ballSpeedPotFiltQ < 0) {
    ballSpeedPotFiltQ = rawQ;
  } else {
    // lekkie wygładzenie, żeby potencjometr nie szarpał prędkością
    ballSpeedPotFiltQ += (rawQ - ballSpeedPotFiltQ) >> 3;
  }

  int32_t rawSmooth = ballSpeedPotFiltQ >> 4;  // z powrotem 0..1023
  int32_t minQ = POT_BALL_SPEED_MIN_Q;
  int32_t rangeQ = POT_BALL_SPEED_MAX_Q - POT_BALL_SPEED_MIN_Q;
  ballSpeedScaleQ = minQ + (rangeQ * rawSmooth) / 1023;
}

static inline void stepBallWithPotSpeed(float dtSec) {
  // Bazowy wektor (bdx/bdy) jest w px/10ms przy speed=BALL_SPEED_FAST.
  float speedPxPerStep = (float)ballSpeedScaleQ / (float)BALL_POS_FP_ONE;  // px / 10ms
  float stepScale = (speedPxPerStep / (float)BALL_SPEED_FAST) * (dtSec * (1000000.0f / (float)BALL_BASE_STEP_US));
  ballFx += (float)bdx * stepScale;
  ballFy += (float)bdy * stepScale;
  bx = (int)(ballFx + 0.5f);
  by = (int)(ballFy + 0.5f);
}

static inline void setBallVelocity(int dx, int dy) {
  bdx = dx;
  bdy = dy;
}

static inline void attachBallToPaddle() {
  bx = paddleX + PADDLE_W / 2;
  by = PADDLE_Y - br - 1;
  syncBallFixedFromInt();
}

static inline void resetBallOnPaddle() {
  ballAttached = true;
  setBallVelocity(BALL_SPEED_SLOW, -BALL_SPEED_FAST);
  attachBallToPaddle();
}

static inline void launchBall() {
  ballAttached = false;
  setBallVelocity(BALL_SPEED_SLOW, -BALL_SPEED_FAST);
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

static void resetGame() {
  paddleX = (gfx.width() - PADDLE_W) / 2;
  paddleXf = (float)paddleX;
  frameLastUs = micros();
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
  int nx = cx;
  if (nx < x0) nx = x0;
  else if (nx > x1) nx = x1;
  int ny = cy;
  if (ny < y0) ny = y0;
  else if (ny > y1) ny = y1;
  int dx = cx - nx, dy = cy - ny;
  return dx * dx + dy * dy <= r * r;
}

// ====== Background sampling ======
static inline uint16_t bgAt(int x, int y) {
  const uint16_t black = FastILI9341::rgb565(0, 0, 0);
  const uint16_t hudLivesColor = FastILI9341::rgb565(255, 255, 255);
  const uint16_t hudScoreColor = FastILI9341::rgb565(255, 220, 120);

  if (y < HUD_H) {
    int ly = y - HUD_TEXT_Y;
    int lx = x - HUD_MARGIN_X;
    if (Font5x7::textPixel(hudLivesText, HUD_FONT_SCALE, lx, ly)) return hudLivesColor;
    if (Font5x7::textPixel(hudScoreText, HUD_FONT_SCALE, x - hudScoreX, ly)) return hudScoreColor;
    return black;
  }

  // PADDLE
  if (y >= PADDLE_Y && y < PADDLE_Y + PADDLE_H && x >= paddleX && x < paddleX + PADDLE_W) {
    return FastILI9341::rgb565(255, 255, 255);
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
  int old = paddleX;
  int dir = 0;
  if (digitalRead(PIN_LEFT) == LOW) dir--;
  if (digitalRead(PIN_RIGHT) == LOW) dir++;
  float dtSteps = dtSec * (1000000.0f / (float)PADDLE_BASE_STEP_US);  // 10ms steps
  paddleXf += (float)dir * (float)paddleSpeed * dtSteps;

  float minX = 0.0f;
  float maxX = (float)(gfx.width() - PADDLE_W);
  if (paddleXf < minX) paddleXf = minX;
  if (paddleXf > maxX) paddleXf = maxX;

  paddleX = (int)(paddleXf + 0.5f);

  if (old != paddleX) {
    dirty.add(old - 2, PADDLE_Y - 2,
              old + PADDLE_W + 2, PADDLE_Y + PADDLE_H + 2);

    dirty.add(paddleX - 2, PADDLE_Y - 2,
              paddleX + PADDLE_W + 2, PADDLE_Y + PADDLE_H + 2);
  }
}

void setup() {
  pinMode(PIN_LEFT, INPUT_PULLUP);
  pinMode(PIN_RIGHT, INPUT_PULLUP);
  pinMode(PIN_FIRE, INPUT_PULLUP);

  rowColor[0] = FastILI9341::rgb565(255, 0, 0);
  rowColor[1] = FastILI9341::rgb565(255, 128, 0);
  rowColor[2] = FastILI9341::rgb565(255, 255, 0);
  rowColor[3] = FastILI9341::rgb565(0, 255, 0);
  rowColor[4] = FastILI9341::rgb565(0, 255, 255);
  rowColor[5] = FastILI9341::rgb565(0, 128, 255);
  rowColor[6] = FastILI9341::rgb565(0, 0, 255);
  rowColor[7] = FastILI9341::rgb565(255, 0, 255);
  rebuildBrickShades();

  bool ok = gfx.begin(24000000, MADCTL);
  if (!ok) {
    while (1) delay(1000);
  }

  resetGame();
  flushDirty();
}

bool bricksRemaining() {
  for (int r = 0; r < BRICK_ROWS; r++) {
    if (brickmask[r] != 0) return true;
  }
  return false;
}

void loop() {
  float frameDtSec = readFrameDtSec();
  uint32_t frameDtUs = (uint32_t)(frameDtSec * 1000000.0f + 0.5f);
  bool firePressed = (digitalRead(PIN_FIRE) == LOW);
  bool fireEdge = firePressed && !prevFirePressed;
  bool fireReleaseEdge = !firePressed && prevFirePressed;
  prevFirePressed = firePressed;

  if (gameOver) {
    if (fireReleaseEdge) {
      resetGame();
      flushDirty();
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
        enterGameOver();
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

  flushDirty();
  advanceBrickFlashes(frameDtUs);
}
