#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <clap/clap.h>

#include "abstract-gui-listener.hh"
#include "platform-window.hh"

struct ImGuiContext;

namespace clap {

   class ImGuiGui;
   class CocoaWindow;
   class WaylandWindow;
   class Win32Window;
   class X11Window;

   struct GuiParam {
      clap_param_info info;
      double value = 0;
      double modulation = 0;

      bool isAdjusting = false;
      double dragValue = 0; // normalized, accumulates the mouse moves while adjusting

      bool hasMappingIndication = false;
      clap_color mappingIndicationColor{};
      std::string mappingIndicationLabel;
      std::string mappingIndicationDescription;

      uint32_t automationIndicationState = CLAP_PARAM_INDICATION_AUTOMATION_NONE;
      clap_color automationIndicationColor{};

      double clip(double v) const;
      double normalize(double v) const;
      double denormalize(double v) const;
      double normalizedValue() const { return normalize(value); }
      double normalizedFinalValue() const { return normalize(clip(value + modulation)); }
   };

   // Draws the GUI of a plugin
   struct Skin {
      const char *name;
      uint32_t backgroundColor; // 0xRRGGBB
      // The skin shows the transport, so the plugin has to send it
      bool usesTransport;
      void (*defaultSize)(const ImGuiGui &gui, uint32_t *width, uint32_t *height);
      void (*draw)(ImGuiGui &gui);
   };

   enum class WindowApi {
      X11,
      Wayland,                 // floating
      WaylandSurfaceEmbedding, // embedded through the host's compositor
      Win32,
      Cocoa,
   };

   // GUI implemented with Dear ImGui and rendered in software.
   //
   // Everything happens on the main thread. On Linux the plugin drives it with a host timer,
   // calling onTimer(); on Windows and macOS the window has its own timer.
   //
   // Sizes are logical, except in getSize(), setSize() and roundSize() which use the CLAP
   // window API's unit: physical pixels for X11 and Win32.
   class ImGuiGui final : private PlatformWindow::Listener {
   public:
      // The CLAP window APIs that this platform supports
      static std::optional<WindowApi> windowApi(const char *api, bool isFloating);
      static void preferredApi(const char **api, bool *isFloating);

      // Without its own timer, the GUI needs onTimer() to be called periodically
      static bool needsTimer();

      ImGuiGui(AbstractGuiListener &listener, WindowApi api, bool isFloating);
      ~ImGuiGui() override;

      void setSkin(const std::string &skinName);
      void setTitle(const std::string &title);
      void setProperty(const std::string &name, const InvocationArgumentType &value);

      void defineParameter(const clap_param_info &paramInfo);
      void updateParameter(clap_id paramId, double value, double modAmount);
      void setParameterMappingIndication(clap_id paramId,
                                         bool hasIndication,
                                         clap_color color,
                                         const char *label,
                                         const char *description);
      void
      setParameterAutomationIndication(clap_id paramId, uint32_t automationState, clap_color color);

      void clearTransport();
      void updateTransport(const clap_event_transport &transport);
      void updateTrackInfo(bool hasTrackInfo, const clap_track_info &info);

      void setCanUndo(bool canUndo) noexcept { _canUndo = canUndo; requestRedraw(); }
      void setCanRedo(bool canRedo) noexcept { _canRedo = canRedo; requestRedraw(); }
      void setUndoName(std::string name) { _undoName = std::move(name); requestRedraw(); }
      void setRedoName(std::string name) { _redoName = std::move(name); requestRedraw(); }

      ////////////
      // Window //
      ////////////

      // Floating windows: creates the window, without showing it
      [[nodiscard]] bool openWindow();

      // Embedded windows: creates the window into the host's one
      [[nodiscard]] bool attach(const clap_window &parent);

      // Floating windows: keeps the window above the host's one. Not for Wayland, which uses the
      // following methods (see clap-wayland-experiment).
      bool setTransient(const clap_window &parent);

      // clap_plugin_gui_xdg_foreign_v2: handle is from zxdg_exported_v2, NULL removes the parent
      bool setWaylandTransient(const char *handle);

      // clap_plugin_gui_xdg_activation_v1
      void setActivationToken(const char *token);

      // clap_plugin_gui_wayland_nested_compositor: embeds the window where the token says.
      // openConnection gives the socket to the host's compositor, the first time.
      bool setWaylandEmbeddingParent(const char *token, const std::function<int()> &openConnection);

      bool isApiPhysical() const noexcept {
         return _api == WindowApi::X11 || _api == WindowApi::Win32;
      }

