#pragma once

#include <stdint.h>

void gamelib_runtime_setup(uint32_t defaultStepUs, uint32_t maxStepUs);
void gamelib_runtime_loop(void);
void gamelib_runtime_reset_clock(void);
