#pragma once

#include <vector>

#include <clap/clap.h>

#include "fixedpoint.hh"

namespace clap {
   // audio processing context
   class Context {
   public:
      double sampleRate;

      std::vector<FixedPoint> beatTime;
   };
} // namespace clap
