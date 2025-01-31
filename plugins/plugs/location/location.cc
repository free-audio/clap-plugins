#include <cstring>
#include <sstream>

#include "location.hh"

namespace clap {
   class LocationModule final : public Module {
   public:
      LocationModule(Location &plugin) : Module(plugin, "", 0) {}

      clap_process_status process(const Context &c, uint32_t numFrames) noexcept override {
         return CLAP_PROCESS_SLEEP;
      }
   };

   const clap_plugin_descriptor *Location::descriptor() {
      static const char *features[] = {
         CLAP_PLUGIN_FEATURE_UTILITY, CLAP_PLUGIN_FEATURE_ANALYZER, nullptr};

      static const clap_plugin_descriptor desc = {CLAP_VERSION,
                                                  "com.github.free-audio.clap.location",
                                                  "Location Test",
                                                  "clap",
                                                  "https://github.com/free-audio/clap",
                                                  nullptr,
                                                  nullptr,
                                                  "1",
                                                  "Displays current plugin location",
                                                  features};

      return &desc;
   }

   Location::Location(const std::string &pluginPath, const clap_host &host)
      : CorePlugin(PathProvider::create(pluginPath, "track-info"), descriptor(), host) {
      _rootModule = std::make_unique<LocationModule>(*this);
   }

   void Location::locationSetLocation(const clap_plugin_location_element_t *path,
                                      uint32_t num_elements) noexcept {
      std::ostringstream loc;

      loc << "plugin location: [";
      for (uint32_t i = 0; i < num_elements; ++i) {
         auto &elt = path[i];

         loc << "{ " << "kind: " << elt.kind << ", name: " << elt.name << ", id: " << elt.id
             << ", index: " << elt.index << ", color: {" << (int)elt.color.red << ", " << (int)elt.color.green
             << ", " << (int)elt.color.blue << ", " << (int)elt.color.alpha << " }, " << " }, ";
      }
      loc << "]";
      auto msg = loc.str();

      _host.log(CLAP_LOG_INFO, msg.c_str());
   }
} // namespace clap
