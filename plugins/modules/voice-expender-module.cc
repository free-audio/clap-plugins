#include "../container-of.hh"

#include "voice-expender-module.hh"
#include "voice-module.hh"

namespace clap {

   VoiceExpanderModule::VoiceExpanderModule(CorePlugin &plugin,
                                            clap_id paramIdStart,
                                            std::unique_ptr<Module> module)
      : Module(plugin, "voice-expander", paramIdStart) {

      _voices[0] = std::make_unique<VoiceModule>(_plugin, std::move(module));
      for (uint32_t i = 1; i < _voices.size(); ++i)
         _voices[i] = std::make_unique<VoiceModule>(*_voices[0]);
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
         auto voice = containerOf(it.item(), &VoiceModule::_activeVoicesHook);
         voice->process(c, numFrames);
         // TODO if the voice returns sleep, then it is over
      }

      return CLAP_PROCESS_SLEEP;
   }
} // namespace clap
