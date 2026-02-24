#pragma once

#include <stdint.h>

#include "Ball.h"
#include "GameOverScene.h"
#include "PlayingScene.h"
#include "SGF/DirtyRects.h"
#include "SGF/FastILI9341.h"
#include "SGF/Actions.h"
#include "SGF/Game.h"
#include "SGF/Scene.h"
#include "Hud.h"
#include "Paddle.h"
#include "SGF/RectFlashAnim.h"
#include "SGF/TileFlusher.h"
#include "SGF/IRenderTarget.h"
#include "SGF/Sprites.h"
#include "TitleScene.h"

static constexpr uint32_t ARKANOID_FRAME_DEFAULT_STEP_US = 10000u;
static constexpr uint32_t ARKANOID_FRAME_MAX_STEP_US = 30000u;

class ArkanoidGame : public Game {
public:
  ArkanoidGame(
    FastILI9341& gfx,
    uint8_t leftPin,
    uint8_t rightPin,
    uint8_t firePin,
    uint8_t ballSpeedPotPin
  );

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
  static constexpr int START_LIVES = 3;
  static constexpr uint16_t SCORE_PER_BRICK = 10;

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
  uint8_t pinBallSpeedPot = 0;

  uint16_t regionBuf[MAX_RW * MAX_RH]{};
  uint16_t paddleSpritePixels[Paddle::DEFAULT_W * Paddle::DEFAULT_H]{};
  static constexpr int BALL_SPRITE_SIZE = BALL_R * 2 + 1;
  uint16_t ballSpritePixels[BALL_SPRITE_SIZE * BALL_SPRITE_SIZE]{};

  Paddle paddle{};
  int lives = START_LIVES;
  uint32_t score = 0;
  uint32_t gameOverScore = 0;
  DigitalAction fireAction;
  PressReleaseAction fireConfirmAction;

  Ball ball{BALL_R, BALL_SPEED_SLOW, BALL_SPEED_FAST, BALL_POS_FP_ONE};

  uint32_t brickmask[BRICK_ROWS]{};
  uint16_t rowColor[BRICK_ROWS]{};
  uint16_t rowColorLight[BRICK_ROWS]{};
  uint16_t rowColorDark[BRICK_ROWS]{};
  RectFlashAnimSlot brickFlashSlots[BRICK_FLASH_SLOTS]{};
  RectFlashAnim brickFlashAnim;
  Hud hud;
  TileFlusher flusher;
  SpriteLayer sprites;
  SceneSwitcher sceneSwitcher;
  TitleScene titleScene;
  PlayingScene playingScene;
  GameOverScene gameOverScene;

  friend class TitleScene;
  friend class PlayingScene;
  friend class GameOverScene;

  void onSetup() override;
  void onPhysics(float delta) override;
  void onProcess(float delta) override;

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
  void resetGame();

  uint16_t bgAt(int x, int y) const;
  void renderRegionToBuffer(int x0, int y0, int w, int h, uint16_t* buf);
  void flushDirty();
  void buildSprites();
  void updateSpriteLayer();
};
