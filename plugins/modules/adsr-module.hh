#pragma once

#include "../domain-converter.hh"
#include "../module.hh"

namespace clap {
   class AdsrModule : public Module {
   public:
      AdsrModule(CorePlugin &plugin, std::string name, clap_id paramIdStart);
      ~AdsrModule() override;

      void trigger(int32_t voiceIndex, double velocity);
      void release(int32_t voiceIndex);
      void choke(int32_t voiceIndex);

   protected:
      bool wantsNoteEvents() const noexcept override;
      void onNoteOn(int32_t voiceIndex, const clap_event_note &note) noexcept override;
      void onNoteOff(int32_t voiceIndex, const clap_event_note &note) noexcept override;
      void onNoteChoke(int32_t voiceIndex, const clap_event_note &note) noexcept override;

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
            Choke,
         };

         Phase phase = Rest;
         double level = 0;
         std::unique_ptr<AudioBuffer<float>> buffer;
      };

      Voices<State> _states;
   };
} // namespace clap
