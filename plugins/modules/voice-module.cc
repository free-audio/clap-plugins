#include "voice-module.hh"

namespace clap {
   VoiceModule::VoiceModule(CorePlugin &plugin, std::unique_ptr<Module> module)
      : Module(plugin, "", 0), _module(std::move(module)) {
      _module->setVoiceModule(this);
   }

   VoiceModule::VoiceModule(const VoiceModule &m) : Module(m), _module(m._module->cloneVoice()) {
      _module->setVoiceModule(this);
   }

   std::unique_ptr<Module> VoiceModule::cloneVoice() const {
      return std::make_unique<VoiceModule>(*this);
   }

   void VoiceModule::reset() noexcept { _module->reset(); }

   bool VoiceModule::match(int32_t key_, int32_t channel_) const {
      return key_ == key() && channel_ == channel();
   }

   bool VoiceModule::wantsNoteEvents() const noexcept { return true; }

   void VoiceModule::onNoteOn(const clap_event_note &note) noexcept {
      _key = note.key;
      _channel = note.channel;
      _velocity = note.velocity;
      // TODO: pitch, note expression, ...

      if (_module->wantsNoteEvents()) [[likely]]
         _module->onNoteOn(note);
   }

   void VoiceModule::onNoteOff(const clap_event_note &note) noexcept {
      if (_module->wantsNoteEvents()) [[likely]]
         _module->onNoteOff(note);
   }

   void VoiceModule::onNoteChoke(const clap_event_note &note) noexcept {
      if (_module->wantsNoteEvents()) [[likely]]
         _module->onNoteOff(note);
   }
} // namespace clap
