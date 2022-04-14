#pragma once

#include <memory>
#include <vector>
#include <array>

#include <clap/clap.h>

#include "constants.hh"
#include "audio-buffer.hh"
#include "fixed-point.hh"

namespace clap {
   // audio processing context
   class Context {
   public:
      // number of frames to process
      uint32_t numFrames;

      // current sampleRate
      double sampleRateD;
      float sampleRateF;

      // 1 / sampleRate
      double sampleRateInvD;
      double sampleRateInvF;

      bool isPlaying;
      bool isRecording;

      // pre-calculated array of beat time for each frames
      std::array<FixedPoint, BLOCK_SIZE> beatTime;

      std::vector<std::unique_ptr<AudioBuffer<double>>> audioInputs;
      std::vector<std::unique_ptr<AudioBuffer<double>>> audioOutputs;
   };
} // namespace clap
