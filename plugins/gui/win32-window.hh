#pragma once

#include <cstdint>
#include <vector>

#include "platform-window.hh"

namespace clap {

   // A child window of the host's window.
   //
   // The host runs a message loop on the main thread, so the window has its own timer.
   //
   // Sizes are logical, the window is scale() times bigger. The host gives the scale with
   // clap_plugin_gui.set_scale(), as CLAP's Win32 API uses physical pixels.
   class Win32Window final : public PlatformWindow {
   public:
      explicit Win32Window(Listener &listener);
      ~Win32Window() override;

      // parent is the host's HWND
      [[nodiscard]] bool open(void *parent);

      void setSize(uint32_t width, uint32_t height) override;
      void setVisible(bool isVisible) override;
      double scale() const override { return _scale; }
      void setScale(double scale) override;
      void dispatchEvents() override {}
      [[nodiscard]] Framebuffer *beginFrame() override;
      void endFrame() override;

      // Called by the window procedure, with the Win32 types
      intptr_t handleMessage(void *hwnd, unsigned message, uintptr_t wParam, intptr_t lParam);

   private:
      void destroy();
      void applySize();
      void paint(void *dc);
      void onButton(int index, bool isPressed, intptr_t lParam);
      void onMouseMove(intptr_t lParam);
      void trackLeave();
      void leavePointer();

      void *_hwnd = nullptr;
      bool _hasClass = false;

      // Logical sizes
      uint32_t _width = 1;
      uint32_t _height = 1;
      // Physical size of the window
      uint32_t _physicalWidth = 0;
      uint32_t _physicalHeight = 0;
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
