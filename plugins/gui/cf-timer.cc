#include "cf-timer.hh"

namespace clap {

   CFTimer::CFTimer(uint32_t durationMs, const std::function<void()> &callback) : _cb(callback) {
      if (!_cb)
         throw std::invalid_argument("callback can't be null");

      auto runLoop = CFRunLoopGetCurrent();

      _ctx.copyDescription = nullptr;
      _ctx.info = this;
      _ctx.release = nullptr;
      _ctx.retain = nullptr;
      _ctx.version = 0;

      _timer =
         CFRunLoopTimerCreate(kCFAllocatorDefault, 0, 1000.0 / 60, 0, 0, &CFTimer::onTimer, &_ctx);
      if (_timer)
         CFRunLoopAddTimer(runLoop, _timer, kCFRunLoopCommonModes);
   }

   CFTimer::~CFTimer() {
      if (_timer) {
         CFRelease(_timer);
         _timer = nullptr;
      }
   }

   void CFTimer::onTimer(CFRunLoopTimerRef timer, void *ctx) {
      auto self = static_cast<CFTimer *>(ctx);
      self->_cb();
   }
} // namespace clap