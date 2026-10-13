#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iterator>

#include <xcb/xcb.h>

#include "x11-window.hh"

namespace clap {

   namespace {
      // https://specifications.freedesktop.org/xembed-spec/xembed-spec-latest.html
      constexpr uint32_t kXEmbedVersion = 0;
      constexpr uint32_t kXEmbedMapped = 1 << 0;

      // ICCCM WM_SIZE_HINTS
      constexpr uint32_t kSizeHintsMinSize = 1 << 4;
      constexpr uint32_t kSizeHintsLength = 18;

      constexpr uint32_t kPutImageHeaderSize = 24;

      constexpr uint32_t kEventMask =
         XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
         XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_ENTER_WINDOW |
         XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

      // Scroll distance of a wheel step, in lines
      constexpr double kScrollLinesPerStep = 1;
   } // namespace

   X11Window::X11Window(Listener &listener) : PlatformWindow(listener) {}

   X11Window::~X11Window() { destroy(); }

   bool X11Window::open(uint32_t parent) {
      int screenIndex = 0;
      _connection = xcb_connect(nullptr, &screenIndex);
      if (xcb_connection_has_error(_connection)) {
         std::cerr << "clap-gui: failed to connect to the X server" << std::endl;
         destroy();
         return false;
      }

      auto screens = xcb_setup_roots_iterator(xcb_get_setup(_connection));
      for (int i = 0; i < screenIndex && screens.rem > 0; ++i)
         xcb_screen_next(&screens);
      const xcb_screen_t *screen = screens.data;
      _root = screen->root;
      _visual = screen->root_visual;
      _depth = screen->root_depth;

      if (!checkPixelFormat() || !initAtoms()) {
         destroy();
         return false;
      }

      _isFloating = parent == 0;
      _physicalWidth = std::max<uint32_t>(1, std::lround(_width * _scale));
      _physicalHeight = std::max<uint32_t>(1, std::lround(_height * _scale));

      // We use the root visual, which may differ from the parent's one: then the colormap and
      // the border pixel are required.
      _colormap = xcb_generate_id(_connection);
      xcb_create_colormap(_connection, XCB_COLORMAP_ALLOC_NONE, _colormap, _root, _visual);

      // No background: the server would clear the window before each Expose, which flickers
      const uint32_t values[] = {XCB_BACK_PIXMAP_NONE, 0, kEventMask, _colormap};
      _window = xcb_generate_id(_connection);
      xcb_create_window(_connection,
                        _depth,
                        _window,
                        _isFloating ? _root : parent,
                        0,
                        0,
                        _physicalWidth,
                        _physicalHeight,
                        0,
                        XCB_WINDOW_CLASS_INPUT_OUTPUT,
                        _visual,
                        XCB_CW_BACK_PIXMAP | XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK |
                           XCB_CW_COLORMAP,
                        values);

      _gc = xcb_generate_id(_connection);
      xcb_create_gc(_connection, _gc, _window, 0, nullptr);

      if (_isFloating) {
         xcb_change_property(_connection,
                             XCB_PROP_MODE_REPLACE,
                             _window,
                             _wmProtocols,
                             XCB_ATOM_ATOM,
                             32,
                             1,
                             &_wmDeleteWindow);

         static const char wmClass[] = "clap-plugins\0clap-plugins";
         xcb_change_property(_connection,
                             XCB_PROP_MODE_REPLACE,
                             _window,
                             XCB_ATOM_WM_CLASS,
                             XCB_ATOM_STRING,
                             8,
                             sizeof(wmClass),
                             wmClass);
         applyMinSize();
      } else
         updateXEmbedInfo();

      xcb_flush(_connection);
      return !xcb_connection_has_error(_connection);
   }

   void X11Window::destroy() {
      if (!_connection)
         return;

      if (_gc)
         xcb_free_gc(_connection, _gc);
      if (_window)
         xcb_destroy_window(_connection, _window);
      if (_colormap)
         xcb_free_colormap(_connection, _colormap);
      _gc = 0;
      _window = 0;
      _colormap = 0;

      xcb_flush(_connection);
      xcb_disconnect(_connection);
      _connection = nullptr;
   }

