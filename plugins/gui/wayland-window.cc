#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <iostream>

#include <linux/input-event-codes.h>
#include <poll.h>
#include <sys/mman.h>
#include <unistd.h>

#include <wayland-client.h>

#include "cursor-shape-v1-client-protocol.h"
#include "fractional-scale-v1-client-protocol.h"
#include "viewporter-client-protocol.h"
#include "wayland-window.hh"
#include "xdg-activation-v1-client-protocol.h"
#include "xdg-decoration-unstable-v1-client-protocol.h"
#include "xdg-foreign-unstable-v2-client-protocol.h"
#include "xdg-shell-client-protocol.h"
// After xdg-shell, whose interfaces it uses
#include "clap-surface-embedding-v1-client-protocol.h"

namespace clap {

   namespace {
      // We don't wait more than that for the compositor to tell that the previous frame is
      // displayed. Frame callbacks may never come for a surface which isn't visible.
      constexpr std::chrono::milliseconds kMaxFrameCallbackWait{250};

      // Scroll distance in surface-local coordinates corresponding to one line
      constexpr double kScrollDistancePerLine = 10;

      // When the compositor doesn't decorate the window
      constexpr uint32_t kTitleBarHeight = 24;

      WaylandWindow &self(void *data) { return *static_cast<WaylandWindow *>(data); }

      const wl_registry_listener registryListener = {
         .global =
            [](void *data,
               wl_registry *registry,
               uint32_t name,
               const char *interface,
               uint32_t version) { self(data).onGlobal(registry, name, interface, version); },
         .global_remove = [](void *data, wl_registry *, uint32_t name) {
            self(data).onGlobalRemove(name);
         },
      };

      const wl_output_listener outputListener = {
         .geometry = [](void *, wl_output *, int32_t, int32_t, int32_t, int32_t, int32_t, const char *, const char *, int32_t) {},
         .mode = [](void *, wl_output *, uint32_t, int32_t, int32_t, int32_t) {},
         .done = [](void *data, wl_output *output) { self(data).onOutputDone(output); },
         .scale = [](void *data, wl_output *output, int32_t factor) {
            self(data).onOutputScale(output, factor);
         },
         .name = [](void *, wl_output *, const char *) {},
         .description = [](void *, wl_output *, const char *) {},
      };

      const xdg_wm_base_listener wmBaseListener = {
         .ping = [](void *, xdg_wm_base *wmBase, uint32_t serial) {
            xdg_wm_base_pong(wmBase, serial);
         },
      };

      const xdg_surface_listener xdgSurfaceListener = {
         .configure = [](void *data, xdg_surface *, uint32_t serial) {
            self(data).onSurfaceConfigure(serial);
         },
      };

      const xdg_toplevel_listener toplevelListener = {
         .configure = [](void *data, xdg_toplevel *, int32_t width, int32_t height, wl_array *) {
            self(data).onToplevelConfigure(width, height);
         },
         .close = [](void *data, xdg_toplevel *) { self(data).onToplevelClose(); },
         .configure_bounds = [](void *, xdg_toplevel *, int32_t, int32_t) {},
         .wm_capabilities = [](void *, xdg_toplevel *, wl_array *) {},
      };

      const zxdg_toplevel_decoration_v1_listener decorationListener = {
         .configure = [](void *data, zxdg_toplevel_decoration_v1 *, uint32_t mode) {
            self(data).onDecorationConfigure(mode);
         },
      };

      const zxdg_imported_v2_listener importedListener = {
         .destroyed = [](void *data, zxdg_imported_v2 *) { self(data).onImportedDestroyed(); },
      };

      const clap_embedded_surface_v1_listener embeddedSurfaceListener = {
         .position = [](void *data, clap_embedded_surface_v1 *, int32_t x, int32_t y) {
            self(data).onEmbeddedPosition(x, y);
         },
         .closed = [](void *data, clap_embedded_surface_v1 *) { self(data).onEmbeddedClosed(); },
      };

