#pragma once

#include "../../core-plugin.hh"

namespace clap {
   template <bool IsDynamic>
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
      bool miniCurveDisplayRender(uint16_t *data, uint32_t data_size) noexcept override;
      void miniCurveDisplaySetObserved(bool is_observed) noexcept override;
      bool miniCurveDisplayGetAxisName(char *x_name,
                                       char *y_name,
                                       uint32_t name_capacity) noexcept override;

   private:
      int _channelCount = 1;
      bool _isObserved = false;
   };
} // namespace clap

extern template class clap::MiniCurveDisplay<false>;
extern template class clap::MiniCurveDisplay<true>;