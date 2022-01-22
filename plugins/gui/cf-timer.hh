#pragma once

#include <functional>

#include <CoreFoundation/CoreFoundation.h>


namespace clap {
class CFTimer final {
public:
   CFTimer(uint32_t durationMs, const std::function<void()> &callback);
   ~CFTimer();

private:
   static void onTimer(CFRunLoopTimerRef timer, void *ctx);

   const std::function<void()> _cb;
   CFRunLoopTimerRef _timer = nullptr;
   CFRunLoopTimerContext _ctx;
};
}