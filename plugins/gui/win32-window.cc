#include <algorithm>
#include <cmath>
#include <cwchar>
#include <iostream>
#include <iterator>

#include <windows.h>
#include <windowsx.h>

#include "win32-window.hh"

namespace clap {

   namespace {
      constexpr UINT_PTR kTimerId = 1;
      // The timers fire on the system tick, about 15.6 ms: asking for less makes it every tick
      constexpr UINT kTimerPeriodMs = 10;

      HINSTANCE moduleHandle() {
         HMODULE module = nullptr;
         GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(&moduleHandle),
                            &module);
         return module;
      }

      LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
         if (message == WM_NCCREATE) {
            auto *cs = reinterpret_cast<CREATESTRUCTW *>(lParam);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
         }

         auto *window = reinterpret_cast<Win32Window *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
         if (message == WM_NCDESTROY)
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);

         if (window)
            return window->handleMessage(hwnd, message, wParam, lParam);
         return DefWindowProcW(hwnd, message, wParam, lParam);
      }

      // Each loaded copy of the plugin registers its own class, which goes away with the last
      // window: the module may be unloaded afterwards.
      class WindowClass {
      public:
         const wchar_t *acquire() {
            if (_refCount++ == 0) {
               swprintf(_name, std::size(_name), L"clap-plugins-gui-%p", (void *)moduleHandle());

               WNDCLASSEXW wc = {};
               wc.cbSize = sizeof(wc);
               wc.lpfnWndProc = windowProc;
               wc.hInstance = moduleHandle();
               wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
               wc.lpszClassName = _name;
               if (!RegisterClassExW(&wc)) {
                  _refCount = 0;
                  return nullptr;
               }
            }
            return _name;
         }

         void release() {
            if (--_refCount == 0)
               UnregisterClassW(_name, moduleHandle());
         }

      private:
         int _refCount = 0;
         wchar_t _name[64] = {};
      };

      WindowClass windowClass;
   } // namespace

   Win32Window::Win32Window(Listener &listener) : PlatformWindow(listener) {}

   Win32Window::~Win32Window() { destroy(); }

   bool Win32Window::open(void *parent) {
      const wchar_t *className = windowClass.acquire();
      if (!className) {
         std::cerr << "clap-gui: failed to register the window class" << std::endl;
         return false;
      }

      _physicalWidth = std::max<uint32_t>(1, std::lround(_width * _scale));
      _physicalHeight = std::max<uint32_t>(1, std::lround(_height * _scale));
      _hwnd = CreateWindowExW(0,
                              className,
                              L"",
                              WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                              0,
                              0,
                              _physicalWidth,
                              _physicalHeight,
                              static_cast<HWND>(parent),
                              nullptr,
                              moduleHandle(),
                              this);
      if (!_hwnd) {
         std::cerr << "clap-gui: failed to create the window" << std::endl;
         windowClass.release();
         return false;
      }

      SetTimer(static_cast<HWND>(_hwnd), kTimerId, kTimerPeriodMs, nullptr);
      _hasClass = true;
      return true;
   }

   void Win32Window::destroy() {
      if (_hwnd) {
         auto hwnd = static_cast<HWND>(_hwnd);
         KillTimer(hwnd, kTimerId);
         // No more messages for us, even those sent during the destruction
         SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
         DestroyWindow(hwnd);
         _hwnd = nullptr;
      }

      if (_hasClass) {
         windowClass.release();
         _hasClass = false;
      }
   }

   void Win32Window::setSize(uint32_t width, uint32_t height) {
      _width = std::max(width, 1u);
      _height = std::max(height, 1u);
      applySize();
   }

   void Win32Window::setScale(double scale) {
      if (scale <= 0 || scale == _scale)
         return;

      _scale = scale;
      applySize();
      _listener.onScaleChanged();
   }

   void Win32Window::applySize() {
      const uint32_t width = std::max<uint32_t>(1, std::lround(_width * _scale));
      const uint32_t height = std::max<uint32_t>(1, std::lround(_height * _scale));
      if (!_hwnd || (width == _physicalWidth && height == _physicalHeight))
         return;

      _physicalWidth = width;
      _physicalHeight = height;
      SetWindowPos(static_cast<HWND>(_hwnd),
                   nullptr,
                   0,
                   0,
                   width,
                   height,
                   SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
   }

   void Win32Window::setVisible(bool isVisible) {
      if (!_hwnd)
         return;

      auto hwnd = static_cast<HWND>(_hwnd);
      ShowWindow(hwnd, isVisible ? SW_SHOWNA : SW_HIDE);
      if (!isVisible) {
         leavePointer();
         if (GetCapture() == hwnd)
            ReleaseCapture();
      }
   }

   ////////////
   // Frames //
   ////////////
   Framebuffer *Win32Window::beginFrame() {
      if (!_hwnd || !IsWindowVisible(static_cast<HWND>(_hwnd)))
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

   void Win32Window::endFrame() {
      _hasFrame = true;

      auto hwnd = static_cast<HWND>(_hwnd);
      HDC dc = GetDC(hwnd);
      paint(dc);
      ReleaseDC(hwnd, dc);
   }

   void Win32Window::paint(void *dc) {
      if (!_hasFrame) {
         RECT rect = {0, 0, LONG(_physicalWidth), LONG(_physicalHeight)};
         FillRect(static_cast<HDC>(dc), &rect, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
         return;
      }

      const auto &fb = _framebuffer;
      BITMAPINFO info = {};
      info.bmiHeader.biSize = sizeof(info.bmiHeader);
      info.bmiHeader.biWidth = fb.stride;
      info.bmiHeader.biHeight = -fb.height; // top-down
      info.bmiHeader.biPlanes = 1;
      info.bmiHeader.biBitCount = 32;
      info.bmiHeader.biCompression = BI_RGB;
      SetDIBitsToDevice(static_cast<HDC>(dc),
                        0,
                        0,
                        fb.width,
                        fb.height,
                        0,
                        0,
                        0,
                        fb.height,
                        fb.pixels,
                        &info,
                        DIB_RGB_COLORS);
   }

   ////////////
   // Events //
   ////////////
   intptr_t Win32Window::handleMessage(void *window, unsigned message, uintptr_t wParam, intptr_t lParam) {
      auto hwnd = static_cast<HWND>(window);

      switch (message) {
      case WM_TIMER:
         if (wParam != kTimerId)
            break;
         _listener.onTimer();
         return 0;

      case WM_PAINT: {
         PAINTSTRUCT ps;
         HDC dc = BeginPaint(hwnd, &ps);
         paint(dc);
         EndPaint(hwnd, &ps);
         return 0;
      }

      // We cover the whole window, erasing it would flicker
      case WM_ERASEBKGND:
         return 1;

      case WM_MOUSEMOVE:
         onMouseMove(lParam);
         return 0;

      case WM_MOUSELEAVE:
         // With the capture, we keep getting the moves outside of the window
         if (_pressedButtons)
            _hasLeftWhilePressed = true;
         else
            leavePointer();
         return 0;

      case WM_LBUTTONDOWN:
      case WM_LBUTTONUP:
         onButton(0, message == WM_LBUTTONDOWN, lParam);
         return 0;

      case WM_RBUTTONDOWN:
      case WM_RBUTTONUP:
         onButton(1, message == WM_RBUTTONDOWN, lParam);
         return 0;

      case WM_MBUTTONDOWN:
      case WM_MBUTTONUP:
         onButton(2, message == WM_MBUTTONDOWN, lParam);
         return 0;

      // Another window took the capture, we won't get the releases
      case WM_CAPTURECHANGED:
         if (reinterpret_cast<HWND>(lParam) != hwnd) {
            for (int i = 0; i < 3; ++i)
               if (_pressedButtons & (1u << i))
                  _listener.onPointerButton(i, false);
            _pressedButtons = 0;
            if (_hasLeftWhilePressed)
               leavePointer();
         }
         return 0;

      // Positive values go up, and right for the horizontal wheel
      case WM_MOUSEWHEEL:
         _listener.onPointerScroll(0, double(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA);
         return 0;

      case WM_MOUSEHWHEEL:
         _listener.onPointerScroll(-double(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA, 0);
         return 0;

      // The host destroyed its window, and ours with it
      case WM_NCDESTROY:
         KillTimer(hwnd, kTimerId);
         _hwnd = nullptr;
         _isPointerInside = false;
         _pressedButtons = 0;
         break;

      // The host resized our window
      case WM_SIZE: {
         const uint32_t width = LOWORD(lParam);
         const uint32_t height = HIWORD(lParam);
         if (width == 0 || height == 0 || (width == _physicalWidth && height == _physicalHeight))
            break;

         // Even if the logical size doesn't change, the frame needs to be redrawn
         _physicalWidth = width;
         _physicalHeight = height;
         _width = std::max<uint32_t>(1, std::lround(width / _scale));
         _height = std::max<uint32_t>(1, std::lround(height / _scale));
         _listener.onResized(_width, _height);
         return 0;
      }

      default:
         break;
      }

      return DefWindowProcW(hwnd, message, wParam, lParam);
   }

   void Win32Window::onMouseMove(intptr_t lParam) {
      const int x = GET_X_LPARAM(lParam);
      const int y = GET_Y_LPARAM(lParam);

      // Back in the window during a drag
      const bool isInside = x >= 0 && y >= 0 && x < int(_physicalWidth) && y < int(_physicalHeight);
      if (_hasLeftWhilePressed && isInside) {
         _hasLeftWhilePressed = false;
         trackLeave();
      }

      if (!_isPointerInside) {
         _isPointerInside = true;
         _hasLeftWhilePressed = false;
         trackLeave();
      }

      _listener.onPointerMove(x / _scale, y / _scale);
   }

   // Asks for WM_MOUSELEAVE, once
   void Win32Window::trackLeave() {
      TRACKMOUSEEVENT tme = {};
      tme.cbSize = sizeof(tme);
      tme.dwFlags = TME_LEAVE;
      tme.hwndTrack = static_cast<HWND>(_hwnd);
      TrackMouseEvent(&tme);
   }

   void Win32Window::onButton(int index, bool isPressed, intptr_t lParam) {
      onMouseMove(lParam);

      auto hwnd = static_cast<HWND>(_hwnd);
      if (isPressed) {
         // Keeps the moves and the release coming when the pointer leaves the window
         if (!_pressedButtons)
            SetCapture(hwnd);
         _pressedButtons |= 1u << index;
      } else
         _pressedButtons &= ~(1u << index);

      _listener.onPointerButton(index, isPressed);

      if (!_pressedButtons && GetCapture() == hwnd) {
         ReleaseCapture();
         if (_hasLeftWhilePressed)
            leavePointer();
      }
   }

   void Win32Window::leavePointer() {
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

} // namespace clap
