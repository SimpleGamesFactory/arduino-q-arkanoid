#pragma once

#include <stdint.h>

#include "SGF/IAudioSource.h"

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
#include "SGF/AudioTypes.h"
#include "SGF/Pattern.h"
#include "SGF/Synth.h"
#include "SGF/ESP32DacAudioOutput.h"
#endif

class ArkanoidAudio : public SGFAudio::IAudioSource {
public:
  static constexpr uint8_t AUDIO_DISABLED_PIN = 0xFF;

  explicit ArkanoidAudio(uint8_t outputPin);

  void setup();
  void startTitleMusic();
  void stopTitleMusic();
  void playBlockHit();
  void playPaddleHit();
  void playBallOut();
  uint32_t sampleRate() const override;
  int16_t renderSample() override;

private:
  static constexpr uint32_t SAMPLE_RATE = 11025u;

  bool enabled() const;

  uint8_t outputPin = AUDIO_DISABLED_PIN;
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
  void playSfx(const SGFAudio::Sfx& sfx, float baseHz, uint8_t velocity = 255u);
  void stopVoices(int firstVoice, int voiceCount);

  SGFAudio::SynthEngine synth;
  SGFAudio::PatternTrack titleBassTrack;
  SGFAudio::PatternTrack titleKickTrack;
  SGFAudio::PatternTrack titleSnareTrack;
  bool titleMusicEnabled = false;
  static constexpr int SFX_VOICE_START = 4;
  static constexpr int SFX_VOICE_COUNT = 4;
  uint8_t nextSfxVoice = SFX_VOICE_START;
  SGFAudio::ESP32DacAudioOutput audioOutput;
#endif
};
