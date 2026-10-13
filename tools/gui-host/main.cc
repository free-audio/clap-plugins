// Minimal host to try the plugins' GUIs on Linux, without a DAW.
//
// Usage: clap-gui-host [--x11 | --wayland] [--floating] [--scale <factor>]
//                      <plugin.clap> [plugin-id]
//
// Without --x11 or --wayland, it uses the plugin's preferred window API. The plugin's GUI is
// embedded into the host's window, or floats above it; then clicking the host's window shows
// or hides the plugin's window. With Wayland, the GUI is always floating, see
// clap-wayland-experiment.
//
// With X11, it gives the plugin the desktop's scale, unless --scale overrides it. Wayland uses
// logical sizes: the host doesn't give the scale, the plugin gets it from the compositor.
//
// It only shows the GUI: the plugin isn't activated, and the parameter changes are flushed with
// clap_plugin_params.flush() and printed.

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <dlfcn.h>
#include <linux/input-event-codes.h>
#include <poll.h>
#include <sys/mman.h>
#include <unistd.h>

#include <clap/clap.h>
#include <clap/ext/draft/wayland-experiment.h>
#include <wayland-client.h>
#include <xcb/xcb.h>

#include "cursor-shape-v1-client-protocol.h"
#include "xdg-activation-v1-client-protocol.h"
#include "xdg-decoration-unstable-v1-client-protocol.h"
#include "xdg-foreign-unstable-v2-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"

namespace {

   using Clock = std::chrono::steady_clock;

   constexpr uint32_t kBackgroundColor = 0x303030;
   constexpr uint32_t kWidth = 480;
   constexpr uint32_t kHeight = 320;
   constexpr const char *kDefaultPluginId = "com.github.free-audio.clap.gain";
   constexpr const char *kTitle = "CLAP GUI Host";

   //////////////////
   // Host windows //
   //////////////////

   // The host's own window, from which the plugin's window is embedded or floating
   class HostWindow {
   public:
      class Listener {
      public:
         virtual ~Listener() = default;

         // The user clicked the window. With Wayland, the token lets the plugin's window get
         // the focus.
         virtual void onHostWindowClicked(const char *activationToken) = 0;
         virtual void onHostWindowResized(uint32_t width, uint32_t height) = 0;
      };

      explicit HostWindow(Listener &listener) : _listener(listener) {}
      virtual ~HostWindow() = default;

      [[nodiscard]] virtual bool open(const char *title) = 0;

      // For clap_plugin_gui.set_parent() and set_transient()
      virtual clap_window clapWindow() const = 0;

      // For clap_plugin_gui_xdg_foreign_v2.set_transient(), with Wayland
      virtual const char *xdgForeignHandle() const { return nullptr; }

      virtual void resize(uint32_t width, uint32_t height) = 0;

      // The scale which the host gives to the plugin, for X11.
      // source tells where it comes from.
      virtual double desktopScale(std::string &source) { return 1; }

      // Waits for the window system's events, up to the timeout
      virtual void dispatch(int timeoutMs) = 0;

      bool isOpen() const { return _isOpen; }

   protected:
      Listener &_listener;
      bool _isOpen = true;
   };

   /////////////
   // Wayland //
   /////////////
   class WaylandHostWindow final : public HostWindow {
   public:
      using HostWindow::HostWindow;
      ~WaylandHostWindow() override;

      bool open(const char *title) override;
      clap_window clapWindow() const override;
      const char *xdgForeignHandle() const override {
         return _exportedHandle.empty() ? nullptr : _exportedHandle.c_str();
      }
      void resize(uint32_t width, uint32_t height) override {}
      void dispatch(int timeoutMs) override;

   private:
      static WaylandHostWindow &self(void *data) { return *static_cast<WaylandHostWindow *>(data); }

      void onGlobal(wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
      void onSeatCapabilities(uint32_t capabilities);
      void onPointerEnter(uint32_t serial);
      void onPointerButton(uint32_t serial, uint32_t button, uint32_t state);
      wl_buffer *createBuffer(uint32_t width, uint32_t height, uint32_t color);

      wl_display *_display = nullptr;
      wl_registry *_registry = nullptr;
      wl_compositor *_compositor = nullptr;
      wl_shm *_shm = nullptr;
      xdg_wm_base *_wmBase = nullptr;
      zxdg_decoration_manager_v1 *_decorationManager = nullptr;
      zxdg_exporter_v2 *_exporter = nullptr;
      xdg_activation_v1 *_activation = nullptr;
      wp_cursor_shape_manager_v1 *_cursorShapeManager = nullptr;
      wl_seat *_seat = nullptr;
      wl_pointer *_pointer = nullptr;
      wp_cursor_shape_device_v1 *_cursorShapeDevice = nullptr;
      wl_surface *_surface = nullptr;
      xdg_surface *_xdgSurface = nullptr;
      xdg_toplevel *_toplevel = nullptr;
      zxdg_toplevel_decoration_v1 *_decoration = nullptr;
      zxdg_exported_v2 *_exported = nullptr;
      std::string _exportedHandle;
      wl_buffer *_background = nullptr;
      bool _isConfigured = false;
   };

