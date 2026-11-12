#include "../MicLevels.h"

#include <cassert>

int main() {
  const int16_t dc[] = {4096, 4096, 4096, 4096};
  const PcmLevel dcLevel = measurePcm(dc, 4, true);
  assert(dcLevel.deviation == 0 && dcLevel.energy == 0);

  const int16_t quiet[] = {3840, 4352, 3840, 4352};
  const int16_t speech[] = {-2048, 2048, -2048, 2048};
  const PcmLevel low = measurePcm(quiet, 4, true);
  const PcmLevel high = measurePcm(speech, 4, true);
  assert(low.deviation == 256 && high.deviation == 2048);
  assert(high.energy == 64 * low.energy);
  assert(measurePcm(speech, 4, false).deviation == high.deviation);

  MicReading reading;
  reading.observe(speech, 4);
  for (unsigned i = 0; i < 9; ++i) reading.observe(quiet, 4);
  assert(reading.frames == 10 && reading.samples == 40);
  assert(reading.energy == high.energy + 9 * low.energy);
  assert(reading.p90() == low.deviation);  // one transient does not set P90

  const int16_t fullScale[] = {INT16_MIN, INT16_MAX, INT16_MAX, INT16_MAX};
  assert(measurePcm(fullScale, 4, true).peak == 32768);
}
