#pragma once

#include "../../core-plugin.hh"

namespace clap {
   class SlowDestroy final : public CorePlugin {
   private:
      using super = CorePlugin;

   public:
      SlowDestroy(const std::string &pluginPath, const clap_host &host);
      ~SlowDestroy() override;

      static const clap_plugin_descriptor *descriptor();

   protected:
      // clap_plugin
      bool init() noexcept override;
      void defineAudioPorts() noexcept;
   };
} // namespace clap
