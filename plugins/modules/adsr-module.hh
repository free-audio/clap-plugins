#pragma once

#include "../module.hh"

namespace clap {
   class AdsrModule : public Module {
   public:
      AdsrModule(CorePlugin &plugin, std::string name, clap_id paramIdStart);
      ~AdsrModule() override;

      void trigger();
      void release();

      void process(float *out, uint32_t nsamples);

   protected:
      Parameter *_attackParam = nullptr;
      Parameter *_decayParam = nullptr;
      Parameter *_sustainParam = nullptr;
      Parameter *_releaseParam = nullptr;

      enum class Phase {
         Rest, // at 0 and sleeping
         Attack,
         Decay,
         Sustain,
         Release,
      };

      double _level = 0;
      Phase _phase = Phase::Rest;
   };
} // namespace clap
