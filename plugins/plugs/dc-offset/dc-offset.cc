#include <cstring>

#include "dc-offset.hh"

namespace clap {
   const clap_plugin_descriptor *DcOffset::descriptor() {
      static const char *features[] = {
         "utility", "audio_effect", nullptr
      };

      static const clap_plugin_descriptor desc = {

         CLAP_VERSION,
         "com.github.free-audio.clap.dc-offset",
         "DC Offset",
         "clap",
         "https://github.com/free-audio/clap",
         nullptr,
         nullptr,
         "0.1",
         "Example DC Offset plugin",
         features
      };
      return &desc;
   }

   enum {
      kParamIdOffset = 0,
   };

   DcOffset::DcOffset(const std::string &pluginPath, const clap_host *host)
      : CorePlugin(PathProvider::create(pluginPath, "dc-offset"), descriptor(), host) {
      _parameters.addParameter(clap_param_info{
         kParamIdOffset,
         CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE | CLAP_PARAM_REQUIRES_PROCESS,
         nullptr,
         "offset",
         "/",
         -1,
         1,
         0,
      });

      _offsetParam = _parameters.getById(kParamIdOffset);
   }

   bool DcOffset::init() noexcept {
      if (!super::init())
         return false;

      defineAudioPorts();
      return true;
   }

   void DcOffset::defineAudioPorts() noexcept {
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

   clap_process_status DcOffset::process(const clap_process *process) noexcept {
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
            const float offset = _offsetParam->step();
            for (int c = 0; c < _channelCount; ++c)
               out[c][i] = in[c][i] + offset;
         }
      }

      _pluginToGuiQueue.producerDone();

      return CLAP_PROCESS_CONTINUE_IF_NOT_QUIET;
   }
} // namespace clap
