#include <chrono>
#include <cstring>
#include <thread>

#include "../../modules/module.hh"
#include "slow-destroy.hh"

namespace clap {

   class SlowDestroyModule final : public Module {
      using super = Module;

   public:
      SlowDestroyModule(SlowDestroy &plugin) : Module(plugin, "", 0) {}

      clap_process_status process(const Context &c, uint32_t numFrames) noexcept override {
         assert(_isActive);

         auto &in = *c.audioInputs[0];
         auto &out = *c.audioOutputs[0];

         out.copy(in, numFrames);

         return CLAP_PROCESS_SLEEP;
      }
   };

   const clap_plugin_descriptor *SlowDestroy::descriptor() {
      static const char *features[] = {
         CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_UTILITY, nullptr};

      static const clap_plugin_descriptor desc = {

         CLAP_VERSION,
         "com.github.free-audio.clap.slow-destroy",
         "Slow Destroy",
         "clap",
         "https://github.com/free-audio/clap-plugins",
         nullptr,
         nullptr,
         "0.1",
         "Test plugin which takes 20 seconds to be destroyed, used to check how the host "
         "handles a slow clap_plugin.destroy()",
         features};
      return &desc;
   }

   SlowDestroy::SlowDestroy(const std::string &pluginPath, const clap_host &host)
      : CorePlugin("slow-destroy", descriptor(), host) {
      _rootModule = std::make_unique<SlowDestroyModule>(*this);
   }

   // Called from clap_plugin.destroy()
   SlowDestroy::~SlowDestroy() { std::this_thread::sleep_for(std::chrono::seconds(20)); }

   bool SlowDestroy::init() noexcept {
      if (!super::init())
         return false;

      defineAudioPorts();
      return true;
   }

   void SlowDestroy::defineAudioPorts() noexcept {
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
