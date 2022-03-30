#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>

namespace clap {
   class SmoothedValue {
   public:
      SmoothedValue() = default;
      explicit SmoothedValue(double v) : _value(v) {}

      double value() const noexcept { return _value; }

      void setImmediately(double val) noexcept {
         _value = val;
         _ramp = 0;
         _steps = 0;
      }

      void setSmoothed(double val, uint32_t steps) noexcept {
         assert(steps > 0);
         _ramp = (val - _value) / steps;
         _steps = steps;
      }

      // Advances the value by 1 samples and return the new value + modulation
      double step() noexcept {
         if (_steps > 0) [[unlikely]] {
            _value += _ramp;
            --_steps;
         }

         return _value;
      }

      // Advances the value by n samples and return the new value + modulation
      double step(uint32_t n) noexcept {
         if (_steps > 0) [[unlikely]] {
            auto k = std::min<uint32_t>(_steps, n);
            _value += k * _ramp;
            _steps -= k;
         }

         return _value;
      }

      [[nodiscard]] bool isSmoothing() const noexcept { return _steps > 0; }

      void render(double *buffer, uint32_t numValues, uint32_t stepSize) noexcept
      {
         buffer[0] = _value;
         for (uint32_t i = 1; i < numValues; ++i)
            buffer[i] = step(stepSize);
      }

   private:
      double _value = 0;
      double _ramp = 0;
      uint32_t _steps = 0;
   };
} // namespace clap
