#pragma once

#include "SGF/Scene.h"

class ArkanoidGame;

class GameOverScene : public Scene {
public:
  explicit GameOverScene(ArkanoidGame& game);

  void onEnter() override;
  void onPhysics(float delta) override;
  void onProcess(float delta) override;

private:
  ArkanoidGame& game;
};
