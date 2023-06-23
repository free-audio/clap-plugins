#include <cstring>

#include "../../sample-delay.hh"
#include "offline-latency.hh"

namespace clap {

   class OfflineLatencyModule final : public Module {
      using super = Module;

   public:
      OfflineLatencyModule(OfflineLatency &plugin) : Module(plugin, "", 0) {}

      bool doActivate(double sampleRate, uint32_t maxFrameCount, bool isRealTime) override {
         _delay.setDelayTime(isRealTime ? 0 : sampleRate);
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

      SampleDelay<double> _delay{1};
   };

   const clap_plugin_descriptor *OfflineLatency::descriptor() {
      static const char *features[] = {
         CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_UTILITY, nullptr};

      static const clap_plugin_descriptor desc = {

         CLAP_VERSION,
         "com.github.free-audio.clap.offline-latency",
         "Offline Latency",
         "clap",
         "https://github.com/free-audio/clap",
         nullptr,
         nullptr,
         "0.1",
         "Test plugin used to determine if the host correctly handles a plugin introducing latency "
         "when activated in offline rendering",
         features};
      return &desc;
   }

   OfflineLatency::OfflineLatency(const std::string &pluginPath, const clap_host *host)
      : CorePlugin(PathProvider::create(pluginPath, "Offline Latency"), descriptor(), host) {
      _rootModule = std::make_unique<OfflineLatencyModule>(*this);
   }

   bool OfflineLatency::init() noexcept {
      if (!super::init())
         return false;

      defineAudioPorts();
      return true;
   }

   void OfflineLatency::defineAudioPorts() noexcept {
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
