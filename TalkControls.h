#pragma once

#include <stdint.h>

enum class KeyEvent : uint8_t { None, Tap, Hold, ReleaseHold };
enum class ReplyEnd : uint8_t { Pending, Played, Failed };
enum class TurnAction : uint8_t { Listen, Submit, Cancel };

struct KeyGesture {
  bool raw = false;
  bool down = false;
  bool held = false;
  uint32_t edgeAt = 0;
  uint32_t downAt = 0;

  KeyEvent update(bool pressed, uint32_t now, uint32_t holdMs) {
    if (raw != pressed) {
      raw = pressed;
      edgeAt = now;
    }
    if (raw != down && now - edgeAt >= 30) {
      down = raw;
      if (!down) return held ? KeyEvent::ReleaseHold : KeyEvent::Tap;
      downAt = edgeAt;
      held = false;
    }
    if (raw && down && !held && now - downAt >= holdMs) {
      held = true;
      return KeyEvent::Hold;
    }
    return KeyEvent::None;
  }

  void resumeHeld() { raw = down = held = true; }
};

struct VoiceTurn {
  static constexpr uint32_t quietMs = 900;
  static constexpr uint32_t noSpeechMs = 10000;
  static constexpr uint32_t maxCaptureMs = 15000;
  // In the same units as the microphone test's P90. Measured on the AlphaPi:
  // ambient P90 826 and speech P90 4859. Keep a margin over the noise floor.
  static constexpr uint16_t onsetFloor = 1600;
  static constexpr uint16_t holdFloor = 1200;
  static constexpr uint8_t onsetFrames = 3;  // 180 ms at 60 ms per frame
  uint32_t lastVoiceAt = 0;
  uint16_t noise = 0;
  uint16_t warmup[5] = {};
  uint16_t startupSpeech = 0;
  uint8_t warmupCount = 0;
  uint8_t voiceFrames = 0;
  bool heardVoice = false;

  uint32_t threshold() const {
    const uint32_t relative = heardVoice ? noise / 3 : noise / 2;
    const uint32_t minimum = heardVoice ? 96 : 128;
    const uint32_t adaptive = noise + (relative > minimum ? relative : minimum);
    const uint32_t floor = heardVoice ? holdFloor : onsetFloor;
    return adaptive > floor ? adaptive : floor;
  }

  // level is the mean deviation from the frame's own mean, in Q8 PCM units.
  void observe(uint16_t level, uint32_t now) {
    if (warmupCount < 5) {
      if (warmupCount == 0 && level == 0) return;  // the capture can start with empty PCM frames
      warmup[warmupCount++] = level;
      if (warmupCount == 5) {
        uint16_t sorted[5] = {};
        for (uint8_t i = 0; i < 5; ++i) {
          uint8_t j = i;
          while (j > 0 && sorted[j - 1] > warmup[i]) {
            sorted[j] = sorted[j - 1];
            --j;
          }
          sorted[j] = warmup[i];
        }
        noise = sorted[2];
        // Require three adjacent loud frames even when speech starts during warmup.
        for (uint8_t i = 0; i + onsetFrames <= 5; ++i) {
          uint16_t lowest = warmup[i];
          for (uint8_t j = 1; j < onsetFrames; ++j) {
            if (warmup[i + j] < lowest) lowest = warmup[i + j];
          }
          if (lowest > startupSpeech) startupSpeech = lowest;
        }
        if (startupSpeech >= threshold()) {
          heardVoice = true;
          lastVoiceAt = now;
        }
      }
      return;
    }
    // A sustained utterance may already be in progress at the start of capture.
    // A much quieter frame corrects that initial estimate and starts the hangover.
    if (!heardVoice && startupSpeech >= onsetFloor && noise >= 2U * level + 192) {
      noise = level;
      if (startupSpeech >= threshold()) {
        heardVoice = true;
        lastVoiceAt = now;
      }
    }
    if (level >= threshold()) {
      if (voiceFrames < onsetFrames) ++voiceFrames;
      if (voiceFrames >= onsetFrames) {
        heardVoice = true;
        lastVoiceAt = now;
      }
    } else {
      voiceFrames = 0;
      if (!heardVoice || level < noise) noise = (15U * noise + level) / 16;
    }
  }

  TurnAction action(uint32_t now, uint32_t captureMs, bool uploadedAudio) const {
    if (heardVoice && (now - lastVoiceAt >= quietMs || captureMs >= maxCaptureMs)) {
      return uploadedAudio ? TurnAction::Submit : TurnAction::Cancel;
    }
    if (!heardVoice && captureMs >= noSpeechMs) return TurnAction::Cancel;
    return TurnAction::Listen;
  }
};
