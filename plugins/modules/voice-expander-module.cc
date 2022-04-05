#include "../container-of.hh"

#include "voice-expander-module.hh"
#include "voice-module.hh"

namespace clap {

   VoiceExpanderModule::VoiceExpanderModule(CorePlugin &plugin,
                                            uint32_t moduleId,
                                            std::unique_ptr<Module> module)
      : Module(plugin, "voice-expander", moduleId) {

      _voices[0] = std::make_unique<VoiceModule>(_plugin, std::move(module));
      for (uint32_t i = 1; i < _voices.size(); ++i)
         _voices[i] = std::make_unique<VoiceModule>(*_voices[0]);

      for (auto &voice : _voices)
         _sleepingVoices.pushBack(&voice->_stateHook);
   }

   bool VoiceExpanderModule::doActivate(double sampleRate, uint32_t maxFrameCount) {
      for (auto &voice : _voices) {
         if (voice) {
            if (!voice->activate(sampleRate, maxFrameCount)) {
               deactivate();
               return false;
            }
         }
      }
      return true;
   }

   void VoiceExpanderModule::doDeactivate() {
      for (auto &voice : _voices)
         if (voice)
            voice->deactivate();
   }

   clap_process_status VoiceExpanderModule::process(Context &c, uint32_t numFrames) noexcept {
      for (auto it = _activeVoices.begin(); !it.end(); ++it) {
         auto voice = containerOf(it.item(), &VoiceModule::_stateHook);
         voice->process(c, numFrames);
         // TODO if the voice returns sleep, then it is over
      }

      return CLAP_PROCESS_SLEEP;
   }

   VoiceModule *VoiceExpanderModule::findActiveVoice(int32_t key, int32_t channel) const {
      for (auto it = _activeVoices.begin(); !it.end(); ++it) {
         auto voice = containerOf(it.item(), &VoiceModule::_stateHook);
         if (voice->match(key, channel))
            return voice;
      }

      return nullptr;
   }

   VoiceModule *VoiceExpanderModule::assignVoice(int32_t key, int32_t channel) {
      if (_sleepingVoices.empty())
         return nullptr; // TODO: steal voice instead

      auto voice = containerOf(_sleepingVoices.front(), &VoiceModule::_stateHook);
      voice->_stateHook.unlink();
      _activeVoices.pushBack(&voice->_stateHook);
      return voice;
   }

   bool VoiceExpanderModule::wantsNoteEvents() const noexcept { return true; }

   void VoiceExpanderModule::onNoteOn(const clap_event_note &note) noexcept {
      auto voice = findActiveVoice(note.key, note.channel);
      if (voice) {
         voice->onNoteOn(note);
         return;
      }

      voice = assignVoice(note.key, note.channel);
      if (!voice)
         return;

      voice->onNoteOn(note);
   }

   void VoiceExpanderModule::onNoteOff(const clap_event_note &note) noexcept {
      auto voice = findActiveVoice(note.key, note.channel);
      if (voice)
         voice->onNoteOff(note);
   }

   void VoiceExpanderModule::onNoteChoke(const clap_event_note &note) noexcept {
      auto voice = findActiveVoice(note.key, note.channel);
      if (voice)
         voice->onNoteChoke(note);
   }
} // namespace clap
