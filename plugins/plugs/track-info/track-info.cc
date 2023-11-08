#include <cstring>

#include "track-info.hh"

namespace clap {
   class TrackInfoModule final : public Module {
   public:
      TrackInfoModule(TrackInfo &plugin) : Module(plugin, "", 0) {}

      clap_process_status process(const Context &c, uint32_t numFrames) noexcept override {
         return CLAP_PROCESS_CONTINUE;
      }
   };

   const clap_plugin_descriptor *TrackInfo::descriptor() {
      static const char *features[] = {
         CLAP_PLUGIN_FEATURE_UTILITY, CLAP_PLUGIN_FEATURE_ANALYZER, nullptr};

      static const clap_plugin_descriptor desc = {CLAP_VERSION,
                                                  "com.github.free-audio.clap.track-info",
                                                  "Track Info",
                                                  "clap",
                                                  "https://github.com/free-audio/clap",
                                                  nullptr,
                                                  nullptr,
                                                  "0.1",
                                                  "Displays current track info",
                                                  features};

      return &desc;
   }

   enum {
      kParamIdOffset = 0,
   };

   TrackInfo::TrackInfo(const std::string &pluginPath, const clap_host &host)
      : CorePlugin(PathProvider::create(pluginPath, "track-info"), descriptor(), host) {
      _rootModule = std::make_unique<TrackInfoModule>(*this);
   }
} // namespace clap
