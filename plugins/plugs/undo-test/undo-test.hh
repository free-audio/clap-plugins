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

      bool implementsUndo() const noexcept;
      void undoGetDeltaProperties(clap_undo_delta_properties_t *properties) noexcept;
      bool undoCanUseDeltaFormatVersion(clap_id format_version) noexcept;
      bool undoUndo(clap_id format_version, const void *delta, size_t delta_size) noexcept;
      bool undoRedo(clap_id format_version, const void *delta, size_t delta_size) noexcept;
      void undoSetCanUndo(bool can_undo) noexcept;
      void undoSetCanRedo(bool can_redo) noexcept;
      void undoSetUndoName(const char *name) noexcept;
      void undoSetRedoName(const char *name) noexcept;

      void incrementState();
      void requestHostUndo();
      void requestHostRedo();

   private:
      uint32_t _state{0};

      bool _canUndo{false};
      bool _canRedo{false};
      std::optional<std::string> _undoName;
      std::optional<std::string> _redoName;
   };
} // namespace clap