#pragma once

#include "../domain-converter.hh"
#include "../module.hh"

namespace clap {
   class AdsrModule : public Module {
   public:
      AdsrModule(CorePlugin &plugin, std::string name, clap_id paramIdStart);
      ~AdsrModule() override;

      void registerParameters() override;

      void trigger(double velocity);
      void release();
      void choke();

   protected:
      bool wantsNoteEvents() const noexcept override;
      void onNoteOn(const clap_event_note &note) noexcept override;
      void onNoteOff(const clap_event_note &note) noexcept override;
      void onNoteChoke(const clap_event_note &note) noexcept override;

      Parameter *_attackParam = nullptr;
      Parameter *_decayParam = nullptr;
      Parameter *_sustainParam = nullptr;
      Parameter *_releaseParam = nullptr;
      Parameter *_velocityParam = nullptr;

      enum class Phase {
         Rest, // at 0 and sleeping
         Attack,
         Decay,
         Sustain,
         Release,
         Choke,
      };

      Phase phase = Phase::Rest;
      double level = 0;
      std::unique_ptr<AudioBuffer<float>> buffer;
   };
} // namespace clap
