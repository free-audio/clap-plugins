#pragma once

#include <clap/clap.h>

namespace clap {
   class TuningProvider {
   public:
      double getFreq(int16_t port, int16_t channel, int16_t key) const;
   };
} // namespace clap
