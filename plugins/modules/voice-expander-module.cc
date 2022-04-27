#include "../container-of.hh"
#include "../merge-process-status.hh"

#include "../container-of.hh"
#include "../parameter.hh"
#include "../value-types/enumerated-value-type.hh"
#include "voice-expander-module.hh"
#include "voice-module.hh"

namespace clap {

   VoiceExpanderModule::VoiceExpanderModule(CorePlugin &plugin,
                                            uint32_t moduleId,
                                            std::unique_ptr<Module> module,
                                            uint32_t channelCount)
      : Module(plugin, "voice-expander", moduleId), _outputBuffer(channelCount, BLOCK_SIZE) {

      _voices[0] = std::make_unique<VoiceModule>(_plugin, std::move(module), 1);
      for (uint32_t i = 1; i < _voices.size(); ++i) {
         _voices[i] = std::make_unique<VoiceModule>(*_voices[0]);
         _voices[i]->setVoiceIndex(i);
      }

      for (auto &voice : _voices)
         _sleepingVoices.pushBack(&voice->_stateHook);

      _voicesParam = addParameter(0,
                                  "Voices",
                                  CLAP_PARAM_IS_STEPPED | CLAP_PARAM_IS_AUTOMATABLE,
                                  std::make_unique<SimpleValueType>(1, MAX_VOICES, MAX_VOICES));
      _voiceModeParam = addParameter(1,
                                     "Voice Mode",
                                     CLAP_PARAM_IS_STEPPED | CLAP_PARAM_IS_AUTOMATABLE,
                                     std::make_unique<EnumeratedValueType>(
                                        std::vector<std::string>{"Legato", "Mono", "Poly"}, 2));
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

   clap_process_status VoiceExpanderModule::process(const Context &c, uint32_t numFrames) noexcept {
      clap_process_status status = CLAP_PROCESS_SLEEP;

      _outputBuffer.clear(0);

      for (auto it = _activeVoices.begin(); !it.end();) {
         auto voice = containerOf(it.item(), &VoiceModule::_stateHook);
         auto voiceStatus = voice->process(c, numFrames);
         status = mergeProcessStatus(status, voiceStatus);
         ++it;

         _outputBuffer.sum(_outputBuffer, voice->_outputBuffer, numFrames);

         if (voiceStatus == CLAP_PROCESS_SLEEP)
            releaseVoice(*voice);
      }

      return status;
   }

   VoiceModule *VoiceExpanderModule::findActiveVoice(int32_t key, int32_t channel) const {
      for (auto it = _activeVoices.begin(); !it.end(); ++it) {
         auto voice = containerOf(it.item(), &VoiceModule::_stateHook);
         assert(voice->isAssigned());
         if (voice->match(key, channel))
            return voice;
      }

      return nullptr;
   }

   VoiceModule *VoiceExpanderModule::assignVoice() {
      if (_sleepingVoices.empty() || _activeVoiceCount >= getVoiceCount())
         return nullptr; // TODO: steal voice instead

      assert(_activeVoiceCount < MAX_VOICES);
      auto voice = containerOf(_sleepingVoices.front(), &VoiceModule::_stateHook);
      assert(!voice->isAssigned());
      voice->_stateHook.unlink();
      voice->assign();
      _activeVoices.pushBack(&voice->_stateHook);
      ++_activeVoiceCount;
      return voice;
   }

   void VoiceExpanderModule::releaseVoice(VoiceModule &voice) {
      assert(voice.isAssigned());
      assert(_activeVoiceCount > 0);
      voice._stateHook.unlink();
      voice._isAssigned = false;
      _sleepingVoices.pushBack(&voice._stateHook);
      --_activeVoiceCount;
      _noteEndQueue.onNoteEnd(0, voice.channel(), voice.key());

      while (!voice._parametersToReset.empty()) {
         auto paramVoice =
            containerOf(voice._parametersToReset.front(), &Parameter::Voice::_resetHook);
         paramVoice->_hasValue = false;
         paramVoice->_hasModulation = false;
         paramVoice->_hasModulatedValue = false;
         paramVoice->_resetHook.unlink();
         paramVoice->_valueToProcessHook.unlink();
         paramVoice->_modulationToProcessHook.unlink();
         paramVoice->_modulatedValueToProcessHook.unlink();
      }
   }

   bool VoiceExpanderModule::wantsNoteEvents() const noexcept { return true; }

   void VoiceExpanderModule::onNoteOn(const clap_event_note &note) noexcept {
      _noteEndQueue.onNoteOn(0, note.channel, note.key);

      auto voice = findActiveVoice(note.key, note.channel);
      if (voice) {
         voice->onNoteOn(note);
         return;
      }

      voice = assignVoice();
      if (!voice) {
         _noteEndQueue.onNoteEnd(0, note.channel, note.key);
         return;
      }

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

   void VoiceExpanderModule::onNoteExpression(const clap_event_note_expression &noteExp) noexcept {
      auto voice = findActiveVoice(noteExp.key, noteExp.channel);
      if (voice)
         voice->onNoteExpression(noteExp);
   }

   uint32_t VoiceExpanderModule::getVoiceCount() const noexcept {
      int voices = _voicesParam->value();

      assert(voices > 0);
      assert(voices <= MAX_VOICES);

      return std::clamp<uint32_t>(voices, 1, MAX_VOICES);
   }

   uint32_t VoiceExpanderModule::getVoiceCapacity() const noexcept { return MAX_VOICES; }
} // namespace clap
