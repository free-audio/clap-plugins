#include "synth.hh"

#include <cstring>

#include "../../audio-buffer.hh"
#include "../../modules/voice-expander-module.hh"
#include "../modules/module.hh"
#include "synth.hh"

namespace clap {

   const clap_plugin_descriptor *Synth::descriptor() {
      static const char *features[] = {"instrument", nullptr};

      static const clap_plugin_descriptor desc = {

         CLAP_VERSION,
         "com.github.free-audio.clap.synth",
         "CLAP Synth",
         "clap",
         "https://github.com/free-audio/clap-plugins",
         nullptr,
         nullptr,
         "0.1",
         "Example Synthesizer plugin",
         features};
      return &desc;
   }

   Synth::Synth(const std::string &pluginPath, const clap_host *host)
      : CorePlugin(PathProvider::create(pluginPath, "synth"), descriptor(), host) {
      auto aeg = std::make_unique<AdsrModule>(*this, "Amp Env", 1);
      _rootModule = std::make_unique<VoiceExpanderModule>(*this, 0, std::move(aeg));
   }

   bool Synth::init() noexcept {
      if (!super::init())
         return false;

      defineAudioPorts();
      return true;
   }

   void Synth::defineAudioPorts() noexcept {
      assert(!isActive());

      clap_audio_port_info info;
      info.id = 0;
      strncpy(info.name, "main", sizeof(info.name));
      info.flags = CLAP_AUDIO_PORT_IS_MAIN;
      info.in_place_pair = CLAP_INVALID_ID;
      info.channel_count = 2;
      info.port_type = CLAP_PORT_STEREO;

      _audioInputs.clear();
      _audioOutputs.clear();
      _audioOutputs.push_back(info);
   }
} // namespace clap
