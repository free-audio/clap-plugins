#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <string>

#include "platform-window.hh"

struct wl_buffer;
struct wl_callback;
struct wl_compositor;
struct wl_display;
struct wl_pointer;
struct wl_registry;
struct wl_seat;
struct wl_shm;
struct wl_surface;
struct wp_cursor_shape_device_v1;
struct wp_cursor_shape_manager_v1;
struct wp_fractional_scale_manager_v1;
struct wp_fractional_scale_v1;
struct wp_viewport;
struct wp_viewporter;
struct xdg_activation_v1;
struct xdg_surface;
struct xdg_toplevel;
struct xdg_wm_base;
struct zxdg_decoration_manager_v1;
struct zxdg_imported_v2;
struct zxdg_importer_v2;
struct zxdg_toplevel_decoration_v1;

namespace clap {

   // A floating window, with its own connection to the compositor.
   // See clap-wayland-experiment for the contract with the host.
   class WaylandWindow final : public PlatformWindow {
   public:
      explicit WaylandWindow(Listener &listener);
      ~WaylandWindow() override;

      // Connects to the compositor and creates the window, without showing it
      [[nodiscard]] bool open();

      // Keeps the window above the host's window, exported with xdg-foreign.
      // NULL removes the parent.
      bool setTransient(const char *exportedHandle);

      // xdg-activation token, used the next time the window is shown
      void setActivationToken(const char *token);

      void setSize(uint32_t width, uint32_t height) override;
      void setMinSize(uint32_t width, uint32_t height) override;
      void setTitle(const std::string &title) override;
      void setVisible(bool isVisible) override;
      double scale() const override { return _scale; }
      uint32_t titleBarHeight() const override;
      void beginMove() override;
      void dispatchEvents() override;
      [[nodiscard]] Framebuffer *beginFrame() override;
      void endFrame() override;

      // Called by the Wayland listeners
      void onGlobal(wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
      void onPreferredBufferScale(int32_t factor);
      void onPreferredFractionalScale(uint32_t scale120);
      void onSurfaceConfigure(uint32_t serial);
      void onToplevelConfigure(int32_t width, int32_t height);
      void onToplevelClose();
      void onDecorationConfigure(uint32_t mode);
      void onImportedDestroyed();
      void onFrameDone();
      void onSeatCapabilities(uint32_t capabilities);
      void onPointerEnter(uint32_t serial, wl_surface *surface, double x, double y);
      void onPointerLeave(wl_surface *surface);
      void onPointerMotion(double x, double y);
      void onPointerButton(uint32_t serial, uint32_t button, uint32_t state);
      void onPointerAxis(uint32_t axis, double value);
      void onPointerAxisDiscrete(uint32_t axis, int32_t discrete);
      void onPointerFrame();

   private:
      struct ShmBuffer {
         wl_buffer *buffer = nullptr;
         void *data = nullptr;
         size_t size = 0;
         bool isBusy = false;
         Framebuffer framebuffer;
      };

      void destroy();
      void destroyPointer();
      void destroyImported();
      void unmap();
      void leavePointer();
      void activate();
      bool createBuffer(ShmBuffer &buffer, int width, int height);
      void destroyBuffer(ShmBuffer &buffer);
      void destroyFrameCallback();
      void applyMinSize();
      bool checkConnection();
      void flush();

      wl_display *_display = nullptr;
      wl_registry *_registry = nullptr;
      wl_compositor *_compositor = nullptr;
      wl_shm *_shm = nullptr;
      xdg_wm_base *_wmBase = nullptr;
      wp_viewporter *_viewporter = nullptr;
      wp_fractional_scale_manager_v1 *_fractionalScaleManager = nullptr;
      zxdg_decoration_manager_v1 *_decorationManager = nullptr;
      zxdg_importer_v2 *_importer = nullptr;
      xdg_activation_v1 *_activation = nullptr;
      wp_cursor_shape_manager_v1 *_cursorShapeManager = nullptr;
      wl_seat *_seat = nullptr;
      wl_pointer *_pointer = nullptr;
      wp_cursor_shape_device_v1 *_cursorShapeDevice = nullptr;

      wl_surface *_surface = nullptr;
      xdg_surface *_xdgSurface = nullptr;
      xdg_toplevel *_toplevel = nullptr;
      zxdg_toplevel_decoration_v1 *_decoration = nullptr;
      zxdg_imported_v2 *_imported = nullptr;
      wp_viewport *_viewport = nullptr;
      wp_fractional_scale_v1 *_fractionalScale = nullptr;
      wl_callback *_frameCallback = nullptr;

      uint32_t _seatVersion = 0;

      // GUI size, without the title bar
      uint32_t _width = 1;
      uint32_t _height = 1;
      uint32_t _minWidth = 1;
      uint32_t _minHeight = 1;
      std::string _title;

      // With wp_fractional_scale_v1, the buffer is round(size * scale) and the viewport maps it
      // to the logical size. Otherwise the scale is an integer, from wl_surface.
      double _scale = 1;

      bool _isVisible = false;
      // An xdg_surface is mapped in three steps: a commit without buffer, the acknowledgement
      // of the configure event which follows, and a commit with a buffer. Unmapping it starts
      // over.
      bool _isInitialCommitDone = false;
      bool _isConfigured = false;
      bool _isMapped = false;
      bool _isServerSideDecorated = false;
      // Size from the last xdg_toplevel.configure, applied with the next xdg_surface.configure
      int32_t _pendingWidth = 0;
      int32_t _pendingHeight = 0;
      std::string _activationToken;

      std::array<ShmBuffer, 2> _buffers;
      ShmBuffer *_currentBuffer = nullptr;
      std::chrono::steady_clock::time_point _frameRequestTime;

      // pointer state
      bool _isPointerInside = false;
      uint32_t _pressedButtons = 0; // bit mask of the listener's button indexes
      uint32_t _lastButtonSerial = 0;
      double _scrollX = 0;
      double _scrollY = 0;
      int _scrollDiscreteX = 0;
      int _scrollDiscreteY = 0;
   };

} // namespace clap
