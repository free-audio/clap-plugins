#include <cstring>

#include "../../gui/abstract-gui.hh"
#include "../../gui/gui-handle.hh"
#include "undo-test.hh"

namespace clap {
   namespace {
      static constexpr clap_id UNDO_FORMAT_VERSION = 3716;
      struct UndoDelta {
         uint32_t old_value;
         uint32_t new_value;
      };
   } // namespace

   template <bool hasDelta, bool areDeltasPersistant>
   class UndoTestModule final : public Module {
   public:
      UndoTestModule(UndoTest<hasDelta, areDeltasPersistant> &plugin) : Module(plugin, "", 0) {}

      clap_process_status process(const Context &c, uint32_t numFrames) noexcept override {
         return CLAP_PROCESS_SLEEP;
      }
   };

   template <bool hasDelta, bool areDeltasPersistant>
   const clap_plugin_descriptor *UndoTest<hasDelta, areDeltasPersistant>::descriptor() {
      static const char *features[] = {
         CLAP_PLUGIN_FEATURE_UTILITY, CLAP_PLUGIN_FEATURE_ANALYZER, nullptr};

      static const clap_plugin_descriptor desc = {
         CLAP_VERSION,
         areDeltasPersistant ? "com.github.free-audio.clap.undo-test"
         : hasDelta          ? "com.github.free-audio.clap.undo-test-not-persistent"
                             : "com.github.free-audio.clap.undo-test-no-deltas",
         areDeltasPersistant ? "Undo Test"
         : hasDelta          ? "Undo Test (deltas not persistent)"
                             : "Undo Test (no deltas)",
         "clap",
         "https://github.com/free-audio/clap",
         nullptr,
         nullptr,
         "0.1",
         "Help testing the undo extension",
         features};

      return &desc;
   }

   template <bool hasDelta, bool areDeltasPersistant>
   UndoTest<hasDelta, areDeltasPersistant>::UndoTest(const std::string &pluginPath,
                                                     const clap_host &host)
      : CorePlugin(PathProvider::create(pluginPath, "undo-test"), descriptor(), host) {
      _rootModule = std::make_unique<UndoTestModule<hasDelta, areDeltasPersistant>>(*this);
   }

   template <bool hasDelta, bool areDeltasPersistant>
   bool UndoTest<hasDelta, areDeltasPersistant>::implementsUndoDelta() const noexcept {
      return true;
   }

   template <bool hasDelta, bool areDeltasPersistant>
   void UndoTest<hasDelta, areDeltasPersistant>::undoDeltaGetDeltaProperties(
      clap_undo_delta_properties_t *properties) noexcept {
      properties->has_delta = hasDelta;
      properties->are_deltas_persistent = areDeltasPersistant;
      properties->format_version = hasDelta ? UNDO_FORMAT_VERSION : CLAP_INVALID_ID;
   }

   template <bool hasDelta, bool areDeltasPersistant>
   bool UndoTest<hasDelta, areDeltasPersistant>::undoDeltaCanUseDeltaFormatVersion(
      clap_id format_version) noexcept {
      if constexpr (!hasDelta)
         return false;

      return format_version == UNDO_FORMAT_VERSION;
   }

   template <bool hasDelta, bool areDeltasPersistant>
   bool UndoTest<hasDelta, areDeltasPersistant>::undoDeltaUndo(clap_id format_version,
                                                               const void *delta,
                                                               size_t delta_size) noexcept {
      if constexpr (!hasDelta)
         return false;

      if (format_version != UNDO_FORMAT_VERSION) {
         hostMisbehaving("invalid undo delta format version");
         return false;
      }

      if (delta_size != sizeof(UndoDelta)) {
         hostMisbehaving("invalid undo delta size");
         return false;
      }

      auto undoDelta = static_cast<const UndoDelta *>(delta);

      char buffer[128];
      snprintf(buffer, sizeof(buffer), "UNDO undo %d -> %d", _counter, undoDelta->old_value);
      _host.log(CLAP_LOG_INFO, buffer);

      _counter = undoDelta->old_value;

      notifyGuiStateProperties();
      return true;
   }

   template <bool hasDelta, bool areDeltasPersistant>
   bool UndoTest<hasDelta, areDeltasPersistant>::undoDeltaRedo(clap_id format_version,
                                                               const void *delta,
                                                               size_t delta_size) noexcept {
      if constexpr (!hasDelta)
         return false;

      if (format_version != UNDO_FORMAT_VERSION) {
         hostMisbehaving("invalid undo delta format version");
         return false;
      }

      if (delta_size != sizeof(UndoDelta)) {
         hostMisbehaving("invalid undo delta size");
         return false;
      }

      auto undoDelta = static_cast<const UndoDelta *>(delta);

      char buffer[128];
      snprintf(buffer, sizeof(buffer), "UNDO redo %d -> %d", _counter, undoDelta->new_value);
      _host.log(CLAP_LOG_INFO, buffer);

      _counter = undoDelta->new_value;

      notifyGuiStateProperties();
      return true;
   }

   template <bool hasDelta, bool areDeltasPersistant>
   void UndoTest<hasDelta, areDeltasPersistant>::incrementState() {
      if (!_host.canUseUndo())
         return;

      UndoDelta delta;
      delta.old_value = _counter;
      delta.new_value = ++_counter;

      char buffer[128];
      snprintf(buffer, sizeof(buffer), "UNDO increment %d -> %d", delta.old_value, delta.new_value);
      _host.log(CLAP_LOG_INFO, buffer);

      if constexpr (hasDelta)
         _host.undoChangeMade(buffer, &delta, sizeof(delta), true);
      else
         _host.undoChangeMade(buffer, nullptr, 0, 0);

      notifyGuiStateProperties();
   }

   template <bool hasDelta, bool areDeltasPersistant>
   bool UndoTest<hasDelta, areDeltasPersistant>::init() noexcept {
      if (!super::init())
         return false;

      if (_host.canUseUndo()) {
         _host.undoSetWantsContextUpdates(true);
      }
      return true;
   }

#ifndef CLAP_PLUGINS_HEADLESS
   template <bool hasDelta, bool areDeltasPersistant>
   void UndoTest<hasDelta, areDeltasPersistant>::onGuiInvoke(
      const std::string &method,
      const std::vector<std::variant<bool, int64_t, double, std::string>> &args) {
      if (method == "incrementState")
         incrementState();
      else
         super::onGuiInvoke(method, args);
   }

   template <bool hasDelta, bool areDeltasPersistant>
   void UndoTest<hasDelta, areDeltasPersistant>::guiPopulateProperties() {
      super::guiPopulateProperties();
      notifyGuiStateProperties();
   }
#endif

   template <bool hasDelta, bool areDeltasPersistant>
   void UndoTest<hasDelta, areDeltasPersistant>::notifyGuiStateProperties() {
#ifndef CLAP_PLUGINS_HEADLESS
      if (_guiHandle)
         _guiHandle->gui().setGuiProperty("counter", _counter);
#endif
   }

   template <bool hasDelta, bool areDeltasPersistant>
   std::vector<uint8_t> UndoTest<hasDelta, areDeltasPersistant>::stateSaveExtra() noexcept {
      return std::vector<uint8_t>((const uint8_t *)&_counter, (const uint8_t *)(&_counter + 1));
   }

   template <bool hasDelta, bool areDeltasPersistant>
   bool UndoTest<hasDelta, areDeltasPersistant>::stateLoadExtra(
      const std::vector<uint8_t> &data) noexcept {
      if (data.size() != sizeof(_counter))
         return false;
      _counter = *(const uint32_t *)data.data();
      return true;
   }

} // namespace clap
