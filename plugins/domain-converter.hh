#pragma once

#include <cmath>

namespace clap {
   template <typename Operator>
   class DomainConverter {
   public:
      DomainConverter(const Operator &op, double initialValue)
         : _op(op), _in(initialValue), _out(_op(initialValue)) {}

      DomainConverter(double initialValue) : _op(), _in(initialValue), _out(_op(initialValue)) {}

      DomainConverter<Operator>& operator=(const DomainConverter<Operator>&o) = default;

      [[nodiscard]] double convert(double in) {
         if (in == _in) [[likely]]
            return _out;

         _out = _op(in);
         return _out;
      }

   private:
      Operator _op;
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