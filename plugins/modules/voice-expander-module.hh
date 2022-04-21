#pragma once

#include <array>
#include <memory>

#include <clap/helpers/note-end-queue.hh>

#include "../constants.hh"
#include "../intrusive-list.hh"
#include "module.hh"
#include "voice-module.hh"

namespace clap {
   class VoiceExpanderModule : public Module {
   public:
      VoiceExpanderModule(CorePlugin &plugin, uint32_t moduleId, std::unique_ptr<Module> module, uint32_t channelCount);

      bool doActivate(double sampleRate, uint32_t maxFrameCount) override;
      void doDeactivate() override;

      clap_process_status process(const Context &c, uint32_t numFrames) noexcept override;

      bool wantsNoteEvents() const noexcept override;
      void onNoteOn(const clap_event_note &note) noexcept override;
      void onNoteOff(const clap_event_note &note) noexcept override;
      void onNoteChoke(const clap_event_note &note) noexcept override;
      void onNoteExpression(const clap_event_note_expression &noteExp) noexcept override;

      auto& outputBuffer() const noexcept { return _outputBuffer; }

      VoiceModule* findActiveVoice(int32_t key, int32_t channel) const;
      VoiceModule* getVoice(uint32_t voiceIndex) noexcept { return _voices[voiceIndex].get(); }
      const VoiceModule* getVoice(uint32_t voiceIndex) const noexcept { return _voices[voiceIndex].get(); }

      auto& noteEndQueue() { return _noteEndQueue; }

   private:
      VoiceModule* assignVoice();
      void releaseVoice(VoiceModule &);

      IntrusiveList _activeVoices;   // uses VoiceModule._stateHook
      IntrusiveList _sleepingVoices; // uses VoiceModule._stateHook

      std::array<std::unique_ptr<VoiceModule>, MAX_VOICES> _voices;

      AudioBuffer<double> _outputBuffer;

      helpers::NoteEndQueue _noteEndQueue;
   };
} // namespace clap
