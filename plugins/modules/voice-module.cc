#include "voice-module.hh"

namespace clap {
   VoiceModule::VoiceModule(CorePlugin &plugin, std::unique_ptr<Module> module)
      : Module(plugin, "", 0), _module(std::move(module)) {
         _module->setVoiceModule(this);
      }

   VoiceModule::VoiceModule(const VoiceModule &m) : Module(m), _module(m._module->cloneVoice()) {}

   std::unique_ptr<Module> VoiceModule::cloneVoice() const {
      return std::make_unique<VoiceModule>(*this);
   }

   void VoiceModule::reset() noexcept {
      _module->reset();
   }
} // namespace clap
