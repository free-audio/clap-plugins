#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>

namespace clap {
   class SmoothedValue {
   public:
      double value() const noexcept { return _value; }

      void setImmediately(double val) noexcept {
         _value = val;
         _ramp = 0;
         _steps = 0;
      }

      void setSmoothed(double val, uint16_t steps) noexcept {
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

   private:
      double _value = 0;
      double _ramp = 0;
      uint16_t _steps = 0;
   };
} // namespace clap