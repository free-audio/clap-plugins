#include <cstring>

#include "transport-info.hh"

namespace clap {
   class TransportInfoModule final : public Module {
   public:
      TransportInfoModule(TransportInfo &plugin) : Module(plugin, "", 0) {}

      clap_process_status process(const Context &c, uint32_t numFrames) noexcept override {
         return CLAP_PROCESS_CONTINUE;
      }
   };

   const clap_plugin_descriptor *TransportInfo::descriptor() {
      static const char *features[] = {"utility", "analyzer", nullptr};

      static const clap_plugin_descriptor desc = {
         CLAP_VERSION,
         "com.github.free-audio.clap.transport-info",
         "Transport Info",
         "clap",
         "https://github.com/free-audio/clap",
         nullptr,
         nullptr,
         "0.1",
         "Displays transport info",
         features
      };

      return &desc;
   }

   enum {
      kParamIdOffset = 0,
   };

   TransportInfo::TransportInfo(const std::string &pluginPath, const clap_host *host)
      : CorePlugin(PathProvider::create(pluginPath, "transport-info"), descriptor(), host) {
         _rootModule = std::make_unique<TransportInfoModule>(*this);
      }
} // namespace clap
