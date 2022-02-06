#ifdef __APPLE__

#   include "cf-timer.hh"

namespace clap {

   CFTimer::CFTimer(uint32_t durationMs, const std::function<void()> &callback) : _cb(callback) {
      if (!_cb)
         throw std::invalid_argument("callback can't be null");

      _ctx.copyDescription = nullptr;
      _ctx.info = this;
      _ctx.release = nullptr;
      _ctx.retain = nullptr;
      _ctx.version = 0;
   }

   void CFTimer::start() {
      auto runLoop = CFRunLoopGetCurrent();

      _timer = CFRunLoopTimerCreate(
         kCFAllocatorDefault, 0, durationMs * 0.0001, 0, 0, &CFTimer::onTimer, &_ctx);
      if (_timer)
         CFRunLoopAddTimer(runLoop, _timer, kCFRunLoopCommonModes);
   }

   void CFTimer::stop() {
      auto runLoop = CFRunLoopGetCurrent();

      if (_timer)
         CFRunLoopRemoveTimer(runLoop, _timer, kCFRunLoopCommonModes);
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

#endif