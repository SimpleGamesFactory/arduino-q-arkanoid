#include "PlayingScene.h"

#include <Arduino.h>

#include "ArkanoidGame.h"
#include "SGF/Collision.h"

PlayingScene::PlayingScene(ArkanoidGame& game) : game(game) {}

void PlayingScene::onPhysics(float delta) {
  int dir = 0;
  if (digitalRead(game.pinLeft) == LOW) {
    dir--;
  }
  if (digitalRead(game.pinRight) == LOW) {
    dir++;
  }
  game.paddle.velocityX = static_cast<float>(dir) * game.paddle.speedPxPerSec;
  auto& paddleSprite = game.sprites.sprite(0);
  auto& ballSprite = game.sprites.sprite(1);
  Paddle::MoveResult paddleMove = game.paddle.onPhysics(delta);
  if (paddleMove.moved) {
    int px0 = 0;
    int py0 = 0;
    int px1 = 0;
    int py1 = 0;
    int savedPaddleX = paddleSprite.x;
    paddleSprite.x = paddleMove.oldX;
    SpriteLayer::spriteBoundsPadded(paddleSprite, 2, &px0, &py0, &px1, &py1);
    game.dirty.add(px0, py0, px1, py1);
    paddleSprite.x = savedPaddleX;
    SpriteLayer::spriteBoundsPadded(paddleSprite, 2, &px0, &py0, &px1, &py1);
    game.dirty.add(px0, py0, px1, py1);
  }
  game.markBrickFlashesDirty();
  int oldx = game.ball.x;
  int oldy = game.ball.y;

  bool fell = false;
  if (game.ball.attached) {
    game.ball.attachToPaddle(game.paddle);
    if (game.fireAction.justPressed()) {
      game.ball.launch();
    }
  } else {
    game.ball.updateSpeedFromPot(
      analogRead(game.pinBallSpeedPot),
      game.ball.speedPotMinQ(),
      game.ball.speedPotMaxQ()
    );
    bool resyncBallPos = false;

    game.ball.onPhysics(delta);

    if (game.ball.x - game.ball.r < 0) {
      game.ball.x = game.ball.r;
      game.ball.dx = -game.ball.dx;
      resyncBallPos = true;
    }
    if (game.ball.x + game.ball.r >= game.gfx.width()) {
      game.ball.x = game.gfx.width() - game.ball.r - 1;
      game.ball.dx = -game.ball.dx;
      resyncBallPos = true;
    }
    if (game.ball.y - game.ball.r < Hud::HEIGHT) {
      game.ball.y = Hud::HEIGHT + game.ball.r;
      game.ball.dy = -game.ball.dy;
      resyncBallPos = true;
    }

    if (game.ball.dy > 0 &&
        game.ball.y + game.ball.r >= game.paddle.y &&
        game.ball.y + game.ball.r <= game.paddle.y + game.paddle.h &&
        game.ball.x >= game.paddle.x &&
        game.ball.x <= game.paddle.x + game.paddle.w) {
      game.ball.y = game.paddle.y - game.ball.r - 1;
      game.ball.bounceFromPaddleHit(game.ball.x - game.paddle.x, game.paddle.w);
      resyncBallPos = true;
    }

    if (game.ball.y + game.ball.r >= game.gfx.height()) {
      game.lives--;
      if (game.lives <= 0) {
        game.gfx.fadeOutBacklight(game.GAMEOVER_FADE_OUT_MS);
        game.gameOverScore = game.score;
        game.sceneSwitcher.switchTo(game.gameOverScene);
        game.gfx.fadeInBacklight(game.GAMEOVER_FADE_IN_MS);
        game.resetClock();
        return;
      } else {
        game.hud.update(game.lives, game.score, game.gfx.width());
        game.hud.markDirty(game.gfx.width());
        game.dirty.add(
          oldx - game.ball.r - 3,
          oldy - game.ball.r - 3,
          oldx + game.ball.r + 3,
          oldy + game.ball.r + 3
        );
        game.ball.resetOnPaddle(game.paddle);
      }
      fell = true;
    }

    if (!fell) {
      bool hit = false;

      int cx0 = max(0, (game.ball.x - game.ball.r) / game.BRICK_W);
      int cx1 = min(game.BRICK_COLS - 1, (game.ball.x + game.ball.r) / game.BRICK_W);
      int ry0 = max(0, (game.ball.y - game.ball.r - game.BRICK_Y0) / game.BRICK_H);
      int ry1 = min(game.BRICK_ROWS - 1, (game.ball.y + game.ball.r - game.BRICK_Y0) / game.BRICK_H);

      for (int r = ry0; r <= ry1 && !hit; r++) {
        for (int c = cx0; c <= cx1; c++) {
          if (!game.brickPresent(c, r)) {
            continue;
          }
          int x0 = c * game.BRICK_W;
          int y0 = game.BRICK_Y0 + r * game.BRICK_H;
          int x1 = x0 + game.BRICK_W - 1;
          int y1 = y0 + game.BRICK_H - 1;
          if (::circleRectHit(game.ball.x, game.ball.y, game.ball.r, x0, y0, x1, y1)) {
            hit = true;

            bool prevOutY =
              (oldy < y0 - game.ball.r) || (oldy > y1 + game.ball.r);
            if (prevOutY) {
              game.ball.dy = -game.ball.dy;
            } else {
              game.ball.dx = -game.ball.dx;
            }

            game.ball.snapAngles();

            game.brickClear(c, r);
            game.spawnBrickFlash(x0, y0, x1, y1, game.rowColor[r], game.rowColorLight[r]);
            game.score += game.SCORE_PER_BRICK;
            game.hud.update(game.lives, game.score, game.gfx.width());
            game.hud.markDirty(game.gfx.width());

            if (!game.bricksRemaining()) {
              game.resetBricks();
              game.clearBrickFlashes();

              game.invalidateScreen();
            }

            game.dirty.add(x0 - 2, y0 - 2, x1 + 2, y1 + 2);
            break;
          }
        }
      }
    }

    if (resyncBallPos) {
      game.ball.syncFixedFromInt();
    }
  }

  bool ballMoved = (game.ball.x != oldx) || (game.ball.y != oldy);
  int bx0 = 0;
  int by0 = 0;
  int bx1 = 0;
  int by1 = 0;

  if (!fell && ballMoved) {
    SpriteLayer::Sprite oldBallSprite = ballSprite;
    oldBallSprite.active = true;
    oldBallSprite.setPosition(oldx, oldy);
    SpriteLayer::spriteBoundsPadded(oldBallSprite, 3, &bx0, &by0, &bx1, &by1);
    game.dirty.add(bx0, by0, bx1, by1);
  }
  if (ballMoved || fell) {
    game.ball.updateSprite(ballSprite);
    SpriteLayer::spriteBoundsPadded(ballSprite, 3, &bx0, &by0, &bx1, &by1);
    game.dirty.add(bx0, by0, bx1, by1);
  }

  game.paddle.updateSprite(paddleSprite);
  game.ball.updateSprite(ballSprite);
}

void PlayingScene::onProcess(float delta) {
  uint32_t frameDtUs = (uint32_t)(delta * 1000000.0f + 0.5f);
  game.flushDirty();
  game.advanceBrickFlashes(frameDtUs);
}
