#pragma once

#include <stdint.h>

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
#include "SGF/AudioTypes.h"
#include "SGF/Synth.h"
#include "SGF/ESP32DacAudioOutput.h"
#endif

class ArkanoidAudio {
public:
  static constexpr uint8_t AUDIO_DISABLED_PIN = 0xFF;

  explicit ArkanoidAudio(uint8_t outputPin);

  void setup();
  void playBlockHit();
  void playPaddleHit();
  void playBallOut();

private:
  bool enabled() const;

  uint8_t outputPin = AUDIO_DISABLED_PIN;
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
  static constexpr uint32_t SAMPLE_RATE = 11025u;

  void playSfx(const SGFAudio::Sfx& sfx, float baseHz, uint8_t velocity = 255u);

  SGFAudio::SynthEngine synth;
  SGFAudio::ESP32DacAudioOutput audioOutput;
#endif
};
