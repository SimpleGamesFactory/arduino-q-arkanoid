#include "FastILI9341.h"
#include "DirtyRects.h"

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


// Paddle

static constexpr int PADDLE_W = 60;
static constexpr int PADDLE_H = 8;
static constexpr int PADDLE_Y = 220;
static constexpr int START_LIVES = 3;
static constexpr uint16_t SCORE_PER_BRICK = 10;

static int paddleX = (320 - PADDLE_W) / 2;
static int paddleSpeed = 5;
static int lives = START_LIVES;
static uint32_t score = 0;



// ====== Bricks bitmask ======
static constexpr int BRICK_W = 20;
static constexpr int BRICK_H = 12;
static constexpr int BRICK_COLS = 320 / BRICK_W;  // 16
static constexpr int BRICK_ROWS = 8;
static constexpr int BRICK_Y0 = 20;



static uint32_t brickmask[BRICK_ROWS];

static uint16_t rowColor[BRICK_ROWS];

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
static constexpr int BALL_SPEED_SLOW = 2;
static constexpr int BALL_SPEED_FAST = 3;
static constexpr int BALL_PADDLE_BOUNCE_ZONES = 7;

struct BallVel {
  int dx;
  int dy;
};

static constexpr BallVel kPaddleBounceVel[BALL_PADDLE_BOUNCE_ZONES] = {
  { -BALL_SPEED_FAST, -BALL_SPEED_SLOW },
  { -BALL_SPEED_SLOW, -BALL_SPEED_FAST },
  { -1,               -BALL_SPEED_FAST },
  {  0,               -BALL_SPEED_FAST },
  {  1,               -BALL_SPEED_FAST },
  {  BALL_SPEED_SLOW, -BALL_SPEED_FAST },
  {  BALL_SPEED_FAST, -BALL_SPEED_SLOW },
};

static int bx = 60, by = 120;
static int bdx = BALL_SPEED_SLOW, bdy = -BALL_SPEED_FAST;
static int br = BALL_R;
static bool ballAttached = true;
static bool prevFirePressed = false;

static inline void setBallVelocity(int dx, int dy) {
  bdx = dx;
  bdy = dy;
}

static inline void attachBallToPaddle() {
  bx = paddleX + PADDLE_W / 2;
  by = PADDLE_Y - br - 1;
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

static void resetGame() {
  paddleX = (gfx.width() - PADDLE_W) / 2;
  lives = START_LIVES;
  score = 0;
  resetBricks();
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
static inline uint16_t bgAt(int x,int y)
{
  const uint16_t black = FastILI9341::rgb565(0,0,0);

  // PADDLE
  if (y >= PADDLE_Y && y < PADDLE_Y + PADDLE_H &&
      x >= paddleX && x < paddleX + PADDLE_W)
  {
    return FastILI9341::rgb565(255,255,255);
  }

  // BRICKS
  if (y >= BRICK_Y0)
  {
    int yy = y - BRICK_Y0;
    int row = yy / BRICK_H;

    if (row >= 0 && row < BRICK_ROWS)
    {
      int col = x / BRICK_W;

      if (col >= 0 && col < BRICK_COLS)
      {
        if (brickPresent(col,row))
          return rowColor[row];
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

void updatePaddle() {
  int old = paddleX;

  if (digitalRead(PIN_LEFT) == LOW)
    paddleX -= paddleSpeed;

  if (digitalRead(PIN_RIGHT) == LOW)
    paddleX += paddleSpeed;

  if (paddleX < 0)
    paddleX = 0;

  if (paddleX + PADDLE_W >= gfx.width())
    paddleX = gfx.width() - PADDLE_W;

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
  updatePaddle();
  int oldx = bx, oldy = by;
  bool firePressed = (digitalRead(PIN_FIRE) == LOW);
  bool fireEdge = firePressed && !prevFirePressed;
  prevFirePressed = firePressed;

  bool fell = false;
  if (ballAttached) {
    attachBallToPaddle();
    if (fireEdge) {
      launchBall();
    }
  } else {
    // ruch
    bx += bdx;
    by += bdy;

    // ściany
    if (bx - br < 0) {
      bx = br;
      bdx = -bdx;
    }
    if (bx + br >= gfx.width()) {
      bx = gfx.width() - br - 1;
      bdx = -bdx;
    }
    if (by - br < 0) {
      by = br;
      bdy = -bdy;
    }

    // paddle collision
    if (bdy > 0 && by + br >= PADDLE_Y && by + br <= PADDLE_Y + PADDLE_H && bx >= paddleX && bx <= paddleX + PADDLE_W) {
      by = PADDLE_Y - br - 1;
      applyPaddleBounceAngle();
    }

    if (by + br >= gfx.height()) {
      lives--;
      if (lives <= 0) {
        resetGame();
      } else {
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
            score += SCORE_PER_BRICK;

            if (!bricksRemaining()) {
              resetBricks();

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

  // FPS limit
  delay(10);
}
