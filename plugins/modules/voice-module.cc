#include "voice-module.hh"

namespace clap {
   VoiceModule::VoiceModule(CorePlugin &plugin,
                            std::unique_ptr<Module> module,
                            uint32_t channelCount)
      : Module(plugin, "", 0), _module(std::move(module)), _outputBuffer(channelCount, BLOCK_SIZE) {
      _module->setVoiceModule(this);
   }

   VoiceModule::VoiceModule(const VoiceModule &m)
      : Module(m), _module(m._module->cloneVoice()),
        _outputBuffer(m._outputBuffer.channelCount(), m._outputBuffer.frameCount()) {
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

   clap_process_status VoiceModule::process(Context &c, uint32_t numFrames) noexcept {
      assert(_isActive);
      return _module->process(c, numFrames);
   }

   bool VoiceModule::doActivate(double sampleRate, uint32_t maxFrameCount) {
      return _module->activate(sampleRate, maxFrameCount);
   }

   void VoiceModule::doDeactivate() { _module->deactivate(); }
} // namespace clap
