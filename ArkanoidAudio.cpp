#include "ArkanoidAudio.h"

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
namespace {

constexpr uint16_t TITLE_UNIT_MS = 140u;

constexpr float NOTE_D1 = 36.71f;
constexpr float NOTE_E1 = 41.20f;
constexpr float NOTE_G1 = 49.00f;
constexpr float NOTE_A1 = 55.00f;
constexpr float NOTE_KICK = 46.0f;
constexpr float NOTE_SNARE = 220.0f;
constexpr float NOTE_E2 = 82.41f;
constexpr float NOTE_G2 = 98.00f;
constexpr float NOTE_A2 = 110.00f;
constexpr float NOTE_AS2 = 116.54f;
constexpr float NOTE_B2 = 123.47f;
constexpr int TITLE_BASS_VOICE = 0;
constexpr int TITLE_KICK_VOICE = 1;
constexpr int TITLE_SNARE_VOICE = 2;
constexpr int TITLE_ORGAN1_VOICE = 3;
constexpr int TITLE_ORGAN2_VOICE = 4;

constexpr SGFAudio::PitchPoint kKickPitchEnv[] = {
  {0u, 1200},
  {24u, -200},
  {72u, -1400},
};
constexpr SGFAudio::Adsr kBassEnv{4u, 36u, 170u, 32u};
constexpr SGFAudio::Instrument kBassInstrument{
  SGFAudio::Waveform::Saw,
  kBassEnv,
  {},
  nullptr,
  0u,
  AUDIO_FILTER_LP,
  240.0f,
  0.0f,
  155u
};
constexpr SGFAudio::Adsr kKickEnv{0u, 54u, 0u, 0u};
constexpr SGFAudio::Instrument kKickInstrument{
  SGFAudio::Waveform::Sine,
  kKickEnv,
  {},
  kKickPitchEnv,
  sizeof(kKickPitchEnv) / sizeof(kKickPitchEnv[0]),
  AUDIO_FILTER_LP,
  420.0f,
  0.0f,
  255u
};
constexpr SGFAudio::Adsr kSnareEnv{0u, 28u, 0u, 12u};
constexpr SGFAudio::Instrument kSnareInstrument{
  SGFAudio::Waveform::Noise,
  kSnareEnv,
  {},
  nullptr,
  0u,
  AUDIO_FILTER_HP,
  0.0f,
  1450.0f,
  210u
};
constexpr SGFAudio::Lfo kOrgan1Lfo{
  true,
  SGFAudio::Waveform::Sine,
  5.2f,
  9.0f
};
constexpr SGFAudio::Adsr kOrgan1Env{10u, 60u, 210u, 140u};
constexpr SGFAudio::Instrument kOrgan1Instrument{
  SGFAudio::Waveform::Square,
  kOrgan1Env,
  kOrgan1Lfo,
  nullptr,
  0u,
  AUDIO_FILTER_LP,
  980.0f,
  0.0f,
  58u
};
constexpr SGFAudio::Lfo kOrgan2Lfo{
  true,
  SGFAudio::Waveform::Sine,
  4.8f,
  7.0f
};
constexpr SGFAudio::Adsr kOrgan2Env{10u, 70u, 205u, 150u};
constexpr SGFAudio::Instrument kOrgan2Instrument{
  SGFAudio::Waveform::Saw,
  kOrgan2Env,
  kOrgan2Lfo,
  nullptr,
  0u,
  AUDIO_FILTER_LP,
  900.0f,
  0.0f,
  54u
};

constexpr SGFAudio::PatternStep kBassSteps[] = {
  {NOTE_G1, 1u, 220u},
  {0.0f, 1u, 0u},
  {NOTE_E1, 1u, 210u},
  {NOTE_D1, 1u, 210u},
  {0.0f, 1u, 0u},
  {NOTE_G1, 1u, 220u},
  {0.0f, 1u, 0u},
  {NOTE_E1, 1u, 210u},
  {0.0f, 8u, 0u},
  {NOTE_G1, 1u, 220u},
  {0.0f, 1u, 0u},
  {NOTE_E1, 1u, 210u},
  {NOTE_D1, 1u, 210u},
  {0.0f, 1u, 0u},
  {NOTE_G1, 1u, 220u},
  {0.0f, 1u, 0u},
  {NOTE_A1, 1u, 220u},
  {0.0f, 6u, 0u},
  {NOTE_D1, 2u, 220u},
};
constexpr SGFAudio::Pattern kBassPattern{
  kBassSteps,
  sizeof(kBassSteps) / sizeof(kBassSteps[0]),
  TITLE_UNIT_MS,
  true
};

constexpr SGFAudio::PatternStep kKickSteps[] = {
  {NOTE_KICK, 1u, 255u},
  {NOTE_KICK, 1u, 245u},
  {0.0f, 1u, 0u},
  {0.0f, 1u, 0u},
  {NOTE_KICK, 1u, 255u},
  {0.0f, 1u, 0u},
  {0.0f, 1u, 0u},
  {0.0f, 1u, 0u},
};
constexpr SGFAudio::Pattern kKickPattern{
  kKickSteps,
  sizeof(kKickSteps) / sizeof(kKickSteps[0]),
  TITLE_UNIT_MS,
  true
};

constexpr SGFAudio::PatternStep kSnareSteps[] = {
  {0.0f, 1u, 0u},
  {0.0f, 1u, 0u},
  {NOTE_SNARE, 1u, 210u},
  {NOTE_SNARE, 1u, 225u},
  {0.0f, 1u, 0u},
  {0.0f, 1u, 0u},
  {NOTE_SNARE, 1u, 220u},
  {0.0f, 1u, 0u},
};
constexpr SGFAudio::Pattern kSnarePattern{
  kSnareSteps,
  sizeof(kSnareSteps) / sizeof(kSnareSteps[0]),
  TITLE_UNIT_MS,
  true
};

constexpr SGFAudio::PatternStep kOrganSilenceSteps[] = {
  {0.0f, 32u, 0u},
};
constexpr SGFAudio::Pattern kOrganSilencePattern{
  kOrganSilenceSteps,
  sizeof(kOrganSilenceSteps) / sizeof(kOrganSilenceSteps[0]),
  TITLE_UNIT_MS,
  false
};

constexpr SGFAudio::PatternStep kOrgan1PhraseSteps[] = {
  {NOTE_B2, 4u, 180u},
  {0.0f, 1u, 0u},
  {NOTE_G2, 1u, 172u},
  {0.0f, 2u, 0u},
  {0.0f, 8u, 0u},
  {NOTE_A2, 4u, 184u},
  {0.0f, 12u, 0u},
};
constexpr SGFAudio::Pattern kOrgan1PhrasePattern{
  kOrgan1PhraseSteps,
  sizeof(kOrgan1PhraseSteps) / sizeof(kOrgan1PhraseSteps[0]),
  TITLE_UNIT_MS,
  false
};

constexpr SGFAudio::PatternStep kOrgan2PhraseSteps[] = {
  {NOTE_E2, 4u, 166u},
  {0.0f, 1u, 0u},
  {NOTE_E2, 1u, 160u},
  {0.0f, 2u, 0u},
  {0.0f, 8u, 0u},
  {NOTE_E2, 4u, 168u},
  {0.0f, 1u, 0u},
  {NOTE_E2, 1u, 160u},
  {0.0f, 2u, 0u},
  {0.0f, 2u, 0u},
  {NOTE_G2, 1u, 168u},
  {NOTE_E2, 1u, 160u},
  {NOTE_G2, 2u, 172u},
  {NOTE_AS2, 2u, 176u},
};
constexpr SGFAudio::Pattern kOrgan2PhrasePattern{
  kOrgan2PhraseSteps,
  sizeof(kOrgan2PhraseSteps) / sizeof(kOrgan2PhraseSteps[0]),
  TITLE_UNIT_MS,
  false
};

const SGFAudio::SongClip kBassClips[] = {
  {&kBassPattern, 1u},
};
const SGFAudio::SongClip kKickClips[] = {
  {&kKickPattern, 1u},
};
const SGFAudio::SongClip kSnareClips[] = {
  {&kSnarePattern, 1u},
};
const SGFAudio::SongClip kOrgan1Clips[] = {
  {&kOrganSilencePattern, 2u},
  {&kOrgan1PhrasePattern, 2u},
};
const SGFAudio::SongClip kOrgan2Clips[] = {
  {&kOrganSilencePattern, 2u},
  {&kOrgan2PhrasePattern, 2u},
};

const SGFAudio::SongLane kTitleSongLanes[] = {
  {TITLE_BASS_VOICE, SGFAudio::makeProgramRef(kBassInstrument), kBassClips,
   sizeof(kBassClips) / sizeof(kBassClips[0])},
  {TITLE_KICK_VOICE, SGFAudio::makeProgramRef(kKickInstrument), kKickClips,
   sizeof(kKickClips) / sizeof(kKickClips[0])},
  {TITLE_SNARE_VOICE, SGFAudio::makeProgramRef(kSnareInstrument), kSnareClips,
   sizeof(kSnareClips) / sizeof(kSnareClips[0])},
  {TITLE_ORGAN1_VOICE, SGFAudio::makeProgramRef(kOrgan1Instrument), kOrgan1Clips,
   sizeof(kOrgan1Clips) / sizeof(kOrgan1Clips[0])},
  {TITLE_ORGAN2_VOICE, SGFAudio::makeProgramRef(kOrgan2Instrument), kOrgan2Clips,
   sizeof(kOrgan2Clips) / sizeof(kOrgan2Clips[0])},
};

const SGFAudio::Song kTitleSong{
  kTitleSongLanes,
  sizeof(kTitleSongLanes) / sizeof(kTitleSongLanes[0]),
  90u,
  4u
};

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
    music(SAMPLE_RATE),
    audioOutput(music, outputPin)
{}

void ArkanoidAudio::setup() {
  music.setVolume(190u);
  if (enabled()) {
    audioOutput.begin();
  }
}

void ArkanoidAudio::startTitleMusic() {
  if (!enabled()) {
    return;
  }
  music.play(kTitleSong);
}

void ArkanoidAudio::stopTitleMusic() {
  music.stop();
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
  int voice = nextSfxVoice;
  for (int i = 0; i < SFX_VOICE_COUNT; ++i) {
    int candidate = SFX_VOICE_START + ((nextSfxVoice - SFX_VOICE_START + i) % SFX_VOICE_COUNT);
    if (!music.synth().voiceActive(candidate)) {
      voice = candidate;
      break;
    }
  }
  music.synth().triggerSfx(voice, sfx, baseHz, velocity);
  nextSfxVoice = SFX_VOICE_START + ((voice - SFX_VOICE_START + 1) % SFX_VOICE_COUNT);
}

bool ArkanoidAudio::enabled() const {
  return outputPin != AUDIO_DISABLED_PIN;
}
#else
ArkanoidAudio::ArkanoidAudio(uint8_t outputPin) : outputPin(outputPin) {}

void ArkanoidAudio::setup() {}

void ArkanoidAudio::startTitleMusic() {}

void ArkanoidAudio::stopTitleMusic() {}

void ArkanoidAudio::playBlockHit() {}

void ArkanoidAudio::playPaddleHit() {}

void ArkanoidAudio::playBallOut() {}

bool ArkanoidAudio::enabled() const {
  return false;
}
#endif
