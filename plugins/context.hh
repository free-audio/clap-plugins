#pragma once

#include <vector>

#include <clap/clap.h>

#include "fixedpoint.hh"

namespace clap {
   // audio processing context
   class Context {
   public:
      // number of frames to process
      uint32_t numFrames;

      // current sampleRate
      double sampleRate;
      float sampleRateF;

      bool isPlaying;
      bool isRecording;

      std::vector<FixedPoint> beatTime;
   };
} // namespace clap
