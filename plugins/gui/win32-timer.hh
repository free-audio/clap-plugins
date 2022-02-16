#pragma once

#ifdef _WIN32

#include <functional>

#include <Windows.h>

#include "timer.hh"

namespace clap {
class Win32Timer final : public Timer {
public:
   Win32Timer(uint32_t durationMs, const std::function<void()> &callback);
   ~Win32Timer() override;

   void start() override;
   void stop() override;

private:
   static void onTimer(HWND hwnd,
                       UINT message,
                       UINT_PTR idTimer,
                       DWORD dwTime);

   const uint32_t _durationMs;
   const std::function<void()> _cb;
   UINT_PTR _timer = 0;
};
}

#endif
