#pragma once

#include <stdint.h>

#include "SGF/ActionBinding.h"
#include "SGF/ActionState.h"
#include "SGF/DebouncedInputPin.h"
#include "ArkanoidAudio.h"
#include "Ball.h"
#include "GameOverScene.h"
#include "PlayingScene.h"
#include "SGF/DirtyRects.h"
#include "SGF/FontRenderer.h"
#include "SGF/Game.h"
#include "SGF/HardwareProfile.h"
#include "SGF/IRenderTarget.h"
#include "SGF/IScreen.h"
#include "Hud.h"
#include "Paddle.h"
#include "SGF/RectFlashAnim.h"
#include "SGF/Renderer2D.h"
#if defined(ENABLE_PROFILER) && ENABLE_PROFILER
#include "SGF/SerialMonitor.h"
#endif
#include "TitleScene.h"

static constexpr uint32_t ARKANOID_FRAME_DEFAULT_STEP_US = 10000u;
static constexpr uint32_t ARKANOID_FRAME_MAX_STEP_US = 30000u;

class ArkanoidGame : public Game {
public:
  ArkanoidGame(
    IRenderTarget& renderTarget,
    IScreen& screen,
    const SGFHardware::HardwareProfile& hardwareProfile,
    uint8_t ballSpeedPotPin,
    uint8_t audioOutputPin
  );

  void setup();
  int screenWidth() const { return renderTarget.size().x; }
  int screenHeight() const { return renderTarget.size().y; }
  void fillScreen(uint16_t color565) { screen.fillScreen565(color565); }
  void fillRect(int x0, int y0, int w, int h, uint16_t color565) {
    screen.fillRect565(x0, y0, w, h, color565);
  }
  void drawCenteredText(int y, const char* text, int scale, uint16_t color565);
  void fadeBacklightTo(uint8_t targetLevel, uint16_t durationMs);
  void fadeInBacklight(uint16_t durationMs) {
    fadeBacklightTo(hardwareProfile.display.backlightLevel, durationMs);
  }
  void fadeOutBacklight(uint16_t durationMs) {
    fadeBacklightTo(0, durationMs);
  }

private:
  static constexpr uint8_t BALL_SPEED_CONTROL_DISABLED_PIN = 0xFF;

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
  static constexpr uint16_t TITLE_EXIT_FADE_OUT_MS = 140;
  static constexpr uint16_t TITLE_EXIT_FADE_IN_MS = 180;
  static constexpr uint16_t GAMEOVER_FADE_OUT_MS = 140;
  static constexpr uint16_t GAMEOVER_FADE_IN_MS = 180;
  static constexpr uint16_t GAMEOVER_EXIT_FADE_OUT_MS = 140;
  static constexpr uint16_t GAMEOVER_EXIT_FADE_IN_MS = 180;

  IRenderTarget& renderTarget;
  IScreen& screen;
  SGFHardware::HardwareProfile hardwareProfile;
  DirtyRects dirty;

  uint8_t pinLeft = 0;
  uint8_t pinRight = 0;
  uint8_t pinFire = 0;
  uint8_t pinBallSpeedPot = 0;
  ArkanoidAudio audio;
  DebouncedInputPin fireInput;
  ActionBinding actionBindings[1];

  uint16_t regionBuf[MAX_RW * MAX_RH]{};

  Paddle paddle{};
  int lives = START_LIVES;
  uint32_t score = 0;
  uint32_t gameOverScore = 0;
  ActionState fireAction;

  Ball ball{};

  uint32_t brickmask[BRICK_ROWS]{};
  uint16_t rowColor[BRICK_ROWS]{};
  uint16_t rowColorLight[BRICK_ROWS]{};
  uint16_t rowColorDark[BRICK_ROWS]{};
  RectFlashAnimSlot brickFlashSlots[BRICK_FLASH_SLOTS]{};
  RectFlashAnim brickFlashAnim;
  Hud hud;
#if defined(ENABLE_PROFILER) && ENABLE_PROFILER
  SerialMonitor serialMonitor;
#endif
  Renderer2D renderer;
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

  void resetGame();
  void invalidateScreen();
  void updateBallSpeedControl();
  void setGameplaySpritesVisible(bool visible);
  void transitionFromTitleToPlaying();
  void transitionToGameOver();
  void transitionFromGameOverToPlaying();

  uint16_t bgAt(int x, int y) const;
  void renderBackgroundToBuffer(int x0, int y0, int w, int h, uint16_t* buf);
  void flushDirty();
};
