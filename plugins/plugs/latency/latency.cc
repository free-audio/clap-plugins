#include <cstring>

#include "../../sample-delay.hh"
#include "latency.hh"

namespace clap {

   class LatencyModule final : public Module {
      using super = Module;

   public:
      LatencyModule(Latency &plugin) : Module(plugin, "", 0) {}

      bool doActivate(double sampleRate, uint32_t maxFrameCount, bool isRealTime) override {
         _delay.setDelayTime(.2 * sampleRate);
         _delay.reset(0);
         return true;
      }

      clap_process_status process(const Context &c, uint32_t numFrames) noexcept override {
         assert(_isActive);

         auto &in = *c.audioInputs[0];
         auto &out = *c.audioOutputs[0];

         _delay.process(in, out, numFrames);

         return CLAP_PROCESS_CONTINUE;
      }

      uint32_t latency() const noexcept override { return _delay.getDelayTime(); }

      SampleDelay<double> _delay{2};
   };

   const clap_plugin_descriptor *Latency::descriptor() {
      static const char *features[] = {
         CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_UTILITY, nullptr};

      static const clap_plugin_descriptor desc = {

         CLAP_VERSION,
         "com.github.free-audio.clap.latency",
         "Latency",
         "clap",
         "https://github.com/free-audio/clap",
         nullptr,
         nullptr,
         "1",
         "Test plugin used to determine if the host correctly handles a plugin introducing latency.",
         features};
      return &desc;
   }

   Latency::Latency(const std::string &pluginPath, const clap_host &host)
      : CorePlugin(PathProvider::create(pluginPath, "Latency"), descriptor(), host) {
      _rootModule = std::make_unique<LatencyModule>(*this);
   }

   bool Latency::init() noexcept {
      if (!super::init())
         return false;

      defineAudioPorts();
      return true;
   }

   void Latency::defineAudioPorts() noexcept {
      assert(!isActive());

      clap_audio_port_info info;
      info.id = 0;
      strncpy(info.name, "main", sizeof(info.name));
      info.flags = CLAP_AUDIO_PORT_IS_MAIN;
      info.in_place_pair = CLAP_INVALID_ID;
      info.channel_count = 2;
      info.port_type = nullptr;

      _audioInputs.clear();
      _audioInputs.push_back(info);
      _audioOutputs.clear();
      _audioOutputs.push_back(info);
   }
} // namespace clap
