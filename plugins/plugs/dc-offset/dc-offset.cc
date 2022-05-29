#include <cstring>

#include "../audio-buffer.hh"
#include "../modules/module.hh"
#include "dc-offset.hh"

namespace clap {

   class DcOffsetModule final : public Module {
   public:
      DcOffsetModule(DcOffset &plugin) : Module(plugin, "", 0) {
         _offsetParam = addParameter(0,
                                     "offset",
                                     CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE |
                                        CLAP_PARAM_REQUIRES_PROCESS,
                                     std::make_unique<SimpleValueType>(-1, 1, 0));
      }

      clap_process_status process(const Context &c, uint32_t numFrames) noexcept override {
         assert(_isActive);

         c.audioOutputs[0]->sum(
            *c.audioInputs[0], _offsetParam->voiceBuffer(_voiceModule), numFrames);

         return CLAP_PROCESS_SLEEP;
      }

      Parameter *_offsetParam = nullptr;
   };

   const clap_plugin_descriptor *DcOffset::descriptor() {
      static const char *features[] = {
         CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_UTILITY, nullptr};

      static const clap_plugin_descriptor desc = {

         CLAP_VERSION,
         "com.github.free-audio.clap.dc-offset",
         "DC Offset",
         "clap",
         "https://github.com/free-audio/clap-plugins",
         nullptr,
         nullptr,
         "0.2",
         "Example DC Offset plugin",
         features};
      return &desc;
   }

   DcOffset::DcOffset(const std::string &pluginPath, const clap_host *host)
      : CorePlugin(PathProvider::create(pluginPath, "dc-offset"), descriptor(), host) {
      _rootModule = std::make_unique<DcOffsetModule>(*this);
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
} // namespace clap
