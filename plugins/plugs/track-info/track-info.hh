#pragma once

#include "../../core-plugin.hh"

namespace clap {
   class TrackInfo final : public CorePlugin {
   private:
      using super = CorePlugin;

   public:
      TrackInfo(const std::string& pluginPath, const clap_host &host);

      static const clap_plugin_descriptor *descriptor();
   };
} // namespace clap