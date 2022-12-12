#pragma once

#include "../../core-plugin.hh"

namespace clap {
   class Synth final : public CorePlugin {
   private:
      using super = CorePlugin;

   public:
      Synth(const std::string &pluginPath, const clap_host *host);

      static const clap_plugin_descriptor *descriptor();

   protected:
      bool init() noexcept override;
      void defineNotePorts() noexcept;
      void defineAudioPorts() noexcept;
   };
} // namespace clap
