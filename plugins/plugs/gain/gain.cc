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
                                   -40,
                                   40,
                                   0,
                                   DecibelValueType::instance);
      }

      clap_process_status process(Context &c, uint32_t numFrames) noexcept override {
         assert(_isActive);

         const uint32_t N = numFrames * c.audioInputs[0]->channelCount();
         auto in = c.audioInputs[0]->data();
         const auto inStride = c.audioInputs[0]->stride();
         auto out = c.audioOutputs[0]->data();

         auto &gainValueBuffer = _gainParam->valueBuffer();
         auto gainValue = gainValueBuffer.data();
         auto gainValueStride = gainValueBuffer.stride();

         auto &gainModulationBuffer = _gainParam->modulationBuffer();
         auto gainModulation = gainModulationBuffer.data();
         auto gainModulationStride = gainModulationBuffer.stride();

         for (uint32_t i = 0; i < N; ++i) {
            const double gaindB =
               gainValue[i * gainValueStride] + gainModulation[i * gainModulationStride];
            const double gain = _gainConv.convert(gaindB);
            out[i] = gain * in[i * inStride];
         }

         return CLAP_PROCESS_SLEEP;
      }

      Parameter *_gainParam = nullptr;
      GainConverter _gainConv{0};
   };

   const clap_plugin_descriptor *Gain::descriptor() {
      static const char *features[] = {"mix", "gain", "audio_effect", nullptr};

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
         features};
      return &desc;
   }

   enum {
      kParamIdGain = 0,
   };

   Gain::Gain(const std::string &pluginPath, const clap_host *host)
      : CorePlugin(PathProvider::create(pluginPath, "gain"), descriptor(), host) {}

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