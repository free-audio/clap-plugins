#pragma once

#ifdef __APPLE__

#include <functional>

#include <CoreFoundation/CoreFoundation.h>

#include "timer.hh"

namespace clap {
class CFTimer final : public Timer {
public:
   CFTimer(uint32_t durationMs, const std::function<void()> &callback);
   ~CFTimer() override;

   void start() override;
   void stop() override;

private:
   static void onTimer(CFRunLoopTimerRef timer, void *ctx);

   const uint32_t _durationMs;
   const std::function<void()> _cb;
   CFRunLoopTimerRef _timer = nullptr;
   CFRunLoopTimerContext _ctx;
};
}

#endif
