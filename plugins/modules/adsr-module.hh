#pragma once

#include "../module.hh"
#include "../domain-converter.hh"

namespace clap {
   class AdsrModule : public Module {
   public:
      AdsrModule(CorePlugin &plugin, std::string name, clap_id paramIdStart);
      ~AdsrModule() override;

      void trigger();
      void release();

      clap_process_status processVoice(const clap_process *process,
                                       uint32_t voiceIndex) noexcept override;

   protected:
      Parameter *_attackParam = nullptr;
      Parameter *_decayParam = nullptr;
      Parameter *_sustainParam = nullptr;
      Parameter *_releaseParam = nullptr;

      struct State {
         enum Phase {
            Rest, // at 0 and sleeping
            Attack,
            Decay,
            Sustain,
            Release,
         };

         Phase phase = Rest;
         double level = 0;
         std::unique_ptr<AudioBuffer<float>> buffer;
      };

      std::array<State, Voice::max_voices> _states;
   };
} // namespace clap
