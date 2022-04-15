#pragma once

#include "../intrusive-list.hh"
#include "module.hh"

namespace clap {
   class VoiceExpanderModule;
   class VoiceModule : public Module {
      friend class VoiceExpanderModule;

   public:
      VoiceModule(CorePlugin &plugin, std::unique_ptr<Module> module, uint32_t channelCount);
      VoiceModule(const VoiceModule &m);

      std::unique_ptr<Module> cloneVoice() const override;

      auto &outputBuffer() const { return _outputBuffer; }

      void setVoiceIndex(uint32_t voiceIndex) noexcept { _voiceIndex = voiceIndex; }
      [[nodiscard]] uint32_t voiceIndex() const noexcept { return _voiceIndex; }
      [[nodiscard]] bool isAssigned() const noexcept { return _isAssigned; }

      // key and channel info
      [[nodiscard]] bool match(int32_t key, int32_t channel) const;
      [[nodiscard]] int32_t channel() const noexcept { return _channel; }
      [[nodiscard]] int32_t key() const noexcept { return _key; }
      [[nodiscard]] int32_t lowestKey() const noexcept;
      [[nodiscard]] int32_t highestKey() const noexcept;
      [[nodiscard]] int32_t startKey() const noexcept;

      // note expressions for this voice
      [[nodiscard]] double velocity() const noexcept { return _velocity; }
      [[nodiscard]] auto &pitch() const noexcept { return _pitchBuffer; }
      [[nodiscard]] auto &pitchFreq() const noexcept { return _pitchFreqBuffer; }
      [[nodiscard]] auto &brightness() const noexcept { return _brigthnessBuffer; }
      [[nodiscard]] auto &pressure() const noexcept { return _pressureBuffer; }
      [[nodiscard]] auto &vibrato() const noexcept { return _vibratoBuffer; }
      [[nodiscard]] auto &expression() const noexcept { return _expressionBuffer; }
      [[nodiscard]] auto &gain() const noexcept { return _gainBuffer; }
      [[nodiscard]] auto &pan() const noexcept { return _panBuffer; }

      bool doActivate(double sampleRate, uint32_t maxFrameCount) override;
      void doDeactivate() override;

      bool wantsNoteEvents() const noexcept override;
      void onNoteOn(const clap_event_note &note) noexcept override;
      void onNoteOff(const clap_event_note &note) noexcept override;
      void onNoteChoke(const clap_event_note &note) noexcept override;

      clap_process_status process(const Context &c, uint32_t numFrames) noexcept override;

      void reset() noexcept override;

   private:
      const std::unique_ptr<Module> _module;

      uint32_t _voiceIndex = 0;
      bool _isAssigned = false;
      int32_t _key;
      int32_t _channel;

      IntrusiveList::Hook _stateHook;

      double _velocity;

      SmoothedValue _pitch;
      SmoothedValue _brigthness;
      SmoothedValue _pressure;
      SmoothedValue _vibrato;
      SmoothedValue _expression;
      SmoothedValue _gain;
      SmoothedValue _pan;

      AudioBuffer<double> _pitchBuffer{1, BLOCK_SIZE, 0};
      AudioBuffer<double> _pitchFreqBuffer{1, BLOCK_SIZE, 0};
      AudioBuffer<double> _brigthnessBuffer{1, BLOCK_SIZE, 0};
      AudioBuffer<double> _pressureBuffer{1, BLOCK_SIZE, 0};
      AudioBuffer<double> _vibratoBuffer{1, BLOCK_SIZE, 0};
      AudioBuffer<double> _expressionBuffer{1, BLOCK_SIZE, 0};
      AudioBuffer<double> _gainBuffer{1, BLOCK_SIZE, 0};
      AudioBuffer<double> _panBuffer{1, BLOCK_SIZE, 0};

      mutable AudioBuffer<double> _outputBuffer;
   };
} // namespace clap
