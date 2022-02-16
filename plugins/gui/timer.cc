#include "timer.hh"

#if defined(__APPLE__)
#   include "cf-timer.hh"
#elif defined(_WIN32)
#   include "win32-timer.hh"
#endif

namespace clap {
   Timer::~Timer() = default;

   std::unique_ptr<Timer> Timer::createNative(uint32_t durationMs,
                                              const std::function<void()> &callback) {
#if defined(__APPLE__)
      return std::make_unique<CFTimer>(durationMs, callback);
#elif defined(_WIN32)
      return std::make_unique<Win32Timer>(durationMs, callback);
#endif
      return {};
   }
} // namespace clap
