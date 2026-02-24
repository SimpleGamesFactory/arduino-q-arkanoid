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
  Paddle::Position paddlePos = game.paddle.getPosition();
  Paddle::MoveResult paddleMove = game.paddle.onPhysics(delta);
  paddlePos = game.paddle.getPosition();
  if (paddleMove.moved) {
    int px0 = 0;
    int py0 = 0;
    int px1 = 0;
    int py1 = 0;
    SpriteLayer::Sprite oldPaddleSprite = paddleSprite;
    oldPaddleSprite.active = true;
    oldPaddleSprite.setPosition(paddleMove.oldX, paddlePos.y);
    SpriteLayer::spriteBoundsPadded(oldPaddleSprite, 2, &px0, &py0, &px1, &py1);
    game.dirty.add(px0, py0, px1, py1);
    SpriteLayer::Sprite newPaddleSprite = paddleSprite;
    newPaddleSprite.active = true;
    newPaddleSprite.setPosition(paddlePos.x, paddlePos.y);
    SpriteLayer::spriteBoundsPadded(newPaddleSprite, 2, &px0, &py0, &px1, &py1);
    game.dirty.add(px0, py0, px1, py1);
  }
  game.markBrickFlashesDirty();
  Ball::Position ballPos = game.ball.getPosition();
  int oldx = ballPos.x;
  int oldy = ballPos.y;

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

    ballPos = game.ball.getPosition();
    if (ballPos.x - game.ball.r < 0) {
      game.ball.setX(game.ball.r);
      game.ball.dx = -game.ball.dx;
      resyncBallPos = true;
    }
    ballPos = game.ball.getPosition();
    if (ballPos.x + game.ball.r >= game.gfx.width()) {
      game.ball.setX(game.gfx.width() - game.ball.r - 1);
      game.ball.dx = -game.ball.dx;
      resyncBallPos = true;
    }
    ballPos = game.ball.getPosition();
    if (ballPos.y - game.ball.r < Hud::HEIGHT) {
      game.ball.setY(Hud::HEIGHT + game.ball.r);
      game.ball.dy = -game.ball.dy;
      resyncBallPos = true;
    }

    ballPos = game.ball.getPosition();
    if (game.ball.dy > 0 &&
        ballPos.y + game.ball.r >= paddlePos.y &&
        ballPos.y + game.ball.r <= paddlePos.y + game.paddle.h &&
        ballPos.x >= paddlePos.x &&
        ballPos.x <= paddlePos.x + game.paddle.w) {
      game.ball.setY(paddlePos.y - game.ball.r - 1);
      game.ball.bounceFromPaddleHit(ballPos.x - paddlePos.x, game.paddle.w);
      resyncBallPos = true;
    }

    ballPos = game.ball.getPosition();
    if (ballPos.y + game.ball.r >= game.gfx.height()) {
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

      ballPos = game.ball.getPosition();
      int cx0 = max(0, (ballPos.x - game.ball.r) / game.BRICK_W);
      int cx1 = min(game.BRICK_COLS - 1, (ballPos.x + game.ball.r) / game.BRICK_W);
      int ry0 = max(0, (ballPos.y - game.ball.r - game.BRICK_Y0) / game.BRICK_H);
      int ry1 = min(game.BRICK_ROWS - 1, (ballPos.y + game.ball.r - game.BRICK_Y0) / game.BRICK_H);

      for (int r = ry0; r <= ry1 && !hit; r++) {
        for (int c = cx0; c <= cx1; c++) {
          if (!game.brickPresent(c, r)) {
            continue;
          }
          int x0 = c * game.BRICK_W;
          int y0 = game.BRICK_Y0 + r * game.BRICK_H;
          int x1 = x0 + game.BRICK_W - 1;
          int y1 = y0 + game.BRICK_H - 1;
          ballPos = game.ball.getPosition();
          if (::circleRectHit(ballPos.x, ballPos.y, game.ball.r, x0, y0, x1, y1)) {
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

  ballPos = game.ball.getPosition();
  bool ballMoved = (ballPos.x != oldx) || (ballPos.y != oldy);
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
    SpriteLayer::Sprite newBallSprite = ballSprite;
    newBallSprite.active = true;
    newBallSprite.setPosition(ballPos.x, ballPos.y);
    SpriteLayer::spriteBoundsPadded(newBallSprite, 3, &bx0, &by0, &bx1, &by1);
    game.dirty.add(bx0, by0, bx1, by1);
  }
}

void PlayingScene::onProcess(float delta) {
  uint32_t frameDtUs = (uint32_t)(delta * 1000000.0f + 0.5f);
  game.flushDirty();
  game.advanceBrickFlashes(frameDtUs);
}
