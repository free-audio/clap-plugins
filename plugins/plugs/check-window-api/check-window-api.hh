#pragma once

#include "../../core-plugin.hh"

namespace clap {

   // The window API and mode which the plugin forces
   enum class CheckWindowApiKind {
      // Linux
      X11Floating,
      X11Embed,
      WaylandFloating,
      WaylandEmbed,

      // Windows
      Win32Floating,
      Win32Embed,

      // macOS
      CocoaFloating,
      CocoaEmbed,
   };

   // Plugins which support only one window API, floating or embedded, to test the hosts.
   // Their GUI shows the window API, the mode, the scale and the size.
   //
   // Each one is only available on its window API's platform.
   template <CheckWindowApiKind kind>
   class CheckWindowApi final : public CorePlugin {
   private:
      using super = CorePlugin;

   public:
      CheckWindowApi(const std::string &pluginPath, const clap_host &host);

      static const clap_plugin_descriptor *descriptor();

   protected:
      // clap_plugin
      bool init() noexcept override;
      const void *extension(const char *id) noexcept override;

      // clap_plugin_gui
      bool guiIsApiSupported(const char *api, bool isFloating) noexcept override;
      bool guiGetPreferredApi(const char **api, bool *isFloating) noexcept override;
      bool guiCreate(const char *api, bool isFloating) noexcept override;

   private:
      static const char *forcedApi() noexcept;
      static constexpr bool isForcedFloating() noexcept {
         return kind == CheckWindowApiKind::X11Floating ||
                kind == CheckWindowApiKind::WaylandFloating ||
                kind == CheckWindowApiKind::Win32Floating ||
                kind == CheckWindowApiKind::CocoaFloating;
      }
      static bool isForced(const char *api, bool isFloating) noexcept;

      void defineAudioPorts() noexcept;
   };

   extern template class CheckWindowApi<CheckWindowApiKind::X11Floating>;
   extern template class CheckWindowApi<CheckWindowApiKind::X11Embed>;
   extern template class CheckWindowApi<CheckWindowApiKind::WaylandFloating>;
   extern template class CheckWindowApi<CheckWindowApiKind::WaylandEmbed>;
   extern template class CheckWindowApi<CheckWindowApiKind::Win32Floating>;
   extern template class CheckWindowApi<CheckWindowApiKind::Win32Embed>;
   extern template class CheckWindowApi<CheckWindowApiKind::CocoaFloating>;
   extern template class CheckWindowApi<CheckWindowApiKind::CocoaEmbed>;

} // namespace clap
