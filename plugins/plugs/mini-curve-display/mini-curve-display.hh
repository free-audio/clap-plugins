#pragma once

#include "../../core-plugin.hh"

namespace clap {
   class MiniCurveDisplay final : public CorePlugin {
   private:
      using super = CorePlugin;

   public:
      MiniCurveDisplay(const std::string &pluginPath, const clap_host &host);

      static const clap_plugin_descriptor *descriptor();

   protected:
      // clap_plugin
      bool init() noexcept override;
      void defineAudioPorts() noexcept;
      clap_process_status processBackup(const clap_process *process) noexcept;

      bool implementsMiniCurveDisplay() const noexcept override;
      virtual bool miniCurveDisplayRender(uint16_t *data, uint32_t data_size) noexcept;
      virtual void miniCurveDisplaySetObserved(bool is_observed) noexcept;
      virtual bool
      miniCurveDisplayGetAxisName(char *x_name, char *y_name, uint32_t name_capacity) noexcept;

   private:
      int _channelCount = 1;
      bool _isObserved = false;
   };
} // namespace clap