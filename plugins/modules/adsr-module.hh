#pragma once

#include "../domain-converter.hh"
#include "module.hh"

namespace clap {
   class AdsrModule : public Module {
   public:
      AdsrModule(CorePlugin &plugin, std::string name, uint32_t moduleId);
      AdsrModule(const AdsrModule &);
      ~AdsrModule() override;

      std::unique_ptr<Module> cloneVoice() const override;

      void trigger(double velocity);
      void release();
      void choke();

      clap_process_status process(Context &c, uint32_t numFrames) noexcept override;

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

      Phase _phase = Phase::Rest;
      double _level = 0;
      double _velocity = 0.8;
      std::unique_ptr<AudioBuffer<float>> _buffer;
   };
} // namespace clap