   WaylandHostWindow::~WaylandHostWindow() {
      if (_background)
         wl_buffer_destroy(_background);
      if (_exported)
         zxdg_exported_v2_destroy(_exported);
      if (_decoration)
         zxdg_toplevel_decoration_v1_destroy(_decoration);
      if (_toplevel)
         xdg_toplevel_destroy(_toplevel);
      if (_xdgSurface)
         xdg_surface_destroy(_xdgSurface);
      if (_surface)
         wl_surface_destroy(_surface);
      if (_cursorShapeDevice)
         wp_cursor_shape_device_v1_destroy(_cursorShapeDevice);
      if (_pointer)
         wl_pointer_release(_pointer);
      if (_seat)
         wl_seat_release(_seat);
      if (_cursorShapeManager)
         wp_cursor_shape_manager_v1_destroy(_cursorShapeManager);
      if (_activation)
         xdg_activation_v1_destroy(_activation);
      if (_exporter)
         zxdg_exporter_v2_destroy(_exporter);
      if (_decorationManager)
         zxdg_decoration_manager_v1_destroy(_decorationManager);
      if (_wmBase)
         xdg_wm_base_destroy(_wmBase);
      if (_shm)
         wl_shm_destroy(_shm);
      if (_compositor)
         wl_compositor_destroy(_compositor);
      if (_registry)
         wl_registry_destroy(_registry);
      if (_display)
         wl_display_disconnect(_display);
   }

