#pragma once

#include <clap/clap.h>

namespace clap {
   class FixedPoint {
   public:
      FixedPoint() = default;
      explicit FixedPoint(int64_t fp) : _fp(fp) {}

      [[nodiscard]] static FixedPoint fromDouble(double value) noexcept;

      [[nodiscard]] int64_t raw() const noexcept { return _fp; }

   private:
      int64_t _fp = 0;
   };

   [[nodiscard]] inline bool operator<(const FixedPoint &a, const FixedPoint &b) noexcept {
      return a.raw() < b.raw();
   }

   [[nodiscard]] inline bool operator<=(const FixedPoint &a, const FixedPoint &b) noexcept {
      return a.raw() <= b.raw();
   }

   [[nodiscard]] inline bool operator==(const FixedPoint &a, const FixedPoint &b) noexcept {
      return a.raw() == b.raw();
   }

   [[nodiscard]] inline bool operator!=(const FixedPoint &a, const FixedPoint &b) noexcept {
      return a.raw() != b.raw();
   }

   [[nodiscard]] inline bool operator>=(const FixedPoint &a, const FixedPoint &b) noexcept {
      return a.raw() >= b.raw();
   }

   [[nodiscard]] inline bool operator>(const FixedPoint &a, const FixedPoint &b) noexcept {
      return a.raw() > b.raw();
   }
} // namespace clap