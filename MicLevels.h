#pragma once

#include <stddef.h>
#include <stdint.h>

struct PcmLevel {
  uint16_t deviation = 0;
  uint16_t peak = 0;
  uint64_t energy = 0;
};

inline PcmLevel measurePcm(const int16_t* samples, size_t count, bool power) {
  PcmLevel result;
  if (!count) return result;
  int32_t sum = 0;
  for (size_t i = 0; i < count; ++i) sum += samples[i];
  const int32_t mean = sum / (int32_t)count;
  uint32_t deviation = 0;
  for (size_t i = 0; i < count; ++i) {
    const int32_t delta = samples[i] - mean;
    const uint32_t distance = delta < 0 ? -delta : delta;
    deviation += distance;
    if (power) {
      result.energy += distance * distance;
      const uint16_t absolute = samples[i] < 0 ? -(int32_t)samples[i] : samples[i];
      if (absolute > result.peak) result.peak = absolute;
    }
  }
  result.deviation = deviation / count;
  return result;
}

struct MicReading {
  uint64_t energy = 0;
  uint32_t samples = 0;
  uint16_t peak = 0;
  uint16_t levels[80] = {};
  uint8_t frames = 0;

  void observe(const int16_t* pcm, size_t count) {
    if (!count || frames == sizeof(levels) / sizeof(levels[0])) return;
    const PcmLevel level = measurePcm(pcm, count, true);
    energy += level.energy;
    samples += count;
    if (level.peak > peak) peak = level.peak;
    uint8_t i = frames++;
    while (i > 0 && levels[i - 1] > level.deviation) {
      levels[i] = levels[i - 1];
      --i;
    }
    levels[i] = level.deviation;
  }

  uint16_t p90() const { return frames ? levels[(frames - 1) * 9 / 10] : 0; }
};
