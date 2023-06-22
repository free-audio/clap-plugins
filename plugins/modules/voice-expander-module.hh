#pragma once

#include <array>
#include <memory>

#include <clap/helpers/note-end-queue.hh>

#include "../constants.hh"
#include "../intrusive-list.hh"
#include "../container-of.hh"
#include "module.hh"
#include "voice-module.hh"

namespace clap {
   class VoiceExpanderModule : public Module {
   public:
      VoiceExpanderModule(CorePlugin &plugin,
                          uint32_t moduleId,
                          std::unique_ptr<Module> module,
                          uint32_t channelCount);

      bool doActivate(double sampleRate, uint32_t maxFrameCount, bool isRealTime) override;
      void doDeactivate() override;

      clap_process_status process(const Context &c, uint32_t numFrames) noexcept override;

      bool wantsNoteEvents() const noexcept override;
      void onNoteOn(const clap_event_note &note) noexcept override;
      void onNoteOff(const clap_event_note &note) noexcept override;
      void onNoteChoke(const clap_event_note &note) noexcept override;
      void onNoteExpression(const clap_event_note_expression &noteExp) noexcept override;

      auto &outputBuffer() const noexcept { return _outputBuffer; }

      template <class Callback>
      void foreachActiveVoice(int16_t noteId,
                              int16_t port,
                              int32_t channel,
                              int16_t key,
                              const Callback &callback) const {
         for (auto it = _activeVoices.begin(); !it.end(); ++it) {
            auto voice = containerOf(it.item(), &VoiceModule::_stateHook);
            assert(voice->isAssigned());
            if (voice->match(noteId, port, channel, key))
               callback(*voice);
         }
      }

      VoiceModule *getVoice(uint32_t voiceIndex) noexcept { return _voices[voiceIndex].get(); }
      const VoiceModule *getVoice(uint32_t voiceIndex) const noexcept {
         return _voices[voiceIndex].get();
      }

      auto &noteEndQueue() { return _noteEndQueue; }

      uint32_t getVoiceCount() const noexcept;
      uint32_t getVoiceCapacity() const noexcept;

   private:
      VoiceModule *assignVoice();
      void releaseVoice(VoiceModule &);

      IntrusiveList _activeVoices;   // uses VoiceModule._stateHook
      IntrusiveList _sleepingVoices; // uses VoiceModule._stateHook

      uint32_t _activeVoiceCount = 0;

      std::array<std::unique_ptr<VoiceModule>, MAX_VOICES> _voices;

      AudioBuffer<double> _outputBuffer;

      helpers::NoteEndQueue _noteEndQueue;

      Parameter *_voicesParam = nullptr;
      Parameter *_voiceModeParam = nullptr;
   };
} // namespace clap
