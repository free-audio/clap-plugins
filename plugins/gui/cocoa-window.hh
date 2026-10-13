#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "platform-window.hh"

namespace clap {

   // An NSView, added to the host's view, or the content of a floating NSWindow.
   //
   // The host runs the main run loop, so the view has its own timer. CLAP's Cocoa API uses
   // logical sizes, and the scale comes from the window's backing store.
   class CocoaWindow final : public PlatformWindow {
   public:
      explicit CocoaWindow(Listener &listener);
      ~CocoaWindow() override;

      // parent is the host's NSView, or nullptr for a floating window
      [[nodiscard]] bool open(void *parent);

      // Floating windows: keeps the window above the host's one, whose NSView it is
      bool setTransient(void *hostView);

      void setSize(uint32_t width, uint32_t height) override;
      void setMinSize(uint32_t width, uint32_t height) override;
      void setTitle(const std::string &title) override;
      void setVisible(bool isVisible) override;
      double scale() const override { return _scale; }
      void dispatchEvents() override {}
      [[nodiscard]] Framebuffer *beginFrame() override;
      void endFrame() override;

      // Called by the view and the timer
      void draw(void *cgContext);
      // hasNoButton: no button is held, according to AppKit
      void onMouseMove(double x, double y, bool hasNoButton);
      void onMouseButton(int index, bool isPressed, double x, double y);
      void onMouseExited();
      void onScroll(double dx, double dy);
      void updateScale();
      void onTimerFired() { _listener.onTimer(); }
      void onWindowClosed();
      void onWindowResized();

   private:
      // contentView is our NSView
      void openWindow(void *contentView);
      void destroy();
      void leavePointer();

      void *_view = nullptr;            // NSView, retained
      void *_timer = nullptr;           // CFRunLoopTimerRef
      void *_window = nullptr;          // NSWindow, retained, for a floating window
      void *_transientParent = nullptr; // NSWindow, retained
      void *_observers = nullptr;       // NSArray of the notification observers, retained
      bool _isFloating = false;
      std::string _title;

      uint32_t _width = 1;
      uint32_t _height = 1;
      uint32_t _minWidth = 1;
      uint32_t _minHeight = 1;
      double _scale = 1;

      std::vector<uint32_t> _pixels;
      Framebuffer _framebuffer;
      bool _hasFrame = false;

      // pointer state
      bool _isPointerInside = false;
      uint32_t _pressedButtons = 0; // bit mask of the listener's button indexes
      bool _hasLeftWhilePressed = false;
   };

} // namespace clap
