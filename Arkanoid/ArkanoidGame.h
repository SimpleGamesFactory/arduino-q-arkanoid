#pragma once

#include <stdint.h>

#include "Ball.h"
#include "SGF/DirtyRects.h"
#include "SGF/FastILI9341.h"
#include "SGF/Game.h"
#include "Hud.h"
#include "Paddle.h"
#include "SGF/RectFlashAnim.h"
#include "SGF/TileFlusher.h"
#include "SGF/IRenderTarget.h"

static constexpr uint32_t ARKANOID_FRAME_DEFAULT_STEP_US = 10000u;
static constexpr uint32_t ARKANOID_FRAME_MAX_STEP_US = 30000u;

class ArkanoidGame : public Game {
public:
  ArkanoidGame(FastILI9341& gfx, uint8_t leftPin, uint8_t rightPin, uint8_t firePin);

  void setup();

private:
  static constexpr uint32_t DEFAULT_SPI_HZ = 24000000u;
  static constexpr FastILI9341::ScreenRotation DEFAULT_ROTATION = FastILI9341::ScreenRotation::Landscape;
  static constexpr uint32_t BACKLIGHT_PWM_MAX = 4095u;
  static constexpr uint8_t BACKLIGHT_START = 0;

  // Render region parameters
  static constexpr int MAX_RW = 120;
  static constexpr int MAX_RH = 80;

  // Paddle
  static constexpr int PADDLE_W = 60;
  static constexpr int PADDLE_H = 8;
  static constexpr int PADDLE_Y = 220;
  static constexpr int START_LIVES = 3;
  static constexpr uint16_t SCORE_PER_BRICK = 10;
  static constexpr uint32_t PADDLE_BASE_STEP_US = 10000;

  // Bricks
  static constexpr int BRICK_W = 20;
  static constexpr int BRICK_H = 12;
  static constexpr int BRICK_COLS = 16;  // 320 / BRICK_W
  static constexpr int BRICK_ROWS = 8;
  static constexpr int BRICK_Y0 = Hud::HEIGHT + 10;
  static constexpr int BRICK_FLASH_SLOTS = 40;
  static constexpr uint32_t BRICK_FLASH_TOTAL_US = 50000;
  static constexpr uint16_t START_FADE_IN_MS = 220;
  static constexpr uint16_t GAMEOVER_FADE_OUT_MS = 140;
  static constexpr uint16_t GAMEOVER_FADE_IN_MS = 180;
  static constexpr uint16_t GAMEOVER_EXIT_FADE_OUT_MS = 140;
  static constexpr uint16_t GAMEOVER_EXIT_FADE_IN_MS = 180;

  // Ball
  static constexpr int BALL_R = 6;
  static constexpr int BALL_SPEED_SLOW = 1;
  static constexpr int BALL_SPEED_FAST = 3;
  static constexpr int BALL_PADDLE_BOUNCE_ZONES = 7;
  static constexpr int BALL_POS_FP_SHIFT = 8;
  static constexpr int BALL_POS_FP_ONE = 1 << BALL_POS_FP_SHIFT;
  static constexpr uint32_t BALL_BASE_STEP_US = 10000;
  static constexpr int32_t POT_BALL_SPEED_MIN_Q = BALL_POS_FP_ONE / 2;
  static constexpr int32_t POT_BALL_SPEED_MAX_Q = BALL_POS_FP_ONE * 5;

  struct BallVel {
    int dx;
    int dy;
  };
  static const BallVel kPaddleBounceVel[BALL_PADDLE_BOUNCE_ZONES];

  FastILI9341& gfx;
  DirtyRects dirty;

  uint8_t pinLeft = 0;
  uint8_t pinRight = 0;
  uint8_t pinFire = 0;

  uint16_t regionBuf[MAX_RW * MAX_RH]{};

  Paddle paddle{};
  int paddleSpeed = 5;
  int lives = START_LIVES;
  uint32_t score = 0;
  bool gameOver = false;
  uint32_t gameOverScore = 0;
  bool prevFirePressed = false;

  Ball ball{BALL_R, BALL_SPEED_SLOW, BALL_SPEED_FAST, BALL_POS_FP_ONE};

  uint32_t brickmask[BRICK_ROWS]{};
  uint16_t rowColor[BRICK_ROWS]{};
  uint16_t rowColorLight[BRICK_ROWS]{};
  uint16_t rowColorDark[BRICK_ROWS]{};
  RectFlashAnimSlot brickFlashSlots[BRICK_FLASH_SLOTS]{};
  RectFlashAnim brickFlashAnim;
  Hud hud;
  TileFlusher flusher;

  void onSetup() override;
  void onPhysics(float dtSec) override;
  void onProcess(float dtSec) override;

  void rebuildBrickShades();
  void clearBrickFlashes();
  void spawnBrickFlashWithDurationUs(int x0, int y0, int x1, int y1, uint32_t durationUs, uint16_t baseColor, uint16_t lightColor);
  void spawnBrickFlash(int x0, int y0, int x1, int y1, uint16_t baseColor, uint16_t lightColor);
  uint16_t brickFlashColorAt(int x, int y) const;
  void markBrickFlashesDirty();
  void advanceBrickFlashes(uint32_t dtUs);

  bool brickPresent(int c, int r) const;
  void brickClear(int c, int r);
  void resetBricks();
  bool bricksRemaining() const;

  void fillRect565(int x0, int y0, int w, int h, uint16_t color565);
  void drawText(int x, int y, const char* text, int scale, uint16_t color565);
  void drawCenteredText(int y, const char* text, int scale, uint16_t color565);
  void drawGameOverScreen();
  void enterGameOver();
  void resetGameFromGameOverWithFade();
  void resetGame();

  bool insideBall(int x, int y) const;
  bool circleRectHit(int cx, int cy, int r, int x0, int y0, int x1, int y1) const;
  bool paddleRoundedBodyAt(int x, int y) const;
  bool paddleShadowAt(int x, int y) const;

  uint16_t bgAt(int x, int y) const;
  void renderRegionToBuffer(int x0, int y0, int w, int h, uint16_t* buf);
  void flushDirty();
  void snapAngles();
  void updatePaddle(float dtSec);
  void applyPaddleBounceAngle();
};
