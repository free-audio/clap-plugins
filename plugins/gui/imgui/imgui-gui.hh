#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../abstract-gui.hh"
#include "platform-window.hh"

struct ImGuiContext;

namespace clap {

   class ImGuiGui;
   class WaylandWindow;

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
      void (*defaultSize)(const ImGuiGui &gui, uint32_t *width, uint32_t *height);
      void (*draw)(ImGuiGui &gui);
   };

   // GUI implemented with Dear ImGui and rendered in software.
   //
   // It doesn't have an event loop: everything happens on the main thread, driven by the
   // plugin's timer.
   class ImGuiGui final : public AbstractGui, private PlatformWindow::Listener {
   public:
      explicit ImGuiGui(AbstractGuiListener &listener);
      ~ImGuiGui() override;

      /////////////////
      // AbstractGui //
      /////////////////
      void addImportPath(const std::string &importPath) override {}
      void setSkin(const std::string &skinName) override;
      void setGuiProperty(const std::string &name, const PropertyValue &value) override;

      void defineParameter(const clap_param_info &paramInfo) override;
      void updateParameter(clap_id paramId, double value, double modAmount) override;
      void setParameterMappingIndication(clap_id paramId,
                                         bool hasIndication,
                                         clap_color color,
                                         const char *label,
                                         const char *description) override;
      void setParameterAutomationIndication(clap_id paramId,
                                            uint32_t automationState,
                                            clap_color color) override;

      void clearTransport() override;
      void updateTransport(const clap_event_transport &transport) override;
      void updateTrackInfo(bool hasTrackInfo, const clap_track_info &info) override;

      // Opens a floating window
      bool openWindow() override;

      bool attachCocoa(clap_nsview nsView) override { return false; }
      bool attachWin32(clap_hwnd window) override { return false; }
      bool attachX11(clap_xwnd window) override { return false; }

      bool setTransientX11(clap_xwnd window) override { return false; }
      bool setTransientWin32(clap_hwnd window) override { return false; }
      bool setTransientCocoa(clap_nsview nsView) override { return false; }
      bool setTransientWayland(const char *exportedHandle) override;
      void setWaylandActivationToken(const char *token) override;

      void setTitle(const std::string &title) override;

      bool canResize() override { return true; }
      bool getSize(uint32_t *width, uint32_t *height) override;
      bool setSize(uint32_t width, uint32_t height) override;
      bool roundSize(uint32_t *width, uint32_t *height) override;

      // We get the scale from the window system
      bool setScale(double scale) override { return false; }

      bool show() override;
      bool hide() override;

      void destroy() override;

      void setCanUndo(bool canUndo) override { _canUndo = canUndo; }
      void setCanRedo(bool canRedo) override { _canRedo = canRedo; }
      void setUndoName(std::string name) override { _undoName = std::move(name); }
      void setRedoName(std::string name) override { _redoName = std::move(name); }

      void onTimer() override;

      // Shows the GUI in this window, openWindow() and co use it
      [[nodiscard]] bool attachWindow(std::unique_ptr<PlatformWindow> window);

      // To create the platform window
      PlatformWindow::Listener &windowListener() noexcept { return *this; }

      ///////////////
      // For skins //
      ///////////////
      uint32_t width() const noexcept { return _width; }
      uint32_t height() const noexcept { return _height; }

      // In definition order
      const std::vector<clap_id> &paramIds() const noexcept { return _paramIds; }
      GuiParam *param(clap_id paramId);

      void beginAdjust(GuiParam &param);
      void adjust(GuiParam &param, double value);
      void endAdjust(GuiParam &param);
      void setToDefault(GuiParam &param);

      // To be called during the frame, for the parameter under the mouse
      void setHoveredParam(clap_id paramId) noexcept { _hoveredParamInFrame = paramId; }

   private:
      // PlatformWindow::Listener
      void onPointerMove(double x, double y) override;
      void onPointerLeave() override;
      void onPointerButton(int button, bool isPressed) override;
      void onPointerScroll(double dx, double dy) override;
      void onScaleChanged() override;
      void onResized(uint32_t width, uint32_t height) override;
      void onCloseRequested() override;

      void requestRedraw() noexcept;
      void render();
      void drawTitleBar(float height);

      ImGuiContext *_context = nullptr;
      std::unique_ptr<PlatformWindow> _window;
#ifdef __linux__
      // Same as _window
      WaylandWindow *_waylandWindow = nullptr;
#endif
      const Skin *_skin = nullptr;

      // Without the title bar
      uint32_t _width = 200;
      uint32_t _height = 200;
      bool _isVisible = false;
      std::string _title;

      // The host may destroy the GUI when we tell it that the window is closed, so we do it
      // at the end of onTimer().
      bool _isCloseRequested = false;

      // ImGui needs a few frames to process the input events and update its state
      int _framesToRender = 0;
      std::optional<std::chrono::steady_clock::time_point> _lastFrameTime;

      std::unordered_map<clap_id, GuiParam> _params;
      std::vector<clap_id> _paramIds;
      clap_id _hoveredParam = CLAP_INVALID_ID;
      clap_id _hoveredParamInFrame = CLAP_INVALID_ID;

      std::unordered_map<std::string, PropertyValue> _properties;

      std::optional<clap_event_transport> _transport;
      std::optional<clap_track_info> _trackInfo;

      bool _canUndo = false;
      bool _canRedo = false;
      std::string _undoName;
      std::string _redoName;
   };

} // namespace clap