   // We draw XRGB8888 pixels, the visual must match
   bool X11Window::checkPixelFormat() {
      const xcb_setup_t *setup = xcb_get_setup(_connection);

      bool hasFormat = false;
      for (auto it = xcb_setup_pixmap_formats_iterator(setup); it.rem > 0; xcb_format_next(&it)) {
         if (it.data->depth == _depth)
            hasFormat = it.data->bits_per_pixel == 32;
      }

      const xcb_visualtype_t *visual = nullptr;
      for (auto screens = xcb_setup_roots_iterator(setup); screens.rem > 0 && !visual;
           xcb_screen_next(&screens)) {
         for (auto depths = xcb_screen_allowed_depths_iterator(screens.data);
              depths.rem > 0 && !visual;
              xcb_depth_next(&depths)) {
            for (auto visuals = xcb_depth_visuals_iterator(depths.data); visuals.rem > 0;
                 xcb_visualtype_next(&visuals)) {
               if (visuals.data->visual_id == _visual) {
                  visual = visuals.data;
                  break;
               }
            }
         }
      }

      if (!hasFormat || !visual || visual->_class != XCB_VISUAL_CLASS_TRUE_COLOR ||
          visual->red_mask != 0xff0000 || visual->green_mask != 0xff00 ||
          visual->blue_mask != 0xff) {
         std::cerr << "clap-gui: unsupported X11 visual, a 24 bits true color one is required"
                   << std::endl;
         return false;
      }

      _swapBytes = setup->image_byte_order != XCB_IMAGE_ORDER_LSB_FIRST;
      return true;
   }

   bool X11Window::initAtoms() {
      static const char *const names[] = {
         "WM_PROTOCOLS", "WM_DELETE_WINDOW", "_XEMBED_INFO", "_NET_WM_NAME", "UTF8_STRING"};
      uint32_t *const atoms[] = {&_wmProtocols, &_wmDeleteWindow, &_xembedInfo, &_netWmName, &_utf8String};

      xcb_intern_atom_cookie_t cookies[std::size(names)];
      for (size_t i = 0; i < std::size(names); ++i)
         cookies[i] = xcb_intern_atom(_connection, 0, strlen(names[i]), names[i]);

      bool succeeded = true;
      for (size_t i = 0; i < std::size(names); ++i) {
         auto reply = xcb_intern_atom_reply(_connection, cookies[i], nullptr);
         if (reply)
            *atoms[i] = reply->atom;
         else
            succeeded = false;
         free(reply);
      }
      return succeeded;
   }

   bool X11Window::setTransient(uint32_t window) {
      if (!_isFloating || !_window)
         return false;

      xcb_change_property(_connection,
                          XCB_PROP_MODE_REPLACE,
                          _window,
                          XCB_ATOM_WM_TRANSIENT_FOR,
                          XCB_ATOM_WINDOW,
                          32,
                          1,
                          &window);
      xcb_flush(_connection);
      return true;
   }

   void X11Window::setTitle(const std::string &title) {
      if (!_isFloating || !_window)
         return;

      xcb_change_property(_connection,
                          XCB_PROP_MODE_REPLACE,
                          _window,
                          XCB_ATOM_WM_NAME,
                          XCB_ATOM_STRING,
                          8,
                          title.size(),
                          title.data());
      xcb_change_property(_connection,
                          XCB_PROP_MODE_REPLACE,
                          _window,
                          _netWmName,
                          _utf8String,
                          8,
                          title.size(),
                          title.data());
      xcb_flush(_connection);
   }

   void X11Window::setSize(uint32_t width, uint32_t height) {
      _width = std::max(width, _minWidth);
      _height = std::max(height, _minHeight);
      applySize();
   }

   void X11Window::setMinSize(uint32_t width, uint32_t height) {
      _minWidth = std::max(width, 1u);
      _minHeight = std::max(height, 1u);
      applyMinSize();
   }

   void X11Window::setScale(double scale) {
      if (scale <= 0 || scale == _scale)
         return;

      _scale = scale;
      applyMinSize();
      applySize();
      _listener.onScaleChanged();
   }

