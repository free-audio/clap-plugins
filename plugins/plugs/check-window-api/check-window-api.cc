#include <cstring>

#include "../../modules/module.hh"
#include "../../value-types/decibel-value-type.hh"
#include "check-window-api.hh"

namespace clap {

   namespace {
      // Something to adjust in the GUI
      class CheckWindowApiModule final : public Module {
      public:
         explicit CheckWindowApiModule(CorePlugin &plugin) : Module(plugin, "", 0) {
            _gainParam = addParameter(0,
                                      "gain",
                                      CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE |
                                         CLAP_PARAM_REQUIRES_PROCESS,
                                      std::make_unique<DecibelValueType>(-40, 40, 0, 0));
         }

         clap_process_status process(const Context &c, uint32_t numFrames) noexcept override {
            assert(_isActive);

            c.audioOutputs[0]->product(
               *c.audioInputs[0], _gainParam->voiceBuffer(_voiceModule), numFrames);
            return CLAP_PROCESS_SLEEP;
         }

      private:
         Parameter *_gainParam = nullptr;
      };

      template <CheckWindowApiKind kind>
      constexpr const char *pluginId() {
         switch (kind) {
         case CheckWindowApiKind::X11Floating:
            return "com.github.free-audio.clap.check-x11-floating";
         case CheckWindowApiKind::X11Embed:
            return "com.github.free-audio.clap.check-x11-embed";
         case CheckWindowApiKind::WaylandFloating:
            return "com.github.free-audio.clap.check-wayland-floating";
         case CheckWindowApiKind::WaylandEmbed:
            return "com.github.free-audio.clap.check-wayland-embed";
         }
      }

      template <CheckWindowApiKind kind>
      constexpr const char *pluginName() {
         switch (kind) {
         case CheckWindowApiKind::X11Floating:
            return "Check X11 Floating";
         case CheckWindowApiKind::X11Embed:
            return "Check X11 Embed";
         case CheckWindowApiKind::WaylandFloating:
            return "Check Wayland Floating";
         case CheckWindowApiKind::WaylandEmbed:
            return "Check Wayland Embed";
         }
      }
   } // namespace

   template <CheckWindowApiKind kind>
   const clap_plugin_descriptor *CheckWindowApi<kind>::descriptor() {
      static const char *features[] = {
         CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_UTILITY, "validation", nullptr};

      static const clap_plugin_descriptor desc = {
         CLAP_VERSION,
         pluginId<kind>(),
         pluginName<kind>(),
         "clap",
         "https://github.com/free-audio/clap",
         nullptr,
         nullptr,
         "0.1",
         "only supports one window API, floating or embedded, to test the hosts",
         features};
      return &desc;
   }

   template <CheckWindowApiKind kind>
   CheckWindowApi<kind>::CheckWindowApi(const std::string &pluginPath, const clap_host &host)
      : CorePlugin("check-window-api", descriptor(), host) {
      _rootModule = std::make_unique<CheckWindowApiModule>(*this);
   }

   template <CheckWindowApiKind kind>
   bool CheckWindowApi<kind>::init() noexcept {
      if (!super::init())
         return false;

      defineAudioPorts();
      return true;
   }

   template <CheckWindowApiKind kind>
   void CheckWindowApi<kind>::defineAudioPorts() noexcept {
      assert(!isActive());

      clap_audio_port_info info;
      info.id = 0;
      strncpy(info.name, "main", sizeof(info.name));
      info.flags = CLAP_AUDIO_PORT_IS_MAIN;
      info.in_place_pair = 0;
      info.channel_count = trackChannelCount();
      info.port_type = nullptr;

      _audioInputs.clear();
      _audioInputs.push_back(info);
      _audioOutputs.clear();
      _audioOutputs.push_back(info);
   }

   template <CheckWindowApiKind kind>
   const char *CheckWindowApi<kind>::forcedApi() noexcept {
      switch (kind) {
      case CheckWindowApiKind::X11Floating:
      case CheckWindowApiKind::X11Embed:
         return CLAP_WINDOW_API_X11;
      case CheckWindowApiKind::WaylandFloating:
         return CLAP_WINDOW_API_WAYLAND;
      case CheckWindowApiKind::WaylandEmbed:
         return CLAP_WINDOW_API_WAYLAND_SURFACE_EMBEDDING_V1;
      }
      return nullptr;
   }

   // The host may let a floating window choose its API
   template <CheckWindowApiKind kind>
   bool CheckWindowApi<kind>::isForced(const char *api, bool isFloating) noexcept {
      if (isFloating != isForcedFloating())
         return false;
      if (!api || !*api)
         return isFloating;
      return !strcmp(api, forcedApi());
   }

   template <CheckWindowApiKind kind>
   bool CheckWindowApi<kind>::guiIsApiSupported(const char *api, bool isFloating) noexcept {
      return isForced(api, isFloating) && super::guiIsApiSupported(forcedApi(), isFloating);
   }

   template <CheckWindowApiKind kind>
   bool CheckWindowApi<kind>::guiGetPreferredApi(const char **api, bool *isFloating) noexcept {
      *api = forcedApi();
      *isFloating = isForcedFloating();
      return true;
   }

   template <CheckWindowApiKind kind>
   bool CheckWindowApi<kind>::guiCreate(const char *api, bool isFloating) noexcept {
      return isForced(api, isFloating) && super::guiCreate(forcedApi(), isFloating);
   }

   // Only the extensions of the forced window API
   template <CheckWindowApiKind kind>
   const void *CheckWindowApi<kind>::extension(const char *id) noexcept {
      if (!strcmp(id, CLAP_EXT_GUI_XDG_FOREIGN_V2) || !strcmp(id, CLAP_EXT_GUI_XDG_ACTIVATION_V1))
         return kind == CheckWindowApiKind::WaylandFloating ? super::extension(id) : nullptr;

      if (!strcmp(id, CLAP_EXT_GUI_WAYLAND_NESTED_COMPOSITOR))
         return kind == CheckWindowApiKind::WaylandEmbed ? super::extension(id) : nullptr;

      return super::extension(id);
   }

   template class CheckWindowApi<CheckWindowApiKind::X11Floating>;
   template class CheckWindowApi<CheckWindowApiKind::X11Embed>;
   template class CheckWindowApi<CheckWindowApiKind::WaylandFloating>;
   template class CheckWindowApi<CheckWindowApiKind::WaylandEmbed>;

} // namespace clap
