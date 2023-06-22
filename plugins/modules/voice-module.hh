#pragma once

#include "../intrusive-list.hh"
#include "../smoothed-value.hh"
#include "module.hh"

namespace clap {
   class VoiceExpanderModule;
   class VoiceModule : public Module {
      friend class VoiceExpanderModule;

   public:
      VoiceModule(CorePlugin &plugin, std::unique_ptr<Module> module, uint32_t channelCount);
      VoiceModule(const VoiceModule &m);
      ~VoiceModule() override;

      std::unique_ptr<Module> cloneVoice() const override;

      auto &outputBuffer() const { return _outputBuffer; }

      void setVoiceIndex(uint32_t voiceIndex) noexcept { _voiceIndex = voiceIndex; }
      [[nodiscard]] uint32_t voiceIndex() const noexcept { return _voiceIndex; }
      void assign() noexcept;
      [[nodiscard]] bool isAssigned() const noexcept { return _isAssigned; }

      // key and channel info
      [[nodiscard]] bool match(int32_t noteId, int16_t port, int16_t channel, int16_t key) const;
      [[nodiscard]] int32_t nodeId() const noexcept { return _noteId; }
      [[nodiscard]] int16_t port() const noexcept { return _port; }
      [[nodiscard]] int16_t channel() const noexcept { return _channel; }
      [[nodiscard]] int16_t key() const noexcept { return _key; }
      [[nodiscard]] int16_t lowestKey() const noexcept;
      [[nodiscard]] int16_t highestKey() const noexcept;
      [[nodiscard]] int16_t startKey() const noexcept;

      // note expressions for this voice
      [[nodiscard]] double velocity() const noexcept { return _velocity; }
      [[nodiscard]] double keyFreq() const noexcept { return _keyFreq; }
      [[nodiscard]] auto &tuning() const noexcept { return _tuningBuffer; }
      [[nodiscard]] auto &pitch() const noexcept { return _pitchBuffer; }
      [[nodiscard]] auto &brightness() const noexcept { return _brigthnessBuffer; }
      [[nodiscard]] auto &pressure() const noexcept { return _pressureBuffer; }
      [[nodiscard]] auto &vibrato() const noexcept { return _vibratoBuffer; }
      [[nodiscard]] auto &expression() const noexcept { return _expressionBuffer; }
      [[nodiscard]] auto &volume() const noexcept { return _volumeBuffer; }
      [[nodiscard]] auto &pan() const noexcept { return _panBuffer; }

      bool doActivate(double sampleRate, uint32_t maxFrameCount, bool isRealTime) override;
      void doDeactivate() override;

      bool wantsNoteEvents() const noexcept override;
      void onNoteOn(const clap_event_note &note) noexcept override;
      void onNoteOff(const clap_event_note &note) noexcept override;
      void onNoteChoke(const clap_event_note &note) noexcept override;
      void onNoteExpression(const clap_event_note_expression &noteExp) noexcept override;

      clap_process_status process(const Context &c, uint32_t numFrames) noexcept override;

      void reset() noexcept override;

      mutable IntrusiveList _parametersToReset;

   private:
      const std::unique_ptr<Module> _module;

      uint32_t _voiceIndex = 0;
      bool _isAssigned = false;
      bool _wasJustAssigned = false;
      int32_t _noteId = -1;
      int16_t _port = 0;
      int16_t _key = 0;
      int16_t _channel = 0;
      double _keyFreq = 440;

      IntrusiveList::Hook _stateHook;

      double _velocity = 0.75;

      SmoothedValue _tuning;
      SmoothedValue _brigthness;
      SmoothedValue _pressure;
      SmoothedValue _vibrato;
      SmoothedValue _expression;
      SmoothedValue _volume;
      SmoothedValue _pan;

      AudioBuffer<double> _tuningBuffer{1, BLOCK_SIZE, 0};
      AudioBuffer<double> _pitchBuffer{1, BLOCK_SIZE, 0};
      AudioBuffer<double> _brigthnessBuffer{1, BLOCK_SIZE, 0};
      AudioBuffer<double> _pressureBuffer{1, BLOCK_SIZE, 0};
      AudioBuffer<double> _vibratoBuffer{1, BLOCK_SIZE, 0};
      AudioBuffer<double> _expressionBuffer{1, BLOCK_SIZE, 0};
      AudioBuffer<double> _volumeBuffer{1, BLOCK_SIZE, 0};
      AudioBuffer<double> _panBuffer{1, BLOCK_SIZE, 0};

      mutable AudioBuffer<double> _outputBuffer;
   };
} // namespace clap
