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
      void setSampleRate(double sr)
      {
         sampleRateD = sr;
         sampleRateF = static_cast<float>(sr);
         sampleRateInvD = 1.0 / sr;
         sampleRateInvF = static_cast<float>(sampleRateInvD);
      }

      [[nodiscard]] bool isRealTimeRendering() const noexcept { return renderMode == CLAP_RENDER_REALTIME; }

      // current sampleRate
      double sampleRateD;
      float sampleRateF;

      // 1 / sampleRate
      double sampleRateInvD;
      double sampleRateInvF;

      bool isPlaying;
      bool isRecording;

      clap_plugin_render_mode renderMode = CLAP_RENDER_REALTIME;

      // pre-calculated array of beat time for each frames
      std::array<FixedPoint, BLOCK_SIZE> beatTime;

      std::vector<std::unique_ptr<AudioBuffer<double>>> audioInputs;
      std::vector<std::unique_ptr<AudioBuffer<double>>> audioOutputs;
   };
} // namespace clap
