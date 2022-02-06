#pragma once

#include <cstdint>
#include <functional>
#include <memory>

namespace clap {
   class Timer {
   public:
      virtual ~Timer();

      virtual void start() = 0;
      virtual void stop() = 0;

      /* Tries to create a native timer. Returns null if the timer can't be created. */
      static std::unique_ptr<Timer> createNative(uint32_t durationMs,
                                                 const std::function<void()> &callback);
   };

} // namespace clap