   void X11Window::applySize() {
      const uint32_t width = std::max<uint32_t>(1, std::lround(_width * _scale));
      const uint32_t height = std::max<uint32_t>(1, std::lround(_height * _scale));
      if (!_window || (width == _physicalWidth && height == _physicalHeight))
         return;

      _physicalWidth = width;
      _physicalHeight = height;
      const uint32_t values[] = {width, height};
      xcb_configure_window(
         _connection, _window, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
      xcb_flush(_connection);
   }

   // For floating windows, the window manager enforces it
   void X11Window::applyMinSize() {
      if (!_isFloating || !_window)
         return;

      uint32_t hints[kSizeHintsLength] = {};
      hints[0] = kSizeHintsMinSize;
      hints[5] = std::lround(_minWidth * _scale);
      hints[6] = std::lround(_minHeight * _scale);
      xcb_change_property(_connection,
                          XCB_PROP_MODE_REPLACE,
                          _window,
                          XCB_ATOM_WM_NORMAL_HINTS,
                          XCB_ATOM_WM_SIZE_HINTS,
                          32,
                          kSizeHintsLength,
                          hints);
      xcb_flush(_connection);
   }

   // Tells an XEmbed embedder whether to show the window. We also map it ourselves, for the
   // hosts which don't implement XEmbed.
   void X11Window::updateXEmbedInfo() {
      const uint32_t info[] = {kXEmbedVersion, _isVisible ? kXEmbedMapped : 0};
      xcb_change_property(
         _connection, XCB_PROP_MODE_REPLACE, _window, _xembedInfo, _xembedInfo, 32, 2, info);
   }

   void X11Window::setVisible(bool isVisible) {
      if (_isVisible == isVisible || !_window)
         return;

      _isVisible = isVisible;
      if (!_isFloating)
         updateXEmbedInfo();

      if (isVisible)
         xcb_map_window(_connection, _window);
      else {
         xcb_unmap_window(_connection, _window);
         leavePointer();
      }
      xcb_flush(_connection);
   }

   ////////////
   // Events //
   ////////////
   void X11Window::dispatchEvents() {
      if (!_connection)
         return;

      while (auto *event = xcb_poll_for_event(_connection)) {
         handleEvent(*event);
         free(event);
      }

      if (xcb_connection_has_error(_connection)) {
         std::cerr << "clap-gui: lost the connection to the X server" << std::endl;
         destroy();
         if (_isFloating)
            _listener.onCloseRequested();
      }
   }

   void X11Window::handleEvent(const xcb_generic_event_t &event) {
      // The errors are about requests on a window which the host already destroyed
      switch (event.response_type & ~0x80) {
      case XCB_EXPOSE:
         if (reinterpret_cast<const xcb_expose_event_t &>(event).count == 0)
            putImage();
         break;

      case XCB_BUTTON_PRESS: {
         auto &e = reinterpret_cast<const xcb_button_press_event_t &>(event);
         onButton(e.detail, true, e.event_x / _scale, e.event_y / _scale);
         break;
      }

      case XCB_BUTTON_RELEASE: {
         auto &e = reinterpret_cast<const xcb_button_release_event_t &>(event);
         onButton(e.detail, false, e.event_x / _scale, e.event_y / _scale);
         break;
      }

      case XCB_MOTION_NOTIFY: {
         auto &e = reinterpret_cast<const xcb_motion_notify_event_t &>(event);
         _listener.onPointerMove(e.event_x / _scale, e.event_y / _scale);
         break;
      }

      case XCB_ENTER_NOTIFY: {
         auto &e = reinterpret_cast<const xcb_enter_notify_event_t &>(event);
         _isPointerInside = true;
         _hasLeftWhilePressed = false;
         _listener.onPointerMove(e.event_x / _scale, e.event_y / _scale);
         break;
      }

      case XCB_LEAVE_NOTIFY:
         // While a button is pressed, the pointer is grabbed: we keep getting its motions
         if (_pressedButtons)
            _hasLeftWhilePressed = true;
         else
            leavePointer();
         break;

      case XCB_CONFIGURE_NOTIFY: {
         auto &e = reinterpret_cast<const xcb_configure_notify_event_t &>(event);
         if (e.window == _window)
            onConfigure(e.width, e.height);
         break;
      }

      case XCB_CLIENT_MESSAGE: {
         auto &e = reinterpret_cast<const xcb_client_message_event_t &>(event);
         if (e.type == _wmProtocols && e.data.data32[0] == _wmDeleteWindow)
            _listener.onCloseRequested();
         break;
      }

      case XCB_DESTROY_NOTIFY:
         // The host destroyed its window, and ours with it
         if (reinterpret_cast<const xcb_destroy_notify_event_t &>(event).window == _window)
            _window = 0;
         break;

      default:
         break;
      }
   }

   void X11Window::onButton(uint8_t button, bool isPressed, double x, double y) {
      // The wheel steps come as buttons 4 to 7
      if (button >= 4 && button <= 7) {
         if (isPressed) {
            const double step = kScrollLinesPerStep;
            _listener.onPointerScroll(button == 6 ? step : button == 7 ? -step : 0,
                                      button == 4 ? step : button == 5 ? -step : 0);
         }
         return;
      }

      int index;
      switch (button) {
      case 1:
         index = 0;
         break;
      case 3:
         index = 1;
         break;
      case 2:
         index = 2;
         break;
      default:
         return;
      }

      _listener.onPointerMove(x, y);
      if (isPressed)
         _pressedButtons |= 1u << index;
      else
         _pressedButtons &= ~(1u << index);
      _listener.onPointerButton(index, isPressed);

      if (!_pressedButtons && _hasLeftWhilePressed)
         leavePointer();
   }

   void X11Window::leavePointer() {
      _hasLeftWhilePressed = false;
      if (!_isPointerInside)
         return;

      for (int i = 0; i < 3; ++i)
         if (_pressedButtons & (1u << i))
            _listener.onPointerButton(i, false);
      _pressedButtons = 0;

      _isPointerInside = false;
      _listener.onPointerLeave();
   }

   // The window manager or the host resized the window
   void X11Window::onConfigure(uint32_t width, uint32_t height) {
      if (width == _physicalWidth && height == _physicalHeight)
         return;

      // Even if the logical size doesn't change, the frame needs to be redrawn
      _physicalWidth = width;
      _physicalHeight = height;
      _width = std::max<uint32_t>(1, std::lround(width / _scale));
      _height = std::max<uint32_t>(1, std::lround(height / _scale));
      _listener.onResized(_width, _height);
   }

   ////////////
   // Frames //
   ////////////
   Framebuffer *X11Window::beginFrame() {
      if (!_window || !_isVisible)
         return nullptr;

      const int width = std::max<int>(1, _physicalWidth);
      const int height = std::max<int>(1, _physicalHeight);
      _pixels.resize(size_t(width) * height);
      _framebuffer.pixels = _pixels.data();
      _framebuffer.width = width;
      _framebuffer.height = height;
      _framebuffer.stride = width;
      return &_framebuffer;
   }

   void X11Window::endFrame() {
      _hasFrame = true;
      putImage();
   }

   // X11 has no frame synchronization: we draw as soon as the frame is ready
   void X11Window::putImage() {
      if (!_window || !_hasFrame)
         return;

      const auto &fb = _framebuffer;
      const uint32_t *pixels = fb.pixels;
      if (_swapBytes) {
         _swappedPixels.resize(_pixels.size());
         for (size_t i = 0; i < _pixels.size(); ++i) {
            const uint32_t p = _pixels[i];
            _swappedPixels[i] = (p >> 24) | ((p >> 8) & 0xff00) | ((p << 8) & 0xff0000) | (p << 24);
         }
         pixels = _swappedPixels.data();
      }

      // A request can't be bigger than the server's limit, so we send the image in bands
      const uint32_t rowSize = fb.stride * 4;
      const uint32_t maxRequestSize = xcb_get_maximum_request_length(_connection) * 4;
      const int rowsPerRequest =
         std::max<int>(1, (maxRequestSize - kPutImageHeaderSize) / rowSize);

      for (int y = 0; y < fb.height; y += rowsPerRequest) {
         const int rows = std::min(rowsPerRequest, fb.height - y);
         xcb_put_image(_connection,
                       XCB_IMAGE_FORMAT_Z_PIXMAP,
                       _window,
                       _gc,
                       fb.width,
                       rows,
                       0,
                       y,
                       0,
                       _depth,
                       rows * rowSize,
                       reinterpret_cast<const uint8_t *>(pixels + size_t(y) * fb.stride));
      }
      xcb_flush(_connection);
   }

} // namespace clap
