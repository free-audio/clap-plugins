// Minimal host to try floating plugin windows on Wayland.
// See plugins/gui/wayland-floating.h for the contract.
//
// Usage: clap-wayland-host <plugin.clap> [plugin-id]
//
// Clicking in the host's window shows or hides the plugin's window.
//
// It only shows the GUI: the plugin isn't activated, and the parameter changes are flushed with
// clap_plugin_params.flush() and printed.

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <dlfcn.h>
#include <linux/input-event-codes.h>
#include <poll.h>
#include <sys/mman.h>
#include <unistd.h>

#include <clap/clap.h>
#include <wayland-client.h>

#include "cursor-shape-v1-client-protocol.h"
#include "wayland-floating.h"
#include "xdg-activation-v1-client-protocol.h"
#include "xdg-decoration-unstable-v1-client-protocol.h"
#include "xdg-foreign-unstable-v2-client-protocol.h"
#include "xdg-shell-client-protocol.h"

namespace {

   using Clock = std::chrono::steady_clock;

   constexpr uint32_t kBackgroundColor = 0x303030;
   constexpr uint32_t kWidth = 480;
   constexpr uint32_t kHeight = 320;
   constexpr const char *kDefaultPluginId = "com.github.free-audio.clap.gain";

   class Host {
   public:
      Host();
      ~Host();

      bool loadPlugin(const char *path, const char *pluginId);
      bool connect();
      bool createWindow();
      bool createGui();
      void run();

   private:
      static Host &self(const clap_host *host) { return *static_cast<Host *>(host->host_data); }
      static Host &self(void *data) { return *static_cast<Host *>(data); }

      const void *getExtension(const char *id);
      bool isMainThread() const { return std::this_thread::get_id() == _mainThread; }

      // Wayland
      void onGlobal(wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
      void onSeatCapabilities(uint32_t capabilities);
      void onPointerEnter(uint32_t serial);
      void onPointerButton(uint32_t serial, uint32_t button, uint32_t state);
      void onActivationToken(xdg_activation_token_v1 *token, const char *tokenString);
      wl_buffer *createBuffer(uint32_t width, uint32_t height, uint32_t color);
      void dispatchWayland(int timeoutMs);

      // GUI
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
      const clap_plugin_gui_wayland *_pluginGuiWayland = nullptr;
      const clap_plugin_params *_pluginParams = nullptr;
      const clap_plugin_timer_support *_pluginTimerSupport = nullptr;
      bool _isGuiCreated = false;
      bool _isGuiVisible = false;

      std::vector<Timer> _timers;
      clap_id _nextTimerId = 0;
      bool _isCallbackRequested = false;
      bool _isFlushRequested = false;

      // Wayland
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
      bool _isRunning = true;
   };

