#pragma once

#include "../../core-plugin.hh"

namespace clap {
   class OfflineLatency final : public CorePlugin {
   private:
      using super = CorePlugin;

   public:
      OfflineLatency(const std::string &pluginPath, const clap_host &host);

      static const clap_plugin_descriptor *descriptor();

   protected:
      // clap_plugin
      bool init() noexcept override;
      void defineAudioPorts() noexcept;
   };
} // namespace clap