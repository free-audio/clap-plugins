#pragma once

#include <string>
#include <optional>

#include "../../core-plugin.hh"

namespace clap {
   class UndoTest final : public CorePlugin {
   private:
      using super = CorePlugin;

   public:
      UndoTest(const std::string &pluginPath, const clap_host &host);

      static const clap_plugin_descriptor *descriptor();

      bool init() noexcept override;

      bool implementsUndo() const noexcept override;
      void undoGetDeltaProperties(clap_undo_delta_properties_t *properties) noexcept override;
      bool undoCanUseDeltaFormatVersion(clap_id format_version) noexcept override;
      bool undoUndo(clap_id format_version, const void *delta, size_t delta_size) noexcept override;
      bool undoRedo(clap_id format_version, const void *delta, size_t delta_size) noexcept override;

      void incrementState();

   private:
      uint32_t _state{0};
   };
} // namespace clap