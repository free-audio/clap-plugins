#include <cstring>

#include "../../sample-delay.hh"
#include "realtime-requirement.hh"

namespace clap {

   class RealtimeRequirementModule final : public Module {
      using super = Module;

   public:
      RealtimeRequirementModule(RealtimeRequirement &plugin) : Module(plugin, "", 0) {}

      clap_process_status process(const Context &c, uint32_t numFrames) noexcept override {
         assert(_isActive);

         auto &in = *c.audioInputs[0];
         auto &out = *c.audioOutputs[0];

         out.copy(in, numFrames);

         return CLAP_PROCESS_SLEEP;
      }
   };

   const clap_plugin_descriptor *RealtimeRequirement::descriptor() {
      static const char *features[] = {
         CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_UTILITY, nullptr};

      static const clap_plugin_descriptor desc = {

         CLAP_VERSION,
         "com.github.free-audio.clap.realtime-requirement",
         "Realtime Requirement",
         "clap",
         "https://github.com/free-audio/clap",
         nullptr,
         nullptr,
         "0.1",
         "Test plugin used to determine if the host correctly handles a plugin having a realtime "
         "requirement",
         features};
      return &desc;
   }

   RealtimeRequirement::RealtimeRequirement(const std::string &pluginPath, const clap_host *host)
      : CorePlugin(PathProvider::create(pluginPath, "Offline Latency"), descriptor(), host) {
      _rootModule = std::make_unique<RealtimeRequirementModule>(*this);
   }

   bool RealtimeRequirement::init() noexcept {
      if (!super::init())
         return false;

      defineAudioPorts();
      return true;
   }

   void RealtimeRequirement::defineAudioPorts() noexcept {
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

   bool RealtimeRequirement::renderHasHardRealtimeRequirement() noexcept { return true; }
} // namespace clap
