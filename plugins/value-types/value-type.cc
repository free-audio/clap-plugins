#include "value-type.hh"

namespace clap {

   ValueType::~ValueType() = default;

   void ValueType::toEngine(AudioBuffer<double> &buffer, uint32_t numFrames) const {
      if (!hasEngineDomain())
         return;

      struct Op {
         Op(const ValueType &vt) : _valueType(vt) {}

         double operator()(double v) const { return _valueType.toEngine(v); }

         const ValueType &_valueType;
      } op(*this);

      buffer.compute(op, numFrames);
   }

   bool ValueType::hasEngineDomain() const { return false; }
   double ValueType::toParam(double engineValue) const { return engineValue; }
   double ValueType::toEngine(double paramValue) const { return paramValue; }
} // namespace clap
