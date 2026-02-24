#pragma once

#include "SGF/Scene.h"

class ArkanoidGame;

class TitleScene : public Scene {
public:
  explicit TitleScene(ArkanoidGame& game);

  void onEnter() override;
  void onPhysics(float delta) override;
  void onProcess(float delta) override;

private:
  ArkanoidGame& game;
};
