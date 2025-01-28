#pragma once

#include "../../core-plugin.hh"

namespace clap {
   class GainMetering final : public CorePlugin {
   private:
      using super = CorePlugin;

   public:
      GainMetering(const std::string &pluginPath, const clap_host &host);

      static const clap_plugin_descriptor *descriptor();

   protected:
      // clap_plugin
      bool init() noexcept override;
      void defineAudioPorts() noexcept;
      clap_process_status processBackup(const clap_process *process) noexcept;

      bool implementsGainAdjustmentMetering() const noexcept override;
      double gainAdjustmentMeteringGet() noexcept override;

   private:
      int _channelCount = 1;
   };
} // namespace clap