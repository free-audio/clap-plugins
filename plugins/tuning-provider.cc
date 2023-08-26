#include <cassert>
#include <cmath>

#include "tuning-provider.hh"
#include "tuning-utilities.hh"

namespace clap {
   double TuningProvider::getFreq(int16_t port, int16_t channel, int16_t key) const {
      assert(key >= 0 && key < 128);
      return 440 * tuningToRatio(key - 69);
   }
} // namespace clap