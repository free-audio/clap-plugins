#pragma once

#include <cmath>

namespace clap {
   /** Calculates the frequency ratio from a relative tuning in semitones. */
   inline double tuningToRatio(double relativeTuning) { return std::exp2(relativeTuning / 12.0); }
} // namespace clap
