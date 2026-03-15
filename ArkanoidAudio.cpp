#include "ArkanoidAudio.h"

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
namespace {

constexpr SGFAudio::Adsr kBlockHitEnv{0u, 18u, 140u, 24u};
constexpr SGFAudio::Instrument kBlockHitInstrument{
  SGFAudio::Waveform::Square,
  kBlockHitEnv,
  {},
  nullptr,
  0u,
  AUDIO_FILTER_LP,
  3200.0f,
  0.0f,
  170u
};
constexpr SGFAudio::SfxStep kBlockHitSteps[] = {
  {16u, 0, 0, 255u, true, true},
  {18u, -4, 0, 220u, true, false},
  {18u, -7, 0, 180u, true, false},
};
constexpr SGFAudio::Sfx kBlockHitSfx{
  &kBlockHitInstrument,
  kBlockHitSteps,
  sizeof(kBlockHitSteps) / sizeof(kBlockHitSteps[0])
};

constexpr SGFAudio::Adsr kPaddleHitEnv{0u, 22u, 168u, 30u};
constexpr SGFAudio::Instrument kPaddleHitInstrument{
  SGFAudio::Waveform::Triangle,
  kPaddleHitEnv,
  {},
  nullptr,
  0u,
  AUDIO_FILTER_LP,
  2200.0f,
  0.0f,
  185u
};
constexpr SGFAudio::SfxStep kPaddleHitSteps[] = {
  {18u, -3, 0, 220u, true, true},
  {22u, 0, 0, 255u, true, false},
  {28u, 2, 0, 185u, true, false},
};
constexpr SGFAudio::Sfx kPaddleHitSfx{
  &kPaddleHitInstrument,
  kPaddleHitSteps,
  sizeof(kPaddleHitSteps) / sizeof(kPaddleHitSteps[0])
};

constexpr SGFAudio::Adsr kBallOutEnv{0u, 36u, 176u, 90u};
constexpr SGFAudio::Instrument kBallOutInstrument{
  SGFAudio::Waveform::Saw,
  kBallOutEnv,
  {},
  nullptr,
  0u,
  AUDIO_FILTER_LP,
  1800.0f,
  0.0f,
  180u
};
constexpr SGFAudio::SfxStep kBallOutSteps[] = {
  {42u, 0, 0, 255u, true, true},
  {56u, -5, 0, 230u, true, false},
  {76u, -10, 0, 190u, true, false},
  {104u, -17, 0, 155u, true, false},
};
constexpr SGFAudio::Sfx kBallOutSfx{
  &kBallOutInstrument,
  kBallOutSteps,
  sizeof(kBallOutSteps) / sizeof(kBallOutSteps[0])
};

}  // namespace

ArkanoidAudio::ArkanoidAudio(uint8_t outputPin)
  : outputPin(outputPin),
    synth(SAMPLE_RATE)
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
    ,
    audioOutput(synth, outputPin)
#endif
{}

void ArkanoidAudio::setup() {
  synth.setMasterVolume(200u);
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
  if (enabled()) {
    audioOutput.begin();
  }
#endif
}

void ArkanoidAudio::playBlockHit() {
  playSfx(kBlockHitSfx, 1046.5f, 220u);
}

void ArkanoidAudio::playPaddleHit() {
  playSfx(kPaddleHitSfx, 392.0f, 255u);
}

void ArkanoidAudio::playBallOut() {
  playSfx(kBallOutSfx, 293.66f, 255u);
}

void ArkanoidAudio::playSfx(const SGFAudio::Sfx& sfx, float baseHz, uint8_t velocity) {
  if (!enabled()) {
    return;
  }
  synth.playSfx(sfx, baseHz, velocity);
}

bool ArkanoidAudio::enabled() const {
  return outputPin != AUDIO_DISABLED_PIN;
}
#else
ArkanoidAudio::ArkanoidAudio(uint8_t outputPin) : outputPin(outputPin) {}

void ArkanoidAudio::setup() {}

void ArkanoidAudio::playBlockHit() {}

void ArkanoidAudio::playPaddleHit() {}

void ArkanoidAudio::playBallOut() {}

bool ArkanoidAudio::enabled() const {
  return false;
}
#endif
