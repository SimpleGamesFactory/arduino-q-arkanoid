#pragma once

#include <stdint.h>

#include "SGF/Scene.h"

class ArkanoidGame;

class GameOverScene : public Scene {
public:
  explicit GameOverScene(ArkanoidGame& game);

  void onEnter() override;
  void onExit() override;
  void onPhysics(float delta) override;
  void onProcess(float delta) override;
  void renderToBuffer(int x0, int y0, int w, int h, uint16_t* buf) const;

private:
  ArkanoidGame& game;
};