   Host::Host() {
      _host = {
         .clap_version = CLAP_VERSION,
         .host_data = this,
         .name = "CLAP Wayland Host",
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
         // The plugin's window is floating, it resizes itself
         .request_resize = [](const clap_host *, uint32_t, uint32_t) { return false; },
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
      if (_isGuiCreated)
         _pluginGui->destroy(_plugin);

      if (_plugin)
         _plugin->destroy(_plugin);

      if (_entry)
         _entry->deinit();

      if (_library)
         dlclose(_library);

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

   ////////////
   // Plugin //
   ////////////
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
         std::cerr << "failed to create the plugin " << pluginId << std::endl;
         return false;
      }

      if (!_plugin->init(_plugin)) {
         std::cerr << "failed to initialize the plugin" << std::endl;
         _plugin->destroy(_plugin);
         _plugin = nullptr;
         return false;
      }

      _pluginGui = static_cast<const clap_plugin_gui *>(_plugin->get_extension(_plugin, CLAP_EXT_GUI));
      _pluginGuiWayland = static_cast<const clap_plugin_gui_wayland *>(
         _plugin->get_extension(_plugin, CLAP_EXT_GUI_WAYLAND));
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

   /////////////
   // Wayland //
   /////////////
   bool Host::connect() {
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
      return true;
   }

   void Host::onGlobal(wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
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

   void Host::onSeatCapabilities(uint32_t capabilities) {
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
         .axis_value120 = [](void *, wl_pointer *, uint32_t, int32_t) {},
         .axis_relative_direction = [](void *, wl_pointer *, uint32_t, uint32_t) {},
      };

      if ((capabilities & WL_SEAT_CAPABILITY_POINTER) && !_pointer) {
         _pointer = wl_seat_get_pointer(_seat);
         wl_pointer_add_listener(_pointer, &pointerListener, this);
         if (_cursorShapeManager)
            _cursorShapeDevice = wp_cursor_shape_manager_v1_get_pointer(_cursorShapeManager, _pointer);
      }
   }

   void Host::onPointerEnter(uint32_t serial) {
      if (_cursorShapeDevice)
         wp_cursor_shape_device_v1_set_shape(
            _cursorShapeDevice, serial, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT);
   }

   // Toggles the plugin's window
   void Host::onPointerButton(uint32_t serial, uint32_t button, uint32_t state) {
      if (button != BTN_LEFT || state != WL_POINTER_BUTTON_STATE_PRESSED || !_isGuiCreated)
         return;

      if (_isGuiVisible) {
         hideGui();
         return;
      }

      if (!_activation) {
         showGui(nullptr);
         return;
      }

      // Lets the compositor focus the plugin's window, because the user asked for it
      static const xdg_activation_token_v1_listener tokenListener = {
         .done = [](void *data, xdg_activation_token_v1 *token, const char *tokenString) {
            self(data).onActivationToken(token, tokenString);
         },
      };
      auto token = xdg_activation_v1_get_activation_token(_activation);
      xdg_activation_token_v1_add_listener(token, &tokenListener, this);
      xdg_activation_token_v1_set_serial(token, serial, _seat);
      xdg_activation_token_v1_set_surface(token, _surface);
      xdg_activation_token_v1_commit(token);
   }

   void Host::onActivationToken(xdg_activation_token_v1 *token, const char *tokenString) {
      showGui(tokenString);
      xdg_activation_token_v1_destroy(token);
   }

   bool Host::createWindow() {
      static const xdg_surface_listener xdgSurfaceListener = {
         .configure =
            [](void *data, xdg_surface *xdgSurface, uint32_t serial) {
               xdg_surface_ack_configure(xdgSurface, serial);
               self(data)._isConfigured = true;
            },
      };

      static const xdg_toplevel_listener toplevelListener = {
         .configure = [](void *, xdg_toplevel *, int32_t, int32_t, wl_array *) {},
         .close = [](void *data, xdg_toplevel *) { self(data)._isRunning = false; },
         .configure_bounds = [](void *, xdg_toplevel *, int32_t, int32_t) {},
         .wm_capabilities = [](void *, xdg_toplevel *, wl_array *) {},
      };

      _surface = wl_compositor_create_surface(_compositor);
      _xdgSurface = xdg_wm_base_get_xdg_surface(_wmBase, _surface);
      xdg_surface_add_listener(_xdgSurface, &xdgSurfaceListener, this);
      _toplevel = xdg_surface_get_toplevel(_xdgSurface);
      xdg_toplevel_add_listener(_toplevel, &toplevelListener, this);
      xdg_toplevel_set_title(_toplevel, "CLAP Wayland Host (click to show or hide the plugin)");
      xdg_toplevel_set_app_id(_toplevel, "clap-wayland-host");
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
      while ((!_isConfigured || (_exported && _exportedHandle.empty())) && _isRunning)
         if (wl_display_dispatch(_display) < 0)
            return false;

      _background = createBuffer(kWidth, kHeight, kBackgroundColor);
      wl_surface_attach(_surface, _background, 0, 0);
      wl_surface_damage_buffer(_surface, 0, 0, kWidth, kHeight);
      wl_surface_commit(_surface);

      return _isRunning;
   }

   wl_buffer *Host::createBuffer(uint32_t width, uint32_t height, uint32_t color) {
      const uint32_t stride = width * 4;
      const size_t size = size_t(stride) * height;

      int fd = memfd_create("clap-wayland-host", MFD_CLOEXEC);
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

   /////////
   // GUI //
   /////////
   bool Host::createGui() {
      if (!_pluginGui->is_api_supported(_plugin, CLAP_WINDOW_API_WAYLAND, true)) {
         std::cerr << "the plugin doesn't support floating " << CLAP_WINDOW_API_WAYLAND
                   << " windows" << std::endl;
         return false;
      }

      if (!_pluginGui->create(_plugin, CLAP_WINDOW_API_WAYLAND, true)) {
         std::cerr << "failed to create the plugin GUI" << std::endl;
         return false;
      }
      _isGuiCreated = true;

      if (!_exportedHandle.empty()) {
         clap_window window;
         window.api = CLAP_WINDOW_API_WAYLAND;
         window.ptr = const_cast<char *>(_exportedHandle.c_str());
         if (!_pluginGui->set_transient(_plugin, &window))
            std::cerr << "the plugin refused the transient window" << std::endl;
      }

      _pluginGui->suggest_title(_plugin, "Plugin (CLAP Wayland Host)");

      // No user input asked for it, so the compositor may not focus it
      showGui(nullptr);
      return true;
   }

   void Host::showGui(const char *activationToken) {
      if (activationToken && _pluginGuiWayland)
         _pluginGuiWayland->set_activation_token(_plugin, activationToken);

      _isGuiVisible = _pluginGui->show(_plugin);
   }

   void Host::hideGui() {
      _pluginGui->hide(_plugin);
      _isGuiVisible = false;
   }

   ///////////////
   // Main loop //
   ///////////////
   void Host::run() {
      while (_isRunning) {
         if (_isCallbackRequested) {
            _isCallbackRequested = false;
            _plugin->on_main_thread(_plugin);
         }

         runTimers();
         flushParams();

         dispatchWayland((_isCallbackRequested || _isFlushRequested) ? 0 : nextTimerTimeoutMs());
      }
   }

   // The plugin has its own connection, it reads it from its timer
   void Host::dispatchWayland(int timeoutMs) {
      while (wl_display_prepare_read(_display) != 0)
         wl_display_dispatch_pending(_display);
      wl_display_flush(_display);

      pollfd pfd = {.fd = wl_display_get_fd(_display), .events = POLLIN, .revents = 0};
      if (poll(&pfd, 1, timeoutMs) > 0 && (pfd.revents & POLLIN)) {
         if (wl_display_read_events(_display) < 0)
            _isRunning = false;
      } else
         wl_display_cancel_read(_display);

      if (wl_display_dispatch_pending(_display) < 0)
         _isRunning = false;
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

} // namespace

int main(int argc, char **argv) {
   if (argc < 2) {
      std::cerr << "usage: " << argv[0] << " <plugin.clap> [plugin-id]" << std::endl;
      return 1;
   }

   Host host;
   if (!host.loadPlugin(argv[1], argc > 2 ? argv[2] : kDefaultPluginId))
      return 1;

   if (!host.connect() || !host.createWindow() || !host.createGui())
      return 1;

   host.run();
   return 0;
}
