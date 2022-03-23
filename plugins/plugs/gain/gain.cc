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
         -40,
         40,
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
      const uint32_t evCount = process->in_events->size(process->in_events);
      uint32_t nextEvIndex = 0;
      uint32_t N = process->frames_count;

      processGuiEvents(process);

      /* foreach frames */
      for (uint32_t i = 0; i < process->frames_count;) {

         N = processEvents(process, nextEvIndex, evCount, i);

         /* Process as many samples as possible until the next event */
         for (; i < N; ++i) {
            const float gaindB = _gainParam->step();
            const float gain = std::pow(10.0, gaindB / 20.0);

            for (int c = 0; c < _channelCount; ++c)
               out[c][i] = gain * in[c][i];
         }
      }

      _pluginToGuiQueue.producerDone();

      return CLAP_PROCESS_CONTINUE_IF_NOT_QUIET;
   }
} // namespace clap