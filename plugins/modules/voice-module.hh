#pragma once

#include "../intrusive-list.hh"
#include "../module.hh"

namespace clap {
   class VoiceExpanderModule;
   class VoiceModule : public Module {
      friend class VoiceExpanderModule;

   public:
      VoiceModule(CorePlugin &plugin, std::unique_ptr<Module> module);

      void registerParameters() override;

      std::unique_ptr<Module> cloneVoice() const override;

      [[nodiscard]] uint32_t voiceIndex() const noexcept;
      [[nodiscard]] bool isAssigned() const noexcept;

      // key and channel info
      [[nodiscard]] int32_t channel() const noexcept;
      [[nodiscard]] int32_t key() const noexcept;
      [[nodiscard]] int32_t lowestKey() const noexcept;
      [[nodiscard]] int32_t highestKey() const noexcept;
      [[nodiscard]] int32_t startKey() const noexcept;

      // note expressions for this voice
      [[nodiscard]] double velocity() const noexcept { return _velocity; }
      [[nodiscard]] auto &pitch() const noexcept { return _pitchBuffer; }
      [[nodiscard]] auto &brightness() const noexcept { return _brigthnessBuffer; }
      [[nodiscard]] auto &pressure() const noexcept { return _pressureBuffer; }
      [[nodiscard]] auto &vibrato() const noexcept { return _vibratoBuffer; }
      [[nodiscard]] auto &expression() const noexcept { return _expressionBuffer; }
      [[nodiscard]] auto &gain() const noexcept { return _gainBuffer; }
      [[nodiscard]] auto &pan() const noexcept { return _panBuffer; }

      void reset() noexcept;

   private:
      const std::unique_ptr<Module> _module;

      bool _isAssigned = false;

      IntrusiveList::Hook _activeVoicesHook;

      double _velocity;
      SmoothedValue _pitch;
      AudioBuffer<double> _pitchBuffer{1, BLOCK_SIZE, 0};
      SmoothedValue _brigthness;
      AudioBuffer<double> _brigthnessBuffer{1, BLOCK_SIZE, 0};
      SmoothedValue _pressure;
      AudioBuffer<double> _pressureBuffer{1, BLOCK_SIZE, 0};
      SmoothedValue _vibrato;
      AudioBuffer<double> _vibratoBuffer{1, BLOCK_SIZE, 0};
      SmoothedValue _expression;
      AudioBuffer<double> _expressionBuffer{1, BLOCK_SIZE, 0};
      SmoothedValue _gain;
      AudioBuffer<double> _gainBuffer{1, BLOCK_SIZE, 0};
      SmoothedValue _pan;
      AudioBuffer<double> _panBuffer{1, BLOCK_SIZE, 0};
   };
} // namespace clap