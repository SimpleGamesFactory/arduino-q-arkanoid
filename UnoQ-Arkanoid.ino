#include "SGF/FastILI9341.h"
#include "Arkanoid/ArkanoidGame.h"

#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8
#define TFT_LED D6

#define PIN_LEFT 2
#define PIN_RIGHT 3
#define PIN_FIRE D4

FastILI9341 gfx(TFT_CS, TFT_DC, TFT_RST, TFT_LED);
ArkanoidGame arkanoid(gfx, PIN_LEFT, PIN_RIGHT, PIN_FIRE);

void setup() {
  arkanoid.setup();
}

void loop() {
  arkanoid.loop();
}
