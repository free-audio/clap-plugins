#pragma once

#include <array>

#include "../domain-converter.hh"
#include "module.hh"

namespace clap {
   /**
    * State Variable Filter
    * Based on https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf by Andy Simper.
    *
    * The input must have ChannelCount channels, the output has ChannelCount channels.
    */
   template <uint32_t ChannelCount>
   class SvfModule : public Module {
      static_assert(ChannelCount > 0 && ChannelCount <= MAX_AUDIO_CHANNELS);

   public:
      SvfModule(CorePlugin &plugin, std::string name, uint32_t moduleId);
      SvfModule(const SvfModule &);
      ~SvfModule() override;

      std::unique_ptr<Module> cloneVoice() const override;

      bool doActivate(double sampleRate, uint32_t maxFrameCount, bool isRealTime) override;
      clap_process_status process(const Context &c, uint32_t numFrames) noexcept override;

      void reset() noexcept override;

      void setInput(const AudioBuffer<double> *in) { _input = in; }
      void setEnvInput(const AudioBuffer<double> *in) { _envInput = in; }
      void setFmInput(const AudioBuffer<double> *in) { _fmInput = in; }
      auto &outputBuffer() const { return _output; }

   protected:
      // Each channel has its own coefficients, because the FM may differ between channels
      struct Channel {
         // coefficients
         double _g = 0.f;
         double _k = 0.f;
         double _a1 = 0.f;
         double _a2 = 0.f;
         double _a3 = 0.f;
         double _lastFreq = 0;
         double _lastReso = 0;

         // state
         double _ic1eq = 0.f;
         double _ic2eq = 0.f;
      };

      // freq: 20 - 20000
      // resonance: 0 - 1 (1: self oscillating, maybe keep it just below that)
      void setFilter(Channel &ch, double freq, double reso);

      std::array<Channel, ChannelCount> _channels;

      double _sampleRate = 0;

      enum class Mode : int {
         LP,
         BP,
         HP
      };

      Parameter *_freqParam = nullptr;
      Parameter *_resoParam = nullptr;
      Parameter *_modeParam = nullptr;
      Parameter *_keytrackParam = nullptr;
      Parameter *_envParam = nullptr;
      Parameter *_fmParam = nullptr;

      const AudioBuffer<double> *_input = nullptr;
      const AudioBuffer<double> *_envInput = nullptr;
      const AudioBuffer<double> *_fmInput = nullptr;

      AudioBuffer<double> _envBuffer{1, BLOCK_SIZE};
      AudioBuffer<double> _fmBuffer{ChannelCount, BLOCK_SIZE};
      AudioBuffer<double> _ktBuffer{1, BLOCK_SIZE};
      AudioBuffer<double> _output{ChannelCount, BLOCK_SIZE};
   };
} // namespace clap

extern template class clap::SvfModule<1>;
extern template class clap::SvfModule<2>;
