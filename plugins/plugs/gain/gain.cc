#include <cstring>

#include "../../domain-converter.hh"
#include "../../value-types/decibel-value-type.hh"
#include "gain.hh"

namespace clap {

   class GainModule final : public Module {
   public:
      GainModule(Gain &plugin) : Module(plugin, "", 0) {
         _gainParam = addParameter(0,
                                   "gain",
                                   CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE |
                                      CLAP_PARAM_REQUIRES_PROCESS,
                                   std::make_unique<DecibelValueType>(-40, 40, 0));
      }

      clap_process_status process(const Context &c, uint32_t numFrames) noexcept override {
         assert(_isActive);

         c.audioOutputs[0]->product(
            *c.audioInputs[0], _gainParam->voiceBuffer(_voiceModule), numFrames);

         return CLAP_PROCESS_SLEEP;
      }

      Parameter *_gainParam = nullptr;
      GainConverter _gainConv{0};
   };

   const clap_plugin_descriptor *Gain::descriptor() {
      static const char *features[] = {
         CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_UTILITY, nullptr};

      static const clap_plugin_descriptor desc = {

         CLAP_VERSION,
         "com.github.free-audio.clap.gain",
         "Gain",
         "clap",
         "https://github.com/free-audio/clap",
         nullptr,
         nullptr,
         "0.1",
         "example gain plugin",
         features};
      return &desc;
   }

   enum {
      kParamIdGain = 0,
   };

   Gain::Gain(const std::string &pluginPath, const clap_host &host)
      : CorePlugin(PathProvider::create(pluginPath, "gain"), descriptor(), host) {
      _rootModule = std::make_unique<GainModule>(*this);
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

} // namespace clap
