#pragma once

#include "../../core-plugin.hh"

namespace clap {
   class DcOffsetWithLatency final : public CorePlugin {
   private:
      using super = CorePlugin;

   public:
      DcOffsetWithLatency(const std::string& pluginPath, const clap_host &host);

      static const clap_plugin_descriptor *descriptor();

   protected:
      bool init() noexcept override;
      void defineAudioPorts() noexcept;

   private:
      int _channelCount = 2;
   };
} // namespace clap