      bool getSize(uint32_t *width, uint32_t *height) const;
      bool setSize(uint32_t width, uint32_t height);
      bool roundSize(uint32_t *width, uint32_t *height) const;

      // Only for the APIs which use physical pixels
      bool setScale(double scale);

      bool show();
      bool hide();

      // Processes the window events and renders when needed
      void onTimer() override;

      ///////////////
      // For skins //
      ///////////////
      uint32_t width() const noexcept { return _width; }
      uint32_t height() const noexcept { return _height; }

      // The CLAP window API, for example CLAP_WINDOW_API_X11
      const char *windowApiName() const noexcept;
      bool isFloating() const noexcept { return _isFloating; }
      // Physical pixels per logical pixel
      double scale() const noexcept { return _window ? _window->scale() : apiScale(); }

      // In definition order
      const std::vector<clap_id> &paramIds() const noexcept { return _paramIds; }
      GuiParam *param(clap_id paramId);

      void beginAdjust(GuiParam &param);
      void adjust(GuiParam &param, double value);
      void endAdjust(GuiParam &param);
      void setToDefault(GuiParam &param);

      // To be called during the frame, for the parameter under the mouse
      void setHoveredParam(clap_id paramId) noexcept { _hoveredParamInFrame = paramId; }

      const std::optional<clap_event_transport> &transport() const noexcept { return _transport; }
      const std::optional<clap_track_info> &trackInfo() const noexcept { return _trackInfo; }
      const InvocationArgumentType *property(const std::string &name) const;

      bool canUndo() const noexcept { return _canUndo; }
      bool canRedo() const noexcept { return _canRedo; }
      const std::string &undoName() const noexcept { return _undoName; }
      const std::string &redoName() const noexcept { return _redoName; }
      void undo() { _listener.onGuiUndo(); }
      void redo() { _listener.onGuiRedo(); }

      // Calls a method of the plugin, see CorePlugin::onGuiInvoke()
      void invoke(const std::string &method, const InvocationArgumentsType &args = {}) {
         _listener.onGuiInvoke(method, args);
      }

   private:
      // PlatformWindow::Listener
      void onPointerMove(double x, double y) override;
      void onPointerLeave() override;
      void onPointerButton(int button, bool isPressed) override;
      void onPointerScroll(double dx, double dy) override;
      void onScaleChanged() override;
      void onResized(uint32_t width, uint32_t height) override;
      void onCloseRequested() override;
      void onWindowLost() override;

      [[nodiscard]] bool attachWindow(std::unique_ptr<PlatformWindow> window);
      PlatformWindow::Listener &windowListener() noexcept { return *this; }
      double apiScale() const noexcept { return isApiPhysical() ? _hostScale : 1; }

      void requestRedraw() noexcept;
      void render();
      void drawTitleBar(float height);

      AbstractGuiListener &_listener;
      const WindowApi _api;
      const bool _isFloating;

      ImGuiContext *_context = nullptr;
      std::unique_ptr<PlatformWindow> _window;
      // Same as _window
#if defined(__linux__)
      WaylandWindow *_waylandWindow = nullptr;
      X11Window *_x11Window = nullptr;
#elif defined(_WIN32)
      Win32Window *_win32Window = nullptr;
#elif defined(__APPLE__)
      CocoaWindow *_cocoaWindow = nullptr;
#endif
      const Skin *_skin = nullptr;

      // Logical size, without the title bar
      uint32_t _width = 200;
      uint32_t _height = 200;
      // Given by the host, for the APIs which use physical pixels
      double _hostScale = 1;
      bool _isVisible = false;
      std::string _title;

      // The host may destroy the GUI when we tell it that the window is closed, so we do it
      // at the end of onTimer().
      bool _isCloseRequested = false;
      bool _isWindowLost = false;

      // ImGui needs a few frames to process the input events and update its state
      int _framesToRender = 0;
      std::optional<std::chrono::steady_clock::time_point> _lastFrameTime;

      std::unordered_map<clap_id, GuiParam> _params;
      std::vector<clap_id> _paramIds;
      clap_id _hoveredParam = CLAP_INVALID_ID;
      clap_id _hoveredParamInFrame = CLAP_INVALID_ID;

      std::unordered_map<std::string, InvocationArgumentType> _properties;

      std::optional<clap_event_transport> _transport;
      std::optional<clap_track_info> _trackInfo;

      bool _canUndo = false;
      bool _canRedo = false;
      std::string _undoName;
      std::string _redoName;
   };

} // namespace clap
