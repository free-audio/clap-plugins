#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <xcb/xcb.h>

#include "platform-window.hh"

namespace clap {

   // An X11 window, embedded into the host's window (XEmbed), or floating.
   //
   // It has its own connection to the X server, with XCB rather than Xlib: Xlib's error
   // handlers are process-wide, so a plugin would override the host's ones.
   //
   // Sizes are logical, the window is scale() times bigger. X11 has no scale, the host gives it.
   class X11Window final : public PlatformWindow {
   public:
      explicit X11Window(Listener &listener);
      ~X11Window() override;

      // parent: the host's window to embed into, or 0 for a floating window
      [[nodiscard]] bool open(uint32_t parent);

      // Floating windows: keeps the window above the host's one
      bool setTransient(uint32_t window);

      void setSize(uint32_t width, uint32_t height) override;
      void setMinSize(uint32_t width, uint32_t height) override;
      void setTitle(const std::string &title) override;
      void setVisible(bool isVisible) override;
      double scale() const override { return _scale; }
      void setScale(double scale) override;
      void dispatchEvents() override;
      [[nodiscard]] Framebuffer *beginFrame() override;
      void endFrame() override;

   private:
      void destroy();
      bool initAtoms();
      bool checkPixelFormat();
      void handleEvent(const xcb_generic_event_t &event);
      void onButton(uint8_t button, bool isPressed, double x, double y);
      void onConfigure(uint32_t width, uint32_t height);
      void applySize();
      void applyMinSize();
      void updateXEmbedInfo();
      void putImage();
      void leavePointer();

      xcb_connection_t *_connection = nullptr;
      uint32_t _root = 0;
      uint32_t _visual = 0;
      uint8_t _depth = 0;
      uint32_t _window = 0;
      uint32_t _colormap = 0;
      uint32_t _gc = 0;
      bool _isFloating = false;
      // The server stores the pixels in the other byte order
      bool _swapBytes = false;

      uint32_t _wmProtocols = 0;
      uint32_t _wmDeleteWindow = 0;
      uint32_t _xembedInfo = 0;
      uint32_t _netWmName = 0;
      uint32_t _utf8String = 0;

      // Logical sizes
      uint32_t _width = 1;
      uint32_t _height = 1;
      uint32_t _minWidth = 1;
      uint32_t _minHeight = 1;
      // Physical size of the window
      uint32_t _physicalWidth = 0;
      uint32_t _physicalHeight = 0;
      double _scale = 1;
      bool _isVisible = false;

      std::vector<uint32_t> _pixels;
      std::vector<uint32_t> _swappedPixels;
      Framebuffer _framebuffer;
      bool _hasFrame = false;

      // pointer state
      bool _isPointerInside = false;
      uint32_t _pressedButtons = 0; // bit mask of the listener's button indexes
      bool _hasLeftWhilePressed = false;
   };

} // namespace clap
