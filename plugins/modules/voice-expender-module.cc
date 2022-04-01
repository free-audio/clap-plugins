#include "../container-of.hh"

#include "voice-expender-module.hh"
#include "voice-module.hh"

namespace clap {

   VoiceExpanderModule::VoiceExpanderModule(CorePlugin &plugin, clap_id paramIdStart)
      : Module(plugin, "voice-expander", paramIdStart) {}

   bool VoiceExpanderModule::activate(double sampleRate, uint32_t maxFrameCount) {
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

   void VoiceExpanderModule::deactivate() {
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