#pragma once

#include <cstdint>
#include <string>

namespace clap {

   // XRGB8888 pixels, in physical pixels.
   struct Framebuffer {
      uint32_t *pixels = nullptr;
      int width = 0;
      int height = 0;
      int stride = 0; // in pixels
   };

   // The GUI is rendered in software, so a platform window only has to show a pixel buffer
   // and to report the input events.
   //
   // Sizes and coordinates are logical. The window's content is the GUI, plus the title bar
   // on top of it if titleBarHeight() isn't 0.
   //
   // Everything happens on the main thread. The windows which have their own timer call
   // Listener::onTimer(), the others rely on the GUI to call dispatchEvents() periodically.
   class PlatformWindow {
   public:
      class Listener {
      public:
         virtual ~Listener() = default;

         // Coordinates are relative to the top left corner of the window, title bar included
         virtual void onPointerMove(double x, double y) = 0;
         virtual void onPointerLeave() = 0;

         // button: 0 = left, 1 = right, 2 = middle
         virtual void onPointerButton(int button, bool isPressed) = 0;

         // In "lines", positive values go up and left
         virtual void onPointerScroll(double dx, double dy) = 0;

         virtual void onScaleChanged() = 0;

         // The window system resized the window, the size excludes the title bar
         virtual void onResized(uint32_t width, uint32_t height) = 0;

         // The window system or the title bar asks to close the window
         virtual void onCloseRequested() = 0;

         // The window is gone: the connection to the window system is lost, or the host ended
         // the embedding
         virtual void onWindowLost() = 0;

         // Periodically, from the windows which have their own timer
         virtual void onTimer() = 0;
      };

      explicit PlatformWindow(Listener &listener) : _listener(listener) {}
      virtual ~PlatformWindow() = default;

      PlatformWindow(const PlatformWindow &) = delete;
      PlatformWindow &operator=(const PlatformWindow &) = delete;

      // Size of the GUI, without the title bar
      virtual void setSize(uint32_t width, uint32_t height) = 0;
      virtual void setMinSize(uint32_t width, uint32_t height) {}

      virtual void setTitle(const std::string &title) {}

      virtual void setVisible(bool isVisible) = 0;

      // Physical pixels per logical pixel
      virtual double scale() const = 0;

      // For the window systems which don't provide it
      virtual void setScale(double scale) {}

      // Height of the title bar that the GUI has to draw above its content, because the window
      // system doesn't decorate the window. 0 if it does.
      virtual uint32_t titleBarHeight() const { return 0; }

      // Lets the user move the window, to be called when a button is pressed in the title bar
      virtual void beginMove() {}

      // Processes the pending window system events
      virtual void dispatchEvents() = 0;

      // Returns a framebuffer to draw into, or nullptr if the window can't present a frame
      // right now (hidden, or the previous frame isn't displayed yet).
      [[nodiscard]] virtual Framebuffer *beginFrame() = 0;
      virtual void endFrame() = 0;

   protected:
      Listener &_listener;
   };

} // namespace clap
