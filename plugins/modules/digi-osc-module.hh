#pragma once

#include "../domain-converter.hh"
#include "module.hh"

namespace clap {
   class DigiOscModule : public Module {
   public:
      DigiOscModule(CorePlugin &plugin, std::string name, uint32_t moduleId);
      DigiOscModule(const DigiOscModule &);
      ~DigiOscModule() override;

      std::unique_ptr<Module> cloneVoice() const override;

      void trigger();

      bool doActivate(double sampleRate, uint32_t maxFrameCount) override;
      clap_process_status process(const Context &c, uint32_t numFrames) noexcept override;

      void setPmInput(const AudioBuffer<double> *in) noexcept { _pmInput = in; }
      auto &outputBuffer() const { return _buffer; }

   protected:
      Parameter *_pdParam = nullptr;

      double _phase = 0;
      AudioBuffer<double> _buffer;

      const AudioBuffer<double> *_pmInput = nullptr;
   };
} // namespace clap