   bool WaylandHostWindow::open(const char *title) {
      _display = wl_display_connect(nullptr);
      if (!_display) {
         std::cerr << "failed to connect to the wayland compositor" << std::endl;
         return false;
      }

      static const wl_registry_listener registryListener = {
         .global =
            [](void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
               self(data).onGlobal(registry, name, interface, version);
            },
         .global_remove = [](void *, wl_registry *, uint32_t) {},
      };

      _registry = wl_display_get_registry(_display);
      wl_registry_add_listener(_registry, &registryListener, this);
      wl_display_roundtrip(_display);

      if (!_compositor || !_shm || !_wmBase) {
         std::cerr << "missing wayland globals" << std::endl;
         return false;
      }

      static const xdg_surface_listener xdgSurfaceListener = {
         .configure =
            [](void *data, xdg_surface *xdgSurface, uint32_t serial) {
               xdg_surface_ack_configure(xdgSurface, serial);
               self(data)._isConfigured = true;
            },
      };

      static const xdg_toplevel_listener toplevelListener = {
         .configure = [](void *, xdg_toplevel *, int32_t, int32_t, wl_array *) {},
         .close = [](void *data, xdg_toplevel *) { self(data)._isOpen = false; },
         .configure_bounds = [](void *, xdg_toplevel *, int32_t, int32_t) {},
         .wm_capabilities = [](void *, xdg_toplevel *, wl_array *) {},
      };

      _surface = wl_compositor_create_surface(_compositor);
      _xdgSurface = xdg_wm_base_get_xdg_surface(_wmBase, _surface);
      xdg_surface_add_listener(_xdgSurface, &xdgSurfaceListener, this);
      _toplevel = xdg_surface_get_toplevel(_xdgSurface);
      xdg_toplevel_add_listener(_toplevel, &toplevelListener, this);
      xdg_toplevel_set_title(_toplevel, title);
      xdg_toplevel_set_app_id(_toplevel, "clap-gui-host");
      xdg_toplevel_set_min_size(_toplevel, kWidth, kHeight);
      xdg_toplevel_set_max_size(_toplevel, kWidth, kHeight);

      if (_decorationManager) {
         _decoration =
            zxdg_decoration_manager_v1_get_toplevel_decoration(_decorationManager, _toplevel);
         zxdg_toplevel_decoration_v1_set_mode(_decoration,
                                              ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
      }

      // Lets the plugin's window refer to ours
      if (_exporter) {
         static const zxdg_exported_v2_listener exportedListener = {
            .handle = [](void *data, zxdg_exported_v2 *, const char *handle) {
               self(data)._exportedHandle = handle;
            },
         };
         _exported = zxdg_exporter_v2_export_toplevel(_exporter, _surface);
         zxdg_exported_v2_add_listener(_exported, &exportedListener, this);
      } else
         std::cerr << "no xdg-foreign: the plugin's window won't have a parent" << std::endl;

      // An initial commit without buffer to get the first configure
      wl_surface_commit(_surface);
      while ((!_isConfigured || (_exported && _exportedHandle.empty())) && _isOpen)
         if (wl_display_dispatch(_display) < 0)
            return false;

      _background = createBuffer(kWidth, kHeight, kBackgroundColor);
      wl_surface_attach(_surface, _background, 0, 0);
      wl_surface_damage_buffer(_surface, 0, 0, kWidth, kHeight);
      wl_surface_commit(_surface);
      wl_display_flush(_display);
      return _isOpen;
   }

   // clap_window has nothing for Wayland, see xdgForeignHandle()
   clap_window WaylandHostWindow::clapWindow() const {
      clap_window window;
      window.api = CLAP_WINDOW_API_WAYLAND;
      window.ptr = nullptr;
      return window;
   }

   void WaylandHostWindow::onGlobal(wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
      if (!strcmp(interface, wl_compositor_interface.name) && version >= 4) {
         _compositor =
            static_cast<wl_compositor *>(wl_registry_bind(registry, name, &wl_compositor_interface, 4));
      } else if (!strcmp(interface, wl_shm_interface.name)) {
         _shm = static_cast<wl_shm *>(wl_registry_bind(registry, name, &wl_shm_interface, 1));
      } else if (!strcmp(interface, xdg_wm_base_interface.name)) {
         static const xdg_wm_base_listener wmBaseListener = {
            .ping = [](void *, xdg_wm_base *wmBase, uint32_t serial) {
               xdg_wm_base_pong(wmBase, serial);
            },
         };
         _wmBase =
            static_cast<xdg_wm_base *>(wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
         xdg_wm_base_add_listener(_wmBase, &wmBaseListener, this);
      } else if (!strcmp(interface, zxdg_decoration_manager_v1_interface.name)) {
         _decorationManager = static_cast<zxdg_decoration_manager_v1 *>(
            wl_registry_bind(registry, name, &zxdg_decoration_manager_v1_interface, 1));
      } else if (!strcmp(interface, zxdg_exporter_v2_interface.name)) {
         _exporter = static_cast<zxdg_exporter_v2 *>(
            wl_registry_bind(registry, name, &zxdg_exporter_v2_interface, 1));
      } else if (!strcmp(interface, xdg_activation_v1_interface.name)) {
         _activation = static_cast<xdg_activation_v1 *>(
            wl_registry_bind(registry, name, &xdg_activation_v1_interface, 1));
      } else if (!strcmp(interface, wp_cursor_shape_manager_v1_interface.name)) {
         _cursorShapeManager = static_cast<wp_cursor_shape_manager_v1 *>(
            wl_registry_bind(registry, name, &wp_cursor_shape_manager_v1_interface, 1));
      } else if (!strcmp(interface, wl_seat_interface.name) && !_seat && version >= 5) {
         static const wl_seat_listener seatListener = {
            .capabilities = [](void *data, wl_seat *, uint32_t capabilities) {
               self(data).onSeatCapabilities(capabilities);
            },
            .name = [](void *, wl_seat *, const char *) {},
         };
         _seat = static_cast<wl_seat *>(wl_registry_bind(registry, name, &wl_seat_interface, 5));
         wl_seat_add_listener(_seat, &seatListener, this);
      }
   }

   void WaylandHostWindow::onSeatCapabilities(uint32_t capabilities) {
      static const wl_pointer_listener pointerListener = {
         .enter = [](void *data, wl_pointer *, uint32_t serial, wl_surface *, wl_fixed_t, wl_fixed_t) {
            self(data).onPointerEnter(serial);
         },
         .leave = [](void *, wl_pointer *, uint32_t, wl_surface *) {},
         .motion = [](void *, wl_pointer *, uint32_t, wl_fixed_t, wl_fixed_t) {},
         .button =
            [](void *data, wl_pointer *, uint32_t serial, uint32_t, uint32_t button, uint32_t state) {
               self(data).onPointerButton(serial, button, state);
            },
         .axis = [](void *, wl_pointer *, uint32_t, uint32_t, wl_fixed_t) {},
         .frame = [](void *, wl_pointer *) {},
         .axis_source = [](void *, wl_pointer *, uint32_t) {},
         .axis_stop = [](void *, wl_pointer *, uint32_t, uint32_t) {},
         .axis_discrete = [](void *, wl_pointer *, uint32_t, int32_t) {},
      };

      if ((capabilities & WL_SEAT_CAPABILITY_POINTER) && !_pointer) {
         _pointer = wl_seat_get_pointer(_seat);
         wl_pointer_add_listener(_pointer, &pointerListener, this);
         if (_cursorShapeManager)
            _cursorShapeDevice = wp_cursor_shape_manager_v1_get_pointer(_cursorShapeManager, _pointer);
      }
   }

   void WaylandHostWindow::onPointerEnter(uint32_t serial) {
      if (_cursorShapeDevice)
         wp_cursor_shape_device_v1_set_shape(
            _cursorShapeDevice, serial, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT);
   }

   void WaylandHostWindow::onPointerButton(uint32_t serial, uint32_t button, uint32_t state) {
      if (button != BTN_LEFT || state != WL_POINTER_BUTTON_STATE_PRESSED)
         return;

      if (!_activation) {
         _listener.onHostWindowClicked(nullptr);
         return;
      }

      // Lets the compositor focus the plugin's window, because the user asked for it
      static const xdg_activation_token_v1_listener tokenListener = {
         .done = [](void *data, xdg_activation_token_v1 *token, const char *tokenString) {
            self(data)._listener.onHostWindowClicked(tokenString);
            xdg_activation_token_v1_destroy(token);
         },
      };
      auto token = xdg_activation_v1_get_activation_token(_activation);
      xdg_activation_token_v1_add_listener(token, &tokenListener, this);
      xdg_activation_token_v1_set_serial(token, serial, _seat);
      xdg_activation_token_v1_set_surface(token, _surface);
      xdg_activation_token_v1_commit(token);
   }

   wl_buffer *WaylandHostWindow::createBuffer(uint32_t width, uint32_t height, uint32_t color) {
      const uint32_t stride = width * 4;
      const size_t size = size_t(stride) * height;

      int fd = memfd_create("clap-gui-host", MFD_CLOEXEC);
      if (fd < 0 || ftruncate(fd, size) < 0) {
         if (fd >= 0)
            close(fd);
         return nullptr;
      }

      void *data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
      if (data == MAP_FAILED) {
         close(fd);
         return nullptr;
      }
      std::fill_n(static_cast<uint32_t *>(data), size_t(width) * height, color);
      munmap(data, size);

      auto pool = wl_shm_create_pool(_shm, fd, size);
      auto buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888);
      wl_shm_pool_destroy(pool);
      close(fd);
      return buffer;
   }

   // The plugin has its own connection, it reads it from its timer
   void WaylandHostWindow::dispatch(int timeoutMs) {
      while (wl_display_prepare_read(_display) != 0)
         wl_display_dispatch_pending(_display);
      wl_display_flush(_display);

      pollfd pfd = {.fd = wl_display_get_fd(_display), .events = POLLIN, .revents = 0};
      if (poll(&pfd, 1, timeoutMs) > 0 && (pfd.revents & POLLIN)) {
         if (wl_display_read_events(_display) < 0)
            _isOpen = false;
      } else
         wl_display_cancel_read(_display);

      if (wl_display_dispatch_pending(_display) < 0)
         _isOpen = false;
   }

   /////////
   // X11 //
   /////////

   // Right edge of the Wayland outputs' logical layout, 0 without a Wayland compositor
   int32_t waylandLogicalWidth() {
      wl_display *display = wl_display_connect(nullptr);
      if (!display)
         return 0;

      struct OutputGeometry {
         int32_t x = 0;
         int32_t width = 0;
      };

      struct State {
         zxdg_output_manager_v1 *manager = nullptr;
         std::vector<wl_output *> outputs;
      } state;

      static const wl_registry_listener registryListener = {
         .global =
            [](void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
               auto &state = *static_cast<State *>(data);
               // We don't need wl_output's events: without listener, they are discarded
               if (!strcmp(interface, wl_output_interface.name))
                  state.outputs.push_back(static_cast<wl_output *>(
                     wl_registry_bind(registry, name, &wl_output_interface, 1)));
               else if (!strcmp(interface, zxdg_output_manager_v1_interface.name) && version >= 2)
                  state.manager = static_cast<zxdg_output_manager_v1 *>(
                     wl_registry_bind(registry, name, &zxdg_output_manager_v1_interface, 2));
            },
         .global_remove = [](void *, wl_registry *, uint32_t) {},
      };

      static const zxdg_output_v1_listener xdgOutputListener = {
         .logical_position = [](void *data, zxdg_output_v1 *, int32_t x, int32_t) {
            static_cast<OutputGeometry *>(data)->x = x;
         },
         .logical_size = [](void *data, zxdg_output_v1 *, int32_t width, int32_t) {
            static_cast<OutputGeometry *>(data)->width = width;
         },
         .done = [](void *, zxdg_output_v1 *) {},
         .name = [](void *, zxdg_output_v1 *, const char *) {},
         .description = [](void *, zxdg_output_v1 *, const char *) {},
      };

      auto registry = wl_display_get_registry(display);
      wl_registry_add_listener(registry, &registryListener, &state);
      wl_display_roundtrip(display);

      int32_t right = 0;
      if (state.manager) {
         std::vector<OutputGeometry> geometries(state.outputs.size());
         std::vector<zxdg_output_v1 *> xdgOutputs;
         for (size_t i = 0; i < state.outputs.size(); ++i) {
            auto xdgOutput = zxdg_output_manager_v1_get_xdg_output(state.manager, state.outputs[i]);
            zxdg_output_v1_add_listener(xdgOutput, &xdgOutputListener, &geometries[i]);
            xdgOutputs.push_back(xdgOutput);
         }
         wl_display_roundtrip(display);

         for (auto &g : geometries)
            right = std::max(right, g.x + g.width);
         for (auto xdgOutput : xdgOutputs)
            zxdg_output_v1_destroy(xdgOutput);
         zxdg_output_manager_v1_destroy(state.manager);
      }

      for (auto output : state.outputs)
         wl_output_destroy(output);
      wl_registry_destroy(registry);
      wl_display_disconnect(display);
      return right;
   }

   class X11HostWindow final : public HostWindow {
   public:
      using HostWindow::HostWindow;
      ~X11HostWindow() override;

      bool open(const char *title) override;
      clap_window clapWindow() const override;
      void resize(uint32_t width, uint32_t height) override;
      double desktopScale(std::string &source) override;
      void dispatch(int timeoutMs) override;

   private:
      uint32_t internAtom(const char *name);
      std::optional<double> xftDpi();

      xcb_connection_t *_connection = nullptr;
      xcb_window_t _root = 0;
      uint32_t _screenWidth = 0;
      xcb_window_t _window = 0;
      uint32_t _wmProtocols = 0;
      uint32_t _wmDeleteWindow = 0;
      uint32_t _width = kWidth;
      uint32_t _height = kHeight;
   };

   X11HostWindow::~X11HostWindow() {
      if (!_connection)
         return;
      if (_window)
         xcb_destroy_window(_connection, _window);
      xcb_disconnect(_connection);
   }

   uint32_t X11HostWindow::internAtom(const char *name) {
      auto reply = xcb_intern_atom_reply(
         _connection, xcb_intern_atom(_connection, 0, strlen(name), name), nullptr);
      const uint32_t atom = reply ? reply->atom : 0;
      free(reply);
      return atom;
   }

   bool X11HostWindow::open(const char *title) {
      int screenIndex = 0;
      _connection = xcb_connect(nullptr, &screenIndex);
      if (xcb_connection_has_error(_connection)) {
         std::cerr << "failed to connect to the X server" << std::endl;
         return false;
      }

      auto screens = xcb_setup_roots_iterator(xcb_get_setup(_connection));
      for (int i = 0; i < screenIndex && screens.rem > 0; ++i)
         xcb_screen_next(&screens);
      const xcb_screen_t *screen = screens.data;
      _root = screen->root;
      _screenWidth = screen->width_in_pixels;

      _window = xcb_generate_id(_connection);
      const uint32_t values[] = {kBackgroundColor,
                                 XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_STRUCTURE_NOTIFY};
      xcb_create_window(_connection,
                        XCB_COPY_FROM_PARENT,
                        _window,
                        screen->root,
                        0,
                        0,
                        _width,
                        _height,
                        0,
                        XCB_WINDOW_CLASS_INPUT_OUTPUT,
                        screen->root_visual,
                        XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,
                        values);

      _wmProtocols = internAtom("WM_PROTOCOLS");
      _wmDeleteWindow = internAtom("WM_DELETE_WINDOW");
      xcb_change_property(_connection,
                          XCB_PROP_MODE_REPLACE,
                          _window,
                          _wmProtocols,
                          XCB_ATOM_ATOM,
                          32,
                          1,
                          &_wmDeleteWindow);
      xcb_change_property(_connection,
                          XCB_PROP_MODE_REPLACE,
                          _window,
                          XCB_ATOM_WM_NAME,
                          XCB_ATOM_STRING,
                          8,
                          strlen(title),
                          title);
      static const char wmClass[] = "clap-gui-host\0clap-gui-host";
      xcb_change_property(_connection,
                          XCB_PROP_MODE_REPLACE,
                          _window,
                          XCB_ATOM_WM_CLASS,
                          XCB_ATOM_STRING,
                          8,
                          sizeof(wmClass),
                          wmClass);

      xcb_map_window(_connection, _window);
      xcb_flush(_connection);
      return !xcb_connection_has_error(_connection);
   }

   clap_window X11HostWindow::clapWindow() const {
      clap_window window;
      window.api = CLAP_WINDOW_API_X11;
      window.x11 = _window;
      return window;
   }

   void X11HostWindow::resize(uint32_t width, uint32_t height) {
      _width = width;
      _height = height;
      const uint32_t values[] = {width, height};
      xcb_configure_window(
         _connection, _window, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
      xcb_flush(_connection);
   }

   // Like a real host, we give the plugin the scale of our own GUI
   double X11HostWindow::desktopScale(std::string &source) {
      if (auto dpi = xftDpi()) {
         source = "Xft.dpi";
         return *dpi / 96;
      }

      // With XWayland, the X11 screen is either in logical pixels, if the compositor scales the
      // X11 clients, or in physical pixels, if it doesn't (COSMIC's descaling)
      if (const int32_t logicalWidth = waylandLogicalWidth(); logicalWidth > 0) {
         source = "the X11 screen's width over the Wayland outputs' logical width";
         return double(_screenWidth) / logicalWidth;
      }

      source = "default";
      return 1;
   }

   // As set by the desktop environment in the X resources
   std::optional<double> X11HostWindow::xftDpi() {
      auto reply = xcb_get_property_reply(
         _connection,
         xcb_get_property(
            _connection, 0, _root, XCB_ATOM_RESOURCE_MANAGER, XCB_ATOM_STRING, 0, 1 << 20),
         nullptr);
      if (!reply)
         return std::nullopt;

      const std::string resources(static_cast<const char *>(xcb_get_property_value(reply)),
                                  xcb_get_property_value_length(reply));
      free(reply);

      constexpr const char key[] = "Xft.dpi:";
      const auto pos = resources.find(key);
      if (pos == std::string::npos)
         return std::nullopt;

      const double dpi = std::strtod(resources.c_str() + pos + strlen(key), nullptr);
      return dpi > 0 ? std::optional(dpi) : std::nullopt;
   }

   void X11HostWindow::dispatch(int timeoutMs) {
      xcb_flush(_connection);

      pollfd pfd = {.fd = xcb_get_file_descriptor(_connection), .events = POLLIN, .revents = 0};
      poll(&pfd, 1, timeoutMs);

      while (auto *event = xcb_poll_for_event(_connection)) {
         switch (event->response_type & ~0x80) {
         case XCB_BUTTON_PRESS:
            if (reinterpret_cast<xcb_button_press_event_t *>(event)->detail == 1)
               _listener.onHostWindowClicked(nullptr);
            break;

         case XCB_CONFIGURE_NOTIFY: {
            auto e = reinterpret_cast<xcb_configure_notify_event_t *>(event);
            if (e->window == _window && (e->width != _width || e->height != _height)) {
               _width = e->width;
               _height = e->height;
               _listener.onHostWindowResized(_width, _height);
            }
            break;
         }

         case XCB_CLIENT_MESSAGE: {
            auto e = reinterpret_cast<xcb_client_message_event_t *>(event);
            if (e->type == _wmProtocols && e->data.data32[0] == _wmDeleteWindow)
               _isOpen = false;
            break;
         }
         }
         free(event);
      }

      if (xcb_connection_has_error(_connection))
         _isOpen = false;
   }

   //////////
   // Host //
   //////////
   struct Options {
      const char *api = nullptr;
      bool isFloating = false;
      double scale = 0;
      const char *pluginPath = nullptr;
      const char *pluginId = kDefaultPluginId;
   };

   class Host final : private HostWindow::Listener {
   public:
      Host();
      ~Host() override;

      bool loadPlugin(const char *path, const char *pluginId);
      bool createGui(const Options &options);
      void run();

   private:
      static Host &self(const clap_host *host) { return *static_cast<Host *>(host->host_data); }

      const void *getExtension(const char *id);
      bool isMainThread() const { return std::this_thread::get_id() == _mainThread; }

      HostWindow::Listener &windowListener() noexcept { return *this; }

      // HostWindow::Listener
      void onHostWindowClicked(const char *activationToken) override;
      void onHostWindowResized(uint32_t width, uint32_t height) override;

      void showGui(const char *activationToken);
      void hideGui();

      // Main loop
      void runTimers();
      int nextTimerTimeoutMs() const;
      void flushParams();

      struct Timer {
         clap_id id;
         std::chrono::milliseconds period;
         Clock::time_point deadline;
      };

      const std::thread::id _mainThread = std::this_thread::get_id();

      // CLAP
      clap_host _host;
      clap_host_log _hostLog;
      clap_host_thread_check _hostThreadCheck;
      clap_host_timer_support _hostTimerSupport;
      clap_host_params _hostParams;
      clap_host_gui _hostGui;

      void *_library = nullptr;
      const clap_plugin_entry *_entry = nullptr;
      const clap_plugin *_plugin = nullptr;
      const clap_plugin_gui *_pluginGui = nullptr;
      const clap_plugin_gui_xdg_foreign_v2 *_pluginXdgForeign = nullptr;
      const clap_plugin_gui_xdg_activation_v1 *_pluginXdgActivation = nullptr;
      const clap_plugin_params *_pluginParams = nullptr;
      const clap_plugin_timer_support *_pluginTimerSupport = nullptr;
      bool _isGuiCreated = false;
      bool _isGuiFloating = false;
      bool _isGuiVisible = false;

      std::vector<Timer> _timers;
      clap_id _nextTimerId = 0;
      bool _isCallbackRequested = false;
      bool _isFlushRequested = false;

      std::unique_ptr<HostWindow> _window;
   };

   Host::Host() {
      _host = {
         .clap_version = CLAP_VERSION,
         .host_data = this,
         .name = kTitle,
         .vendor = "clap",
         .url = "https://github.com/free-audio/clap-plugins",
         .version = "0.1",
         .get_extension = [](const clap_host *h,
                             const char *id) { return self(h).getExtension(id); },
         .request_restart = [](const clap_host *) {},
         .request_process = [](const clap_host *) {},
         .request_callback = [](const clap_host *h) { self(h)._isCallbackRequested = true; },
      };

      _hostLog = {
         .log =
            [](const clap_host *, clap_log_severity severity, const char *msg) {
               std::cerr << "plugin log [" << severity << "]: " << msg << std::endl;
            },
      };

      _hostThreadCheck = {
         .is_main_thread = [](const clap_host *h) { return self(h).isMainThread(); },
         // The plugin isn't activated, so there is no audio thread
         .is_audio_thread = [](const clap_host *) { return false; },
      };

      _hostTimerSupport = {
         .register_timer =
            [](const clap_host *h, uint32_t periodMs, clap_id *timerId) {
               auto &host = self(h);
               const auto period = std::chrono::milliseconds(std::max(periodMs, 1u));
               *timerId = host._nextTimerId++;
               host._timers.push_back({*timerId, period, Clock::now() + period});
               return true;
            },
         .unregister_timer =
            [](const clap_host *h, clap_id timerId) {
               auto &timers = self(h)._timers;
               auto it = std::find_if(
                  timers.begin(), timers.end(), [&](auto &t) { return t.id == timerId; });
               if (it == timers.end())
                  return false;
               timers.erase(it);
               return true;
            },
      };

      _hostParams = {
         .rescan = [](const clap_host *, clap_param_rescan_flags) {},
         .clear = [](const clap_host *, clap_id, clap_param_clear_flags) {},
         .request_flush = [](const clap_host *h) { self(h)._isFlushRequested = true; },
      };

      _hostGui = {
         .resize_hints_changed = [](const clap_host *) {},
         .request_resize =
            [](const clap_host *h, uint32_t width, uint32_t height) {
               auto &host = self(h);
               if (host._isGuiFloating || !host._window)
                  return false;
               host._window->resize(width, height);
               return true;
            },
         .request_show = [](const clap_host *) { return false; },
         .request_hide = [](const clap_host *) { return false; },
         .closed =
            [](const clap_host *h, bool wasDestroyed) {
               std::cout << "the plugin closed its window" << std::endl;
               self(h)._isGuiVisible = false;
            },
      };
   }

   Host::~Host() {
      // Before the host's window, which may be the plugin's window's parent
      if (_isGuiCreated)
         _pluginGui->destroy(_plugin);

      if (_plugin)
         _plugin->destroy(_plugin);

      if (_entry)
         _entry->deinit();

      if (_library)
         dlclose(_library);
   }

   const void *Host::getExtension(const char *id) {
      if (!strcmp(id, CLAP_EXT_LOG))
         return &_hostLog;
      if (!strcmp(id, CLAP_EXT_THREAD_CHECK))
         return &_hostThreadCheck;
      if (!strcmp(id, CLAP_EXT_TIMER_SUPPORT))
         return &_hostTimerSupport;
      if (!strcmp(id, CLAP_EXT_PARAMS))
         return &_hostParams;
      if (!strcmp(id, CLAP_EXT_GUI))
         return &_hostGui;
      return nullptr;
   }

   bool Host::loadPlugin(const char *path, const char *pluginId) {
      _library = dlopen(path, RTLD_NOW | RTLD_LOCAL);
      if (!_library) {
         std::cerr << "failed to load " << path << ": " << dlerror() << std::endl;
         return false;
      }

      auto entry = static_cast<const clap_plugin_entry *>(dlsym(_library, "clap_entry"));
      if (!entry || !entry->init(path)) {
         std::cerr << "invalid clap entry" << std::endl;
         return false;
      }
      _entry = entry;

      auto factory =
         static_cast<const clap_plugin_factory *>(entry->get_factory(CLAP_PLUGIN_FACTORY_ID));
      if (!factory) {
         std::cerr << "no plugin factory" << std::endl;
         return false;
      }

      _plugin = factory->create_plugin(factory, &_host, pluginId);
      if (!_plugin) {
         std::cerr << "failed to create the plugin " << pluginId << ", the plugins are:" << std::endl;
         for (uint32_t i = 0; i < factory->get_plugin_count(factory); ++i)
            std::cerr << "  " << factory->get_plugin_descriptor(factory, i)->id << std::endl;
         return false;
      }

      if (!_plugin->init(_plugin)) {
         std::cerr << "failed to initialize the plugin" << std::endl;
         _plugin->destroy(_plugin);
         _plugin = nullptr;
         return false;
      }

      _pluginGui = static_cast<const clap_plugin_gui *>(_plugin->get_extension(_plugin, CLAP_EXT_GUI));
      _pluginXdgForeign = static_cast<const clap_plugin_gui_xdg_foreign_v2 *>(
         _plugin->get_extension(_plugin, CLAP_EXT_GUI_XDG_FOREIGN_V2));
      _pluginXdgActivation = static_cast<const clap_plugin_gui_xdg_activation_v1 *>(
         _plugin->get_extension(_plugin, CLAP_EXT_GUI_XDG_ACTIVATION_V1));
      _pluginParams =
         static_cast<const clap_plugin_params *>(_plugin->get_extension(_plugin, CLAP_EXT_PARAMS));
      _pluginTimerSupport = static_cast<const clap_plugin_timer_support *>(
         _plugin->get_extension(_plugin, CLAP_EXT_TIMER_SUPPORT));

      if (!_pluginGui) {
         std::cerr << "the plugin has no GUI" << std::endl;
         return false;
      }

      return true;
   }

   bool Host::createGui(const Options &options) {
      const char *api = options.api;
      bool isFloating = options.isFloating;
      if (!api && !_pluginGui->get_preferred_api(_plugin, &api, &isFloating)) {
         std::cerr << "the plugin has no preferred window API, use --x11 or --wayland"
                   << std::endl;
         return false;
      }

      if (strcmp(api, CLAP_WINDOW_API_X11) && strcmp(api, CLAP_WINDOW_API_WAYLAND)) {
         std::cerr << "clap-gui-host can't host the window API " << api << " yet" << std::endl;
         return false;
      }

      // Wayland can only have floating plugin windows
      if (!strcmp(api, CLAP_WINDOW_API_WAYLAND)) {
         isFloating = true;
         _window = std::make_unique<WaylandHostWindow>(windowListener());
      } else
         _window = std::make_unique<X11HostWindow>(windowListener());

      std::cout << "window API: " << api << (isFloating ? ", floating" : ", embedded") << std::endl;
      if (isFloating)
         std::cout << "click in the host's window to show or hide the plugin's window" << std::endl;

      if (!_window->open(kTitle))
         return false;

      if (!_pluginGui->is_api_supported(_plugin, api, isFloating)) {
         std::cerr << "the plugin doesn't support this window API" << std::endl;
         return false;
      }

      if (!_pluginGui->create(_plugin, api, isFloating)) {
         std::cerr << "failed to create the plugin GUI" << std::endl;
         return false;
      }
      _isGuiCreated = true;
      _isGuiFloating = isFloating;

      // Wayland's window APIs use logical sizes, even if CLAP's gui.h still says physical: the
      // plugin gets the scale from the compositor
      if (!strcmp(api, CLAP_WINDOW_API_X11)) {
         std::string scaleSource = "--scale";
         const double scale =
            options.scale > 0 ? options.scale : _window->desktopScale(scaleSource);
         std::cout << "scale: " << scale << ", from " << scaleSource << std::endl;
         if (!_pluginGui->set_scale(_plugin, scale))
            std::cerr << "the plugin ignored the scale" << std::endl;
      } else if (options.scale > 0)
         std::cerr << "--scale is for X11, with Wayland the plugin gets the scale from the "
                      "compositor"
                   << std::endl;

      const clap_window window = _window->clapWindow();
      if (isFloating) {
         // Wayland has its own extension
         if (!strcmp(api, CLAP_WINDOW_API_WAYLAND)) {
            if (const char *handle = _window->xdgForeignHandle()) {
               if (!_pluginXdgForeign)
                  std::cerr << "the plugin doesn't support " << CLAP_EXT_GUI_XDG_FOREIGN_V2
                            << ", its window has no parent" << std::endl;
               else if (!_pluginXdgForeign->set_transient(_plugin, handle))
                  std::cerr << "the plugin refused the xdg-foreign handle" << std::endl;
            }
         } else if (!_pluginGui->set_transient(_plugin, &window))
            std::cerr << "the plugin refused the transient window" << std::endl;

         _pluginGui->suggest_title(_plugin, "Plugin (CLAP GUI Host)");
      } else {
         uint32_t width = 0;
         uint32_t height = 0;
         if (!_pluginGui->get_size(_plugin, &width, &height)) {
            std::cerr << "failed to get the plugin GUI size" << std::endl;
            return false;
         }
         _window->resize(width, height);

         if (!_pluginGui->set_parent(_plugin, &window)) {
            std::cerr << "failed to embed the plugin GUI" << std::endl;
            return false;
         }
      }

      // No user input asked for it, so the compositor may not focus it
      showGui(nullptr);
      return true;
   }

   // Toggles the floating window
   void Host::onHostWindowClicked(const char *activationToken) {
      if (!_isGuiCreated || !_isGuiFloating)
         return;

      if (_isGuiVisible)
         hideGui();
      else
         showGui(activationToken);
   }

   void Host::onHostWindowResized(uint32_t width, uint32_t height) {
      if (!_isGuiCreated || _isGuiFloating || !_pluginGui->can_resize(_plugin))
         return;

      if (_pluginGui->adjust_size(_plugin, &width, &height))
         _pluginGui->set_size(_plugin, width, height);
   }

   void Host::showGui(const char *activationToken) {
      if (activationToken && _pluginXdgActivation)
         _pluginXdgActivation->set_activation_token(_plugin, activationToken);

      _isGuiVisible = _pluginGui->show(_plugin);
   }

   void Host::hideGui() {
      _pluginGui->hide(_plugin);
      _isGuiVisible = false;
   }

   void Host::run() {
      while (_window->isOpen()) {
         if (_isCallbackRequested) {
            _isCallbackRequested = false;
            _plugin->on_main_thread(_plugin);
         }

         runTimers();
         flushParams();

         _window->dispatch((_isCallbackRequested || _isFlushRequested) ? 0 : nextTimerTimeoutMs());
      }
   }

   void Host::runTimers() {
      if (!_pluginTimerSupport)
         return;

      const auto now = Clock::now();
      // The plugin may (un)register timers from its callback
      for (size_t i = 0; i < _timers.size(); ++i) {
         auto timer = _timers[i];
         if (timer.deadline > now)
            continue;

         _timers[i].deadline = now + timer.period;
         _pluginTimerSupport->on_timer(_plugin, timer.id);
      }
   }

   int Host::nextTimerTimeoutMs() const {
      if (_timers.empty())
         return -1;

      auto deadline = _timers.front().deadline;
      for (auto &t : _timers)
         deadline = std::min(deadline, t.deadline);

      const auto delay = std::chrono::ceil<std::chrono::milliseconds>(deadline - Clock::now());
      return std::max<int>(0, delay.count());
   }

   void Host::flushParams() {
      if (!_isFlushRequested || !_pluginParams)
         return;
      _isFlushRequested = false;

      static const clap_input_events inEvents = {
         .ctx = nullptr,
         .size = [](const clap_input_events *) -> uint32_t { return 0; },
         .get = [](const clap_input_events *, uint32_t) -> const clap_event_header * {
            return nullptr;
         },
      };

      static const clap_output_events outEvents = {
         .ctx = nullptr,
         .try_push = [](const clap_output_events *, const clap_event_header *hdr) {
            if (hdr->space_id != CLAP_CORE_EVENT_SPACE_ID)
               return true;

            switch (hdr->type) {
            case CLAP_EVENT_PARAM_VALUE: {
               auto ev = reinterpret_cast<const clap_event_param_value *>(hdr);
               std::cout << "param " << ev->param_id << " = " << ev->value << std::endl;
               break;
            }
            case CLAP_EVENT_PARAM_GESTURE_BEGIN:
               std::cout << "param "
                         << reinterpret_cast<const clap_event_param_gesture *>(hdr)->param_id
                         << ": gesture begin" << std::endl;
               break;
            case CLAP_EVENT_PARAM_GESTURE_END:
               std::cout << "param "
                         << reinterpret_cast<const clap_event_param_gesture *>(hdr)->param_id
                         << ": gesture end" << std::endl;
               break;
            }
            return true;
         },
      };

      _pluginParams->flush(_plugin, &inEvents, &outEvents);
   }

   void usage(const char *program) {
      std::cerr << "usage: " << program
                << " [--x11 | --wayland] [--floating] [--scale <factor>] <plugin.clap> [plugin-id]"
                << std::endl;
   }

} // namespace

int main(int argc, char **argv) {
   Options options;
   std::vector<const char *> positional;
   for (int i = 1; i < argc; ++i) {
      if (!strcmp(argv[i], "--x11"))
         options.api = CLAP_WINDOW_API_X11;
      else if (!strcmp(argv[i], "--wayland"))
         options.api = CLAP_WINDOW_API_WAYLAND;
      else if (!strcmp(argv[i], "--floating"))
         options.isFloating = true;
      else if (!strcmp(argv[i], "--scale") && i + 1 < argc)
         options.scale = std::atof(argv[++i]);
      else if (argv[i][0] == '-') {
         usage(argv[0]);
         return 1;
      } else
         positional.push_back(argv[i]);
   }

   if (positional.empty() || positional.size() > 2) {
      usage(argv[0]);
      return 1;
   }
   options.pluginPath = positional[0];
   if (positional.size() > 1)
      options.pluginId = positional[1];

   Host host;
   if (!host.loadPlugin(options.pluginPath, options.pluginId) || !host.createGui(options))
      return 1;

   host.run();
   return 0;
}
