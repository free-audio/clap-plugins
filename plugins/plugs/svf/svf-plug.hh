#pragma once

#include "../../core-plugin.hh"

namespace clap {
   class SvfPlug final : public CorePlugin {
   private:
      using super = CorePlugin;

   public:
      SvfPlug(const std::string &pluginPath, const clap_host *host);

      static const clap_plugin_descriptor *descriptor();

   protected:
      bool init() noexcept override;
      void defineAudioPorts() noexcept;
   };
} // namespace clap
