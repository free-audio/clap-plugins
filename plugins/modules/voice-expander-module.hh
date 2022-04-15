#pragma once

#include <array>
#include <memory>

#include "../constants.hh"
#include "../intrusive-list.hh"
#include "module.hh"

#include "voice-module.hh"

namespace clap {
   class VoiceExpanderModule : public Module {
   public:
      VoiceExpanderModule(CorePlugin &plugin, uint32_t moduleId, std::unique_ptr<Module> module);

      bool doActivate(double sampleRate, uint32_t maxFrameCount) override;
      void doDeactivate() override;

      clap_process_status process(const Context &c, uint32_t numFrames) noexcept override;

      bool wantsNoteEvents() const noexcept override;
      void onNoteOn(const clap_event_note &note) noexcept override;
      void onNoteOff(const clap_event_note &note) noexcept override;
      void onNoteChoke(const clap_event_note &note) noexcept override;

   private:
      VoiceModule* findActiveVoice(int32_t key, int32_t channel) const;
      VoiceModule* assignVoice(int32_t key, int32_t channel);

      IntrusiveList _activeVoices;   // uses VoiceModule._stateHook
      IntrusiveList _sleepingVoices; // uses VoiceModule._stateHook

      std::array<std::unique_ptr<VoiceModule>, MAX_VOICES> _voices;
   };
} // namespace clap
