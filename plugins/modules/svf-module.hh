#pragma once

#include "../domain-converter.hh"
#include "module.hh"

namespace clap {
   /**
    * State Variable Filter
    * Based on https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf by Andy Simper.
    */
   class SvfModule : public Module {
   public:
      SvfModule(CorePlugin &plugin, std::string name, uint32_t moduleId);
      SvfModule(const SvfModule &);
      ~SvfModule() override;

      std::unique_ptr<Module> cloneVoice() const override;

      bool doActivate(double sampleRate, uint32_t maxFrameCount) override;
      clap_process_status process(Context &c, uint32_t numFrames) noexcept override;

      void reset() noexcept override;

      void setInput(const AudioBuffer<double> *in) { _input = in; }
      auto &outputBuffer() const { return _output; }

   protected:
      // freq: 20 - 20000
      // resonance: 0 - 1 (1: self oscillating, maybe keep it just below that)
      void setFilter(double freq, double reso);

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

      double _sampleRate = 0;

      Parameter *_freqParam = nullptr;
      Parameter *_resoParam = nullptr;

      const AudioBuffer<double> *_input;
      AudioBuffer<double> _output{1, BLOCK_SIZE};
   };
} // namespace clap
