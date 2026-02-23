#pragma once

#include <stdint.h>

class FastILI9341;
class DirtyRects;

static constexpr uint32_t ARKANOID_FRAME_DEFAULT_STEP_US = 10000u;
static constexpr uint32_t ARKANOID_FRAME_MAX_STEP_US = 30000u;

void arkanoid_setup(uint8_t pinLeft, uint8_t pinRight, uint8_t pinFire);
void arkanoid_physics(float dtSec);
void arkanoid_process(float dtSec);
