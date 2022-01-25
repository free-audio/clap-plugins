#include <cstring>

#include "gain.hh"

namespace clap {
   const clap_plugin_descriptor *Gain::descriptor() {
      static const char *features[] = {
         "mix", "gain", "audio_effect", nullptr
      };

      static const clap_plugin_descriptor desc = {

         CLAP_VERSION,
         "com.github.free-audio.clap.gain",
         "gain",
         "clap",
         "https://github.com/free-audio/clap",
         nullptr,
         nullptr,
         "0.1",
         "example gain plugin",
         features
      };
      return &desc;
   }

   enum {
      kParamIdGain = 0,
   };

   Gain::Gain(const std::string &pluginPath, const clap_host *host)
      : CorePlugin(PathProvider::create(pluginPath, "gain"), descriptor(), host) {
      _parameters.addParameter(clap_param_info{
         kParamIdGain,
         0,
         nullptr,
         "gain",
         "/",
         -1,
         1,
         0,
      });
   }

   bool Gain::init() noexcept {
      if (!super::init())
         return false;

      defineAudioPorts();
      return true;
   }

   void Gain::defineAudioPorts() noexcept {
      assert(!isActive());

      _channelCount = trackChannelCount();

      clap_audio_port_info info;
      info.id = 0;
      strncpy(info.name, "main", sizeof(info.name));
      info.flags = CLAP_AUDIO_PORT_IS_MAIN;
      info.in_place_pair = 0;
      info.channel_count = _channelCount;
      info.port_type = nullptr;

      _audioInputs.clear();
      _audioInputs.push_back(info);
      _audioOutputs.clear();
      _audioOutputs.push_back(info);
   }

   void Gain::deactivate() noexcept { _channelCount = 0; }

   clap_process_status Gain::process(const clap_process *process) noexcept {
      float **in = process->audio_inputs[0].data32;
      float **out = process->audio_outputs[0].data32;

      float k = 1;
      for (int i = 0; i < process->frames_count; ++i) {
         for (int c = 0; c < _channelCount; ++c)
            out[c][i] = k * in[c][i];
      }

      return CLAP_PROCESS_CONTINUE_IF_NOT_QUIET;
   }
} // namespace clap