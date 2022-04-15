#include <cassert>
#include <cmath>

#include "tuning-provider.hh"

namespace clap {
   double TuningProvider::getFreq(int16_t port, int16_t channel, int16_t key) const {
      assert(key >= 0 && key < 128);
      return 440 * relativeTuningToFreqRatio(key - 12 * 6);
   }

   double TuningProvider::relativeTuningToFreqRatio(double relativeTuning) {
      return std::pow(2, relativeTuning / 12.0);
   }
} // namespace clap