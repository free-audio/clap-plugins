#include "voice-module.hh"

namespace clap {
   VoiceModule::VoiceModule(CorePlugin &plugin, std::unique_ptr<Module> module)
      : Module(plugin, "", 0), _module(std::move(module)) {}

   std::unique_ptr<Module> VoiceModule::cloneVoice() const {
      return std::make_unique<VoiceModule>(_plugin, _module->cloneVoice());
   }
} // namespace clap
