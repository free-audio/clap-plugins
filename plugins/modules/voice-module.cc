#include "voice-module.hh"
#include "../core-plugin.hh"
#include "../tuning-provider.hh"

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

   VoiceModule::~VoiceModule()
   {
      if (_stateHook.isHooked())
         _stateHook.unlink();
   }

   std::unique_ptr<Module> VoiceModule::cloneVoice() const {
      return std::make_unique<VoiceModule>(*this);
   }

   void VoiceModule::reset() noexcept { _module->reset(); }

   void VoiceModule::assign() noexcept {
      assert(!isAssigned());
      _isAssigned = true;
      _wasJustAssigned = true;

      _tuning.setImmediately(0);
      _vibrato.setImmediately(0);
      _brigthness.setImmediately(0);
      _pressure.setImmediately(0);
      _expression.setImmediately(0);
      _volume.setImmediately(0);
      _pan.setImmediately(0.5);
   }

   bool VoiceModule::match(int32_t noteId, int16_t port, int16_t channel, int16_t key) const {
      if (noteId != -1 && noteId != _noteId)
         return false;

      if (port != -1 && port != _port)
         return false;

      if (channel != -1 && channel != _channel)
         return false;

      if (key != -1 && key != _key)
         return false;

      return true;
   }

   bool VoiceModule::wantsNoteEvents() const noexcept { return true; }

   void VoiceModule::onNoteOn(const clap_event_note &note) noexcept {
      assert(isAssigned());

      _key = note.key;
      _channel = note.channel;
      _velocity = note.velocity;
      _port = note.port_index;
      _noteId = note.note_id;
      _keyFreq = _plugin.tuningProvider().getFreq(0, _channel, _key);

      // TODO: glide

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

   void VoiceModule::onNoteExpression(const clap_event_note_expression &noteExp) noexcept {
      if (_module->wantsNoteEvents()) [[likely]]
         _module->onNoteExpression(noteExp);

      switch (noteExp.expression_id) {
      case CLAP_NOTE_EXPRESSION_VOLUME:
         if (_wasJustAssigned)
            _volume.setImmediately(noteExp.value);
         else
            _volume.setSmoothed(noteExp.value, 256);
         break;

      case CLAP_NOTE_EXPRESSION_PAN:
         if (_wasJustAssigned)
            _pan.setImmediately(noteExp.value);
         else
            _pan.setSmoothed(noteExp.value, 256);
         break;

      case CLAP_NOTE_EXPRESSION_TUNING:
         if (_wasJustAssigned)
            _tuning.setImmediately(noteExp.value);
         else
            _tuning.setSmoothed(noteExp.value, 256);
         break;

      case CLAP_NOTE_EXPRESSION_VIBRATO:
         if (_wasJustAssigned)
            _vibrato.setImmediately(noteExp.value);
         else
            _vibrato.setSmoothed(noteExp.value, 256);
         break;

      case CLAP_NOTE_EXPRESSION_EXPRESSION:
         if (_wasJustAssigned)
            _expression.setImmediately(noteExp.value);
         else
            _expression.setSmoothed(noteExp.value, 256);
         break;

      case CLAP_NOTE_EXPRESSION_BRIGHTNESS:
         if (_wasJustAssigned)
            _brigthness.setImmediately(noteExp.value);
         else
            _brigthness.setSmoothed(noteExp.value, 256);
         break;

      case CLAP_NOTE_EXPRESSION_PRESSURE:
         if (_wasJustAssigned)
            _pressure.setImmediately(noteExp.value);
         else
            _pressure.setSmoothed(noteExp.value, 256);
         break;
      }
   }

   clap_process_status VoiceModule::process(const Context &c, uint32_t numFrames) noexcept {
      assert(isActive());
      assert(isAssigned());

      _tuning.render(_tuningBuffer.data(), numFrames);
      _vibrato.render(_vibratoBuffer.data(), numFrames);
      _brigthness.render(_brigthnessBuffer.data(), numFrames);
      _pressure.render(_pressureBuffer.data(), numFrames);
      _expression.render(_expressionBuffer.data(), numFrames);
      _volume.render(_volumeBuffer.data(), numFrames);
      _pan.render(_panBuffer.data(), numFrames);

      auto status = _module->process(c, numFrames);

      _wasJustAssigned = false;
      return status;
   }

   bool VoiceModule::doActivate(double sampleRate, uint32_t maxFrameCount, bool isRealTime) {
      return _module->activate(sampleRate, maxFrameCount, isRealTime);
   }

   void VoiceModule::doDeactivate() { _module->deactivate(); }
} // namespace clap
