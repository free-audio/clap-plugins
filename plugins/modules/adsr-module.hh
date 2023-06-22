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

      bool doActivate(double sampleRate, uint32_t maxFrameCount, bool isRealTime) override;
      clap_process_status process(const Context &c, uint32_t numFrames) noexcept override;

      bool wantsNoteEvents() const noexcept override;
      void onNoteOn(const clap_event_note &note) noexcept override;
      void onNoteOff(const clap_event_note &note) noexcept override;
      void onNoteChoke(const clap_event_note &note) noexcept override;

      auto& outputBuffer() const { return _buffer; }

   protected:
      struct ExpCoeff {
         static const constexpr double attackOffset = 0.01;
         static const constexpr double decayOffset = 0.002;
         static const constexpr double releaseOffset = 0.001;
         static const constexpr double chokeOffset = 0.005;

         explicit ExpCoeff(double sampleRate) : _sampleRate(sampleRate) {}

         [[nodiscard]] double operator()(double seconds) const noexcept {
            // needs to multiply the result N times to reach 95% of the decay
            const double N = _sampleRate * seconds;
            return std::exp(-3.0 / N);
         }

         double _sampleRate;
      };

      DomainConverter<ExpCoeff> _conv{ExpCoeff(44100), 0};

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
      double _target = 0;
      double _noteOnVelocity = 0.8;

      AudioBuffer<double> _buffer{1, BLOCK_SIZE};
   };
} // namespace clap
