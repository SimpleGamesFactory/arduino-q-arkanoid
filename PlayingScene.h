#pragma once

#include "SGF/Scene.h"

class ArkanoidGame;

class PlayingScene : public Scene {
public:
  explicit PlayingScene(ArkanoidGame& game);

  void onPhysics(float delta) override;
  void onProcess(float delta) override;

private:
  ArkanoidGame& game;
};
