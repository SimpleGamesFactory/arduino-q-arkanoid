// Define SGF_HW_PRESET here or pass it via -DSGF_HW_PRESET=...
// Examples:
// #define SGF_HW_PRESET SGF_HW_PRESET_UNOQ_ILI9341_320X240
// #define SGF_HW_PRESET SGF_HW_PRESET_ESP32_ST7789_240X240

#include "vendor/sgf-hardware-presets/SGFHardwarePresets.h"
#include "ArkanoidGame.h"

#if SGF_HW_PRESET == SGF_HW_PRESET_UNOQ_ILI9341_320X240
static constexpr uint8_t ARKANOID_BALL_SPEED_POT_PIN = A5;
#elif SGF_HW_PRESET == SGF_HW_PRESET_ESP32_ST7789_240X240
static constexpr uint8_t ARKANOID_BALL_SPEED_POT_PIN = 0xFF;
#else
#error "Unsupported SGF_HW_PRESET for Arkanoid ball speed control"
#endif

auto hardware = SGFHardwareProfile::makeRuntime();
ArkanoidGame arkanoid(
  hardware.renderTarget(),
  hardware.screen(),
  hardware.profile,
  ARKANOID_BALL_SPEED_POT_PIN);

void setup() {
  hardware.display.setBacklight(0);
  hardware.display.begin(hardware.profile.display.spiHz);
  hardware.display.setRotation(hardware.profile.display.rotation);
  arkanoid.setup();
}

void loop() {
  arkanoid.loop();
}
