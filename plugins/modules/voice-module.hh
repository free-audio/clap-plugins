#pragma once

#include "../module.hh"

namespace clap {
   class VoiceModule : public Module {
   public:
      VoiceModule(CorePlugin &plugin);

      [[nodiscard]] uint32_t voiceIndex() const noexcept;
      [[nodiscard]] bool isAssigned() const noexcept;

      // key and channel info
      [[nodiscard]] int32_t channel() const noexcept;
      [[nodiscard]] int32_t key() const noexcept;
      [[nodiscard]] int32_t lowestKey() const noexcept;
      [[nodiscard]] int32_t highestKey() const noexcept;
      [[nodiscard]] int32_t startKey() const noexcept;

      // note expressions for this voice
      [[nodiscard]] double pitch() const noexcept;
      [[nodiscard]] double brightness() const noexcept;
      [[nodiscard]] double pressure() const noexcept;
      [[nodiscard]] double vibrato() const noexcept;
      [[nodiscard]] double expression() const noexcept;
      [[nodiscard]] double velocity() const noexcept;
      [[nodiscard]] double gain() const noexcept;
      [[nodiscard]] double pan() const noexcept;

      void reset() noexcept;

   private:
      bool _isAssigned = false;

      double _velocity;
      SmoothedValue _pitch;
      SmoothedValue _brigthness;
      SmoothedValue _pressure;
      SmoothedValue _vibrato;
      SmoothedValue _expression;
      SmoothedValue _gain;
      SmoothedValue _pan;
   };
} // namespace clap