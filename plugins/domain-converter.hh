#pragma once

#include <cmath>

namespace clap {
   template <typename Operator>
   class DomainConverter {
   public:
      DomainConverter(double initialValue)
         : _op(), _in(initialValue), _out(_op(initialValue)) {}

      [[nodiscard]] double convert(double in) {
         if (in == _in) [[likely]]
            return _out;

         _out = _op(in);
         return _out;
      }

   private:
      const Operator _op;
      double _in;
      double _out;
   };

   struct GainOperator {
      [[nodiscard]] double operator()(double gaindB) const noexcept {
         return std::pow(10.0, gaindB / 20.0);
      }
   };

   using GainConverter = DomainConverter<GainOperator>;
} // namespace clap