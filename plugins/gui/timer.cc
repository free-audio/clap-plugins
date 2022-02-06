#include "timer.hh"

#ifdef __APPLE__
#   include "cf-timer.hh"
#endif

namespace clap {
   Timer::~Timer() = default;

   std::unique_ptr<Timer> Timer::createNative(uint32_t durationMs,
                                              const std::function<void()> &callback) {
#ifdef __APPLE__
      return std::make_unique<CFTimer>(durationMs, callback);
#endif
      return {};
   }
} // namespace clap
