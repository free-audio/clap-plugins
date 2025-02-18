#include <cstring>

#include "../../audio-buffer.hh"
#include "../../modules/module.hh"
#include "../../sample-delay.hh"
#include "dc-offset-with-latency.hh"

namespace clap {

   class DcOffsetWithLatencyModule final : public Module {
      using super = Module;

   public:
      DcOffsetWithLatencyModule(DcOffsetWithLatency &plugin) : Module(plugin, "", 0) {
         _offsetParam = addParameter(0,
                                     "offset",
                                     CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE |
                                        CLAP_PARAM_REQUIRES_PROCESS,
                                     std::make_unique<SimpleValueType>(-1, 1, 0));
         _offsetParam->enableSmoothing(false);
      }

      [[nodiscard]] bool
      doActivate(double sampleRate, uint32_t maxFrameCount, bool isRealTime) override {
         const uint32_t delayTimeInFrames = sampleRate * 0.2; // 200ms
         _delay.setDelayTime(delayTimeInFrames);
         _delay.reset(0);

         _paramOut.reset(new AudioBuffer<double>(1, maxFrameCount, sampleRate));

         return super::doActivate(sampleRate, maxFrameCount, isRealTime);
      }

      void doDeactivate() override { _paramOut.reset(); }

      clap_process_status process(const Context &c, uint32_t numFrames) noexcept override {
         assert(_isActive);

         auto &in = *c.audioInputs[0];
         auto &out = *c.audioOutputs[0];

         _delay.process(_offsetParam->voiceBuffer(_voiceModule), *_paramOut, numFrames);

         c.audioOutputs[0]->sum(*c.audioInputs[0], *_paramOut, numFrames);

         return CLAP_PROCESS_SLEEP;
      }

      uint32_t latency() const noexcept override { return _delay.getDelayTime(); }

      Parameter *_offsetParam = nullptr;
      SampleDelay<double> _delay{1};
      std::unique_ptr<AudioBuffer<double>> _paramOut;
   };

   const clap_plugin_descriptor *DcOffsetWithLatency::descriptor() {
      static const char *features[] = {
         CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_UTILITY, nullptr};

      static const clap_plugin_descriptor desc = {

         CLAP_VERSION,
         "com.github.free-audio.clap.dc-offset-with-latency",
         "DC Offset (with latency)",
         "clap",
         "https://github.com/free-audio/clap-plugins",
         nullptr,
         nullptr,
         "0.2",
         "Example DC Offset plugin with latency",
         features};
      return &desc;
   }

   DcOffsetWithLatency::DcOffsetWithLatency(const std::string &pluginPath, const clap_host &host)
      : CorePlugin(PathProvider::create(pluginPath, "dc-offset"), descriptor(), host) {
      _rootModule = std::make_unique<DcOffsetWithLatencyModule>(*this);
   }

   bool DcOffsetWithLatency::init() noexcept {
      if (!super::init())
         return false;

      defineAudioPorts();
      return true;
   }

   void DcOffsetWithLatency::defineAudioPorts() noexcept {
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