      const wl_surface_listener surfaceListener = {
         .enter = [](void *data, wl_surface *, wl_output *output) {
            self(data).onSurfaceEnter(output);
         },
         .leave = [](void *data, wl_surface *, wl_output *output) {
            self(data).onSurfaceLeave(output);
         },
         .preferred_buffer_scale = [](void *data, wl_surface *, int32_t factor) {
            self(data).onPreferredBufferScale(factor);
         },
         .preferred_buffer_transform = [](void *, wl_surface *, uint32_t) {},
      };

      const wp_fractional_scale_v1_listener fractionalScaleListener = {
         .preferred_scale = [](void *data, wp_fractional_scale_v1 *, uint32_t scale120) {
            self(data).onPreferredFractionalScale(scale120);
         },
      };

      const wl_seat_listener seatListener = {
         .capabilities = [](void *data, wl_seat *, uint32_t capabilities) {
            self(data).onSeatCapabilities(capabilities);
         },
         .name = [](void *, wl_seat *, const char *) {},
      };

      const wl_pointer_listener pointerListener = {
         .enter =
            [](void *data,
               wl_pointer *,
               uint32_t serial,
               wl_surface *surface,
               wl_fixed_t x,
               wl_fixed_t y) {
               self(data).onPointerEnter(
                  serial, surface, wl_fixed_to_double(x), wl_fixed_to_double(y));
            },
         .leave = [](void *data, wl_pointer *, uint32_t, wl_surface *surface) {
            self(data).onPointerLeave(surface);
         },
         .motion = [](void *data, wl_pointer *, uint32_t, wl_fixed_t x, wl_fixed_t y) {
            self(data).onPointerMotion(wl_fixed_to_double(x), wl_fixed_to_double(y));
         },
         .button =
            [](void *data,
               wl_pointer *,
               uint32_t serial,
               uint32_t,
               uint32_t button,
               uint32_t state) { self(data).onPointerButton(serial, button, state); },
         .axis = [](void *data, wl_pointer *, uint32_t, uint32_t axis, wl_fixed_t value) {
            self(data).onPointerAxis(axis, wl_fixed_to_double(value));
         },
         .frame = [](void *data, wl_pointer *) { self(data).onPointerFrame(); },
         .axis_source = [](void *, wl_pointer *, uint32_t) {},
         .axis_stop = [](void *, wl_pointer *, uint32_t, uint32_t) {},
         .axis_discrete = [](void *data, wl_pointer *, uint32_t axis, int32_t discrete) {
            self(data).onPointerAxisDiscrete(axis, discrete);
         },
      };

      const wl_buffer_listener bufferListener = {
         .release = [](void *data, wl_buffer *) { *static_cast<bool *>(data) = false; },
      };

      const wl_callback_listener frameListener = {
         .done = [](void *data, wl_callback *, uint32_t) { self(data).onFrameDone(); },
      };

      template <typename T>
      T *bind(wl_registry *registry, uint32_t name, const wl_interface &interface, uint32_t version) {
         return static_cast<T *>(wl_registry_bind(registry, name, &interface, version));
      }
   } // namespace

   WaylandWindow::WaylandWindow(Listener &listener) : PlatformWindow(listener) {}

   WaylandWindow::~WaylandWindow() { destroy(); }

   bool WaylandWindow::connect(wl_display *display) {
      assert(!_display);

      _display = display;
      if (!_display) {
         std::cerr << "clap-gui: failed to connect to the wayland compositor" << std::endl;
         return false;
      }

      _registry = wl_display_get_registry(_display);
      wl_registry_add_listener(_registry, &registryListener, this);
      if (wl_display_roundtrip(_display) < 0 || !_compositor || !_shm) {
         std::cerr << "clap-gui: missing wayland globals" << std::endl;
         destroy();
         return false;
      }
      return true;
   }

   void WaylandWindow::createSurface() {
      _surface = wl_compositor_create_surface(_compositor);
      wl_surface_add_listener(_surface, &surfaceListener, this);

      if (_viewporter && _fractionalScaleManager) {
         _viewport = wp_viewporter_get_viewport(_viewporter, _surface);
         _fractionalScale =
            wp_fractional_scale_manager_v1_get_fractional_scale(_fractionalScaleManager, _surface);
         wp_fractional_scale_v1_add_listener(_fractionalScale, &fractionalScaleListener, this);
      }
   }

