#include <cstring>

#include "gain-metering.hh"

namespace clap {

   class GainMeteringModule final : public Module {
   public:
      GainMeteringModule(GainMetering &plugin) : Module(plugin, "", 0) {}

      clap_process_status process(const Context &c, uint32_t numFrames) noexcept override {
         assert(_isActive);

         c.audioOutputs[0]->copy(*c.audioInputs[0], numFrames);

         const double phaseInc = 0.5 * numFrames * c.sampleRateInvD;
         _phase += phaseInc;
         _phase -= std::floor(_phase);

         assert(_phase >= 0);
         assert(_phase < 1);

         return CLAP_PROCESS_CONTINUE;
      }

      double getGainAdjustmentMetering() const noexcept {
         assert(_phase >= 0);
         assert(_phase < 1);

         // -24..0
         const double g = -12 * (std::sin(_phase * 2 * M_PI) + 1);

         assert(-24 <= g);
         assert(g <= 0);

         return g;
      }

      double _phase = 0;
   };

   const clap_plugin_descriptor *GainMetering::descriptor() {
      static const char *features[] = {
         CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_UTILITY, nullptr};

      static const clap_plugin_descriptor desc = {

         CLAP_VERSION,
         "com.github.free-audio.clap.gain-adjustment-metering",
         "Gain Adjustment Metering",
         "clap",
         "https://github.com/free-audio/clap",
         nullptr,
         nullptr,
         "0.1",
         "example gain adjustment metering plugin",
         features};
      return &desc;
   }

   GainMetering::GainMetering(const std::string &pluginPath, const clap_host &host)
      : CorePlugin(PathProvider::create(pluginPath, "gain-metering"), descriptor(), host) {
      _rootModule = std::make_unique<GainMeteringModule>(*this);
   }

   bool GainMetering::init() noexcept {
      if (!super::init())
         return false;

      defineAudioPorts();
      return true;
   }

   void GainMetering::defineAudioPorts() noexcept {
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

   bool GainMetering::implementsGainAdjustmentMetering() const noexcept { return true; }

   double GainMetering::gainAdjustmentMeteringGet() noexcept {
      const auto *m = reinterpret_cast<const GainMeteringModule *>(_rootModule.get());
      return m->getGainAdjustmentMetering();
   }
} // namespace clap
