#include <cstring>

#include "undo-test.hh"

namespace clap {
   namespace {
      static constexpr clap_id UNDO_FORMAT_VERSION = 3716;
      struct UndoDelta {
         uint32_t old_value;
         uint32_t new_value;
      };
   } // namespace

   class UndoTestModule final : public Module {
   public:
      UndoTestModule(UndoTest &plugin) : Module(plugin, "", 0) {}

      clap_process_status process(const Context &c, uint32_t numFrames) noexcept override {
         return CLAP_PROCESS_SLEEP;
      }
   };

   const clap_plugin_descriptor *UndoTest::descriptor() {
      static const char *features[] = {
         CLAP_PLUGIN_FEATURE_UTILITY, CLAP_PLUGIN_FEATURE_ANALYZER, nullptr};

      static const clap_plugin_descriptor desc = {CLAP_VERSION,
                                                  "com.github.free-audio.clap.undo-test",
                                                  "Undo Test",
                                                  "clap",
                                                  "https://github.com/free-audio/clap",
                                                  nullptr,
                                                  nullptr,
                                                  "0.1",
                                                  "Help testing the undo extension",
                                                  features};

      return &desc;
   }

   UndoTest::UndoTest(const std::string &pluginPath, const clap_host &host)
      : CorePlugin(PathProvider::create(pluginPath, "undo-test"), descriptor(), host) {
      _rootModule = std::make_unique<UndoTestModule>(*this);
   }

   bool UndoTest::implementsUndo() const noexcept { return true; }

   void UndoTest::undoGetDeltaProperties(clap_undo_delta_properties_t *properties) noexcept {
      properties->has_delta = true;
      properties->are_deltas_persistent = true;
      properties->format_version = UNDO_FORMAT_VERSION;
   }

   bool UndoTest::undoCanUseDeltaFormatVersion(clap_id format_version) noexcept {
      return format_version == UNDO_FORMAT_VERSION;
   }

   bool UndoTest::undoUndo(clap_id format_version, const void *delta, size_t delta_size) noexcept {
      if (format_version != UNDO_FORMAT_VERSION) {
         hostMisbehaving("invalid undo delta format version");
         return false;
      }

      if (delta_size != sizeof(UndoDelta)) {
         hostMisbehaving("invalid undo delta size");
         return false;
      }

      return false;
   }

   bool UndoTest::undoRedo(clap_id format_version, const void *delta, size_t delta_size) noexcept {
      if (format_version != UNDO_FORMAT_VERSION) {
         hostMisbehaving("invalid undo delta format version");
         return false;
      }

      if (delta_size != sizeof(UndoDelta)) {
         hostMisbehaving("invalid undo delta size");
         return false;
      }

      return false;
   }

   void UndoTest::incrementState() {
      if (!_host.canUseUndo())
         return;

      UndoDelta delta;
      delta.old_value = _state;
      delta.new_value = ++_state;
      _host.undoChangeMade("inc", &delta, sizeof(delta), true);
   }

   bool UndoTest::init() noexcept {
      if (!super::init())
         return false;

      if (_host.canUseUndo()) {
         _host.undoSetWantsContextUpdates(true);
      }
      return true;
   }

   void UndoTest::onGuiInvoke(
      const std::string &method,
      const std::vector<std::variant<bool, int64_t, double, std::string>> &args) {
      if (method == "incrementState")
         incrementState();
      else
         super::onGuiInvoke(method, args);
   }
} // namespace clap
