#include "../TalkControls.h"

#include <cassert>

int main() {
  KeyGesture c;
  assert(c.update(true, 100, 350) == KeyEvent::None);
  assert(c.update(false, 105, 350) == KeyEvent::None);  // bounce
  assert(c.update(true, 112, 350) == KeyEvent::None);
  assert(c.update(true, 142, 350) == KeyEvent::None);
  assert(c.update(false, 232, 350) == KeyEvent::None);
  assert(c.update(false, 262, 350) == KeyEvent::Tap);
  assert(c.update(false, 400, 350) == KeyEvent::None);

  assert(c.update(true, 500, 350) == KeyEvent::None);
  assert(c.update(true, 849, 350) == KeyEvent::None);
  assert(c.update(true, 850, 350) == KeyEvent::Hold);
  assert(c.update(true, 2000, 350) == KeyEvent::None);
  assert(c.update(false, 2020, 350) == KeyEvent::None);
  assert(c.update(false, 2050, 350) == KeyEvent::ReleaseHold);

  KeyGesture a;
  assert(a.update(true, 1000, 2000) == KeyEvent::None);
  assert(a.update(true, 1030, 2000) == KeyEvent::None);
  assert(a.update(true, 2999, 2000) == KeyEvent::None);
  assert(a.update(true, 3000, 2000) == KeyEvent::Hold);
  assert(a.update(true, 8000, 2000) == KeyEvent::None);  // portal stays open

  KeyGesture b;
  b.resumeHeld();
  assert(b.update(true, 4000, 650) == KeyEvent::None);
  assert(b.update(false, 4100, 650) == KeyEvent::None);
  assert(b.update(false, 4130, 650) == KeyEvent::ReleaseHold);  // no volume change

  VoiceTurn backgroundOnly;
  for (unsigned t = 60; t <= 9960; t += 60) {
    const unsigned level = t <= 120 ? 0 : t == 180 ? 1600 : 256;
    backgroundOnly.observe(level, t);  // empty capture frames and one spike, then sustained background
    assert(backgroundOnly.action(t, t, true) == TurnAction::Listen);
  }
  assert(backgroundOnly.action(10000, 10000, true) == TurnAction::Cancel);

  VoiceTurn voice;
  for (unsigned t = 60; t <= 3900; t += 60) {
    const bool speaking = t >= 1200 && t <= 3000;
    voice.observe(speaking ? 1600 : 256, t);  // 0010 treated every background frame as speech
    assert(voice.action(t, t, true) == (t < 3900 ? TurnAction::Listen : TurnAction::Submit));
  }
  assert(voice.action(3900, 3900, false) == TurnAction::Cancel);

  VoiceTurn earlyVoice;
  for (unsigned t = 60; t <= 300; t += 60) earlyVoice.observe(1600, t);
  earlyVoice.observe(256, 360);
  assert(earlyVoice.action(1259, 1259, true) == TurnAction::Listen);
  assert(earlyVoice.action(1260, 1260, true) == TurnAction::Submit);

  VoiceTurn sustained;
  for (unsigned t = 60; t <= VoiceTurn::maxCaptureMs; t += 60) {
    sustained.observe(t <= 300 ? 256 : 1600, t);
    assert(sustained.action(t, t, true) == (t < VoiceTurn::maxCaptureMs ? TurnAction::Listen : TurnAction::Submit));
  }

  // Synthetic sequences use the user's measured P90 values, not an audio recording.
  VoiceTurn measuredNoise;
  for (unsigned t = 60; t <= 9960; t += 60) {
    const unsigned level = t % 600 == 0 ? 1500 : t <= 300 ? 300 : 826;
    measuredNoise.observe(level, t);
    assert(!measuredNoise.heardVoice);
    assert(measuredNoise.action(t, t, true) == TurnAction::Listen);
  }
  assert(measuredNoise.action(10000, 10000, true) == TurnAction::Cancel);

  VoiceTurn measuredSpeech;
  for (unsigned t = 60; t <= 3900; t += 60) {
    const bool speaking = t >= 1200 && t <= 3000;
    measuredSpeech.observe(speaking ? 4859 : 826, t);
    assert(measuredSpeech.action(t, t, true) == (t < 3900 ? TurnAction::Listen : TurnAction::Submit));
  }

  VoiceTurn transient;
  for (unsigned t = 60; t <= 1800; t += 60) {
    const bool isolatedPeak = t == 900 || t == 1200 || t == 1260;
    transient.observe(isolatedPeak ? 6000 : 826, t);
    assert(!transient.heardVoice);  // one or two loud frames should not create a sentence
  }

  VoiceTurn measuredEarlySpeech;
  for (unsigned t = 60; t <= 300; t += 60) measuredEarlySpeech.observe(4859, t);
  measuredEarlySpeech.observe(826, 360);
  assert(measuredEarlySpeech.heardVoice);
  assert(measuredEarlySpeech.action(1260, 1260, true) == TurnAction::Submit);
}
