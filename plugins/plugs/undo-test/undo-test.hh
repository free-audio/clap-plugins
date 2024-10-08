#pragma once

#include <optional>
#include <string>

#include "../../core-plugin.hh"

namespace clap {
   template <bool hasDelta, bool areDeltasPersistant>
   class UndoTest final : public CorePlugin {
      using super = CorePlugin;

   public:
      UndoTest(const std::string &pluginPath, const clap_host &host);

      static const clap_plugin_descriptor *descriptor();

      bool init() noexcept override;

      std::vector<uint8_t> stateSaveExtra() noexcept override;
      bool stateLoadExtra(const std::vector<uint8_t> &data) noexcept override;

      bool implementsUndoDelta() const noexcept override;
      void undoDeltaGetDeltaProperties(clap_undo_delta_properties_t *properties) noexcept override;
      bool undoDeltaCanUseDeltaFormatVersion(clap_id format_version) noexcept override;
      bool undoDeltaUndo(clap_id format_version, const void *delta, size_t delta_size) noexcept override;
      bool undoDeltaRedo(clap_id format_version, const void *delta, size_t delta_size) noexcept override;

      void incrementState();
      void notifyGuiStateProperties();

#ifndef CLAP_PLUGINS_HEADLESS
      void guiPopulateProperties() override;
      void onGuiInvoke(
         const std::string &method,
         const std::vector<std::variant<bool, int64_t, double, std::string>> &args) override;
#endif

   private:
      uint32_t _counter{0};
   };
} // namespace clap