   bool WaylandWindow::open() {
      if (!connect(wl_display_connect(nullptr)))
         return false;

      if (!_wmBase) {
         std::cerr << "clap-gui: the compositor has no xdg_wm_base" << std::endl;
         destroy();
         return false;
      }

      createSurface();

      _xdgSurface = xdg_wm_base_get_xdg_surface(_wmBase, _surface);
      xdg_surface_add_listener(_xdgSurface, &xdgSurfaceListener, this);
      _toplevel = xdg_surface_get_toplevel(_xdgSurface);
      xdg_toplevel_add_listener(_toplevel, &toplevelListener, this);
      if (!_title.empty())
         xdg_toplevel_set_title(_toplevel, _title.c_str());

      // Without it, or if the compositor prefers it, we draw the title bar
      if (_decorationManager) {
         _decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(_decorationManager, _toplevel);
         zxdg_toplevel_decoration_v1_add_listener(_decoration, &decorationListener, this);
         zxdg_toplevel_decoration_v1_set_mode(_decoration,
                                              ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
      }

      applyMinSize();
      flush();
      return true;
   }

   // wl_display_connect_to_fd() closes the socket, even on failure
   bool WaylandWindow::openEmbedded(int fd) {
      _isEmbedded = true;
      if (!connect(wl_display_connect_to_fd(fd)))
         return false;

      if (!_embedding) {
         std::cerr << "clap-gui: the host's compositor has no clap_surface_embedding_v1"
                   << std::endl;
         destroy();
         return false;
      }

      createSurface();
      flush();
      return true;
   }

   bool WaylandWindow::embed(const char *token) {
      if (!_embedding || !_surface || !token)
         return false;

      // The previous embedding ends, and the surface is unmapped
      if (_embeddedSurface) {
         clap_embedded_surface_v1_destroy(_embeddedSurface);
         _embeddedSurface = nullptr;
         _isMapped = false;
         destroyFrameCallback();
         leavePointer();
      }

      _embeddedSurface =
         clap_surface_embedding_v1_get_embedded_surface(_embedding, _surface, token);
      clap_embedded_surface_v1_add_listener(_embeddedSurface, &embeddedSurfaceListener, this);

      // Unlike an xdg_surface, there is no configure event: the next buffer maps the surface
      _isConfigured = true;
      flush();
      return true;
   }

   // Where the surface is in the host's surface, for the popups: we have none yet
   void WaylandWindow::onEmbeddedPosition(int32_t x, int32_t y) {}

   // The host refused the token, or removed the place
   void WaylandWindow::onEmbeddedClosed() {
      _isConfigured = false;
      _isMapped = false;
      destroyFrameCallback();
      leavePointer();
      _listener.onWindowLost();
   }

   void WaylandWindow::destroy() {
      destroyFrameCallback();

      for (auto &buffer : _buffers)
         destroyBuffer(buffer);
      _currentBuffer = nullptr;

      destroyPointer();

      if (_seat) {
         if (_seatVersion >= WL_SEAT_RELEASE_SINCE_VERSION)
            wl_seat_release(_seat);
         else
            wl_seat_destroy(_seat);
         _seat = nullptr;
      }

      destroyImported();

      if (_embeddedSurface) {
         clap_embedded_surface_v1_destroy(_embeddedSurface);
         _embeddedSurface = nullptr;
      }

      if (_decoration) {
         zxdg_toplevel_decoration_v1_destroy(_decoration);
         _decoration = nullptr;
      }

      if (_toplevel) {
         xdg_toplevel_destroy(_toplevel);
         _toplevel = nullptr;
      }

      if (_xdgSurface) {
         xdg_surface_destroy(_xdgSurface);
         _xdgSurface = nullptr;
      }

      if (_fractionalScale) {
         wp_fractional_scale_v1_destroy(_fractionalScale);
         _fractionalScale = nullptr;
      }

      if (_viewport) {
         wp_viewport_destroy(_viewport);
         _viewport = nullptr;
      }

      if (_surface) {
         wl_surface_destroy(_surface);
         _surface = nullptr;
      }

      if (_cursorShapeManager) {
         wp_cursor_shape_manager_v1_destroy(_cursorShapeManager);
         _cursorShapeManager = nullptr;
      }

      _enteredOutputs.clear();
      for (auto &output : _outputs)
         destroyOutput(output);
      _outputs.clear();

      if (_embedding) {
         clap_surface_embedding_v1_destroy(_embedding);
         _embedding = nullptr;
      }

      if (_activation) {
         xdg_activation_v1_destroy(_activation);
         _activation = nullptr;
      }

      if (_importer) {
         zxdg_importer_v2_destroy(_importer);
         _importer = nullptr;
      }

      if (_decorationManager) {
         zxdg_decoration_manager_v1_destroy(_decorationManager);
         _decorationManager = nullptr;
      }

      if (_wmBase) {
         xdg_wm_base_destroy(_wmBase);
         _wmBase = nullptr;
      }

      if (_fractionalScaleManager) {
         wp_fractional_scale_manager_v1_destroy(_fractionalScaleManager);
         _fractionalScaleManager = nullptr;
      }

      if (_viewporter) {
         wp_viewporter_destroy(_viewporter);
         _viewporter = nullptr;
      }

      if (_shm) {
         wl_shm_destroy(_shm);
         _shm = nullptr;
      }

      if (_compositor) {
         wl_compositor_destroy(_compositor);
         _compositor = nullptr;
      }

      if (_registry) {
         wl_registry_destroy(_registry);
         _registry = nullptr;
      }

      if (_display) {
         wl_display_flush(_display);
         wl_display_disconnect(_display);
         _display = nullptr;
      }

      _isInitialCommitDone = false;
      _isConfigured = false;
      _isMapped = false;
      _isPointerInside = false;
      _pressedButtons = 0;
   }

   void WaylandWindow::destroyPointer() {
      if (_cursorShapeDevice) {
         wp_cursor_shape_device_v1_destroy(_cursorShapeDevice);
         _cursorShapeDevice = nullptr;
      }

      if (_pointer) {
         if (_seatVersion >= WL_POINTER_RELEASE_SINCE_VERSION)
            wl_pointer_release(_pointer);
         else
            wl_pointer_destroy(_pointer);
         _pointer = nullptr;
      }
   }

   void WaylandWindow::destroyImported() {
      if (!_imported)
         return;

      zxdg_imported_v2_destroy(_imported);
      _imported = nullptr;
   }

   void WaylandWindow::onGlobal(wl_registry *registry,
                                uint32_t name,
                                const char *interface,
                                uint32_t version) {
      if (!strcmp(interface, wl_compositor_interface.name)) {
         // v4: damage_buffer, v6: preferred_buffer_scale
         if (version >= 4)
            _compositor =
               bind<wl_compositor>(registry, name, wl_compositor_interface, std::min(version, 6u));
      } else if (!strcmp(interface, wl_shm_interface.name)) {
         _shm = bind<wl_shm>(registry, name, wl_shm_interface, 1);
      } else if (!strcmp(interface, xdg_wm_base_interface.name)) {
         _wmBase = bind<xdg_wm_base>(registry, name, xdg_wm_base_interface, std::min(version, 5u));
         xdg_wm_base_add_listener(_wmBase, &wmBaseListener, this);
      } else if (!strcmp(interface, wp_viewporter_interface.name)) {
         _viewporter = bind<wp_viewporter>(registry, name, wp_viewporter_interface, 1);
      } else if (!strcmp(interface, wp_fractional_scale_manager_v1_interface.name)) {
         _fractionalScaleManager = bind<wp_fractional_scale_manager_v1>(
            registry, name, wp_fractional_scale_manager_v1_interface, 1);
      } else if (!strcmp(interface, zxdg_decoration_manager_v1_interface.name)) {
         _decorationManager =
            bind<zxdg_decoration_manager_v1>(registry, name, zxdg_decoration_manager_v1_interface, 1);
      } else if (!strcmp(interface, zxdg_importer_v2_interface.name)) {
         _importer = bind<zxdg_importer_v2>(registry, name, zxdg_importer_v2_interface, 1);
      } else if (!strcmp(interface, xdg_activation_v1_interface.name)) {
         _activation = bind<xdg_activation_v1>(registry, name, xdg_activation_v1_interface, 1);
      } else if (!strcmp(interface, wp_cursor_shape_manager_v1_interface.name)) {
         _cursorShapeManager =
            bind<wp_cursor_shape_manager_v1>(registry, name, wp_cursor_shape_manager_v1_interface, 1);
      } else if (!strcmp(interface, wl_output_interface.name) && version >= 2) {
         // v2: scale and done, v4: name and description
         const uint32_t outputVersion = std::min(version, 4u);
         auto output = bind<wl_output>(registry, name, wl_output_interface, outputVersion);
         wl_output_add_listener(output, &outputListener, this);
         _outputs.push_back({output, name, outputVersion});
      } else if (!strcmp(interface, clap_surface_embedding_v1_interface.name)) {
         _embedding =
            bind<clap_surface_embedding_v1>(registry, name, clap_surface_embedding_v1_interface, 1);
      } else if (!strcmp(interface, wl_seat_interface.name) && !_seat) {
         // v5: axis_discrete and wl_seat.release
         _seatVersion = std::min(version, 5u);
         _seat = bind<wl_seat>(registry, name, wl_seat_interface, _seatVersion);
         wl_seat_add_listener(_seat, &seatListener, this);
      }
   }

   bool WaylandWindow::setTransient(const char *exportedHandle) {
      if (!_importer || !_toplevel)
         return false;

      destroyImported();
      if (!exportedHandle) {
         flush();
         return true;
      }

      _imported = zxdg_importer_v2_import_toplevel(_importer, exportedHandle);
      zxdg_imported_v2_add_listener(_imported, &importedListener, this);
      zxdg_imported_v2_set_parent_of(_imported, _surface);
      flush();
      return true;
   }

   // The host's window is gone, or the handle was invalid
   void WaylandWindow::onImportedDestroyed() { destroyImported(); }

   void WaylandWindow::setActivationToken(const char *token) {
      _activationToken = token ? token : "";

      // Otherwise, endFrame() activates the window once it is mapped
      if (_isMapped) {
         activate();
         flush();
      }
   }

   void WaylandWindow::activate() {
      if (_activationToken.empty())
         return;

      if (_activation)
         xdg_activation_v1_activate(_activation, _activationToken.c_str(), _surface);
      _activationToken.clear();
   }

   void WaylandWindow::onSeatCapabilities(uint32_t capabilities) {
      const bool hasPointer = capabilities & WL_SEAT_CAPABILITY_POINTER;
      if (hasPointer && !_pointer) {
         _pointer = wl_seat_get_pointer(_seat);
         wl_pointer_add_listener(_pointer, &pointerListener, this);
      } else if (!hasPointer && _pointer) {
         destroyPointer();
         leavePointer();
      }
   }

   ///////////
   // Scale //
   ///////////

   // The host doesn't give the scale: CLAP's Wayland window APIs use logical sizes, and the
   // compositor knows the scale, from the best source to the worst:
   // - wp_fractional_scale_v1, which may be fractional
   // - wl_surface.preferred_buffer_scale, since wl_compositor v6
   // - the biggest scale of the outputs which the surface is on
   void WaylandWindow::updateScale() {
      double scale = 1;
      if (_fractionalScaleValue > 0)
         scale = _fractionalScaleValue;
      else if (_preferredBufferScale > 0)
         scale = _preferredBufferScale;
      else {
         for (auto &output : _outputs)
            if (std::find(_enteredOutputs.begin(), _enteredOutputs.end(), output.output) !=
                _enteredOutputs.end())
               scale = std::max<double>(scale, output.scale);
      }

      if (scale == _scale)
         return;

      _scale = scale;
      _listener.onScaleChanged();
   }

   void WaylandWindow::onPreferredFractionalScale(uint32_t scale120) {
      if (scale120 == 0)
         return;
      _fractionalScaleValue = scale120 / 120.;
      updateScale();
   }

   void WaylandWindow::onPreferredBufferScale(int32_t factor) {
      if (factor < 1)
         return;
      _preferredBufferScale = factor;
      updateScale();
   }

   void WaylandWindow::onSurfaceEnter(wl_output *output) {
      if (std::find(_enteredOutputs.begin(), _enteredOutputs.end(), output) == _enteredOutputs.end())
         _enteredOutputs.push_back(output);
      updateScale();
   }

   void WaylandWindow::onSurfaceLeave(wl_output *output) {
      std::erase(_enteredOutputs, output);
      updateScale();
   }

   void WaylandWindow::onOutputScale(wl_output *output, int32_t factor) {
      for (auto &o : _outputs)
         if (o.output == output)
            o.pendingScale = std::max(factor, 1);
   }

   // The output's properties are atomic
   void WaylandWindow::onOutputDone(wl_output *output) {
      for (auto &o : _outputs)
         if (o.output == output)
            o.scale = o.pendingScale;
      updateScale();
   }

   void WaylandWindow::onGlobalRemove(uint32_t name) {
      auto it = std::find_if(
         _outputs.begin(), _outputs.end(), [name](auto &o) { return o.name == name; });
      if (it == _outputs.end())
         return;

      std::erase(_enteredOutputs, it->output);
      destroyOutput(*it);
      _outputs.erase(it);
      updateScale();
   }

   void WaylandWindow::destroyOutput(Output &output) {
      if (output.version >= WL_OUTPUT_RELEASE_SINCE_VERSION)
         wl_output_release(output.output);
      else
         wl_output_destroy(output.output);
   }

   ///////////////
   // xdg-shell //
   ///////////////
   void WaylandWindow::onToplevelConfigure(int32_t width, int32_t height) {
      _pendingWidth = width;
      _pendingHeight = height;
   }

   void WaylandWindow::onDecorationConfigure(uint32_t mode) {
      const bool isServerSide = mode == ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE;
      if (isServerSide == _isServerSideDecorated)
         return;

      _isServerSideDecorated = isServerSide;
      applyMinSize();
   }

   void WaylandWindow::onSurfaceConfigure(uint32_t serial) {
      xdg_surface_ack_configure(_xdgSurface, serial);
      _isConfigured = true;

      // 0 means that we choose; the size includes the title bar
      if (_pendingWidth > 0 && _pendingHeight > 0) {
         const uint32_t width = std::max<uint32_t>(_pendingWidth, _minWidth);
         const uint32_t height =
            std::max<int64_t>(int64_t(_pendingHeight) - titleBarHeight(), _minHeight);
         if (width != _width || height != _height) {
            _width = width;
            _height = height;
            _listener.onResized(width, height);
         }
      }
      _pendingWidth = 0;
      _pendingHeight = 0;
   }

   void WaylandWindow::onToplevelClose() { _listener.onCloseRequested(); }

   uint32_t WaylandWindow::titleBarHeight() const {
      return _toplevel && !_isServerSideDecorated ? kTitleBarHeight : 0;
   }

   void WaylandWindow::beginMove() {
      if (!_toplevel || !_seat || !_lastButtonSerial)
         return;

      xdg_toplevel_move(_toplevel, _seat, _lastButtonSerial);
      flush();
   }

   void WaylandWindow::setTitle(const std::string &title) {
      _title = title;
      if (_toplevel) {
         xdg_toplevel_set_title(_toplevel, title.c_str());
         flush();
      }
   }

   void WaylandWindow::setSize(uint32_t width, uint32_t height) {
      _width = std::max(width, _minWidth);
      _height = std::max(height, _minHeight);
   }

   void WaylandWindow::setMinSize(uint32_t width, uint32_t height) {
      _minWidth = std::max(width, 1u);
      _minHeight = std::max(height, 1u);
      applyMinSize();
   }

   void WaylandWindow::applyMinSize() {
      if (_toplevel)
         xdg_toplevel_set_min_size(_toplevel, _minWidth, _minHeight + titleBarHeight());
   }

   void WaylandWindow::setVisible(bool isVisible) {
      if (_isVisible == isVisible)
         return;

      _isVisible = isVisible;
      if (!_surface)
         return;

      if (!isVisible) {
         unmap();
         return;
      }

      if (!_isEmbedded && !_isInitialCommitDone) {
         // Asks for the configure event, we map the window once we have it
         wl_surface_commit(_surface);
         _isInitialCommitDone = true;
         flush();
      }
   }

   void WaylandWindow::unmap() {
      destroyFrameCallback();

      if (!_isMapped)
         return;

      wl_surface_attach(_surface, nullptr, 0, 0);
      wl_surface_commit(_surface);
      flush();

      _isMapped = false;
      // An xdg_surface starts over, an embedded surface only needs a buffer
      if (!_isEmbedded) {
         _isConfigured = false;
         _isInitialCommitDone = false;
      }

      // The GUI would still show the hovered items when the window is shown again
      leavePointer();
   }

   /////////////
   // Pointer //
   /////////////
   void WaylandWindow::onPointerEnter(uint32_t serial, wl_surface *surface, double x, double y) {
      _isPointerInside = surface == _surface;
      if (!_isPointerInside)
         return;

      // Otherwise the cursor is whatever the compositor shows
      if (!_cursorShapeDevice && _cursorShapeManager && _pointer)
         _cursorShapeDevice = wp_cursor_shape_manager_v1_get_pointer(_cursorShapeManager, _pointer);
      if (_cursorShapeDevice)
         wp_cursor_shape_device_v1_set_shape(
            _cursorShapeDevice, serial, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT);

      _listener.onPointerMove(x, y);
   }

   void WaylandWindow::onPointerLeave(wl_surface *surface) {
      if (surface == _surface)
         leavePointer();
   }

   void WaylandWindow::leavePointer() {
      if (!_isPointerInside)
         return;

      // We don't get the release events while the compositor moves the window
      for (int i = 0; i < 3; ++i)
         if (_pressedButtons & (1u << i))
            _listener.onPointerButton(i, false);
      _pressedButtons = 0;

      _isPointerInside = false;
      _listener.onPointerLeave();
   }

   void WaylandWindow::onPointerMotion(double x, double y) {
      if (_isPointerInside)
         _listener.onPointerMove(x, y);
   }

   void WaylandWindow::onPointerButton(uint32_t serial, uint32_t button, uint32_t state) {
      if (!_isPointerInside)
         return;

      int index;
      switch (button) {
      case BTN_LEFT:
         index = 0;
         break;
      case BTN_RIGHT:
         index = 1;
         break;
      case BTN_MIDDLE:
         index = 2;
         break;
      default:
         return;
      }

      const bool isPressed = state == WL_POINTER_BUTTON_STATE_PRESSED;
      if (isPressed) {
         _pressedButtons |= 1u << index;
         _lastButtonSerial = serial;
      } else
         _pressedButtons &= ~(1u << index);

      _listener.onPointerButton(index, isPressed);
   }

   void WaylandWindow::onPointerAxis(uint32_t axis, double value) {
      if (!_isPointerInside)
         return;

      if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
         _scrollY += value;
      else if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
         _scrollX += value;
   }

   void WaylandWindow::onPointerAxisDiscrete(uint32_t axis, int32_t discrete) {
      if (!_isPointerInside)
         return;

      if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
         _scrollDiscreteY += discrete;
      else if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
         _scrollDiscreteX += discrete;
   }

   void WaylandWindow::onPointerFrame() {
      // Wheels send discrete steps along with the distance, prefer the steps
      const double dx = _scrollDiscreteX ? _scrollDiscreteX : _scrollX / kScrollDistancePerLine;
      const double dy = _scrollDiscreteY ? _scrollDiscreteY : _scrollY / kScrollDistancePerLine;

      // Wayland's positive values go down and right
      if (dx != 0 || dy != 0)
         _listener.onPointerScroll(-dx, -dy);

      _scrollX = 0;
      _scrollY = 0;
      _scrollDiscreteX = 0;
      _scrollDiscreteY = 0;
   }

   //////////////////
   // Event & draw //
   //////////////////
   void WaylandWindow::dispatchEvents() {
      if (!_display)
         return;

      // Reads what is available without blocking
      while (wl_display_prepare_read(_display) != 0) {
         if (wl_display_dispatch_pending(_display) < 0) {
            checkConnection();
            return;
         }
      }
      wl_display_flush(_display);

      pollfd pfd = {.fd = wl_display_get_fd(_display), .events = POLLIN, .revents = 0};
      if (poll(&pfd, 1, 0) > 0 && (pfd.revents & POLLIN))
         wl_display_read_events(_display);
      else
         wl_display_cancel_read(_display);

      wl_display_dispatch_pending(_display);
      checkConnection();
   }

   bool WaylandWindow::checkConnection() {
      if (!_display)
         return false;

      const int error = wl_display_get_error(_display);
      if (!error)
         return true;

      if (error == EPROTO) {
         const wl_interface *interface = nullptr;
         uint32_t id = 0;
         const uint32_t code = wl_display_get_protocol_error(_display, &interface, &id);
         std::cerr << "clap-gui: wayland protocol error " << code << " on "
                   << (interface ? interface->name : "?") << "@" << id << std::endl;
      } else
         std::cerr << "clap-gui: lost the wayland connection: " << strerror(error) << std::endl;

      destroy();
      _listener.onWindowLost();
      return false;
   }

   Framebuffer *WaylandWindow::beginFrame() {
      assert(!_currentBuffer);

      if (!_surface || !_isVisible || !_isConfigured)
         return nullptr;

      if (_frameCallback) {
         if (std::chrono::steady_clock::now() - _frameRequestTime < kMaxFrameCallbackWait)
            return nullptr;
         destroyFrameCallback();
      }

      const int width = std::max<int>(1, std::lround(_width * _scale));
      const int height = std::max<int>(1, std::lround((_height + titleBarHeight()) * _scale));

      for (auto &buffer : _buffers) {
         if (buffer.buffer &&
             (buffer.framebuffer.width != width || buffer.framebuffer.height != height))
            destroyBuffer(buffer);

         if (buffer.isBusy)
            continue;

         if (!buffer.buffer && !createBuffer(buffer, width, height))
            return nullptr;

         _currentBuffer = &buffer;
         return &buffer.framebuffer;
      }

      // Both buffers are still used by the compositor
      return nullptr;
   }

   void WaylandWindow::endFrame() {
      assert(_currentBuffer);
      if (!_currentBuffer)
         return;

      auto &fb = _currentBuffer->framebuffer;
      if (_viewport)
         wp_viewport_set_destination(_viewport, _width, _height + titleBarHeight());
      else
         wl_surface_set_buffer_scale(_surface, int(_scale));
      wl_surface_attach(_surface, _currentBuffer->buffer, 0, 0);
      wl_surface_damage_buffer(_surface, 0, 0, fb.width, fb.height);

      _frameCallback = wl_surface_frame(_surface);
      wl_callback_add_listener(_frameCallback, &frameListener, this);
      _frameRequestTime = std::chrono::steady_clock::now();

      wl_surface_commit(_surface);
      _isMapped = true;

      _currentBuffer->isBusy = true;
      _currentBuffer = nullptr;

      activate();
      flush();
   }

   void WaylandWindow::onFrameDone() { destroyFrameCallback(); }

   void WaylandWindow::destroyFrameCallback() {
      if (!_frameCallback)
         return;

      wl_callback_destroy(_frameCallback);
      _frameCallback = nullptr;
   }

   bool WaylandWindow::createBuffer(ShmBuffer &buffer, int width, int height) {
      const int stride = width * 4;
      const size_t size = size_t(stride) * height;

      int fd = memfd_create("clap-plugin-gui", MFD_CLOEXEC);
      if (fd < 0)
         return false;

      if (ftruncate(fd, size) < 0) {
         close(fd);
         return false;
      }

      void *data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
      if (data == MAP_FAILED) {
         close(fd);
         return false;
      }

      auto pool = wl_shm_create_pool(_shm, fd, size);
      buffer.buffer =
         wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888);
      wl_buffer_add_listener(buffer.buffer, &bufferListener, &buffer.isBusy);
      wl_shm_pool_destroy(pool);
      close(fd);

      buffer.data = data;
      buffer.size = size;
      buffer.isBusy = false;
      buffer.framebuffer.pixels = static_cast<uint32_t *>(data);
      buffer.framebuffer.width = width;
      buffer.framebuffer.height = height;
      buffer.framebuffer.stride = width;
      return true;
   }

   void WaylandWindow::destroyBuffer(ShmBuffer &buffer) {
      if (buffer.buffer)
         wl_buffer_destroy(buffer.buffer);
      if (buffer.data)
         munmap(buffer.data, buffer.size);
      buffer = ShmBuffer{};
   }

   void WaylandWindow::flush() {
      // If the socket is full, dispatchEvents() flushes the rest
      if (_display)
         wl_display_flush(_display);
   }

} // namespace clap
